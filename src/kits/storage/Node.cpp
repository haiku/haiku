/*
 * Copyright 2002-2011, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Dauwalder
 *		Ingo Weinhold, bonefish@users.sf.net
 */


#include <Node.h>

#include <errno.h>
#include <fcntl.h>
#include <new>
#include <string.h>
#include <unistd.h>

#include <compat/sys/stat.h>

#include <Directory.h>
#include <Entry.h>
#include <fs_attr.h>
#include <String.h>
#include <TypeConstants.h>

#include <syscalls.h>

#include "storage_support.h"


//	#pragma mark - node_ref


node_ref::node_ref()
		: device((dev_t)-1),
		  node((ino_t)-1)
{
}

// copy constructor
node_ref::node_ref(const node_ref &ref)
		: device((dev_t)-1),
		  node((ino_t)-1)
{
	*this = ref;
}

// ==
bool
node_ref::operator==(const node_ref &ref) const
{
	return (device == ref.device && node == ref.node);
}

// !=
bool
node_ref::operator!=(const node_ref &ref) const
{
	return !(*this == ref);
}

// =
node_ref&
node_ref::operator=(const node_ref &ref)
{
	device = ref.device;
	node = ref.node;
	return *this;
}


//	#pragma mark - BNode


BNode::BNode()
	 : fFd(-1),
	   fAttrFd(-1),
	   fCStatus(B_NO_INIT)
{
}


BNode::BNode(const entry_ref *ref)
	 : fFd(-1),
	   fAttrFd(-1),
	   fCStatus(B_NO_INIT)
{
	SetTo(ref);
}


BNode::BNode(const BEntry *entry)
	 : fFd(-1),
	   fAttrFd(-1),
	   fCStatus(B_NO_INIT)
{
	SetTo(entry);
}


BNode::BNode(const char *path)
	 : fFd(-1),
	   fAttrFd(-1),
	   fCStatus(B_NO_INIT)
{
	SetTo(path);
}


BNode::BNode(const BDirectory *dir, const char *path)
	 : fFd(-1),
	   fAttrFd(-1),
	   fCStatus(B_NO_INIT)
{
	SetTo(dir, path);
}


BNode::BNode(const BNode &node)
	 : fFd(-1),
	   fAttrFd(-1),
	   fCStatus(B_NO_INIT)
{
	*this = node;
}


BNode::~BNode()
{
	Unset();
}


status_t
BNode::InitCheck() const
{
	return fCStatus;
}


status_t
BNode::SetTo(const entry_ref *ref)
{
	return _SetTo(ref, false);
}


status_t
BNode::SetTo(const BEntry *entry)
{
	if (!entry) {
		Unset();
		return (fCStatus = B_BAD_VALUE);
	}
	return _SetTo(entry->fDirFd, entry->fName, false);
}


status_t
BNode::SetTo(const char *path)
{
	return _SetTo(-1, path, false);
}


status_t
BNode::SetTo(const BDirectory *dir, const char *path)
{
	if (!dir || !path || BPrivate::Storage::is_absolute_path(path)) {
		Unset();
		return (fCStatus = B_BAD_VALUE);
	}
	return _SetTo(dir->fDirFd, path, false);
}


void
BNode::Unset()
{
	close_fd();
	fCStatus = B_NO_INIT;
}


status_t
BNode::Lock()
{
	if (fCStatus != B_OK)
		return fCStatus;
	return _kern_lock_node(fFd);
}


status_t
BNode::Unlock()
{
	if (fCStatus != B_OK)
		return fCStatus;
	return _kern_unlock_node(fFd);
}


status_t
BNode::Sync()
{
	return (fCStatus != B_OK) ? B_FILE_ERROR : _kern_fsync(fFd);
}


ssize_t
BNode::WriteAttr(const char *attr, type_code type, off_t offset,
				 const void *buffer, size_t len)
{
	if (fCStatus != B_OK)
		return B_FILE_ERROR;
	if (!attr || !buffer)
		return B_BAD_VALUE;

	ssize_t result = fs_write_attr(fFd, attr, type, offset, buffer, len);
	return result < 0 ? errno : result;
}


ssize_t
BNode::ReadAttr(const char *attr, type_code type, off_t offset,
				void *buffer, size_t len) const
{
	if (fCStatus != B_OK)
		return B_FILE_ERROR;
	if (!attr || !buffer)
		return B_BAD_VALUE;

	ssize_t result = fs_read_attr(fFd, attr, type, offset, buffer, len );
	return result == -1 ? errno : result;
}


