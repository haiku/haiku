/*
 * Copyright 2010, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2010, François Revol, <revol@free.fr>.
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */

//!	connection between pure inode and kernel_interface attributes


#include "Attribute.h"
#include "Utility.h"

#include <stdio.h>


//#define TRACE_EXT2
#ifdef TRACE_EXT2
#	define TRACE(x...) dprintf("\33[34mext2:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif


Attribute::Attribute(Inode* inode)
	:
	fVolume(inode->GetVolume()),
	fBlock(fVolume),
	fInode(inode),
	fBodyEntry(NULL),
	fBlockEntry(NULL),
	fName(NULL)
{
	
}


Attribute::Attribute(Inode* inode, attr_cookie* cookie)
	:
	fVolume(inode->GetVolume()),
	fBlock(fVolume),
	fInode(inode),
	fBodyEntry(NULL),
	fBlockEntry(NULL),
	fName(NULL)
{
	Find(cookie->name);
}


Attribute::~Attribute()
{
	Put();
}


status_t
Attribute::InitCheck()
{
	return (fBodyEntry != NULL || fBlockEntry != NULL) ? B_OK : B_NO_INIT;
}


status_t
Attribute::CheckAccess(const char* name, int openMode)
{
	return fInode->CheckPermissions(open_mode_to_access(openMode)
		| (openMode & O_TRUNC ? W_OK : 0));
}


status_t
Attribute::Find(const char* name)
{
	return _Find(name, -1);
}


status_t
Attribute::Find(int32 index)
{
	return _Find(NULL, index);
}


status_t
Attribute::GetName(char* name, size_t* _nameLength)
{
	if (fBodyEntry == NULL && fBlockEntry == NULL)
		return B_NO_INIT;
	if (fBodyEntry != NULL)
		return _PrefixedName(fBodyEntry, name, _nameLength);
	else
		return _PrefixedName(fBlockEntry, name, _nameLength);
}


void
Attribute::Put()
{
	if (fBodyEntry != NULL) {
		recursive_lock_unlock(&fInode->SmallDataLock());
		fBlock.Unset();
		fBodyEntry = NULL;
	}

	if (fBlockEntry != NULL) {
		fBlock.Unset();
		fBlockEntry = NULL;
	}
}


status_t
Attribute::Create(const char* name, type_code type, int openMode,
	attr_cookie** _cookie)
{
	status_t status = CheckAccess(name, openMode);
	if (status < B_OK)
		return status;

	attr_cookie* cookie = new(std::nothrow) attr_cookie;
	if (cookie == NULL)
		return B_NO_MEMORY;

	fName = name;

	// initialize the cookie
	strlcpy(cookie->name, fName, B_ATTR_NAME_LENGTH);
	cookie->type = type;
	cookie->open_mode = openMode;
	cookie->create = true;

	if (Find(name) == B_OK) {
		// attribute already exists
		if ((openMode & O_TRUNC) != 0)
			_Truncate();
	}
	*_cookie = cookie;
	return B_OK;
}


status_t
Attribute::Open(const char* name, int openMode, attr_cookie** _cookie)
{
	TRACE("Open\n");
	status_t status = CheckAccess(name, openMode);
	if (status < B_OK)
		return status;

	status = Find(name);
	if (status < B_OK)
		return status;

	attr_cookie* cookie = new(std::nothrow) attr_cookie;
	if (cookie == NULL)
		return B_NO_MEMORY;

	// initialize the cookie
	strlcpy(cookie->name, fName, B_ATTR_NAME_LENGTH);
	cookie->open_mode = openMode;
	cookie->create = false;

	// Should we truncate the attribute?
	if ((openMode & O_TRUNC) != 0)
		_Truncate();

	*_cookie = cookie;
	return B_OK;
}


status_t
Attribute::Stat(struct stat& stat)
{
	TRACE("Stat\n");
	if (fBodyEntry == NULL && fBlockEntry == NULL)
		return B_NO_INIT;

	stat.st_type = B_XATTR_TYPE;
	
	if (fBodyEntry != NULL)
		stat.st_size = fBodyEntry->ValueSize();
	else if (fBlockEntry != NULL)
		stat.st_size = fBlockEntry->ValueSize();
	
	return B_OK;
}


status_t
Attribute::Read(attr_cookie* cookie, off_t pos, uint8* buffer, size_t* _length)
{
	if (fBodyEntry == NULL && fBlockEntry == NULL)
		return B_NO_INIT;

	if (pos < 0LL)
		return ERANGE;

	size_t length = *_length;
	const uint8* start = (uint8 *)fBlock.Block();
	if (fBlockEntry != NULL) {
		pos += fBlockEntry->ValueOffset();
		if (((uint32)pos + length) > fVolume->BlockSize()
			|| length > fBlockEntry->ValueSize())
			return ERANGE;
	} else {
		start += fVolume->InodeBlockIndex(fInode->ID()) * fVolume->InodeSize();
		const uint8* end = start + fVolume->InodeSize();
		start += EXT2_INODE_NORMAL_SIZE + fInode->Node().ExtraInodeSize()
			+ sizeof(uint32);
		pos += fBodyEntry->ValueOffset();
		if ((pos + length) > (end - start) || length > fBodyEntry->ValueSize())
			return ERANGE;
	}
	memcpy(buffer, start + (uint32)pos, length);

	*_length = length;
	return B_OK;
}


