/*
 * Copyright 2011-2013, Jérôme Duval, korli@users.berlios.de.
 * This file may be used under the terms of the MIT License.
 */


#include "DirectoryIterator.h"
#include "CRCTable.h"


//#define TRACE_BTRFS
#ifdef TRACE_BTRFS
#	define TRACE(x...) dprintf("\33[34mbtrfs:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif
#	define ERROR(x...) dprintf("\33[34mbtrfs:\33[0m " x)


DirectoryIterator::DirectoryIterator(Inode* inode)
	:
	fOffset(0),
	fInode(inode),
	fIterator(NULL)
{
	btrfs_key key;
	key.SetType(BTRFS_KEY_TYPE_DIR_INDEX);
	key.SetObjectID(inode->ID());
	fIterator = new(std::nothrow) TreeIterator(inode->GetVolume()->FSTree(),
		key);
}


DirectoryIterator::~DirectoryIterator()
{
	delete fIterator;
}


status_t
DirectoryIterator::InitCheck()
{
	return fIterator != NULL ? B_OK : B_NO_MEMORY;
}


status_t
DirectoryIterator::GetNext(char* name, size_t* _nameLength, ino_t* _id)
{
	if (fOffset == 0) {
		if (*_nameLength < 3)
			return B_BUFFER_OVERFLOW;
		*_nameLength = 2;
		strlcpy(name, "..", *_nameLength + 1);
		*_id = fInode->ID();
		fOffset = 1;
		return B_OK;
	} else if (fOffset == 1) {
		if (*_nameLength < 2)
			return B_BUFFER_OVERFLOW;
		*_nameLength = 1;
		strlcpy(name, ".", *_nameLength + 1);
		fOffset = 2;
		if (fInode->ID() == BTRFS_OBJECT_ID_CHUNK_TREE) {
			*_id = fInode->ID();
			return B_OK;
		}
		return fInode->FindParent(_id);
	}

	btrfs_key key;
	btrfs_dir_entry* entries;
	size_t entries_length;
	status_t status = fIterator->GetNextEntry(key, (void**)&entries,
		&entries_length);
	if (status != B_OK)
		return status;

	btrfs_dir_entry* entry = entries;
	uint16 current = 0;
	while (current < entries_length) {
		current += entry->Length();
		break;
		// TODO there could be several entries with the same name hash
		entry = (btrfs_dir_entry*)((uint8*)entry + entry->Length());
	}

	size_t length = entry->NameLength();

	TRACE("DirectoryIterator::GetNext() entries_length %ld name_length %d\n",
		entries_length, entry->NameLength());

	if (length + 1 > *_nameLength) {
		free(entries);
		return B_BUFFER_OVERFLOW;
	}

	memcpy(name, entry + 1, length);
	name[length] = '\0';
	*_nameLength = length;
	*_id = entry->InodeID();
	free(entries);

	return B_OK;
}


status_t
DirectoryIterator::Lookup(const char* name, size_t nameLength, ino_t* _id)
{
	if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
		if (strcmp(name, ".") == 0
			|| fInode->ID() == BTRFS_OBJECT_ID_CHUNK_TREE) {
			*_id = fInode->ID();
			return B_OK;
		}
		return fInode->FindParent(_id);
	}

	uint32 hash = calculate_crc((uint32)~1, (uint8*)name, nameLength);
	btrfs_key key;
	key.SetType(BTRFS_KEY_TYPE_DIR_ITEM);
	key.SetObjectID(fInode->ID());
	key.SetOffset(hash);

	btrfs_dir_entry* entries;
	size_t length;
	status_t status = fInode->GetVolume()->FSTree()->FindExact(key,
		(void**)&entries, &length);
	if (status != B_OK) {
		TRACE("DirectoryIterator::Lookup(): Couldn't find entry with hash %" B_PRIu32
			" \"%s\"\n", hash, name);
		return status;
	}

	btrfs_dir_entry* entry = entries;
	uint16 current = 0;
	while (current < length) {
		current += entry->Length();
		break;
		// TODO there could be several entries with the same name hash
		entry = (btrfs_dir_entry*)((uint8*)entry + entry->Length());
	}

	TRACE("DirectoryIterator::Lookup() entries_length %ld name_length %d\n",
		length, entry->NameLength());

	*_id = entry->InodeID();
	free(entries);

	return B_OK;
}


status_t
DirectoryIterator::Rewind()
{
	fIterator->Rewind();
	fOffset = BTREE_BEGIN;
	return B_OK;
}

