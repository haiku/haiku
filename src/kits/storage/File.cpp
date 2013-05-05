/*
 * Copyright 2002-2009, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Dauwalder
 *		Ingo Weinhold, bonefish@users.sf.net
 */


#include <fcntl.h>
#include <unistd.h>

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <fs_interface.h>
#include <NodeMonitor.h>
#include "storage_support.h"

#include <syscalls.h>
#include <umask.h>


// Creates an uninitialized BFile.
BFile::BFile()
	:
	fMode(0)
{
}


// Creates a copy of the supplied BFile.
BFile::BFile(const BFile& file)
	:
	fMode(0)
{
	*this = file;
}


// Creates a BFile and initializes it to the file referred to by
// the supplied entry_ref and according to the specified open mode.
BFile::BFile(const entry_ref* ref, uint32 openMode)
	:
	fMode(0)
{
	SetTo(ref, openMode);
}


// Creates a BFile and initializes it to the file referred to by
// the supplied BEntry and according to the specified open mode.
BFile::BFile(const BEntry* entry, uint32 openMode)
	:
	fMode(0)
{
	SetTo(entry, openMode);
}


// Creates a BFile and initializes it to the file referred to by
// the supplied path name and according to the specified open mode.
BFile::BFile(const char* path, uint32 openMode)
	:
	fMode(0)
{
	SetTo(path, openMode);
}


// Creates a BFile and initializes it to the file referred to by
// the supplied path name relative to the specified BDirectory and
// according to the specified open mode.
BFile::BFile(const BDirectory *dir, const char* path, uint32 openMode)
	:
	fMode(0)
{
	SetTo(dir, path, openMode);
}


// Frees all allocated resources.
BFile::~BFile()
{
	// Also called by the BNode destructor, but we rather try to avoid
	// problems with calling virtual functions in the base class destructor.
	// Depending on the compiler implementation an object may be degraded to
	// an object of the base class after the destructor of the derived class
	// has been executed.
	close_fd();
}


// Re-initializes the BFile to the file referred to by the
// supplied entry_ref and according to the specified open mode.
status_t
BFile::SetTo(const entry_ref* ref, uint32 openMode)
{
	Unset();

	if (!ref)
		return (fCStatus = B_BAD_VALUE);

	// if ref->name is absolute, let the path-only SetTo() do the job
	if (BPrivate::Storage::is_absolute_path(ref->name))
		return SetTo(ref->name, openMode);

	openMode |= O_CLOEXEC;

	int fd = _kern_open_entry_ref(ref->device, ref->directory, ref->name,
		openMode, DEFFILEMODE & ~__gUmask);
	if (fd >= 0) {
		set_fd(fd);
		fMode = openMode;
		fCStatus = B_OK;
	} else
		fCStatus = fd;

	return fCStatus;
}


// Re-initializes the BFile to the file referred to by the
// supplied BEntry and according to the specified open mode.
status_t
BFile::SetTo(const BEntry* entry, uint32 openMode)
{
	Unset();

	if (!entry)
		return (fCStatus = B_BAD_VALUE);
	if (entry->InitCheck() != B_OK)
		return (fCStatus = entry->InitCheck());

	openMode |= O_CLOEXEC;

	int fd = _kern_open(entry->fDirFd, entry->fName, openMode | O_CLOEXEC,
		DEFFILEMODE & ~__gUmask);
	if (fd >= 0) {
		set_fd(fd);
		fMode = openMode;
		fCStatus = B_OK;
	} else
		fCStatus = fd;

	return fCStatus;
}


// Re-initializes the BFile to the file referred to by the
// supplied path name and according to the specified open mode.
status_t
BFile::SetTo(const char* path, uint32 openMode)
{
	Unset();

	if (!path)
		return (fCStatus = B_BAD_VALUE);

	openMode |= O_CLOEXEC;

	int fd = _kern_open(-1, path, openMode, DEFFILEMODE & ~__gUmask);
	if (fd >= 0) {
		set_fd(fd);
		fMode = openMode;
		fCStatus = B_OK;
	} else
		fCStatus = fd;

	return fCStatus;
}