status_t
Attribute::Write(Transaction& transaction, attr_cookie* cookie, off_t pos,
	const uint8* buffer, size_t* _length, bool* _created)
{
	if (!cookie->create && fBodyEntry == NULL && fBlockEntry == NULL)
		return B_NO_INIT;

	// TODO: Implement
	return B_ERROR;
}


status_t
Attribute::_Truncate()
{
	// TODO: Implement
	return B_ERROR;
}


status_t
Attribute::_Find(const char* name, int32 index)
{
	Put();

	fName = name;

	// try to find it in the small data region
	if (fInode->HasExtraAttributes() 
		&& recursive_lock_lock(&fInode->SmallDataLock()) == B_OK) {
		off_t blockNum;
		fVolume->GetInodeBlock(fInode->ID(), blockNum);
		
		if (blockNum != 0) {
			fBlock.SetTo(blockNum);
			const uint8* start = fBlock.Block() 
				+ fVolume->InodeBlockIndex(fInode->ID()) * fVolume->InodeSize();
			const uint8* end = start + fVolume->InodeSize();
			int32 count = 0;
			if (_FindAttributeBody(start + EXT2_INODE_NORMAL_SIZE 
				+ fInode->Node().ExtraInodeSize(), end, name, index, &count, 
				&fBodyEntry) == B_OK)
				return B_OK;
			index -= count;
		}

		recursive_lock_unlock(&fInode->SmallDataLock());
		fBlock.Unset();
	}

	// then, search in the attribute directory
	if (fInode->Node().ExtendedAttributesBlock() != 0) {
		fBlock.SetTo(fInode->Node().ExtendedAttributesBlock());
		if (_FindAttributeBlock(fBlock.Block(), 
			fBlock.Block() + fVolume->BlockSize(), name, index, NULL, 
			&fBlockEntry) == B_OK)
			return B_OK;
		fBlock.Unset();
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
Attribute::_FindAttributeBody(const uint8* start, const uint8* end, 
	const char* name, int32 index, int32 *count, ext2_xattr_entry** _entry)
{
	TRACE("_FindAttributeBody %p %p %s\n", start, end, name);
	if (*((uint32*)start) != EXT2_XATTR_MAGIC)
		return B_BAD_DATA;
	return _FindAttribute(start + sizeof(uint32), end, name, index, count,
		_entry);
}


status_t
Attribute::_FindAttributeBlock(const uint8* start, const uint8* end, const char* name,
	int32 index, int32 *count, ext2_xattr_entry** _entry)
{
	TRACE("_FindAttributeBlock %p %p %s\n", start, end, name);
	ext2_xattr_header *header = (ext2_xattr_header*)start;
	if (!header->IsValid())
		return B_BAD_DATA;
	
	return _FindAttribute(start + sizeof(ext2_xattr_header), end, name, index, 
		count, _entry);
}


status_t
Attribute::_FindAttribute(const uint8* start, const uint8* end, const char* name,
	int32 index, int32 *count, ext2_xattr_entry** _entry)
{
	TRACE("_FindAttribute %p %p %s\n", start, end, name);
	char buffer[EXT2_XATTR_NAME_LENGTH];
	
	int32 i = 0;
	while (start < end) {
		ext2_xattr_entry* entry = (ext2_xattr_entry*)start;
		if (!entry->IsValid())
			break;
		
		size_t length = EXT2_XATTR_NAME_LENGTH;
		if ((name != NULL && _PrefixedName(entry, buffer, &length) == B_OK
				&& strncmp(name, buffer, length) == 0) || index == i) {
			*_entry = entry;
			return B_OK;
		}
		start += entry->Length();
		i++;
	}

	if (count != NULL)
		*count = i;
	return B_ENTRY_NOT_FOUND;
}


status_t
Attribute::_PrefixedName(ext2_xattr_entry* entry, char* _name, size_t* _nameLength)
{
	const char *indexNames[] = { "0", "user" };
	size_t l = 0;
	
	if (entry->NameIndex() < ((sizeof(indexNames) / sizeof(indexNames[0]))))
		l = snprintf(_name, *_nameLength, "%s.%s.%.*s",
			"linux", indexNames[entry->NameIndex()], entry->NameLength(),
			entry->name);
	else
		l = snprintf(_name, *_nameLength, "%s.%d.%.*s",
			"linux", entry->NameIndex(), entry->NameLength(), entry->name);
	if (l < 1 || l > *_nameLength - 1)
		return ENOBUFS;
	
	*_nameLength = l + 1;
	_name[l] = '\0';

	return B_OK;
}

