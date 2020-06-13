/*
 * Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "ShortDirectory.h"


ShortDirectory::ShortDirectory(Inode* inode)
	:
	fInode(inode),
	fTrack(0)
{
	memcpy(&fHeader,
		DIR_DFORK_PTR(fInode->Buffer()), sizeof(xfs_dir2_sf_hdr_t));
}


ShortDirectory::~ShortDirectory()
{
}


xfs_ino_t
ShortDirectory::GetParentIno()
{
	TRACE("GetParentIno: \n");
	if (fHeader.i8count > 0)
		return B_BENDIAN_TO_HOST_INT64(fHeader.parent.i8);
	else
		return B_BENDIAN_TO_HOST_INT32(fHeader.parent.i4);
}


status_t
ShortDirectory::Lookup(const char* name, size_t length, xfs_ino_t* ino)
{
	if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
		xfs_ino_t rootIno = fInode->GetVolume()->Root();
		if (strcmp(name, ".") == 0 || (rootIno == fInode->ID())) {
			*ino = fInode->ID();
			TRACE("ShortDirectory:Lookup: name: \".\" ino: (%d)\n", *ino);
			return B_OK;
		}
		*ino = GetParentIno();
		TRACE("Parent: (%d)\n", *ino);
		return B_OK;
	}

	//TODO: Other entries
	return B_NOT_SUPPORTED;
}


status_t
ShortDirectory::GetNext(char* name, size_t* length, xfs_ino_t* ino)
{
	if (fTrack == 0) {
		// Return '.'
		if (*length < 2)
			return B_BUFFER_OVERFLOW;
		*length = 2;
		strlcpy(name, ".", *length + 1);
		*ino = fInode->ID();
		fTrack = 1;
		TRACE("ShortDirectory:GetNext: name: \".\" ino: (%d)\n", *ino);
		return B_OK;
	}
	if (fTrack == 1) {
		// Return '..'
		if (*length < 3)
			return B_BUFFER_OVERFLOW;
		*length = 3;
		strlcpy(name, "..", *length + 1);
		*ino = GetParentIno();
		fTrack = 2;
		TRACE("ShortDirectory:GetNext: name: \"..\" ino: (%d)\n", *ino);
		return B_OK;
	}

	// TODO: Now iterate through sf entries
	return B_NOT_SUPPORTED;
}
