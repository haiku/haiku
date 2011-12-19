/*
 * Copyright 2002-2009, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Dauwalder
 *		Ingo Weinhold, bonefish@users.sf.net
 */


#include <Entry.h>

#include <fcntl.h>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <compat/sys/stat.h>

#include <Directory.h>
#include <Path.h>
#include <SymLink.h>

#include <syscalls.h>

#include "storage_support.h"


using namespace std;


//	#pragma mark - struct entry_ref


entry_ref::entry_ref()
	:
	device((dev_t)-1),
	directory((ino_t)-1),
	name(NULL)
{
}


entry_ref::entry_ref(dev_t dev, ino_t dir, const char* name)
	:
	device(dev),
	directory(dir),
	name(NULL)
{
	set_name(name);
}


entry_ref::entry_ref(const entry_ref& ref)
	:
	device(ref.device),
	directory(ref.directory),
	name(NULL)
{
	set_name(ref.name);
}


entry_ref::~entry_ref()
{
	free(name);
}


status_t
entry_ref::set_name(const char* name)
{
	free(this->name);

	if (name == NULL) {
		this->name = NULL;
	} else {
		this->name = strdup(name);
		if (!this->name)
			return B_NO_MEMORY;
	}

	return B_OK;
}


bool
entry_ref::operator==(const entry_ref& ref) const
{
	return (device == ref.device
		&& directory == ref.directory
		&& (name == ref.name
			|| (name != NULL && ref.name != NULL
				&& strcmp(name, ref.name) == 0)));
}


bool
entry_ref::operator!=(const entry_ref& ref) const
{
	return !(*this == ref);
}


entry_ref&
entry_ref::operator=(const entry_ref& ref)
{
	if (this == &ref)
		return *this;

	device = ref.device;
	directory = ref.directory;
	set_name(ref.name);
	return *this;
}


//	#pragma mark - BEntry


BEntry::BEntry()
	:
	fDirFd(-1),
	fName(NULL),
	fCStatus(B_NO_INIT)
{
}


BEntry::BEntry(const BDirectory* dir, const char* path, bool traverse)
	:
	fDirFd(-1),
	fName(NULL),
	fCStatus(B_NO_INIT)
{
	SetTo(dir, path, traverse);
}


BEntry::BEntry(const entry_ref* ref, bool traverse)
	:
	fDirFd(-1),
	fName(NULL),
	fCStatus(B_NO_INIT)
{
	SetTo(ref, traverse);
}


BEntry::BEntry(const char* path, bool traverse)
	:
	fDirFd(-1),
	fName(NULL),
	fCStatus(B_NO_INIT)
{
	SetTo(path, traverse);
}


BEntry::BEntry(const BEntry& entry)
	:
	fDirFd(-1),
	fName(NULL),
	fCStatus(B_NO_INIT)
{
	*this = entry;
}


BEntry::~BEntry()
{
	Unset();
}


status_t
BEntry::InitCheck() const
{
	return fCStatus;
}


bool
BEntry::Exists() const
{
	// just stat the beast
	struct stat st;
	return GetStat(&st) == B_OK;
}


status_t
BEntry::SetTo(const BDirectory* dir, const char* path, bool traverse)
{
	// check params
	if (!dir)
		return (fCStatus = B_BAD_VALUE);
	if (path && path[0] == '\0')	// R5 behaviour
		path = NULL;

	// if path is absolute, let the path-only SetTo() do the job
	if (BPrivate::Storage::is_absolute_path(path))
		return SetTo(path, traverse);

	Unset();

	if (dir->InitCheck() != B_OK)
		fCStatus = B_BAD_VALUE;

	// dup() the dir's FD and let set() do the rest
	int dirFD = _kern_dup(dir->get_fd());
	if (dirFD < 0)
		return (fCStatus = dirFD);
	return (fCStatus = _SetTo(dirFD, path, traverse));
}