status_t
BNode::RemoveAttr(const char *name)
{
	return fCStatus != B_OK ? B_FILE_ERROR : _kern_remove_attr(fFd, name);
}


status_t
BNode::RenameAttr(const char *oldname, const char *newname)
{
	if (fCStatus != B_OK)
		return B_FILE_ERROR;

	return _kern_rename_attr(fFd, oldname, fFd, newname);
}


status_t
BNode::GetAttrInfo(const char *name, struct attr_info *info) const
{
	if (fCStatus != B_OK)
		return B_FILE_ERROR;
	if (!name || !info)
		return B_BAD_VALUE;

	return fs_stat_attr(fFd, name, info) < 0 ? errno : B_OK ;
}


status_t
BNode::GetNextAttrName(char *buffer)
{
	// We're allowed to assume buffer is at least
	// B_ATTR_NAME_LENGTH chars long, but NULLs
	// are not acceptable.
	if (buffer == NULL)
		return B_BAD_VALUE;	// /new R5 crashed when passed NULL
	if (InitAttrDir() != B_OK)
		return B_FILE_ERROR;

	BPrivate::Storage::LongDirEntry entry;
	ssize_t result = _kern_read_dir(fAttrFd, &entry, sizeof(entry), 1);
	if (result < 0)
		return result;
	if (result == 0)
		return B_ENTRY_NOT_FOUND;
	strlcpy(buffer, entry.d_name, B_ATTR_NAME_LENGTH);
	return B_OK;
}


status_t
BNode::RewindAttrs()
{
	if (InitAttrDir() != B_OK)
		return B_FILE_ERROR;

	return _kern_rewind_dir(fAttrFd);
}


status_t
BNode::WriteAttrString(const char *name, const BString *data)
{
	status_t error = (!name || !data)  ? B_BAD_VALUE : B_OK;
	if (error == B_OK) {
		int32 len = data->Length() + 1;
		ssize_t sizeWritten = WriteAttr(name, B_STRING_TYPE, 0, data->String(),
										len);
		if (sizeWritten != len)
			error = sizeWritten;
	}
	return error;
}


status_t
BNode::ReadAttrString(const char *name, BString *result) const
{
	if (!name || !result)
		return B_BAD_VALUE;

	attr_info info;
	status_t error;

	error = GetAttrInfo(name, &info);
	if (error != B_OK)
		return error;

	// Lock the string's buffer so we can meddle with it
	char *data = result->LockBuffer(info.size + 1);
	if (!data)
		return B_NO_MEMORY;

	// Read the attribute
	ssize_t bytes = ReadAttr(name, B_STRING_TYPE, 0, data, info.size);
	// Check for failure
	if (bytes < 0) {
		error = bytes;
		bytes = 0;	// In this instance, we simply clear the string
	} else
		error = B_OK;

	// Null terminate the new string just to be sure (since it *is*
	// possible to read and write non-NULL-terminated strings)
	data[bytes] = 0;
	result->UnlockBuffer();
	return error;
}


BNode&
BNode::operator=(const BNode &node)
{
	// No need to do any assignment if already equal
	if (*this == node)
		return *this;

	// Close down out current state
	Unset();
	// We have to manually dup the node, because R5::BNode::Dup()
	// is not declared to be const (which IMO is retarded).
	fFd = _kern_dup(node.fFd);
	fCStatus = (fFd < 0) ? B_NO_INIT : B_OK ;
	return *this;
}


bool
BNode::operator==(const BNode &node) const
{
	if (fCStatus == B_NO_INIT && node.InitCheck() == B_NO_INIT)
		return true;
	if (fCStatus == B_OK && node.InitCheck() == B_OK) {
		// compare the node_refs
		node_ref ref1, ref2;
		if (GetNodeRef(&ref1) != B_OK)
			return false;
		if (node.GetNodeRef(&ref2) != B_OK)
			return false;
		return (ref1 == ref2);
	}
	return false;
}


bool
BNode::operator!=(const BNode &node) const
{
	return !(*this == node);
}


int
BNode::Dup()
{
	int fd = _kern_dup(fFd);
	return (fd >= 0 ? fd : -1);	// comply with R5 return value
}


/*! (currently unused) */
void BNode::_RudeNode1() { }
void BNode::_RudeNode2() { }
void BNode::_RudeNode3() { }
void BNode::_RudeNode4() { }
void BNode::_RudeNode5() { }
void BNode::_RudeNode6() { }


status_t
BNode::set_fd(int fd)
{
	if (fFd != -1)
		close_fd();
	fFd = fd;
	return B_OK;
}


