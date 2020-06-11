/*
 * Copyright 2004-2020, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */


//!	Connection between pure inode and kernel_interface attributes.


#include "Attribute.h"


// TODO: clean this up, find a better separation between Inode and this class
// TODO: even after Create(), the attribute cannot be stat() for until the
// first write


extern void fill_stat_buffer(Inode* inode, struct stat& stat);


Attribute::Attribute(Inode* inode)
	:
	fNodeGetter(inode->GetVolume()),
	fInode(inode),
	fSmall(NULL),
	fAttribute(NULL),
	fName(NULL)
{
}


Attribute::Attribute(Inode* inode, attr_cookie* cookie)
	:
	fNodeGetter(inode->GetVolume()),
	fInode(inode),
	fSmall(NULL),
	fAttribute(NULL),
	fName(NULL)
{
	Get(cookie->name);
}


Attribute::~Attribute()
{
	Put();
}


status_t
Attribute::InitCheck()
{
	return (fSmall != NULL || fAttribute != NULL) ? B_OK : B_NO_INIT;
}


status_t
Attribute::CheckAccess(const char* name, int openMode)
{
	// Opening the name attribute using this function is not allowed,
	// also using the reserved indices name, last_modified, and size
	// shouldn't be allowed.
	// TODO: we might think about allowing to update those values, but
	//	really change their corresponding values in the bfs_inode structure
	if (name[0] == FILE_NAME_NAME && name[1] == '\0'
// TODO: reenable this check -- some WonderBrush locale files used them
/*		|| !strcmp(name, "name")
		|| !strcmp(name, "last_modified")
		|| !strcmp(name, "size")*/)
		RETURN_ERROR(B_NOT_ALLOWED);

	return fInode->CheckPermissions(open_mode_to_access(openMode)
		| (openMode & O_TRUNC ? W_OK : 0));
}


status_t
Attribute::Get(const char* name)
{
	Put();

	fName = name;

	// try to find it in the small data region
	if (recursive_lock_lock(&fInode->SmallDataLock()) == B_OK) {
		status_t status = fNodeGetter.SetTo(fInode);
		if (status != B_OK)
			return status;

		fSmall = fInode->FindSmallData(fNodeGetter.Node(), (const char*)name);
		if (fSmall != NULL)
			return B_OK;

		recursive_lock_unlock(&fInode->SmallDataLock());
		fNodeGetter.Unset();
	}

	// then, search in the attribute directory
	return fInode->GetAttribute(name, &fAttribute);
}


void
Attribute::Put()
{
	if (fSmall != NULL) {
		recursive_lock_unlock(&fInode->SmallDataLock());
		fNodeGetter.Unset();
		fSmall = NULL;
	}

	if (fAttribute != NULL) {
		fInode->ReleaseAttribute(fAttribute);
		fAttribute = NULL;
	}
}


status_t
Attribute::Create(const char* name, type_code type, int openMode,
	attr_cookie** _cookie)
{
	status_t status = CheckAccess(name, openMode);
	if (status != B_OK)
		return status;

	bool exists = Get(name) == B_OK;
	if (exists && (openMode & O_EXCL) != 0)
		return B_FILE_EXISTS;

	attr_cookie* cookie = new(std::nothrow) attr_cookie;
	if (cookie == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	fName = name;

	// initialize the cookie
	strlcpy(cookie->name, fName, B_ATTR_NAME_LENGTH);
	cookie->type = type;
	cookie->open_mode = openMode;
	cookie->create = true;

	if (exists && (openMode & O_TRUNC) != 0)
		_Truncate();

	*_cookie = cookie;
	return B_OK;
}


status_t
Attribute::Open(const char* name, int openMode, attr_cookie** _cookie)
{
	status_t status = CheckAccess(name, openMode);
	if (status < B_OK)
		return status;

	status = Get(name);
	if (status < B_OK)
		return status;

	attr_cookie* cookie = new(std::nothrow) attr_cookie;
	if (cookie == NULL)
		RETURN_ERROR(B_NO_MEMORY);

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
	if (fSmall == NULL && fAttribute == NULL)
		return B_NO_INIT;

	if (fSmall != NULL) {
		fill_stat_buffer(fInode, stat);

		// overwrite some data to suit our needs
		stat.st_type = fSmall->Type();
		stat.st_size = fSmall->DataSize();
		stat.st_mtim = stat.st_ctim;
			// attribute changes cause status_change updates
	}

	if (fAttribute != NULL)
		fill_stat_buffer(fAttribute, stat);

	return B_OK;
}


status_t
Attribute::Read(attr_cookie* cookie, off_t pos, uint8* buffer, size_t* _length)
{
	if (fSmall == NULL && fAttribute == NULL)
		return B_NO_INIT;

	// TODO: move small_data logic from Inode::ReadAttribute() over to here!
	return fInode->ReadAttribute(cookie->name, 0, pos, buffer, _length);
}


status_t
Attribute::Write(Transaction& transaction, attr_cookie* cookie, off_t pos,
	const uint8* buffer, size_t* _length, bool* _created)
{
	if (!cookie->create && fSmall == NULL && fAttribute == NULL)
		return B_NO_INIT;

	return fInode->WriteAttribute(transaction, cookie->name, cookie->type,
		pos, buffer, _length, _created);
}


status_t
Attribute::_Truncate()
{
	if (fSmall != NULL) {
		// TODO: as long as Inode::_AddSmallData() works like it does,
		// we've got nothing to do here
		return B_OK;
	}

	if (fAttribute != NULL) {
		Transaction transaction(fAttribute->GetVolume(),
			fAttribute->BlockNumber());
		fAttribute->WriteLockInTransaction(transaction);

		status_t status = fAttribute->SetFileSize(transaction, 0);
		if (status >= B_OK)
			status = fAttribute->WriteBack(transaction);

		if (status < B_OK)
			return status;

		transaction.Done();
	}

	return B_OK;
}