// Re-initializes the BFile to the file referred to by the
// supplied path name relative to the specified BDirectory and
// according to the specified open mode.
status_t
BFile::SetTo(const BDirectory* dir, const char* path, uint32 openMode)
{
	Unset();

	if (!dir)
		return (fCStatus = B_BAD_VALUE);

	openMode |= O_CLOEXEC;

	int fd = _kern_open(dir->fDirFd, path, openMode, DEFFILEMODE & ~__gUmask);
	if (fd >= 0) {
		set_fd(fd);
		fMode = openMode;
		fCStatus = B_OK;
	} else
		fCStatus = fd;

	return fCStatus;
}


// Reports whether or not the file is readable.
bool
BFile::IsReadable() const
{
	return InitCheck() == B_OK
		&& ((fMode & O_RWMASK) == O_RDONLY || (fMode & O_RWMASK) == O_RDWR);
}


// Reports whether or not the file is writable.
bool
BFile::IsWritable() const
{
	return InitCheck() == B_OK
		&& ((fMode & O_RWMASK) == O_WRONLY || (fMode & O_RWMASK) == O_RDWR);
}


// Reads a number of bytes from the file into a buffer.
ssize_t
BFile::Read(void* buffer, size_t size)
{
	if (InitCheck() != B_OK)
		return InitCheck();
	return _kern_read(get_fd(), -1, buffer, size);
}


// Reads a number of bytes from a certain position within the file
// into a buffer.
ssize_t
BFile::ReadAt(off_t location, void* buffer, size_t size)
{
	if (InitCheck() != B_OK)
		return InitCheck();
	if (location < 0)
		return B_BAD_VALUE;

	return _kern_read(get_fd(), location, buffer, size);
}


// Writes a number of bytes from a buffer into the file.
ssize_t
BFile::Write(const void* buffer, size_t size)
{
	if (InitCheck() != B_OK)
		return InitCheck();
	return _kern_write(get_fd(), -1, buffer, size);
}


// Writes a number of bytes from a buffer at a certain position
// into the file.
ssize_t
BFile::WriteAt(off_t location, const void* buffer, size_t size)
{
	if (InitCheck() != B_OK)
		return InitCheck();
	if (location < 0)
		return B_BAD_VALUE;

	return _kern_write(get_fd(), location, buffer, size);
}


// Seeks to another read/write position within the file.
off_t
BFile::Seek(off_t offset, uint32 seekMode)
{
	if (InitCheck() != B_OK)
		return B_FILE_ERROR;
	return _kern_seek(get_fd(), offset, seekMode);
}


// Gets the current read/write position within the file.
off_t
BFile::Position() const
{
	if (InitCheck() != B_OK)
		return B_FILE_ERROR;
	return _kern_seek(get_fd(), 0, SEEK_CUR);
}


// Sets the size of the file.
status_t
BFile::SetSize(off_t size)
{
	if (InitCheck() != B_OK)
		return InitCheck();
	if (size < 0)
		return B_BAD_VALUE;
	struct stat statData;
	statData.st_size = size;
	return set_stat(statData, B_STAT_SIZE | B_STAT_SIZE_INSECURE);
}


// Gets the size of the file.
status_t
BFile::GetSize(off_t* size) const
{
	return BStatable::GetSize(size);
}


// Assigns another BFile to this BFile.
BFile&
BFile::operator=(const BFile &file)
{
	if (&file != this) {
		// no need to assign us to ourselves
		Unset();
		if (file.InitCheck() == B_OK) {
			// duplicate the file descriptor
			int fd = _kern_dup(file.get_fd());
			// set it
			if (fd >= 0) {
				fFd = fd;
				fMode = file.fMode;
				fCStatus = B_OK;
			} else
				fCStatus = fd;
		}
	}
	return *this;
}


// FBC
void BFile::_PhiloFile1() {}
void BFile::_PhiloFile2() {}
void BFile::_PhiloFile3() {}
void BFile::_PhiloFile4() {}
void BFile::_PhiloFile5() {}
void BFile::_PhiloFile6() {}


// Gets the file descriptor of the BFile.
int
BFile::get_fd() const
{
	return fFd;
}


// Overrides BNode::close_fd() for binary compatibility with BeOS R5.
void
BFile::close_fd()
{
	BNode::close_fd();
}
