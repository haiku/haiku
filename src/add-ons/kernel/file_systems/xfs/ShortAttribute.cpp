/*
 * Copyright 2022, Raghav Sharma, raghavself28@gmail.com
 * Distributed under the terms of the MIT License.
 */


#include "ShortAttribute.h"


ShortAttribute::ShortAttribute(Inode* inode)
	:
	fInode(inode),
	fName(NULL)
{
	fHeader = (AShortFormHeader*)(DIR_AFORK_PTR(fInode->Buffer(),
		fInode->CoreInodeSize(), fInode->ForkOffset()));

	fLastEntryOffset = 0;
}


ShortAttribute::~ShortAttribute()
{
}


uint32
ShortAttribute::_DataLength(AShortFormEntry* entry)
{
	return entry->namelen + entry->valuelen;
}


AShortFormEntry*
ShortAttribute::_FirstEntry()
{
	return (AShortFormEntry*) ((char*) fHeader + sizeof(AShortFormHeader));
}


status_t
ShortAttribute::Open(const char* name, int openMode, attr_cookie** _cookie)
{
	TRACE("ShortAttribute::Open\n");
	status_t status;

	size_t length = strlen(name);
	status = Lookup(name, &length);
	if (status < B_OK)
		return status;

	attr_cookie* cookie = new(std::nothrow) attr_cookie;
	if (cookie == NULL)
		return B_NO_MEMORY;

	fName = name;

	// initialize the cookie
	strlcpy(cookie->name, fName, B_ATTR_NAME_LENGTH);
	cookie->open_mode = openMode;
	cookie->create = false;

	*_cookie = cookie;
	return B_OK;
}


status_t
ShortAttribute::Stat(attr_cookie* cookie, struct stat& stat)
{
	TRACE("Short Attribute : Stat\n");

	fName = cookie->name;

	size_t namelength = strlen(fName);

	// check if this attribute exists
	status_t status = Lookup(fName, &namelength);
	if (status != B_OK)
		return status;

	// We have valid attribute entry to stat
	stat.st_type = B_XATTR_TYPE;
	stat.st_size = _DataLength(fEntry);

	return B_OK;
}


status_t
ShortAttribute::Read(attr_cookie* cookie, off_t pos, uint8* buffer, size_t* length)
{
	TRACE("Short Attribute : Read\n");

	if (pos < 0)
		return B_BAD_VALUE;

	fName = cookie->name;
	uint32 lengthToRead = 0;

	size_t namelength = strlen(fName);

	status_t status = Lookup(fName, &namelength);

	if(status != B_OK) 
		return status;

	if (pos + *length > fEntry->valuelen)
		lengthToRead = fEntry->valuelen - pos;
	else
		lengthToRead = *length;

	char* ptrToOffset = (char*) fHeader + sizeof(AShortFormHeader)
		+ 3 * sizeof(uint8) + fEntry->namelen;

	memcpy(buffer, ptrToOffset, lengthToRead);

	*length = lengthToRead;

	return B_OK;
}


status_t
ShortAttribute::GetNext(char* name, size_t* nameLength)
{
	TRACE("Short Attribute : GetNext\n");

	AShortFormEntry* entry = _FirstEntry();
	uint16 curOffset = 1;
	for (int i = 0; i < fHeader->count; i++) {
		if (curOffset > fLastEntryOffset) {

			fLastEntryOffset = curOffset;

			char* PtrToOffset = (char*)entry + 3 * sizeof(uint8);

			memcpy(name, PtrToOffset, entry->namelen);
			name[entry->namelen] = '\0';
			*nameLength = entry->namelen + 1;
			TRACE("Entry found name : %s, namelength : %ld", name, *nameLength);
			return B_OK;
		}
		entry = (AShortFormEntry*)((char*)entry + 3 * sizeof(uint8) + _DataLength(entry));
		curOffset += 3 * sizeof(uint8) + _DataLength(entry);
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
ShortAttribute::Lookup(const char* name, size_t* nameLength)
{
	TRACE("Short Attribute : Lookup\n");

	AShortFormEntry* entry = _FirstEntry();

	int status;

	for (int i = 0; i < fHeader->count; i++) {
		char* PtrToOffset = (char*)entry + 3 * sizeof(uint8);
		status = strncmp(name, PtrToOffset, *nameLength);
		if (status == 0) {
			fEntry = entry;
			return B_OK;
		}
		entry = (AShortFormEntry*)((char*)entry + 3 * sizeof(uint8) + _DataLength(entry));
	}

	return B_ENTRY_NOT_FOUND;
}