void
BNode::close_fd()
{
	if (fAttrFd >= 0) {
		_kern_close(fAttrFd);
		fAttrFd = -1;
	}
	if (fFd >= 0) {
		_kern_close(fFd);
		fFd = -1;
	}
}


void
BNode::set_status(status_t newStatus)
{
	fCStatus = newStatus;
}


status_t
BNode::_SetTo(int fd, const char *path, bool traverse)
{
	Unset();
	status_t error = (fd >= 0 || path ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		int traverseFlag = (traverse ? 0 : O_NOTRAVERSE);
		fFd = _kern_open(fd, path, O_RDWR | O_CLOEXEC | traverseFlag, 0);
		if (fFd < B_OK && fFd != B_ENTRY_NOT_FOUND) {
			// opening read-write failed, re-try read-only
			fFd = _kern_open(fd, path, O_RDONLY | O_CLOEXEC | traverseFlag, 0);
		}
		if (fFd < 0)
			error = fFd;
	}
	return fCStatus = error;
}


status_t
BNode::_SetTo(const entry_ref *ref, bool traverse)
{
	Unset();
	status_t error = (ref ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		int traverseFlag = (traverse ? 0 : O_NOTRAVERSE);
		fFd = _kern_open_entry_ref(ref->device, ref->directory, ref->name,
			O_RDWR | O_CLOEXEC | traverseFlag, 0);
		if (fFd < B_OK && fFd != B_ENTRY_NOT_FOUND) {
			// opening read-write failed, re-try read-only
			fFd = _kern_open_entry_ref(ref->device, ref->directory, ref->name,
				O_RDONLY | O_CLOEXEC | traverseFlag, 0);
		}
		if (fFd < 0)
			error = fFd;
	}
	return fCStatus = error;
}


status_t
BNode::set_stat(struct stat &st, uint32 what)
{
	if (fCStatus != B_OK)
		return B_FILE_ERROR;

	return _kern_write_stat(fFd, NULL, false, &st, sizeof(struct stat),
		what);
}


status_t
BNode::InitAttrDir()
{
	if (fCStatus == B_OK && fAttrFd < 0) {
		fAttrFd = _kern_open_attr_dir(fFd, NULL, false);
		if (fAttrFd < 0)
			return fAttrFd;

		// set close on exec flag
		fcntl(fAttrFd, F_SETFD, FD_CLOEXEC);
	}
	return fCStatus;
}


status_t
BNode::_GetStat(struct stat *st) const
{
	return fCStatus != B_OK
		? fCStatus
		: _kern_read_stat(fFd, NULL, false, st, sizeof(struct stat));
}


status_t
BNode::_GetStat(struct stat_beos *st) const
{
	struct stat newStat;
	status_t error = _GetStat(&newStat);
	if (error != B_OK)
		return error;

	convert_to_stat_beos(&newStat, st);
	return B_OK;
}


// #pragma mark - symbol versions


#ifdef HAIKU_TARGET_PLATFORM_LIBBE_TEST
#	if __GNUC__ == 2	// gcc 2

	B_DEFINE_SYMBOL_VERSION("_GetStat__C5BNodeP4stat",
		"GetStat__C5BNodeP4stat@@LIBBE_TEST");

#	else	// gcc 4

	B_DEFINE_SYMBOL_VERSION("_ZNK5BNode8_GetStatEP4stat",
		"_ZNK5BNode7GetStatEP4stat@@LIBBE_TEST");

#	endif	// gcc 4
#else	// !HAIKU_TARGET_PLATFORM_LIBBE_TEST
#	if __GNUC__ == 2	// gcc 2

	// BeOS compatible GetStat()
	B_DEFINE_SYMBOL_VERSION("_GetStat__C5BNodeP9stat_beos",
		"GetStat__C5BNodeP4stat@LIBBE_BASE");

	// Haiku GetStat()
	B_DEFINE_SYMBOL_VERSION("_GetStat__C5BNodeP4stat",
		"GetStat__C5BNodeP4stat@@LIBBE_1_ALPHA1");

#	else	// gcc 4

	// BeOS compatible GetStat()
	B_DEFINE_SYMBOL_VERSION("_ZNK5BNode8_GetStatEP9stat_beos",
		"_ZNK5BNode7GetStatEP4stat@LIBBE_BASE");

	// Haiku GetStat()
	B_DEFINE_SYMBOL_VERSION("_ZNK5BNode8_GetStatEP4stat",
		"_ZNK5BNode7GetStatEP4stat@@LIBBE_1_ALPHA1");

#	endif	// gcc 4
#endif	// !HAIKU_TARGET_PLATFORM_LIBBE_TEST