status_t
BEntry::SetTo(const entry_ref* ref, bool traverse)
{
	Unset();
	if (ref == NULL)
		return (fCStatus = B_BAD_VALUE);

	// if ref-name is absolute, let the path-only SetTo() do the job
	if (BPrivate::Storage::is_absolute_path(ref->name))
		return SetTo(ref->name, traverse);

	// open the directory and let set() do the rest
	int dirFD = _kern_open_dir_entry_ref(ref->device, ref->directory, NULL);
	if (dirFD < 0)
		return (fCStatus = dirFD);
	return (fCStatus = _SetTo(dirFD, ref->name, traverse));
}


status_t
BEntry::SetTo(const char* path, bool traverse)
{
	Unset();
	// check the argument
	if (!path)
		return (fCStatus = B_BAD_VALUE);
	return (fCStatus = _SetTo(-1, path, traverse));
}


void
BEntry::Unset()
{
	// Close the directory
	if (fDirFd >= 0)
		_kern_close(fDirFd);

	// Free our leaf name
	free(fName);

	fDirFd = -1;
	fName = NULL;
	fCStatus = B_NO_INIT;
}


status_t
BEntry::GetRef(entry_ref* ref) const
{
	if (fCStatus != B_OK)
		return B_NO_INIT;

	if (ref == NULL)
		return B_BAD_VALUE;

	struct stat st;
	status_t error = _kern_read_stat(fDirFd, NULL, false, &st,
		sizeof(struct stat));
	if (error == B_OK) {
		ref->device = st.st_dev;
		ref->directory = st.st_ino;
		error = ref->set_name(fName);
	}
	return error;
}


status_t
BEntry::GetPath(BPath* path) const
{
	if (fCStatus != B_OK)
		return B_NO_INIT;

	if (path == NULL)
		return B_BAD_VALUE;

	return path->SetTo(this);
}


status_t BEntry::GetParent(BEntry* entry) const
{
	// check parameter and initialization
	if (fCStatus != B_OK)
		return B_NO_INIT;
	if (entry == NULL)
		return B_BAD_VALUE;

	// check whether we are the root directory
	// It is sufficient to check whether our leaf name is ".".
	if (strcmp(fName, ".") == 0)
		return B_ENTRY_NOT_FOUND;

	// open the parent directory
	char leafName[B_FILE_NAME_LENGTH];
	int parentFD = _kern_open_parent_dir(fDirFd, leafName, B_FILE_NAME_LENGTH);
	if (parentFD < 0)
		return parentFD;

	// set close on exec flag on dir FD
	fcntl(parentFD, F_SETFD, FD_CLOEXEC);

	// init the entry
	entry->Unset();
	entry->fDirFd = parentFD;
	entry->fCStatus = entry->_SetName(leafName);
	if (entry->fCStatus != B_OK)
		entry->Unset();
	return entry->fCStatus;
}


status_t
BEntry::GetParent(BDirectory* dir) const
{
	// check initialization and parameter
	if (fCStatus != B_OK)
		return B_NO_INIT;
	if (dir == NULL)
		return B_BAD_VALUE;
	// check whether we are the root directory
	// It is sufficient to check whether our leaf name is ".".
	if (strcmp(fName, ".") == 0)
		return B_ENTRY_NOT_FOUND;
	// get a node ref for the directory and init it
	struct stat st;
	status_t error = _kern_read_stat(fDirFd, NULL, false, &st,
		sizeof(struct stat));
	if (error != B_OK)
		return error;
	node_ref ref;
	ref.device = st.st_dev;
	ref.node = st.st_ino;
	return dir->SetTo(&ref);
	// TODO: This can be optimized: We already have a FD for the directory,
	// so we could dup() it and set it on the directory. We just need a private
	// API for being able to do that.
}


status_t
BEntry::GetName(char* buffer) const
{
	status_t result = B_ERROR;

	if (fCStatus != B_OK) {
		result = B_NO_INIT;
	} else if (buffer == NULL) {
		result = B_BAD_VALUE;
	} else {
		strcpy(buffer, fName);
		result = B_OK;
	}

	return result;
}


