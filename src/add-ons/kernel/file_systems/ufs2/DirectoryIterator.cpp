/*
 * Copyright 2020 Suhel Mehta, mehtasuhel@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "DirectoryIterator.h"

#include <stdlib.h>
#include <string.h>

#include "Inode.h"

#define TRACE_UFS2
#ifdef TRACE_UFS2
#	define TRACE(x...) dprintf("\33[34mufs2:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif

#define ERROR(x...) dprintf("\33[34mufs2:\33[0m " x)


DirectoryIterator::DirectoryIterator(Inode* inode)
	:
	fInode(inode)
{
	fOffset = 0;
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
DirectoryIterator::Lookup(const char* name, ino_t* _id)
{
	if (strcmp(name, ".") == 0) {
		*_id = fInode->ID();
		return B_OK;
	}

	char getname[B_FILE_NAME_LENGTH + 1];

	status_t status;
	while(true) {
		size_t len = sizeof (getname) - 1;
		status = GetNext(getname, &len, _id);
		if (status != B_OK)
			return status;
		if (strcmp(getname, name) == 0)
			return B_OK;
	}

}


status_t
DirectoryIterator::GetNext(char* name, size_t* _nameLength, ino_t* _id)
{
	dir direct;
	size_t size = sizeof(dir);
	status_t status = fInode->ReadAt(fOffset, (uint8_t*)&direct, &size);
	if (size < 8 || direct.reclen < 8)
		return B_ENTRY_NOT_FOUND;
	if (status == B_OK) {
		fOffset += direct.reclen;

		if (direct.next_ino > 0) {
			if ((size_t) (direct.namlen + 1) > *_nameLength)
				return B_BUFFER_OVERFLOW;
			strlcpy(name, direct.name, direct.namlen + 1);
			*_id = direct.next_ino;
			*_nameLength = direct.namlen;
			return B_OK;
		}

		return B_ENTRY_NOT_FOUND;
	}

	return B_ERROR;
}


status_t
DirectoryIterator::Rewind()
{
	fOffset = 0;
	return B_OK;
}
