/*
 * Copyright 2011, Jérôme Duval, korli@users.berlios.de.
 * This file may be used under the terms of the MIT License.
 */


#include "AttributeIterator.h"


//#define TRACE_BTRFS
#ifdef TRACE_BTRFS
#	define TRACE(x...) dprintf("\33[34mbtrfs:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif
#	define ERROR(x...) dprintf("\33[34mbtrfs:\33[0m " x)


AttributeIterator::AttributeIterator(Inode* inode)
	:
	fOffset(-1ULL),
	fInode(inode),
	fIterator(NULL)
{
	btrfs_key key;
	key.SetType(BTRFS_KEY_TYPE_XATTR_ITEM);
	key.SetObjectID(inode->ID());
	key.SetOffset(BTREE_BEGIN);
	fIterator = new(std::nothrow) TreeIterator(inode->GetVolume()->FSTree(),
		key);
}


AttributeIterator::~AttributeIterator()
{
	delete fIterator;
	fIterator = NULL;
}


status_t
AttributeIterator::InitCheck()
{
	return fIterator != NULL ? B_OK : B_NO_MEMORY;
}


status_t
AttributeIterator::GetNext(char* name, size_t* _nameLength)
{
	btrfs_dir_entry* entries;
	uint32 entries_length;
	status_t status = fIterator->GetPreviousEntry((void**)&entries,
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

	TRACE("DirectoryIterator::GetNext() entries_length %ld name_length %d\n",
		entries_length, entry->NameLength());

	memcpy(name, entry + 1, entry->NameLength());
	name[entry->NameLength()] = '\0';
	*_nameLength = entry->NameLength();
	free(entries);

	return B_OK;
}


status_t
AttributeIterator::Rewind()
{
	fIterator->Rewind();
	fOffset = -1ULL;
	return B_OK;
}