status_t
BEntry::Rename(const char* path, bool clobber)
{
	// check parameter and initialization
	if (path == NULL)
		return B_BAD_VALUE;
	if (fCStatus != B_OK)
		return B_NO_INIT;
	// get an entry representing the target location
	BEntry target;
	status_t error;
	if (BPrivate::Storage::is_absolute_path(path)) {
		error = target.SetTo(path);
	} else {
		int dirFD = _kern_dup(fDirFd);
		if (dirFD < 0)
			return dirFD;
		// init the entry
		error = target.fCStatus = target._SetTo(dirFD, path, false);
	}
	if (error != B_OK)
		return error;
	return _Rename(target, clobber);
}


status_t
BEntry::MoveTo(BDirectory* dir, const char* path, bool clobber)
{
	// check parameters and initialization
	if (fCStatus != B_OK)
		return B_NO_INIT;
	if (dir == NULL)
		return B_BAD_VALUE;
	if (dir->InitCheck() != B_OK)
		return B_BAD_VALUE;
	// NULL path simply means move without renaming
	if (path == NULL)
		path = fName;
	// get an entry representing the target location
	BEntry target;
	status_t error = target.SetTo(dir, path);
	if (error != B_OK)
		return error;
	return _Rename(target, clobber);
}


status_t
BEntry::Remove()
{
	if (fCStatus != B_OK)
		return B_NO_INIT;

	if (IsDirectory())
		return _kern_remove_dir(fDirFd, fName);

	return _kern_unlink(fDirFd, fName);
}


bool
BEntry::operator==(const BEntry& item) const
{
	// First check statuses
	if (this->InitCheck() != B_OK && item.InitCheck() != B_OK) {
		return true;
	} else if (this->InitCheck() == B_OK && item.InitCheck() == B_OK) {

		// Directories don't compare well directly, so we'll
		// compare entry_refs instead
		entry_ref ref1, ref2;
		if (this->GetRef(&ref1) != B_OK)
			return false;
		if (item.GetRef(&ref2) != B_OK)
			return false;
		return (ref1 == ref2);

	} else {
		return false;
	}

}


bool
BEntry::operator!=(const BEntry& item) const
{
	return !(*this == item);
}


BEntry&
BEntry::operator=(const BEntry& item)
{
	if (this == &item)
		return *this;

	Unset();
	if (item.fCStatus == B_OK) {
		fDirFd = _kern_dup(item.fDirFd);
		if (fDirFd >= 0)
			fCStatus = _SetName(item.fName);
		else
			fCStatus = fDirFd;

		if (fCStatus != B_OK)
			Unset();
	}

	return *this;
}


void BEntry::_PennyEntry1(){}
void BEntry::_PennyEntry2(){}
void BEntry::_PennyEntry3(){}
void BEntry::_PennyEntry4(){}
void BEntry::_PennyEntry5(){}
void BEntry::_PennyEntry6(){}


status_t
BEntry::set_stat(struct stat& st, uint32 what)
{
	if (fCStatus != B_OK)
		return B_FILE_ERROR;

	return _kern_write_stat(fDirFd, fName, false, &st, sizeof(struct stat),
		what);
}


