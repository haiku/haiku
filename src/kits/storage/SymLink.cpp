/*
 * Copyright 2002-2009, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Dauwalder
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 */


/*!	\file SymLink.cpp
	BSymLink implementation.
*/


#include <new>
#include <string.h>

#include <SymLink.h>
#include <Directory.h>
#include <Entry.h>
#include <Path.h>

#include <syscalls.h>

#include "storage_support.h"

using namespace std;


//! Creates an uninitialized BSymLink object.
BSymLink::BSymLink()
{
}


//! Creates a copy of the supplied BSymLink.
/*!	\param link the BSymLink object to be copied
*/
BSymLink::BSymLink(const BSymLink &link)
	:
	BNode(link)
{
}


/*! \brief Creates a BSymLink and initializes it to the symbolic link referred
	to by the supplied entry_ref.
	\param ref the entry_ref referring to the symbolic link
*/
BSymLink::BSymLink(const entry_ref *ref)
	:
	BNode(ref)
{
}


/*! \brief Creates a BSymLink and initializes it to the symbolic link referred
	to by the supplied BEntry.
	\param entry the BEntry referring to the symbolic link
*/
BSymLink::BSymLink(const BEntry *entry)
		: BNode(entry)
{
}


/*! \brief Creates a BSymLink and initializes it to the symbolic link referred
	to by the supplied path name.
	\param path the symbolic link's path name 
*/
BSymLink::BSymLink(const char *path)
	:
	BNode(path)
{
}


/*! \brief Creates a BSymLink and initializes it to the symbolic link referred
	to by the supplied path name relative to the specified BDirectory.
	\param dir the BDirectory, relative to which the symbolic link's path name
		   is given
	\param path the symbolic link's path name relative to \a dir
*/
BSymLink::BSymLink(const BDirectory *dir, const char *path)
	:
	BNode(dir, path)
{
}


//! Frees all allocated resources.
/*! If the BSymLink is properly initialized, the symbolic link's file
	descriptor is closed.
*/
BSymLink::~BSymLink()
{
}


//! Reads the contents of the symbolic link into a buffer.
/*!	The string written to the buffer will be null-terminated.
	\param buf the buffer
	\param size the size of the buffer
	\return
	- the number of bytes written into the buffer
	- \c B_BAD_VALUE: \c NULL \a buf or the object doesn't refer to a symbolic
	  link.
	- \c B_FILE_ERROR: The object is not initialized.
	- some other error code
*/
ssize_t
BSymLink::ReadLink(char *buffer, size_t size)
{
	if (!buffer)
		return B_BAD_VALUE;
	if (InitCheck() != B_OK)
		return B_FILE_ERROR;

	size_t linkLen = size;
	status_t error = _kern_read_link(get_fd(), NULL, buffer, &linkLen);
	if (error < B_OK)
		return error;

	// null-terminate
	if (linkLen >= size)
		return B_BUFFER_OVERFLOW;
	buffer[linkLen] = '\0';

	return linkLen;
}


/*!	\brief Combines a directory path and the contents of this symbolic link to
	an absolute path.
	\param dirPath the path name of the directory
	\param path the BPath object to be set to the resulting path name
	\return
	- \c the length of the resulting path name,
	- \c B_BAD_VALUE: \c NULL \a dirPath or \a path or the object doesn't
		 refer to a symbolic link.
	- \c B_FILE_ERROR: The object is not initialized.
	- \c B_NAME_TOO_LONG: The resulting path name is too long.
	- some other error code
*/
ssize_t
BSymLink::MakeLinkedPath(const char *dirPath, BPath *path)
{
	// BeOS seems to convert the dirPath to a BDirectory, which causes links to
	// be resolved.
	// This does also mean that the dirPath must exist!
	if (!dirPath || !path)
		return B_BAD_VALUE;
	BDirectory dir(dirPath);
	ssize_t result = dir.InitCheck();
	if (result == B_OK)
		result = MakeLinkedPath(&dir, path);
	return result;
}


/*!	\brief Combines a directory path and the contents of this symbolic link to
	an absolute path.
	\param dir the BDirectory referring to the directory
	\param path the BPath object to be set to the resulting path name
	\return
	- \c the length of the resulting path name,
	- \c B_BAD_VALUE: \c NULL \a dir or \a path or the object doesn't
		 refer to a symbolic link.
	- \c B_FILE_ERROR: The object is not initialized.
	- \c B_NAME_TOO_LONG: The resulting path name is too long.
	- some other error code
*/
ssize_t
BSymLink::MakeLinkedPath(const BDirectory *dir, BPath *path)
{
	if (!dir || !path)
		return B_BAD_VALUE;
	char contents[B_PATH_NAME_LENGTH];
	ssize_t result = ReadLink(contents, sizeof(contents));
	if (result >= 0) {
		if (BPrivate::Storage::is_absolute_path(contents))
			result = path->SetTo(contents);
		else
			result = path->SetTo(dir, contents);
		if (result == B_OK)
			result = strlen(path->Path());
	}
	return result;
}


//!	Returns whether this BSymLink refers to an absolute link.
/*!	/return
	- \c true, if the object is properly initialized and the symbolic link it
	  refers to is an absolute link,
	- \c false, otherwise.
*/
bool
BSymLink::IsAbsolute()
{
	char contents[B_PATH_NAME_LENGTH];
	bool result = (ReadLink(contents, sizeof(contents)) >= 0);
	if (result)
		result = BPrivate::Storage::is_absolute_path(contents);
	return result;
}


void BSymLink::_MissingSymLink1() {}
void BSymLink::_MissingSymLink2() {}
void BSymLink::_MissingSymLink3() {}
void BSymLink::_MissingSymLink4() {}
void BSymLink::_MissingSymLink5() {}
void BSymLink::_MissingSymLink6() {}


//! Returns the BSymLink's file descriptor.
/*! To be used instead of accessing the BNode's private \c fFd member directly.
	\return the file descriptor, or -1, if not properly initialized.
*/
int
BSymLink::get_fd() const
{
	return fFd;
}
