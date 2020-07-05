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

	fHeader = (ShortFormHeader*)(DIR_DFORK_PTR(fInode->Buffer()));
}


ShortDirectory::~ShortDirectory()
{
}


uint8
ShortDirectory::GetFileType(ShortFormEntry* entry)
{
	ASSERT(fInode->HasFileTypeField() == true);
	return entry->name[entry->namelen];
}


ShortFormEntry*
ShortDirectory::FirstEntry()
{
	return (ShortFormEntry*) ((char*) fHeader + HeaderSize());
}


size_t
ShortDirectory::HeaderSize()
{
	if (fHeader->i8count)
		return sizeof(ShortFormHeader);
	else
		return sizeof(ShortFormHeader) - sizeof(uint32);
}


xfs_ino_t
ShortDirectory::GetIno(ShortFormInodeUnion* inum)
{
	if (fHeader->i8count)
		return B_BENDIAN_TO_HOST_INT64(inum->i8);
	else
		return B_BENDIAN_TO_HOST_INT32(inum->i4);
}


xfs_ino_t
ShortDirectory::GetEntryIno(ShortFormEntry* entry)
{
	if (fInode->HasFileTypeField())
		return GetIno((ShortFormInodeUnion*)(entry->name
				+ entry->namelen + sizeof(uint8)));
	else
		return GetIno((ShortFormInodeUnion*)(entry->name + entry->namelen));
}


size_t
ShortDirectory::EntrySize(int namelen)
{
	return sizeof(ShortFormEntry) + namelen
			+ (fInode->HasFileTypeField()? sizeof(uint8) : 0)
			+ (fHeader->i8count? sizeof(uint64):sizeof(uint32));
}


status_t
ShortDirectory::Lookup(const char* name, size_t length, xfs_ino_t* ino)
{
	TRACE("ShortDirectory::Lookup\n");

	if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
		xfs_ino_t rootIno = fInode->GetVolume()->Root();
		if (strcmp(name, ".") == 0 || (rootIno == fInode->ID())) {
			*ino = fInode->ID();
			TRACE("ShortDirectory:Lookup: name: \".\" ino: (%d)\n", *ino);
			return B_OK;
		}
		*ino = GetIno(&fHeader->parent);
		TRACE("Parent: (%d)\n", *ino);
		return B_OK;
	}

	ShortFormEntry* entry = FirstEntry();
	TRACE("Length of first entry: (%d),offset of first entry:"
		"(%d)\n", entry->namelen, B_BENDIAN_TO_HOST_INT16(entry->offset.i));

	int status;
	for (int i = 0; i < fHeader->count; i++) {
		status = strncmp(name, (char*)entry->name, entry->namelen);
		if (status == 0) {
			*ino = GetEntryIno(entry);
			return B_OK;
		}
		entry = (ShortFormEntry*)
			((char*) entry + EntrySize(entry->namelen));
	}

	return B_ENTRY_NOT_FOUND;
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
		*ino = GetIno(&fHeader->parent);
		fTrack = 2;
		TRACE("ShortDirectory:GetNext: name: \"..\" ino: (%d)\n", *ino);
		return B_OK;
	}

	ShortFormEntry* entry = FirstEntry();
	TRACE("Length of first entry: (%d), offset of first entry:"
		"(%d)\n", entry->namelen, B_BENDIAN_TO_HOST_INT16(entry->offset.i));

	for (int i = 0; i < fHeader->count; i++) {
		uint16 curOffset = B_BENDIAN_TO_HOST_INT16(entry->offset.i);
		if (curOffset > fLastEntryOffset) {

			if (entry->namelen > *length)
				return B_BUFFER_OVERFLOW;

			fLastEntryOffset = curOffset;
			memcpy(name, entry->name, entry->namelen);
			name[entry->namelen] = '\0';
			*length = entry->namelen + 1;
			*ino = GetEntryIno(entry);

			TRACE("Entry found. Name: (%s), Length: (%ld),ino: (%ld)\n", name,
				*length, *ino);
			return B_OK;
		}
		entry = (ShortFormEntry*)((char*)entry + EntrySize(entry->namelen));
	}

	return B_ENTRY_NOT_FOUND;
}
