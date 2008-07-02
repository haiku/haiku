/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */


#include "DirectoryIterator.h"

#include <string.h>

#include "Inode.h"


//#define TRACE_EXT2
#ifdef TRACE_EXT2
#	define TRACE(x...) dprintf("\33[34mext2:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif


DirectoryIterator::DirectoryIterator(Inode* inode)
	:
	fInode(inode),
	fOffset(0)
{
}


DirectoryIterator::~DirectoryIterator()
{
}


status_t
DirectoryIterator::GetNext(char* name, size_t* _nameLength, ino_t* _id)
{
	if (fOffset + sizeof(ext2_dir_entry) >= fInode->Size())
		return B_ENTRY_NOT_FOUND;

	ext2_dir_entry entry;

	while (true) {
		size_t length = ext2_dir_entry::MinimumSize();
		status_t status = fInode->ReadAt(fOffset, (uint8*)&entry, &length);
		if (status != B_OK)
			return status;
		if (length < ext2_dir_entry::MinimumSize() || entry.Length() == 0)
			return B_ENTRY_NOT_FOUND;
		if (!entry.IsValid())
			return B_BAD_DATA;

		if (entry.NameLength() != 0)
			break;

		fOffset += entry.Length();
	}

	TRACE("offset %Ld: entry ino %lu, length %u, name length %u, type %u\n",
		fOffset, entry.InodeID(), entry.Length(), entry.NameLength(),
		entry.FileType());

	// read name

	size_t length = entry.NameLength();
	status_t status = fInode->ReadAt(fOffset + ext2_dir_entry::MinimumSize(),
		(uint8*)entry.name, &length);
	if (status == B_OK) {
		if (*_nameLength < length)
			length = *_nameLength - 1;
		memcpy(name, entry.name, length);
		name[length] = '\0';

		*_id = entry.InodeID();
		*_nameLength = length;

		fOffset += entry.Length();
	}

	return status;
}


status_t
DirectoryIterator::Rewind()
{
	fOffset = 0;
	return B_OK;
}
