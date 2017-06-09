/*
 * Copyright 2010-2011, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2010, François Revol, <revol@free.fr>.
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */

//!	connection between pure inode and kernel_interface attributes


#include "Attribute.h"
#include "BTree.h"
#include "CRCTable.h"
#include "Utility.h"


//#define TRACE_BTRFS
#ifdef TRACE_BTRFS
#	define TRACE(x...) dprintf("\33[34mbtrfs:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif


Attribute::Attribute(Inode* inode)
	:
	fVolume(inode->GetVolume()),
	fInode(inode),
	fName(NULL)
{
}


Attribute::Attribute(Inode* inode, attr_cookie* cookie)
	:
	fVolume(inode->GetVolume()),
	fInode(inode),
	fName(cookie->name)
{
}


Attribute::~Attribute()
{
}


status_t
Attribute::CheckAccess(const char* name, int openMode)
{
	return fInode->CheckPermissions(open_mode_to_access(openMode)
		| (openMode & O_TRUNC ? W_OK : 0));
}


status_t
Attribute::Open(const char* name, int openMode, attr_cookie** _cookie)
{
	TRACE("Open\n");
	status_t status = CheckAccess(name, openMode);
	if (status < B_OK)
		return status;

	status = _Lookup(name, strlen(name));
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
Attribute::Stat(struct stat& stat)
{
	TRACE("Stat\n");

	size_t nameLength = strlen(fName);
	btrfs_dir_entry* entries;
	size_t length;
	status_t status = _Lookup(fName, nameLength, &entries, &length);
	if (status < B_OK)
		return status;

	btrfs_dir_entry* entry;
	status = _FindEntry(entries, length, fName, nameLength, &entry);
	if (status != B_OK) {
		free(entries);
		return status;
	}

	// found an entry to stat
	stat.st_type = B_XATTR_TYPE;
	stat.st_size = entry->DataLength();
	free(entries);
	return B_OK;
}


status_t
Attribute::Read(attr_cookie* cookie, off_t pos, uint8* buffer, size_t* _length)
{
	if (pos < 0LL)
		return ERANGE;

	size_t nameLength = strlen(fName);
	btrfs_dir_entry* entries;
	size_t length;
	status_t status = _Lookup(fName, nameLength, &entries, &length);
	if (status < B_OK)
		return status;

	btrfs_dir_entry* entry;
	status = _FindEntry(entries, length, fName, nameLength, &entry);
	if (status != B_OK) {
		free(entries);
		return status;
	}

	// found an entry to read
	if (pos + *_length > entry->DataLength())
		length = entry->DataLength() - pos;
	else
		length = *_length - pos;
	memcpy(buffer, (uint8*)entry + entry->NameLength()
		+ sizeof(btrfs_dir_entry) + (uint32)pos, length);
	*_length = length;

	free(entries);
	return B_OK;
}


status_t
Attribute::_Lookup(const char* name, size_t nameLength,
	btrfs_dir_entry** _entries, size_t* _length)
{
	uint32 hash = calculate_crc((uint32)~1, (uint8*)name, nameLength);
	struct btrfs_key key;
	key.SetType(BTRFS_KEY_TYPE_XATTR_ITEM);
	key.SetObjectID(fInode->ID());
	key.SetOffset(hash);

	btrfs_dir_entry* entries;
	size_t length;
	status_t status = fInode->GetVolume()->FSTree()->FindExact(key,
		(void**)&entries, &length);
	if (status != B_OK) {
		TRACE("AttributeIterator::Lookup(): Couldn't find entry with hash %"
			B_PRIu32 " \"%s\"\n", hash, name);
		return status;
	}

	if (_entries == NULL)
		free(entries);
	else
		*_entries = entries;

	if (_length != NULL)
		*_length = length;

	return B_OK;
}


status_t
Attribute::_FindEntry(btrfs_dir_entry* entries, size_t length,
	const char* name, size_t nameLength, btrfs_dir_entry** _entry)
{
	btrfs_dir_entry* entry = entries;
	uint16 current = 0;
	while (current < length) {
		current += entry->Length();
		break;
		// TODO there could be several entries with the same name hash
		entry = (btrfs_dir_entry*)((uint8*)entry + entry->Length());
	}

	*_entry = entry;
	return B_OK;
}
