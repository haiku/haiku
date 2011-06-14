//----------------------------------------------------------------------
//  This software is part of the Haiku distribution and is covered
//  by the MIT license.
//---------------------------------------------------------------------
/*!
	\file File.cpp
	BFile implementation.
*/

#include <fcntl.h>
#include <unistd.h>

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <NodeMonitor.h>

#include <syscalls.h>


extern mode_t __gUmask;
	// declared in sys/umask.c


#ifdef USE_OPENBEOS_NAMESPACE
namespace OpenBeOS {
#endif

// constructor
//! Creates an uninitialized BFile.
BFile::BFile()
	 : BNode(),
	   BPositionIO(),
	   fMode(0)
{
}

// copy constructor
//! Creates a copy of the supplied BFile.
/*! If \a file is uninitialized, the newly constructed BFile will be, too.
	\param file the BFile object to be copied
*/
BFile::BFile(const BFile &file)
	 : BNode(),
	   BPositionIO(),
	   fMode(0)
{
	*this = file;
}

// constructor
/*! \brief Creates a BFile and initializes it to the file referred to by
		   the supplied entry_ref and according to the specified open mode.
	\param ref the entry_ref referring to the file
	\param openMode the mode in which the file should be opened
	\see SetTo() for values for \a openMode
*/
BFile::BFile(const entry_ref *ref, uint32 openMode)
	 : BNode(),
	   BPositionIO(),
	   fMode(0)
{
	SetTo(ref, openMode);
}

// constructor
/*! \brief Creates a BFile and initializes it to the file referred to by
		   the supplied BEntry and according to the specified open mode.
	\param entry the BEntry referring to the file
	\param openMode the mode in which the file should be opened
	\see SetTo() for values for \a openMode
*/
BFile::BFile(const BEntry *entry, uint32 openMode)
	 : BNode(),
	   BPositionIO(),
	   fMode(0)
{
	SetTo(entry, openMode);
}

// constructor
/*! \brief Creates a BFile and initializes it to the file referred to by
		   the supplied path name and according to the specified open mode.
	\param path the file's path name
	\param openMode the mode in which the file should be opened
	\see SetTo() for values for \a openMode
*/
BFile::BFile(const char *path, uint32 openMode)
	 : BNode(),
	   BPositionIO(),
	   fMode(0)
{
	SetTo(path, openMode);
}

// constructor
/*! \brief Creates a BFile and initializes it to the file referred to by
		   the supplied path name relative to the specified BDirectory and
		   according to the specified open mode.
	\param dir the BDirectory, relative to which the file's path name is
		   given
	\param path the file's path name relative to \a dir
	\param openMode the mode in which the file should be opened
	\see SetTo() for values for \a openMode
*/
BFile::BFile(const BDirectory *dir, const char *path, uint32 openMode)
	 : BNode(),
	   BPositionIO(),
	   fMode(0)
{
	SetTo(dir, path, openMode);
}

// destructor
//! Frees all allocated resources.
/*!	If the file is properly initialized, the file's file descriptor is closed.
*/
BFile::~BFile()
{
	// Also called by the BNode destructor, but we rather try to avoid
	// problems with calling virtual functions in the base class destructor.
	// Depending on the compiler implementation an object may be degraded to
	// an object of the base class after the destructor of the derived class
	// has been executed.
	close_fd();
}

// SetTo
/*! \brief Re-initializes the BFile to the file referred to by the
		   supplied entry_ref and according to the specified open mode.
	\param ref the entry_ref referring to the file
	\param openMode the mode in which the file should be opened
	\a openMode must be a bitwise or of exactly one of the flags
	- \c B_READ_ONLY: The file is opened read only.
	- \c B_WRITE_ONLY: The file is opened write only.
	- \c B_READ_WRITE: The file is opened for random read/write access.
	and any number of the flags
	- \c B_CREATE_FILE: A new file will be created, if it does not already
	  exist.
	- \c B_FAIL_IF_EXISTS: If the file does already exist and B_CREATE_FILE is
	  set, SetTo() fails.
	- \c B_ERASE_FILE: An already existing file is truncated to zero size.
	- \c B_OPEN_AT_END: Seek() to the end of the file after opening.
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a ref or bad \a openMode.
	- \c B_ENTRY_NOT_FOUND: File not found or failed to create file.
	- \c B_FILE_EXISTS: File exists and \c B_FAIL_IF_EXISTS was passed.
	- \c B_PERMISSION_DENIED: File permissions didn't allow operation.
	- \c B_NO_MEMORY: Insufficient memory for operation.
	- \c B_LINK_LIMIT: Indicates a cyclic loop within the file system.
	- \c B_BUSY: A node was busy.
	- \c B_FILE_ERROR: A general file error.
	- \c B_NO_MORE_FDS: The application has run out of file descriptors.
*/
status_t
BFile::SetTo(const entry_ref *ref, uint32 openMode)
{
	Unset();

	if (!ref)
		return (fCStatus = B_BAD_VALUE);

	int fd = _kern_open_entry_ref(ref->device, ref->directory, ref->name,
		openMode, DEFFILEMODE & ~__gUmask);
	if (fd >= 0) {
		set_fd(fd);
		fMode = openMode;
		fCStatus = B_OK;

		fcntl(fd, F_SETFD, FD_CLOEXEC);

	} else
		fCStatus = fd;

	return fCStatus;
}

// SetTo
/*! \brief Re-initializes the BFile to the file referred to by the
		   supplied BEntry and according to the specified open mode.
	\param entry the BEntry referring to the file
	\param openMode the mode in which the file should be opened
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a entry or bad \a openMode.
	- \c B_ENTRY_NOT_FOUND: File not found or failed to create file.
	- \c B_FILE_EXISTS: File exists and \c B_FAIL_IF_EXISTS was passed.
	- \c B_PERMISSION_DENIED: File permissions didn't allow operation.
	- \c B_NO_MEMORY: Insufficient memory for operation.
	- \c B_LINK_LIMIT: Indicates a cyclic loop within the file system.
	- \c B_BUSY: A node was busy.
	- \c B_FILE_ERROR: A general file error.
	- \c B_NO_MORE_FDS: The application has run out of file descriptors.
	\todo Implemented using SetTo(entry_ref*, uint32). Check, if necessary
		  to reimplement!
*/
status_t
BFile::SetTo(const BEntry *entry, uint32 openMode)
{
	Unset();

	if (!entry)
		return (fCStatus = B_BAD_VALUE);
	if (entry->InitCheck() != B_OK)
		return (fCStatus = entry->InitCheck());

	int fd = _kern_open(entry->fDirFd, entry->fName, openMode,
		DEFFILEMODE & ~__gUmask);
	if (fd >= 0) {
		set_fd(fd);
		fMode = openMode;
		fCStatus = B_OK;

		fcntl(fd, F_SETFD, FD_CLOEXEC);
	} else
		fCStatus = fd;

	return fCStatus;
}

// SetTo
/*! \brief Re-initializes the BFile to the file referred to by the
		   supplied path name and according to the specified open mode.
	\param path the file's path name
	\param openMode the mode in which the file should be opened
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a path or bad \a openMode.
	- \c B_ENTRY_NOT_FOUND: File not found or failed to create file.
	- \c B_FILE_EXISTS: File exists and \c B_FAIL_IF_EXISTS was passed.
	- \c B_PERMISSION_DENIED: File permissions didn't allow operation.
	- \c B_NO_MEMORY: Insufficient memory for operation.
	- \c B_LINK_LIMIT: Indicates a cyclic loop within the file system.
	- \c B_BUSY: A node was busy.
	- \c B_FILE_ERROR: A general file error.
	- \c B_NO_MORE_FDS: The application has run out of file descriptors.
*/
status_t
BFile::SetTo(const char *path, uint32 openMode)
{
	Unset();

	if (!path)
		return (fCStatus = B_BAD_VALUE);

	int fd = _kern_open(-1, path, openMode, DEFFILEMODE & ~__gUmask);
	if (fd >= 0) {
		set_fd(fd);
		fMode = openMode;
		fCStatus = B_OK;

		fcntl(fd, F_SETFD, FD_CLOEXEC);
	} else
		fCStatus = fd;

	return fCStatus;
}

// SetTo
/*! \brief Re-initializes the BFile to the file referred to by the
		   supplied path name relative to the specified BDirectory and
		   according to the specified open mode.
	\param dir the BDirectory, relative to which the file's path name is
		   given
	\param path the file's path name relative to \a dir
	\param openMode the mode in which the file should be opened
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a dir or \a path or bad \a openMode.
	- \c B_ENTRY_NOT_FOUND: File not found or failed to create file.
	- \c B_FILE_EXISTS: File exists and \c B_FAIL_IF_EXISTS was passed.
	- \c B_PERMISSION_DENIED: File permissions didn't allow operation.
	- \c B_NO_MEMORY: Insufficient memory for operation.
	- \c B_LINK_LIMIT: Indicates a cyclic loop within the file system.
	- \c B_BUSY: A node was busy.
	- \c B_FILE_ERROR: A general file error.
	- \c B_NO_MORE_FDS: The application has run out of file descriptors.
	\todo Implemented using SetTo(BEntry*, uint32). Check, if necessary
		  to reimplement!
*/
status_t
BFile::SetTo(const BDirectory *dir, const char *path, uint32 openMode)
{
	Unset();

	if (!dir)
		return (fCStatus = B_BAD_VALUE);

	int fd = _kern_open(dir->fDirFd, path, openMode, DEFFILEMODE & ~__gUmask);
	if (fd >= 0) {
		set_fd(fd);
		fMode = openMode;
		fCStatus = B_OK;

		fcntl(fd, F_SETFD, FD_CLOEXEC);
	} else
		fCStatus = fd;

	return fCStatus;
}

// IsReadable
//! Returns whether the file is readable.
/*!	\return
	- \c true, if the BFile has been initialized properly and the file has
	  been been opened for reading,
	- \c false, otherwise.
*/
bool
BFile::IsReadable() const
{
	return (InitCheck() == B_OK
			&& ((fMode & O_ACCMODE) == O_RDONLY
				|| (fMode & O_ACCMODE) == O_RDWR));
}

// IsWritable
//!	Returns whether the file is writable.
/*!	\return
	- \c true, if the BFile has been initialized properly and the file has
	  been opened for writing,
	- \c false, otherwise.
*/
bool
BFile::IsWritable() const
{
	return (InitCheck() == B_OK
			&& ((fMode & O_ACCMODE) == O_WRONLY
				|| (fMode & O_ACCMODE) == O_RDWR));
}

// Read
//!	Reads a number of bytes from the file into a buffer.
/*!	\param buffer the buffer the data from the file shall be written to
	\param size the number of bytes that shall be read
	\return the number of bytes actually read or an error code
*/
ssize_t
BFile::Read(void *buffer, size_t size)
{
	if (InitCheck() != B_OK)
		return InitCheck();
	return _kern_read(get_fd(), -1, buffer, size);
}

// ReadAt
/*!	\brief Reads a number of bytes from a certain position within the file
		   into a buffer.
	\param location the position (in bytes) within the file from which the
		   data shall be read
	\param buffer the buffer the data from the file shall be written to
	\param size the number of bytes that shall be read
	\return the number of bytes actually read or an error code
*/
ssize_t
BFile::ReadAt(off_t location, void *buffer, size_t size)
{
	if (InitCheck() != B_OK)
		return InitCheck();
	if (location < 0)
		return B_BAD_VALUE;
	return _kern_read(get_fd(), location, buffer, size);
}

// Write
//!	Writes a number of bytes from a buffer into the file.
/*!	\param buffer the buffer containing the data to be written to the file
	\param size the number of bytes that shall be written
	\return the number of bytes actually written or an error code
*/
ssize_t
BFile::Write(const void *buffer, size_t size)
{
	if (InitCheck() != B_OK)
		return InitCheck();
	return _kern_write(get_fd(), -1, buffer, size);
}

// WriteAt
/*!	\brief Writes a number of bytes from a buffer at a certain position
		   into the file.
	\param location the position (in bytes) within the file at which the data
		   shall be written
	\param buffer the buffer containing the data to be written to the file
	\param size the number of bytes that shall be written
	\return the number of bytes actually written or an error code
*/
ssize_t
BFile::WriteAt(off_t location, const void *buffer, size_t size)
{
	if (InitCheck() != B_OK)
		return InitCheck();
	if (location < 0)
		return B_BAD_VALUE;
	return _kern_write(get_fd(), location, buffer, size);
}

// Seek
//!	Seeks to another read/write position within the file.
/*!	It is allowed to seek past the end of the file. A subsequent call to
	Write() will pad the file with undefined data. Seeking before the
	beginning of the file will fail and the behavior of subsequent Read()
	or Write() invocations will be undefined.
	\param offset new read/write position, depending on \a seekMode relative
		   to the beginning or the end of the file or the current position
	\param seekMode:
		- \c SEEK_SET: move relative to the beginning of the file
		- \c SEEK_CUR: move relative to the current position
		- \c SEEK_END: move relative to the end of the file
	\return
	- the new read/write position relative to the beginning of the file
	- \c B_ERROR when trying to seek before the beginning of the file
	- \c B_FILE_ERROR, if the file is not properly initialized
*/
off_t
BFile::Seek(off_t offset, uint32 seekMode)
{
	if (InitCheck() != B_OK)
		return B_FILE_ERROR;
	return _kern_seek(get_fd(), offset, seekMode);
}

// Position
//!	Returns the current read/write position within the file.
/*!	\return
	- the current read/write position relative to the beginning of the file
	- \c B_ERROR, after a Seek() before the beginning of the file
	- \c B_FILE_ERROR, if the file has not been initialized
*/
off_t
BFile::Position() const
{
	if (InitCheck() != B_OK)
		return B_FILE_ERROR;
	return _kern_seek(get_fd(), 0, SEEK_CUR);
}

// SetSize
//!	Sets the size of the file.
/*!	If the file is shorter than \a size bytes it will be padded with
	unspecified data to the requested size. If it is larger, it will be
	truncated.
	Note: There's no problem with setting the size of a BFile opened in
	\c B_READ_ONLY mode, unless the file resides on a read only volume.
	\param size the new file size
	\return
	- \c B_OK, if everything went fine
	- \c B_NOT_ALLOWED, if trying to set the size of a file on a read only
	  volume
	- \c B_DEVICE_FULL, if there's not enough space left on the volume
*/
status_t
BFile::SetSize(off_t size)
{
	if (InitCheck() != B_OK)
		return InitCheck();
	if (size < 0)
		return B_BAD_VALUE;
	struct stat statData;
	statData.st_size = size;
	return set_stat(statData, B_STAT_SIZE);
}


status_t
BFile::GetSize(off_t* size) const
{
	return BStatable::GetSize(size);
}


// =
//!	Assigns another BFile to this BFile.
/*!	If the other BFile is uninitialized, this one will be too. Otherwise it
	will refer to the same file using the same mode, unless an error occurs.
	\param file the original BFile
	\return a reference to this BFile
*/
BFile &
BFile::operator=(const BFile &file)
{
	if (&file != this) {	// no need to assign us to ourselves
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


// get_fd
/*!	Returns the file descriptor.
	To be used instead of accessing the BNode's private \c fFd member directly.
	\return the file descriptor, or -1, if not properly initialized.
*/
int
BFile::get_fd() const
{
	return fFd;
}

// close_fd
/*!	Overrides BNode::close_fd() solely for R5 binary compatibility.
*/
void
BFile::close_fd()
{
	BNode::close_fd();
}


#ifdef USE_OPENBEOS_NAMESPACE
};		// namespace OpenBeOS
#endif

