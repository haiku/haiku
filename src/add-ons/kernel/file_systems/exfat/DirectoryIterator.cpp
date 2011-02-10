/*
 * Copyright 2011, Jérôme Duval, korli@users.berlios.de.
 * This file may be used under the terms of the MIT License.
 */


#include "DirectoryIterator.h"

#include "encodings.h"
#include "Inode.h"


//#define TRACE_EXFAT
#ifdef TRACE_EXFAT
#	define TRACE(x...) dprintf("\33[34mexfat:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif
#	define ERROR(x...) dprintf("\33[34mexfat:\33[0m " x)


DirectoryIterator::DirectoryIterator(Inode* inode)
	:
	fOffset(-2),
	fCluster(inode->StartCluster()),
	fInode(inode),
	fBlock(inode->GetVolume()),
	fCurrent(NULL)
{
	TRACE("DirectoryIterator::DirectoryIterator() %ld\n", fCluster);
}


DirectoryIterator::~DirectoryIterator()
{
}


status_t
DirectoryIterator::InitCheck()
{
	return B_OK;
}


status_t
DirectoryIterator::GetNext(char* name, size_t* _nameLength, ino_t* _id,
	EntryVisitor* visitor)
{
	if (fOffset == -2) {
		*_nameLength = 3;
		strlcpy(name, "..", *_nameLength);
		if (fInode->ID() == 1)
			*_id = fInode->ID();
		else
			*_id = fInode->Parent();
		fOffset = -1;
		TRACE("DirectoryIterator::GetNext() found ..\n");
		return B_OK;
	} else if (fOffset == -1) {
		*_nameLength = 2;
		strlcpy(name, ".", *_nameLength);
		*_id = fInode->ID();
		fOffset = 0;
		TRACE("DirectoryIterator::GetNext() found .\n");
		return B_OK;
	}

	uchar unicodeName[EXFAT_FILENAME_MAX_LENGTH];
	size_t nameLength = EXFAT_FILENAME_MAX_LENGTH;
	status_t status = _GetNext(unicodeName, &nameLength, _id, visitor);
	if (status == B_OK && name != NULL) {
		unicode_to_utf8(unicodeName, nameLength, (uint8 *)name , _nameLength);
		TRACE("DirectoryIterator::GetNext() %ld %s, %" B_PRIdINO "\n", 
			fInode->Cluster(), name, *_id);
	}

	return status;
}


status_t
DirectoryIterator::Lookup(const char* name, size_t nameLength, ino_t* _id)
{
	if (strcmp(name, ".") == 0) {
		*_id = fInode->ID();
		return B_OK;
	} else if (strcmp(name, "..") == 0) {
		if (fInode->ID() == 1)
			*_id = fInode->ID();
		else
			*_id = fInode->Parent();
		return B_OK;
	}

	Rewind();
	fOffset = 0;

	uchar currentName[EXFAT_FILENAME_MAX_LENGTH];
	size_t currentLength = EXFAT_FILENAME_MAX_LENGTH;
	while (_GetNext((uchar*)currentName, &currentLength, _id) == B_OK) {
		char utfName[EXFAT_FILENAME_MAX_LENGTH];
		size_t utfLength = EXFAT_FILENAME_MAX_LENGTH;
		unicode_to_utf8(currentName, currentLength, (uint8*)utfName, &utfLength);
		if (nameLength == utfLength
			&& strncmp(utfName, name, nameLength) == 0) {
			TRACE("DirectoryIterator::Lookup() found ID %" B_PRIdINO "\n", *_id);
			return B_OK;
		}
		currentLength = EXFAT_FILENAME_MAX_LENGTH;
	}

	TRACE("DirectoryIterator::Lookup() not found %s\n", name);

	return B_ENTRY_NOT_FOUND;
}


status_t
DirectoryIterator::LookupEntry(EntryVisitor* visitor)
{
	fCluster = fInode->Cluster();
	fOffset = fInode->Offset();

	uchar unicodeName[EXFAT_FILENAME_MAX_LENGTH];
	size_t nameLength = EXFAT_FILENAME_MAX_LENGTH;
	return _GetNext(unicodeName, &nameLength, NULL, visitor);
}


status_t
DirectoryIterator::Rewind()
{
	fOffset = -2;
	fCluster = fInode->StartCluster();
	return B_OK;
}


