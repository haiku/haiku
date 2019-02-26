/*
 * Copyright 2011, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2014 Haiku, Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval, korli@users.berlios.de
 *		John Scipione, jscipione@gmail.com
 */


#include "DirectoryIterator.h"

#include <stdlib.h>

#include "convertutf.h"

#include "Inode.h"


//#define TRACE_EXFAT
#ifdef TRACE_EXFAT
#	define TRACE(x...) dprintf("\33[34mexfat:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif

#define ERROR(x...) dprintf("\33[34mexfat:\33[0m " x)


//	#pragma mark - DirectoryIterator


DirectoryIterator::DirectoryIterator(Inode* inode)
	:
	fOffset(-2),
	fCluster(inode->StartCluster()),
	fInode(inode),
	fBlock(inode->GetVolume()),
	fCurrent(NULL)
{
	TRACE("DirectoryIterator::DirectoryIterator() %" B_PRIu32 "\n", fCluster);
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
	if (fCluster == EXFAT_CLUSTER_END)
		return B_ENTRY_NOT_FOUND;

	if (fOffset == -2) {
		if (*_nameLength < 3)
			return B_BUFFER_OVERFLOW;

		*_nameLength = 2;
		strlcpy(name, "..", *_nameLength + 1);
		if (fInode->ID() == 1)
			*_id = fInode->ID();
		else
			*_id = fInode->Parent();

		fOffset = -1;
		TRACE("DirectoryIterator::GetNext() found \"..\"\n");

		return B_OK;
	} else if (fOffset == -1) {
		if (*_nameLength < 2)
			return B_BUFFER_OVERFLOW;

		*_nameLength = 1;
		strlcpy(name, ".", *_nameLength + 1);
		*_id = fInode->ID();
		fOffset = 0;
		TRACE("DirectoryIterator::GetNext() found \".\"\n");

		return B_OK;
	}

	size_t utf16CodeUnitCount = EXFAT_FILENAME_MAX_LENGTH / sizeof(uint16);
	uint16 utf16Name[utf16CodeUnitCount];
	status_t status = _GetNext(utf16Name, &utf16CodeUnitCount, _id, visitor);
	if (status == B_OK && utf16CodeUnitCount > 0) {
		ssize_t lengthOrStatus = utf16le_to_utf8(utf16Name, utf16CodeUnitCount,
			name, *_nameLength);
		if (lengthOrStatus < 0) {
			status = (status_t)lengthOrStatus;
			if (status == B_NAME_TOO_LONG)
				*_nameLength = strlen(name);
		} else
			*_nameLength = (size_t)lengthOrStatus;
	}

	if (status == B_OK) {
		TRACE("DirectoryIterator::GetNext() cluster: %" B_PRIu32 " id: "
			"%" B_PRIdINO " name: \"%s\", length: %zu\n", fInode->Cluster(),
			*_id, name, *_nameLength);
	} else if (status != B_ENTRY_NOT_FOUND) {
		ERROR("DirectoryIterator::GetNext() (%s)\n", strerror(status));
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

	size_t utf16CodeUnitCount = EXFAT_FILENAME_MAX_LENGTH / sizeof(uint16);
	uint16 utf16Name[utf16CodeUnitCount];
	while (_GetNext(utf16Name, &utf16CodeUnitCount, _id) == B_OK) {
		char utf8Name[nameLength + 1];
		ssize_t lengthOrStatus = utf16le_to_utf8(utf16Name, utf16CodeUnitCount,
			utf8Name, sizeof(utf8Name));
		if (lengthOrStatus > 0 && (size_t)lengthOrStatus == nameLength
			&& strncmp(utf8Name, name, nameLength) == 0) {
			TRACE("DirectoryIterator::Lookup() found ID %" B_PRIdINO "\n",
				*_id);
			return B_OK;
		}
		utf16CodeUnitCount = EXFAT_FILENAME_MAX_LENGTH / sizeof(uint16);
	}

	TRACE("DirectoryIterator::Lookup() not found %s\n", name);

	return B_ENTRY_NOT_FOUND;
}


status_t
DirectoryIterator::LookupEntry(EntryVisitor* visitor)
{
	fCluster = fInode->Cluster();
	fOffset = fInode->Offset();

	size_t utf16CodeUnitCount = EXFAT_FILENAME_MAX_LENGTH / sizeof(uint16);
	uint16 utf16Name[utf16CodeUnitCount];
	return _GetNext(utf16Name, &utf16CodeUnitCount, NULL, visitor);
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
DirectoryIterator::_GetNext(uint16* utf16Name, size_t* _codeUnitCount,
	ino_t* _id, EntryVisitor* visitor)
{
	size_t nameMax = *_codeUnitCount;
	size_t nameIndex = 0;
	status_t status;
	int32 chunkCount = 1;

	while ((status = _NextEntry()) == B_OK) {
		TRACE("DirectoryIterator::_GetNext() %" B_PRIu32 "/%p, type 0x%x, "
			"offset %" B_PRId64 "\n", fInode->Cluster(), fCurrent,
			fCurrent->type, fOffset);
		if (fCurrent->type == EXFAT_ENTRY_TYPE_FILE) {
			chunkCount = fCurrent->file.chunkCount;
			if (_id != NULL) {
				*_id = fInode->GetVolume()->GetIno(fCluster, fOffset - 1,
					fInode->ID());
			}
			TRACE("DirectoryIterator::_GetNext() File chunkCount %" B_PRId32
				"\n", chunkCount);
			if (visitor != NULL)
				visitor->VisitFile(fCurrent);
		} else if (fCurrent->type == EXFAT_ENTRY_TYPE_FILEINFO) {
			chunkCount--;
			*_codeUnitCount = (size_t)fCurrent->file_info.name_length;
			TRACE("DirectoryIterator::_GetNext() Filename chunk: %" B_PRId32
				", code unit count: %" B_PRIu8 "\n", chunkCount, *_codeUnitCount);
			if (visitor != NULL)
				visitor->VisitFileInfo(fCurrent);
		} else if (fCurrent->type == EXFAT_ENTRY_TYPE_FILENAME) {
			chunkCount--;
			size_t utf16Length = sizeof(fCurrent->file_name.name);
			memcpy(utf16Name + nameIndex, fCurrent->file_name.name, utf16Length);
			nameIndex += utf16Length / sizeof(uint16);
			TRACE("DirectoryIterator::_GetNext() Filename index: %zu\n",
				nameIndex);
			if (visitor != NULL)
				visitor->VisitFilename(fCurrent);
		}

		if (chunkCount == 0 || nameIndex >= nameMax)
			break;
	}

#ifdef TRACE_EXFAT
	if (status == B_OK) {
		size_t utf8Length = B_FILE_NAME_LENGTH * 4;
		char utf8Name[utf8Length + 1];
		ssize_t length = utf16le_to_utf8(utf16Name, *_codeUnitCount, utf8Name,
			utf8Length);
		if (length > 0) {
			TRACE("DirectoryIterator::_GetNext() found name: \"%s\", "
				"length: %d\n", utf8Name, length);
		}
	}
#endif

	return status;
}


status_t
DirectoryIterator::_NextEntry()
{
	if (fCurrent == NULL) {
		fsblock_t block;
		if (fInode->GetVolume()->ClusterToBlock(fCluster, block) != B_OK)
			return B_BAD_DATA;
		block += (fOffset / fInode->GetVolume()->EntriesPerBlock())
			% (1 << fInode->GetVolume()->SuperBlock().BlocksPerClusterShift());
		TRACE("DirectoryIterator::_NextEntry() init to block %" B_PRIu64 "\n",
			block);
		fCurrent = (struct exfat_entry*)fBlock.SetTo(block)
			+ fOffset % fInode->GetVolume()->EntriesPerBlock();
	} else if ((fOffset % fInode->GetVolume()->EntriesPerBlock()) == 0) {
		fsblock_t block;
		if ((fOffset % fInode->GetVolume()->EntriesPerCluster()) == 0) {
			fCluster = fInode->NextCluster(fCluster);
			if (fCluster == EXFAT_CLUSTER_END)
				return B_ENTRY_NOT_FOUND;

			if (fInode->GetVolume()->ClusterToBlock(fCluster, block) != B_OK)
				return B_BAD_DATA;
		} else
			block = fBlock.BlockNumber() + 1;

		TRACE("DirectoryIterator::_NextEntry() block %" B_PRIu64 "\n", block);
		fCurrent = (struct exfat_entry*)fBlock.SetTo(block);
	} else
		fCurrent++;

	fOffset++;
	return fCurrent->type == 0 ? B_ENTRY_NOT_FOUND : B_OK;
}
