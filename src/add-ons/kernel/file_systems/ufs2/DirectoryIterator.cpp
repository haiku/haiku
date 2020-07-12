/*
 * Copyright 2020 Suhel Mehta, mehtasuhel@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "DirectoryIterator.h"

#include <stdlib.h>

#include "Inode.h"

//#define TRACE_UFS2
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
	fOffset = fInode->GetBlockPointer() * MINBSIZE;
	TRACE("DirectoryIterator::DirectoryIterator() \n");
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
DirectoryIterator::Lookup(const char* name, size_t length, ino_t* _id)
{
	if (strcmp(name, ".") == 0) {
		*_id = fInode->ID();
		return B_OK;
	}

	char getname[B_FILE_NAME_LENGTH];

	status_t status;
	while(true) {
		status = GetNext(getname, &length, _id);
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
	int fd = fInode->GetVolume()->Device();

	if (read_pos(fd, fOffset, &direct, sizeof(dir)) != sizeof(dir)) {
		return B_BAD_DATA;
	}

	int remainder = direct.namlen % 4;
	if(remainder != 0) {
		remainder = 4 - remainder;
		remainder = direct.namlen + remainder;
	} else {
		remainder = direct.namlen + 4;
	}

	fOffset = fOffset + 8 + remainder;

	if (direct.next_ino > 0) {
		TRACE("direct.next_ino %d\n",direct.next_ino);

		strlcpy(name, direct.name, remainder);
		*_id = direct.next_ino;
		*_nameLength = direct.namlen;
		return B_OK;
	}

	return B_ENTRY_NOT_FOUND;

}