void
DirectoryIterator::Iterate(EntryVisitor &visitor)
{
	fOffset = 0;
	fCluster = fInode->StartCluster();
	
	while (_NextEntry() != B_ENTRY_NOT_FOUND) {
		switch (fCurrent->type) {
			case EXFAT_ENTRY_TYPE_BITMAP:
				visitor.VisitBitmap(fCurrent);
				break;
			case EXFAT_ENTRY_TYPE_UPPERCASE:
				visitor.VisitUppercase(fCurrent);
				break;
			case EXFAT_ENTRY_TYPE_LABEL:
				visitor.VisitLabel(fCurrent);
				break;
			case EXFAT_ENTRY_TYPE_FILE:
				visitor.VisitFile(fCurrent);
				break;
			case EXFAT_ENTRY_TYPE_FILEINFO:
				visitor.VisitFileInfo(fCurrent);
				break;
			case EXFAT_ENTRY_TYPE_FILENAME:
				visitor.VisitFilename(fCurrent);
				break;
		}
	}
}


status_t
DirectoryIterator::_GetNext(uchar* name, size_t* _nameLength, ino_t* _id,
	EntryVisitor* visitor)
{
	size_t nameMax = *_nameLength;
	size_t nameIndex = 0;
	status_t status;
	int32 chunkCount = 1;
	while ((status = _NextEntry()) == B_OK) {
		TRACE("DirectoryIterator::_GetNext() %ld/%p, type 0x%x, offset %lld\n",
			fInode->Cluster(), fCurrent, fCurrent->type, fOffset);
		if (fCurrent->type == EXFAT_ENTRY_TYPE_FILE) {
			chunkCount = fCurrent->file.chunkCount;
			if (_id != NULL) {
				*_id = fInode->GetVolume()->GetIno(fCluster, fOffset - 1,
					fInode->ID());
			}
			if (visitor != NULL)
				visitor->VisitFile(fCurrent);
			TRACE("DirectoryIterator::_GetNext() File chunkCount %ld\n",
				chunkCount);
		} else if (fCurrent->type == EXFAT_ENTRY_TYPE_FILEINFO) {
			chunkCount--;
			TRACE("DirectoryIterator::_GetNext() Filename length %d\n",
				fCurrent->file_info.name_length);
			*_nameLength = fCurrent->file_info.name_length * 2;
			if (visitor != NULL)
				visitor->VisitFileInfo(fCurrent);
		} else if (fCurrent->type == EXFAT_ENTRY_TYPE_FILENAME) {
			TRACE("DirectoryIterator::_GetNext() Filename\n");
			memcpy((uint8*)name + nameIndex, fCurrent->name_label.name,
				sizeof(fCurrent->name_label.name));
			nameIndex += sizeof(fCurrent->name_label.name);
			name[nameIndex] = '\0';
			chunkCount--;
			if (visitor != NULL)
				visitor->VisitFilename(fCurrent);
		}

		if (chunkCount == 0 || nameIndex >= nameMax)
			break;
	}

	if (status == B_OK) {
		//*_nameLength = nameIndex;
#ifdef TRACE_EXFAT
		char utfName[EXFAT_FILENAME_MAX_LENGTH];
		size_t utfLen = EXFAT_FILENAME_MAX_LENGTH;
		unicode_to_utf8(name, nameIndex, (uint8*)utfName, &utfLen);
		TRACE("DirectoryIterator::_GetNext() Found %s %ld\n", utfName,
			*_nameLength);
#endif
	}

	return status;
}


status_t
DirectoryIterator::_NextEntry()
{
	if (fCurrent == NULL) {
		fsblock_t block;
		fInode->GetVolume()->ClusterToBlock(fCluster, block);
		block += (fOffset / fInode->GetVolume()->EntriesPerBlock())
			% (1 << fInode->GetVolume()->SuperBlock().BlocksPerClusterShift());
		TRACE("DirectoryIterator::_NextEntry() init to block %lld\n", block);
		fCurrent = (struct exfat_entry*)fBlock.SetTo(block)
			+ fOffset % fInode->GetVolume()->EntriesPerBlock();
	} else if ((fOffset % fInode->GetVolume()->EntriesPerBlock()) == 0) {	
		fsblock_t block;
		if ((fOffset % fInode->GetVolume()->EntriesPerCluster()) == 0) {
			fCluster = fInode->NextCluster(fCluster);
			if (fCluster == EXFAT_CLUSTER_END)
				return B_ENTRY_NOT_FOUND;
			fInode->GetVolume()->ClusterToBlock(fCluster, block);
		} else
			block = fBlock.BlockNumber() + 1;
		TRACE("DirectoryIterator::_NextEntry() block %lld\n", block);
		fCurrent = (struct exfat_entry*)fBlock.SetTo(block);
	} else
		fCurrent++;
	fOffset++;

	return fCurrent->type == 0 ? B_ENTRY_NOT_FOUND : B_OK;
}