status_t
BEntry::_SetTo(int dirFD, const char* path, bool traverse)
{
	bool requireConcrete = false;
	FDCloser fdCloser(dirFD);
	char tmpPath[B_PATH_NAME_LENGTH];
	char leafName[B_FILE_NAME_LENGTH];
	int32 linkLimit = B_MAX_SYMLINKS;
	while (true) {
		if (!path || strcmp(path, ".") == 0) {
			// "."
			// if no dir FD is supplied, we need to open the current directory
			// first
			if (dirFD < 0) {
				dirFD = _kern_open_dir(-1, ".");
				if (dirFD < 0)
					return dirFD;
				fdCloser.SetTo(dirFD);
			}
			// get the parent directory
			int parentFD = _kern_open_parent_dir(dirFD, leafName,
				B_FILE_NAME_LENGTH);
			if (parentFD < 0)
				return parentFD;
			dirFD = parentFD;
			fdCloser.SetTo(dirFD);
			break;
		} else if (strcmp(path, "..") == 0) {
			// ".."
			// open the parent directory
			int parentFD = _kern_open_dir(dirFD, "..");
			if (parentFD < 0)
				return parentFD;
			dirFD = parentFD;
			fdCloser.SetTo(dirFD);
			// get the parent's parent directory
			parentFD = _kern_open_parent_dir(dirFD, leafName,
				B_FILE_NAME_LENGTH);
			if (parentFD < 0)
				return parentFD;
			dirFD = parentFD;
			fdCloser.SetTo(dirFD);
			break;
		} else {
			// an ordinary path; analyze it
			char dirPath[B_PATH_NAME_LENGTH];
			status_t error = BPrivate::Storage::parse_path(path, dirPath,
				leafName);
			if (error != B_OK)
				return error;
			// special case: root directory ("/")
			if (leafName[0] == '\0' && dirPath[0] == '/')
				strcpy(leafName, ".");
			if (leafName[0] == '\0') {
				// the supplied path is already a leaf
				error = BPrivate::Storage::check_entry_name(dirPath);
				if (error != B_OK)
					return error;
				strcpy(leafName, dirPath);
				// if no directory was given, we need to open the current dir
				// now
				if (dirFD < 0) {
					char* cwd = getcwd(tmpPath, B_PATH_NAME_LENGTH);
					if (!cwd)
						return B_ERROR;
					dirFD = _kern_open_dir(-1, cwd);
					if (dirFD < 0)
						return dirFD;
					fdCloser.SetTo(dirFD);
				}
			} else if (strcmp(leafName, ".") == 0
					|| strcmp(leafName, "..") == 0) {
				// We have to resolve this to get the entry name. Just open
				// the dir and let the next iteration deal with it.
				dirFD = _kern_open_dir(-1, path);
				if (dirFD < 0)
					return dirFD;
				fdCloser.SetTo(dirFD);
				path = NULL;
				continue;
			} else {
				int parentFD = _kern_open_dir(dirFD, dirPath);
				if (parentFD < 0)
					return parentFD;
				dirFD = parentFD;
				fdCloser.SetTo(dirFD);
			}
			// traverse symlinks, if desired
			if (!traverse)
				break;
			struct stat st;
			error = _kern_read_stat(dirFD, leafName, false, &st,
				sizeof(struct stat));
			if (error == B_ENTRY_NOT_FOUND && !requireConcrete) {
				// that's fine -- the entry is abstract and was not target of
				// a symlink we resolved
				break;
			}
			if (error != B_OK)
				return error;
			// the entry is concrete
			if (!S_ISLNK(st.st_mode))
				break;
			requireConcrete = true;
			// we need to traverse the symlink
			if (--linkLimit < 0)
				return B_LINK_LIMIT;
			size_t bufferSize = B_PATH_NAME_LENGTH - 1;
			error = _kern_read_link(dirFD, leafName, tmpPath, &bufferSize);
			if (error < 0)
				return error;
			tmpPath[bufferSize] = '\0';
			path = tmpPath;
			// next round...
		}
	}

	// set close on exec flag on dir FD
	fcntl(dirFD, F_SETFD, FD_CLOEXEC);

	// set the result
	status_t error = _SetName(leafName);
	if (error != B_OK)
		return error;
	fdCloser.Detach();
	fDirFd = dirFD;
	return B_OK;
}


