/*
 * Copyright 2002-2009 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Dauwalder
 *		Ingo Weinhold, ingo_weinhold@gmx.de
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


// Creates an uninitialized BSymLink object.
BSymLink::BSymLink()
{
}


// Creates a copy of the supplied BSymLink object.
BSymLink::BSymLink(const BSymLink& other)
	:
	BNode(other)
{
}


// Creates a BSymLink object and initializes it to the symbolic link referred
// to by the supplied entry_ref.
BSymLink::BSymLink(const entry_ref* ref)
	:
	BNode(ref)
{
}


// Creates a BSymLink object and initializes it to the symbolic link referred
// to by the supplied BEntry.
BSymLink::BSymLink(const BEntry* entry)
		: BNode(entry)
{
}


// Creates a BSymLink object and initializes it to the symbolic link referred
// to by the supplied path name.
BSymLink::BSymLink(const char* path)
	:
	BNode(path)
{
}


// Creates a BSymLink object and initializes it to the symbolic link referred
// to by the supplied path name relative to the specified BDirectory.
BSymLink::BSymLink(const BDirectory* dir, const char* path)
	:
	BNode(dir, path)
{
}


// Destroys the object and frees all allocated resources.
BSymLink::~BSymLink()
{
}


// Reads the contents of the symbolic link into a buffer.
ssize_t
BSymLink::ReadLink(char* buffer, size_t size)
{
	if (buffer == NULL)
		return B_BAD_VALUE;

	if (InitCheck() != B_OK)
		return B_FILE_ERROR;

	size_t linkLen = size;
	status_t result = _kern_read_link(get_fd(), NULL, buffer, &linkLen);
	if (result < B_OK)
		return result;

	if (linkLen < size)
		buffer[linkLen] = '\0';
	else if (size > 0)
		buffer[size - 1] = '\0';

	return linkLen;
}


// Combines a directory path and the contents of this symbolic link to form an
// absolute path.
ssize_t
BSymLink::MakeLinkedPath(const char* dirPath, BPath* path)
{
	// BeOS seems to convert the dirPath to a BDirectory, which causes links
	// to be resolved. This means that the dirPath must exist!
	if (dirPath == NULL || path == NULL)
		return B_BAD_VALUE;

	BDirectory dir(dirPath);
	ssize_t result = dir.InitCheck();
	if (result == B_OK)
		result = MakeLinkedPath(&dir, path);

	return result;
}


// Combines a directory path and the contents of this symbolic link to form an
// absolute path.
ssize_t
BSymLink::MakeLinkedPath(const BDirectory* dir, BPath* path)
{
	if (dir == NULL || path == NULL)
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


// Returns whether or not the object refers to an absolute path.
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


/*!	Returns the file descriptor of the BSymLink.

	This method should be used instead of accessing the private \c fFd member
	of the BNode directly.

	\return The object's file descriptor, or -1 if not properly initialized.
*/
int
BSymLink::get_fd() const
{
	return fFd;
}
