/*
 * Copyright 2002-2009, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Dauwalder
 *		Ingo Weinhold, bonefish@users.sf.net
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "storage_support.h"

#include <fcntl.h>
#include <string.h>

#include <compat/sys/stat.h>

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <fs_info.h>
#include <Path.h>
#include <SymLink.h>

#include <syscalls.h>
#include <umask.h>


BDirectory::BDirectory()
	:
	fDirFd(-1)
{
}


BDirectory::BDirectory(const BDirectory& dir)
	:
	fDirFd(-1)
{
	*this = dir;
}


BDirectory::BDirectory(const entry_ref* ref)
	:
	fDirFd(-1)
{
	SetTo(ref);
}


BDirectory::BDirectory(const node_ref* nref)
	:
	fDirFd(-1)
{
	SetTo(nref);
}


BDirectory::BDirectory(const BEntry* entry)
	:
	fDirFd(-1)
{
	SetTo(entry);
}


BDirectory::BDirectory(const char* path)
	:
	fDirFd(-1)
{
	SetTo(path);
}


BDirectory::BDirectory(const BDirectory* dir, const char* path)
	:
	fDirFd(-1)
{
	SetTo(dir, path);
}


BDirectory::~BDirectory()
{
	// Also called by the BNode destructor, but we rather try to avoid
	// problems with calling virtual functions in the base class destructor.
	// Depending on the compiler implementation an object may be degraded to
	// an object of the base class after the destructor of the derived class
	// has been executed.
	close_fd();
}


status_t
BDirectory::SetTo(const entry_ref* ref)
{
	// open node
	status_t error = _SetTo(ref, true);
	if (error != B_OK)
		return error;

	// open dir
	fDirFd = _kern_open_dir_entry_ref(ref->device, ref->directory, ref->name);
	if (fDirFd < 0) {
		status_t error = fDirFd;
		Unset();
		return (fCStatus = error);
	}

	// set close on exec flag on dir FD
	fcntl(fDirFd, F_SETFD, FD_CLOEXEC);

	return B_OK;
}


status_t
BDirectory::SetTo(const node_ref* nref)
{
	Unset();
	status_t error = (nref ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		entry_ref ref(nref->device, nref->node, ".");
		error = SetTo(&ref);
	}
	set_status(error);
	return error;
}


status_t
BDirectory::SetTo(const BEntry* entry)
{
	if (!entry) {
		Unset();
		return (fCStatus = B_BAD_VALUE);
	}

	// open node
	status_t error = _SetTo(entry->fDirFd, entry->fName, true);
	if (error != B_OK)
		return error;

	// open dir
	fDirFd = _kern_open_dir(entry->fDirFd, entry->fName);
	if (fDirFd < 0) {
		status_t error = fDirFd;
		Unset();
		return (fCStatus = error);
	}

	// set close on exec flag on dir FD
	fcntl(fDirFd, F_SETFD, FD_CLOEXEC);

	return B_OK;
}


status_t
BDirectory::SetTo(const char* path)
{
	// open node
	status_t error = _SetTo(-1, path, true);
	if (error != B_OK)
		return error;

	// open dir
	fDirFd = _kern_open_dir(-1, path);
	if (fDirFd < 0) {
		status_t error = fDirFd;
		Unset();
		return (fCStatus = error);
	}

	// set close on exec flag on dir FD
	fcntl(fDirFd, F_SETFD, FD_CLOEXEC);

	return B_OK;
}


status_t
BDirectory::SetTo(const BDirectory* dir, const char* path)
{
	if (!dir || !path || BPrivate::Storage::is_absolute_path(path)) {
		Unset();
		return (fCStatus = B_BAD_VALUE);
	}

	int dirFD = dir->fDirFd;
	if (dir == this) {
		// prevent that our file descriptor goes away in _SetTo()
		fDirFd = -1;
	}

	// open node
	status_t error = _SetTo(dirFD, path, true);
	if (error != B_OK)
		return error;

	// open dir
	fDirFd = _kern_open_dir(dirFD, path);
	if (fDirFd < 0) {
		status_t error = fDirFd;
		Unset();
		return (fCStatus = error);
	}

	if (dir == this) {
		// cleanup after _SetTo()
		_kern_close(dirFD);
	}

	// set close on exec flag on dir FD
	fcntl(fDirFd, F_SETFD, FD_CLOEXEC);

	return B_OK;
}


status_t
BDirectory::GetEntry(BEntry* entry) const
{
	if (!entry)
		return B_BAD_VALUE;
	if (InitCheck() != B_OK)
		return B_NO_INIT;
	return entry->SetTo(this, ".", false);
}


bool
BDirectory::IsRootDirectory() const
{
	// compare the directory's node ID with the ID of the root node of the FS
	bool result = false;
	node_ref ref;
	fs_info info;
	if (GetNodeRef(&ref) == B_OK && fs_stat_dev(ref.device, &info) == 0)
		result = (ref.node == info.root);
	return result;
}


status_t
BDirectory::FindEntry(const char* path, BEntry* entry, bool traverse) const
{
	if (path == NULL || entry == NULL)
		return B_BAD_VALUE;

	entry->Unset();

	// init a potentially abstract entry
	status_t status;
	if (InitCheck() == B_OK)
		status = entry->SetTo(this, path, traverse);
	else
		status = entry->SetTo(path, traverse);

	// fail, if entry is abstract
	if (status == B_OK && !entry->Exists()) {
		status = B_ENTRY_NOT_FOUND;
		entry->Unset();
	}

	return status;
}


bool
BDirectory::Contains(const char* path, int32 nodeFlags) const
{
	// check initialization and parameters
	if (InitCheck() != B_OK)
		return false;
	if (!path)
		return true;	// mimic R5 behavior

	// turn the path into a BEntry and let the other version do the work
	BEntry entry;
	if (BPrivate::Storage::is_absolute_path(path))
		entry.SetTo(path);
	else
		entry.SetTo(this, path);

	return Contains(&entry, nodeFlags);
}


bool
BDirectory::Contains(const BEntry* entry, int32 nodeFlags) const
{
	// check, if the entry exists at all
	if (entry == NULL || !entry->Exists() || InitCheck() != B_OK)
		return false;

	if (nodeFlags != B_ANY_NODE) {
		// test the node kind
		bool result = false;
		if ((nodeFlags & B_FILE_NODE) != 0)
			result = entry->IsFile();
		if (!result && (nodeFlags & B_DIRECTORY_NODE) != 0)
			result = entry->IsDirectory();
		if (!result && (nodeFlags & B_SYMLINK_NODE) != 0)
			result = entry->IsSymLink();
		if (!result)
			return false;
	}

	// If the directory is initialized, get the canonical paths of the dir and
	// the entry and check, if the latter is a prefix of the first one.
	BPath dirPath(this, ".", true);
	BPath entryPath(entry);
	if (dirPath.InitCheck() != B_OK || entryPath.InitCheck() != B_OK)
		return false;

	uint32 dirLen = strlen(dirPath.Path());

	if (!strncmp(dirPath.Path(), entryPath.Path(), dirLen)) {
		// if the paths are identical, return a match to stay consistent with
		// BeOS behavior.
		if (entryPath.Path()[dirLen] == '\0' || entryPath.Path()[dirLen] == '/')
			return true;
	}
	return false;
}


status_t
BDirectory::GetNextEntry(BEntry* entry, bool traverse)
{
	status_t error = (entry ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		entry_ref ref;
		error = GetNextRef(&ref);
		if (error == B_OK)
			error = entry->SetTo(&ref, traverse);
	}
	return error;
}


status_t
BDirectory::GetNextRef(entry_ref* ref)
{
	status_t error = (ref ? B_OK : B_BAD_VALUE);
	if (error == B_OK && InitCheck() != B_OK)
		error = B_FILE_ERROR;
	if (error == B_OK) {
		BPrivate::Storage::LongDirEntry entry;
		bool next = true;
		while (error == B_OK && next) {
			if (GetNextDirents(&entry, sizeof(entry), 1) != 1) {
				error = B_ENTRY_NOT_FOUND;
			} else {
				next = (!strcmp(entry.d_name, ".")
					|| !strcmp(entry.d_name, ".."));
			}
		}
		if (error == B_OK) {
			ref->device = entry.d_pdev;
			ref->directory = entry.d_pino;
			error = ref->set_name(entry.d_name);
		}
	}
	return error;
}


int32
BDirectory::GetNextDirents(dirent* buf, size_t bufSize, int32 count)
{
	if (!buf)
		return B_BAD_VALUE;
	if (InitCheck() != B_OK)
		return B_FILE_ERROR;
	return _kern_read_dir(fDirFd, buf, bufSize, count);
}


status_t
BDirectory::Rewind()
{
	if (InitCheck() != B_OK)
		return B_FILE_ERROR;
	return _kern_rewind_dir(fDirFd);
}


int32
BDirectory::CountEntries()
{
	status_t error = Rewind();
	if (error != B_OK)
		return error;
	int32 count = 0;
	BPrivate::Storage::LongDirEntry entry;
	while (error == B_OK) {
		if (GetNextDirents(&entry, sizeof(entry), 1) != 1)
			break;
		if (strcmp(entry.d_name, ".") != 0 && strcmp(entry.d_name, "..") != 0)
			count++;
	}
	Rewind();
	return (error == B_OK ? count : error);
}


status_t
BDirectory::CreateDirectory(const char* path, BDirectory* dir)
{
	if (!path)
		return B_BAD_VALUE;

	// create the dir
	status_t error = _kern_create_dir(fDirFd, path,
		(S_IRWXU | S_IRWXG | S_IRWXO) & ~__gUmask);
	if (error != B_OK)
		return error;

	if (dir == NULL)
		return B_OK;

	// init the supplied BDirectory
	if (InitCheck() != B_OK || BPrivate::Storage::is_absolute_path(path))
		return dir->SetTo(path);

	return dir->SetTo(this, path);
}


status_t
BDirectory::CreateFile(const char* path, BFile* file, bool failIfExists)
{
	if (!path)
		return B_BAD_VALUE;

	// Let BFile do the dirty job.
	uint32 openMode = B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE
		| (failIfExists ? B_FAIL_IF_EXISTS : 0);
	BFile tmpFile;
	BFile* realFile = file ? file : &tmpFile;
	status_t error = B_OK;
	if (InitCheck() == B_OK && !BPrivate::Storage::is_absolute_path(path))
		error = realFile->SetTo(this, path, openMode);
	else
		error = realFile->SetTo(path, openMode);
	if (error != B_OK && file) // mimic R5 behavior
		file->Unset();
	return error;
}


status_t
BDirectory::CreateSymLink(const char* path, const char* linkToPath,
	BSymLink* link)
{
	if (!path || !linkToPath)
		return B_BAD_VALUE;

	// create the symlink
	status_t error = _kern_create_symlink(fDirFd, path, linkToPath,
		(S_IRWXU | S_IRWXG | S_IRWXO) & ~__gUmask);
	if (error != B_OK)
		return error;

	if (link == NULL)
		return B_OK;

	// init the supplied BSymLink
	if (InitCheck() != B_OK || BPrivate::Storage::is_absolute_path(path))
		return link->SetTo(path);

	return link->SetTo(this, path);
}


BDirectory&
BDirectory::operator=(const BDirectory& dir)
{
	if (&dir != this) {	// no need to assign us to ourselves
		Unset();
		if (dir.InitCheck() == B_OK)
			SetTo(&dir, ".");
	}
	return *this;
}


status_t
BDirectory::_GetStatFor(const char* path, struct stat* st) const
{
	if (!st)
		return B_BAD_VALUE;
	if (InitCheck() != B_OK)
		return B_NO_INIT;

	if (path != NULL) {
		if (path[0] == '\0')
			return B_ENTRY_NOT_FOUND;
		return _kern_read_stat(fDirFd, path, false, st, sizeof(struct stat));
	}
	return GetStat(st);
}


status_t
BDirectory::_GetStatFor(const char* path, struct stat_beos* st) const
{
	struct stat newStat;
	status_t error = _GetStatFor(path, &newStat);
	if (error != B_OK)
		return error;

	convert_to_stat_beos(&newStat, st);
	return B_OK;
}


// FBC
void BDirectory::_ErectorDirectory1() {}
void BDirectory::_ErectorDirectory2() {}
void BDirectory::_ErectorDirectory3() {}
void BDirectory::_ErectorDirectory4() {}
void BDirectory::_ErectorDirectory5() {}
void BDirectory::_ErectorDirectory6() {}


//! Closes the BDirectory's file descriptor.
void
BDirectory::close_fd()
{
	if (fDirFd >= 0) {
		_kern_close(fDirFd);
		fDirFd = -1;
	}
	BNode::close_fd();
}


int
BDirectory::get_fd() const
{
	return fDirFd;
}


//	#pragma mark - C functions


// TODO: Check this method for efficiency.
status_t
create_directory(const char* path, mode_t mode)
{
	if (!path)
		return B_BAD_VALUE;

	// That's the strategy: We start with the first component of the supplied
	// path, create a BPath object from it and successively add the following
	// components. Each time we get a new path, we check, if the entry it
	// refers to exists and is a directory. If it doesn't exist, we try
	// to create it. This goes on, until we're done with the input path or
	// an error occurs.
	BPath dirPath;
	char* component;
	int32 nextComponent;
	do {
		// get the next path component
		status_t error = BPrivate::Storage::parse_first_path_component(path,
			component, nextComponent);
		if (error != B_OK)
			return error;

		// append it to the BPath
		if (dirPath.InitCheck() == B_NO_INIT)	// first component
			error = dirPath.SetTo(component);
		else
			error = dirPath.Append(component);
		delete[] component;
		if (error != B_OK)
			return error;
		path += nextComponent;

		// create a BEntry from the BPath
		BEntry entry;
		error = entry.SetTo(dirPath.Path(), true);
		if (error != B_OK)
			return error;

		// check, if it exists
		if (entry.Exists()) {
			// yep, it exists
			if (!entry.IsDirectory())	// but is no directory
				return B_NOT_A_DIRECTORY;
		} else {
			// it doesn't exist -- create it
			error = _kern_create_dir(-1, dirPath.Path(), mode & ~__gUmask);
			if (error != B_OK)
				return error;
		}
	} while (nextComponent != 0);
	return B_OK;
}


// #pragma mark - symbol versions


#ifdef HAIKU_TARGET_PLATFORM_LIBBE_TEST
#	if __GNUC__ == 2	// gcc 2

	B_DEFINE_SYMBOL_VERSION("_GetStatFor__C10BDirectoryPCcP4stat",
		"GetStatFor__C10BDirectoryPCcP4stat@@LIBBE_TEST");

#	else	// gcc 4

	B_DEFINE_SYMBOL_VERSION("_ZNK10BDirectory11_GetStatForEPKcP4stat",
		"_ZNK10BDirectory10GetStatForEPKcP4stat@@LIBBE_TEST");

#	endif	// gcc 4
#else	// !HAIKU_TARGET_PLATFORM_LIBBE_TEST
#	if __GNUC__ == 2	// gcc 2

	// BeOS compatible GetStatFor()
	B_DEFINE_SYMBOL_VERSION("_GetStatFor__C10BDirectoryPCcP9stat_beos",
		"GetStatFor__C10BDirectoryPCcP4stat@LIBBE_BASE");

	// Haiku GetStatFor()
	B_DEFINE_SYMBOL_VERSION("_GetStatFor__C10BDirectoryPCcP4stat",
		"GetStatFor__C10BDirectoryPCcP4stat@@LIBBE_1_ALPHA1");

#	else	// gcc 4

	// BeOS compatible GetStatFor()
	B_DEFINE_SYMBOL_VERSION("_ZNK10BDirectory11_GetStatForEPKcP9stat_beos",
		"_ZNK10BDirectory10GetStatForEPKcP4stat@LIBBE_BASE");

	// Haiku GetStatFor()
	B_DEFINE_SYMBOL_VERSION("_ZNK10BDirectory11_GetStatForEPKcP4stat",
		"_ZNK10BDirectory10GetStatForEPKcP4stat@@LIBBE_1_ALPHA1");

#	endif	// gcc 4
#endif	// !HAIKU_TARGET_PLATFORM_LIBBE_TEST