status_t
BEntry::_SetName(const char* name)
{
	if (name == NULL)
		return B_BAD_VALUE;

	free(fName);

	fName = strdup(name);
	if (fName == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


status_t
BEntry::_Rename(BEntry& target, bool clobber)
{
	// check, if there's an entry in the way
	if (!clobber && target.Exists())
		return B_FILE_EXISTS;
	// rename
	status_t error = _kern_rename(fDirFd, fName, target.fDirFd, target.fName);
	if (error == B_OK) {
		Unset();
		fCStatus = target.fCStatus;
		fDirFd = target.fDirFd;
		fName = target.fName;
		target.fCStatus = B_NO_INIT;
		target.fDirFd = -1;
		target.fName = NULL;
	}
	return error;
}


void
BEntry::_Dump(const char* name)
{
	if (name != NULL) {
		printf("------------------------------------------------------------\n");
		printf("%s\n", name);
		printf("------------------------------------------------------------\n");
	}

	printf("fCStatus == %ld\n", fCStatus);

	struct stat st;
	if (fDirFd != -1
		&& _kern_read_stat(fDirFd, NULL, false, &st,
				sizeof(struct stat)) == B_OK) {
		printf("dir.device == %ld\n", st.st_dev);
		printf("dir.inode  == %lld\n", st.st_ino);
	} else {
		printf("dir == NullFd\n");
	}

	printf("leaf == '%s'\n", fName);
	printf("\n");

}


status_t
BEntry::_GetStat(struct stat* st) const
{
	if (fCStatus != B_OK)
		return B_NO_INIT;

	return _kern_read_stat(fDirFd, fName, false, st, sizeof(struct stat));
}


status_t
BEntry::_GetStat(struct stat_beos* st) const
{
	struct stat newStat;
	status_t error = _GetStat(&newStat);
	if (error != B_OK)
		return error;

	convert_to_stat_beos(&newStat, st);
	return B_OK;
}


// #pragma mark -


status_t
get_ref_for_path(const char* path, entry_ref* ref)
{
	status_t error = path && ref ? B_OK : B_BAD_VALUE;
	if (error == B_OK) {
		BEntry entry(path);
		error = entry.InitCheck();
		if (error == B_OK)
			error = entry.GetRef(ref);
	}
	return error;
}


bool
operator<(const entry_ref& a, const entry_ref& b)
{
	return (a.device < b.device
		|| (a.device == b.device
			&& (a.directory < b.directory
			|| (a.directory == b.directory
				&& ((a.name == NULL && b.name != NULL)
				|| (a.name != NULL && b.name != NULL
					&& strcmp(a.name, b.name) < 0))))));
}


// #pragma mark - symbol versions


#ifdef HAIKU_TARGET_PLATFORM_LIBBE_TEST
#	if __GNUC__ == 2	// gcc 2

	B_DEFINE_SYMBOL_VERSION("_GetStat__C6BEntryP4stat",
		"GetStat__C6BEntryP4stat@@LIBBE_TEST");

#	else	// gcc 4

	// Haiku GetStat()
	B_DEFINE_SYMBOL_VERSION("_ZNK6BEntry8_GetStatEP4stat",
		"_ZNK6BEntry7GetStatEP4stat@@LIBBE_TEST");

#	endif	// gcc 4
#else	// !HAIKU_TARGET_PLATFORM_LIBBE_TEST
#	if __GNUC__ == 2	// gcc 2

	// BeOS compatible GetStat()
	B_DEFINE_SYMBOL_VERSION("_GetStat__C6BEntryP9stat_beos",
		"GetStat__C6BEntryP4stat@LIBBE_BASE");

	// Haiku GetStat()
	B_DEFINE_SYMBOL_VERSION("_GetStat__C6BEntryP4stat",
		"GetStat__C6BEntryP4stat@@LIBBE_1_ALPHA1");

#	else	// gcc 4

	// BeOS compatible GetStat()
	B_DEFINE_SYMBOL_VERSION("_ZNK6BEntry8_GetStatEP9stat_beos",
		"_ZNK6BEntry7GetStatEP4stat@LIBBE_BASE");

	// Haiku GetStat()
	B_DEFINE_SYMBOL_VERSION("_ZNK6BEntry8_GetStatEP4stat",
		"_ZNK6BEntry7GetStatEP4stat@@LIBBE_1_ALPHA1");

#	endif	// gcc 4
#endif	// !HAIKU_TARGET_PLATFORM_LIBBE_TEST
