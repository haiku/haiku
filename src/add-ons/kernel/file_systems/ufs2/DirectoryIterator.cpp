/*
 * Copyright 2020 Suhel Mehta, mehtasuhel@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "DirectoryIterator.h"

#include <stdlib.h>

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
	TRACE("DirectoryIterator::DirectoryIterator() \n");
}


int DirectoryIterator::countDir = 0;

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
	int count = 0;
	if (strcmp(name, ".") == 0) {
		*_id = fInode->ID();
		return B_OK;
	} else if (strcmp(name, "..") == 0) {
		if (fInode->ID() == 1)
			*_id = fInode->ID();

	/*	else
			*_id = fInode->Parent();*/

	}
	while(_GetNext(name, &length, _id, count) != B_OK) {
		count++;
	}
	return B_OK;
}


status_t
DirectoryIterator::GetNext(char* name, size_t* _nameLength, ino_t* _id)
{
	TRACE("In GetNext function\n");
	int64_t offset = fInode->GetBlockPointer() * MINBSIZE;
	dir direct;
	dir_info direct_info;
	int fd = fInode->GetVolume()->Device();
	if (read_pos(fd, offset, &direct_info,
				sizeof(dir_info)) != sizeof(dir_info)) {
		return B_BAD_DATA;
	}

	offset = offset + sizeof(dir_info) + 16 * countDir;
	if (read_pos(fd, offset, &direct, sizeof(dir)) != sizeof(dir)) {
		return B_BAD_DATA;
	}

	if (direct.next_ino != 0) {
	strlcpy(name, direct.name, sizeof(name));
	*_id = direct.next_ino;
	*_nameLength = direct.namlen;
	countDir++;
	return B_OK;
	}
	countDir = 0;
	return B_ENTRY_NOT_FOUND;
}


status_t
DirectoryIterator::_GetNext(const char* name, size_t* _nameLength,
						ino_t* _id, int count)
{
	TRACE("In GetNext function\n");
	int64_t offset = fInode->GetBlockPointer() * MINBSIZE;
	dir direct;
	dir_info direct_info;
	int fd = fInode->GetVolume()->Device();
	if (read_pos(fd, offset, &direct_info,
				sizeof(dir_info)) != sizeof(dir_info)) {
		return B_BAD_DATA;
	}
	if(strcmp(name, "..") == 0)
	{
		*_id = direct_info.dotdot_ino;
		return B_OK;
	}

	offset = offset + sizeof(dir_info) + (16 * count);
	if (read_pos(fd, offset, &direct, sizeof(dir)) != sizeof(dir)) {
		return B_BAD_DATA;
	}

	char getname;
	strlcpy(&getname, direct.name, sizeof(name));
	if(strcmp(name, &getname) == 0) {
		*_id = direct.next_ino;
		return B_OK;
	}
	else {
	return B_ENTRY_NOT_FOUND;
	}
}
