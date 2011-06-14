/*
 * Copyright 2002-2008, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Dauwalder
 *		Ingo Weinhold, bonefish@users.sf.net
 */

/*!
	\file Entry.cpp
	BEntry and entry_ref implementations.
*/

#include <Entry.h>

#include <fcntl.h>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <Directory.h>
#include <Path.h>
#include <SymLink.h>

#include <syscalls.h>

#include "storage_support.h"


using namespace std;

// SYMLINK_MAX is needed by B_SYMLINK_MAX
// I don't know, why it isn't defined.
#ifndef SYMLINK_MAX
#define SYMLINK_MAX (16)
#endif


/*! \struct entry_ref
	\brief A filesystem entry represented as a name in a concrete directory.

	entry_refs may refer to pre-existing (concrete) files, as well as non-existing
	(abstract) files. However, the parent directory of the file \b must exist.

	The result of this dichotomy is a blending of the persistence gained by referring
	to entries with a reference to their internal filesystem node and the flexibility gained
	by referring to entries by name.

	For example, if the directory in which the entry resides (or a
	directory further up in the hierarchy) is moved or renamed, the entry_ref will
	still refer to the correct file (whereas a pathname to the previous location of the
	file would now be invalid).

	On the other hand, say that the entry_ref refers to a concrete file. If the file
	itself is renamed, the entry_ref now refers to an abstract file with the old name
	(the upside in this case is that abstract entries may be represented by entry_refs
	without	preallocating an internal filesystem node for them).
*/


//! Creates an unitialized entry_ref.
entry_ref::entry_ref()
	:
	device((dev_t)-1),
	directory((ino_t)-1),
	name(NULL)
{
}


/*! \brief Creates an entry_ref initialized to the given file name in the given
	directory on the given device.

	\p name may refer to either a pre-existing file in the given
	directory, or a non-existent file. No explicit checking is done to verify validity of the given arguments, but
	later use of the entry_ref will fail if \p dev is not a valid device or \p dir
	is a not a directory on \p dev.

	\param dev the device on which the entry's parent directory resides
	\param dir the directory in which the entry resides
	\param name the leaf name of the entry, which is not required to exist
*/
entry_ref::entry_ref(dev_t dev, ino_t dir, const char *name)
	:
	device(dev),
	directory(dir),
	name(NULL)
{
	set_name(name);
}


/*! \brief Creates a copy of the given entry_ref.
	\param ref a reference to an entry_ref to copy
*/
entry_ref::entry_ref(const entry_ref &ref)
	:
	device(ref.device),
	directory(ref.directory),
	name(NULL)
{
	set_name(ref.name);
}


/*!	Destroys the object and frees the storage allocated for the leaf name,
	if necessary.
*/
entry_ref::~entry_ref()
{
	free(name);
}


/*! \brief Set the entry_ref's leaf name, freeing the storage allocated for any previous
	name and then making a copy of the new name.

	\param name pointer to a null-terminated string containing the new name for
	the entry. May be \c NULL.
*/
status_t
entry_ref::set_name(const char *newName)
{
	free(name);

	if (newName == NULL) {
		name = NULL;
	} else {
		name = strdup(newName);
		if (!name)
			return B_NO_MEMORY;
	}

	return B_OK;
}


/*! \brief Compares the entry_ref with another entry_ref, returning true if
	they are equal.

	\return
	- \c true - The entry_refs are equal
	- \c false - The entry_refs are not equal
*/
bool
entry_ref::operator==(const entry_ref &ref) const
{
	return device == ref.device
		&& directory == ref.directory
		&& (name == ref.name
			|| (name != NULL && ref.name != NULL && !strcmp(name, ref.name)));
}


/*! \brief Compares the entry_ref with another entry_ref, returning true if they are not equal.
	\return
	- \c true - The entry_refs are not equal
	- \c false - The entry_refs are equal
*/
bool
entry_ref::operator!=(const entry_ref &ref) const
{
	return !(*this == ref);
}


/*! \brief Makes the entry_ref a copy of the entry_ref specified by \a ref.
	\param ref the entry_ref to copy
	\return
	- A reference to the copy
*/
entry_ref&
entry_ref::operator=(const entry_ref &ref)
{
	if (this == &ref)
		return *this;

	device = ref.device;
	directory = ref.directory;
	set_name(ref.name);
	return *this;
}

/*!
	\var dev_t entry_ref::device
	\brief The device id of the storage device on which the entry resides

*/

/*!
	\var ino_t entry_ref::directory
	\brief The inode number of the directory in which the entry resides
*/

/*!
	\var char *entry_ref::name
	\brief The leaf name of the entry
*/

/*!
	\class BEntry
	\brief A location in the filesystem

	The BEntry class defines objects that represent "locations" in the file system
	hierarchy.  Each location (or entry) is given as a name within a directory. For
	example, when you create a BEntry thus:

	\code
	BEntry entry("/boot/home/fido");
	\endcode

	...you're telling the BEntry object to represent the location of the file
	called fido within the directory \c "/boot/home".

	\author <a href='mailto:bonefish@users.sf.net'>Ingo Weinhold</a>
	\author <a href='mailto:tylerdauwalder@users.sf.net'>Tyler Dauwalder</a>
	\author <a href='mailto:scusack@users.sf.net'>Simon Cusack</a>

	\version 0.0.0
*/


//	#pragma mark -


//! Creates an uninitialized BEntry object.
/*!	Should be followed by a	call to one of the SetTo functions,
	or an assignment:
	- SetTo(const BDirectory*, const char*, bool)
	- SetTo(const entry_ref*, bool)
	- SetTo(const char*, bool)
	- operator=(const BEntry&)
*/
BEntry::BEntry()
	:
	fDirFd(-1),
	fName(NULL),
	fCStatus(B_NO_INIT)
{
}


//! Creates a BEntry initialized to the given directory and path combination.
/*!	If traverse is true and \c dir/path refers to a symlink, the BEntry will
	refer to the linked file; if false,	the BEntry will refer to the symlink itself.

	\param dir directory in which \a path resides
	\param path relative path reckoned off of \a dir
	\param traverse whether or not to traverse symlinks
	\see SetTo(const BDirectory*, const char *, bool)

*/
BEntry::BEntry(const BDirectory *dir, const char *path, bool traverse)
	:
	fDirFd(-1),
	fName(NULL),
	fCStatus(B_NO_INIT)
{
	SetTo(dir, path, traverse);
}


//! Creates a BEntry for the file referred to by the given entry_ref.
/*!	If traverse is true and \a ref refers to a symlink, the BEntry
	will refer to the linked file; if false, the BEntry will refer
	to the symlink itself.

	\param ref the entry_ref referring to the given file
	\param traverse whether or not symlinks are to be traversed
	\see SetTo(const entry_ref*, bool)
*/
BEntry::BEntry(const entry_ref *ref, bool traverse)
	:
	fDirFd(-1),
	fName(NULL),
	fCStatus(B_NO_INIT)
{
	SetTo(ref, traverse);
}


//! Creates a BEntry initialized to the given path.
/*!	If \a path is relative, it will
	be reckoned off the current working directory. If \a path refers to a symlink and
	traverse is true, the BEntry will refer to the linked file. If traverse is false,
	the BEntry will refer to the symlink itself.

	\param path the file of interest
	\param traverse whether or not symlinks are to be traversed
	\see SetTo(const char*, bool)

*/
BEntry::BEntry(const char *path, bool traverse)
	:
	fDirFd(-1),
	fName(NULL),
	fCStatus(B_NO_INIT)
{
	SetTo(path, traverse);
}


//! Creates a copy of the given BEntry.
/*! \param entry the entry to be copied
	\see operator=(const BEntry&)
*/
BEntry::BEntry(const BEntry &entry)
	  : fDirFd(-1),
		fName(NULL),
		fCStatus(B_NO_INIT)
{
	*this = entry;
}


//! Frees all of the BEntry's allocated resources.
/*! \see Unset()
*/
BEntry::~BEntry()
{
	Unset();
}


//! Returns the result of the most recent construction or SetTo() call.
/*! \return
		- \c B_OK Success
		- \c B_NO_INIT The object has been Unset() or is uninitialized
		- <code>some error code</code>
*/
status_t
BEntry::InitCheck() const
{
	return fCStatus;
}


/*! \brief Returns true if the Entry exists in the filesytem, false otherwise.
	\return
		- \c true - The entry exists
		- \c false - The entry does not exist
*/
bool
BEntry::Exists() const
{
	// just stat the beast
	struct stat st;
	return (GetStat(&st) == B_OK);
}


/*! \brief Fills in a stat structure for the entry. The information is copied into
	the \c stat structure pointed to by \a result.

	\b NOTE: The BStatable object does not cache the stat structure; every time you
	call GetStat(), fresh stat information is retrieved.

	\param result pointer to a pre-allocated structure into which the stat information will be copied
	\return
	- \c B_OK - Success
	- "error code" - Failure
*/
status_t
BEntry::GetStat(struct stat *result) const
{
	if (fCStatus != B_OK)
		return B_NO_INIT;
	return _kern_read_stat(fDirFd, fName, false, result, sizeof(struct stat));
}


/*! \brief Reinitializes the BEntry to the path or directory path combination,
	resolving symlinks if traverse is true

	\return
	- \c B_OK - Success
	- "error code" - Failure
*/
status_t
BEntry::SetTo(const BDirectory *dir, const char *path, bool traverse)
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
	return (fCStatus = set(dirFD, path, traverse));
}


/*! \brief Reinitializes the BEntry to the entry_ref, resolving symlinks if
	traverse is true

	\return
	- \c B_OK - Success
	- "error code" - Failure
*/
status_t
BEntry::SetTo(const entry_ref *ref, bool traverse)
{
	Unset();
	if (ref == NULL)
		return (fCStatus = B_BAD_VALUE);

	// open the directory and let set() do the rest
	int dirFD = _kern_open_dir_entry_ref(ref->device, ref->directory, NULL);
	if (dirFD < 0)
		return (fCStatus = dirFD);
	return (fCStatus = set(dirFD, ref->name, traverse));
}


/*! \brief Reinitializes the BEntry object to the path, resolving symlinks if
	traverse is true

	\return
	- \c B_OK - Success
	- "error code" - Failure
*/
status_t
BEntry::SetTo(const char *path, bool traverse)
{
	Unset();
	// check the argument
	if (!path)
		return (fCStatus = B_BAD_VALUE);
	return (fCStatus = set(-1, path, traverse));
}


/*! \brief Reinitializes the BEntry to an uninitialized BEntry object */
void
BEntry::Unset()
{
	// Close the directory
	if (fDirFd >= 0) {
		_kern_close(fDirFd);
//		BPrivate::Storage::close_dir(fDirFd);
	}

	// Free our leaf name
	free(fName);

	fDirFd = -1;
	fName = NULL;
	fCStatus = B_NO_INIT;
}


/*! \brief Gets an entry_ref structure for the BEntry.

	\param ref pointer to a preallocated entry_ref into which the result is copied
	\return
	- \c B_OK - Success
	- "error code" - Failure

 */
status_t
BEntry::GetRef(entry_ref *ref) const
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


/*! \brief Gets the path for the BEntry.

	\param path pointer to a pre-allocated BPath object into which the result is stored
	\return
	- \c B_OK - Success
	- "error code" - Failure

*/
status_t
BEntry::GetPath(BPath *path) const
{
	if (fCStatus != B_OK)
		return B_NO_INIT;

	if (path == NULL)
		return B_BAD_VALUE;

	return path->SetTo(this);
}


/*! \brief Gets the parent of the BEntry as another BEntry.

	If the function fails, the argument is Unset(). Destructive calls to GetParent() are
	allowed, i.e.:

	\code
	BEntry entry("/boot/home/fido");
	status_t err;
	char name[B_FILE_NAME_LENGTH];

	// Spit out the path components backwards, one at a time.
	do {
		entry.GetName(name);
		printf("> %s\n", name);
	} while ((err=entry.GetParent(&entry)) == B_OK);

	// Complain for reasons other than reaching the top.
	if (err != B_ENTRY_NOT_FOUND)
		printf(">> Error: %s\n", strerror(err));
	\endcode

	will output:

	\code
	> fido
	> home
	> boot
	> .
	\endcode

	\param entry pointer to a pre-allocated BEntry object into which the result is stored
	\return
	- \c B_OK - Success
	- \c B_ENTRY_NOT_FOUND - Attempted to get the parent of the root directory \c "/"
	- "error code" - Failure
*/
status_t
BEntry::GetParent(BEntry *entry) const
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
	entry->fCStatus = entry->set_name(leafName);
	if (entry->fCStatus != B_OK)
		entry->Unset();
	return entry->fCStatus;
}


/*! \brief Gets the parent of the BEntry as a BDirectory.

	If the function fails, the argument is Unset().

	\param dir pointer to a pre-allocated BDirectory object into which the result is stored
	\return
	- \c B_OK - Success
	- \c B_ENTRY_NOT_FOUND - Attempted to get the parent of the root directory \c "/"
	- "error code" - Failure
*/
status_t
BEntry::GetParent(BDirectory *dir) const
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


/*! \brief Gets the name of the entry's leaf.

	\c buffer must be pre-allocated and of sufficient
	length to hold the entire string. A length of \c B_FILE_NAME_LENGTH is recommended.

	\param buffer pointer to a pre-allocated string into which the result is copied
	\return
	- \c B_OK - Success
	- "error code" - Failure
*/
status_t
BEntry::GetName(char *buffer) const
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


/*! \brief Renames the BEntry to path, replacing an existing entry if clobber is true.

	NOTE: The BEntry must refer to an existing file. If it is abstract, this method will fail.

	\param path Pointer to a string containing the new name for the entry.  May
	            be absolute or relative. If relative, the entry is renamed within its
	            current directory.
	\param clobber If \c false and a file with the name given by \c path already exists,
	               the method will fail. If \c true and such a file exists, it will
	               be overwritten.
	\return
	- \c B_OK - Success
	- \c B_ENTRY_EXISTS - The new location is already taken and \c clobber was \c false
	- \c B_ENTRY_NOT_FOUND - Attempted to rename an abstract entry
	- "error code" - Failure

*/
status_t
BEntry::Rename(const char *path, bool clobber)
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
		error = target.fCStatus = target.set(dirFD, path, false);
	}
	if (error != B_OK)
		return error;
	return _Rename(target, clobber);
}


/*! \brief Moves the BEntry to directory or directory+path combination, replacing an existing entry if clobber is true.

	NOTE: The BEntry must refer to an existing file. If it is abstract, this method will fail.

	\param dir Pointer to a pre-allocated BDirectory into which the entry should be moved.
	\param path Optional new leaf name for the entry. May be a simple leaf or a relative path;
	            either way, \c path is reckoned off of \c dir. If \c NULL, the entry retains
	            its previous leaf name.
	\param clobber If \c false and an entry already exists at the specified destination,
	               the method will fail. If \c true and such an entry exists, it will
	               be overwritten.
	\return
	- \c B_OK - Success
	- \c B_ENTRY_EXISTS - The new location is already taken and \c clobber was \c false
	- \c B_ENTRY_NOT_FOUND - Attempted to move an abstract entry
	- "error code" - Failure
*/
status_t
BEntry::MoveTo(BDirectory *dir, const char *path, bool clobber)
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


/*! \brief Removes the entry from the file system.

	NOTE: If any file descriptors are open on the file when Remove() is called,
	the chunk of data they refer to will continue to exist until all such file
	descriptors are closed. The BEntry object, however, becomes abstract and
	no longer refers to any actual data in the filesystem.

	\return
	- B_OK - Success
	- "error code" - Failure
*/
status_t
BEntry::Remove()
{
	if (fCStatus != B_OK)
		return B_NO_INIT;
	return _kern_unlink(fDirFd, fName);
}


/*! \brief	Returns true if the BEntry and \c item refer to the same entry or
			if they are both uninitialized.

	\return
	- true - Both BEntry objects refer to the same entry or they are both uninitialzed
	- false - The BEntry objects refer to different entries
 */
bool
BEntry::operator==(const BEntry &item) const
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


/*! \brief	Returns false if the BEntry and \c item refer to the same entry or
			if they are both uninitialized.

	\return
	- true - The BEntry objects refer to different entries
	- false - Both BEntry objects refer to the same entry or they are both uninitialzed
 */
bool
BEntry::operator!=(const BEntry &item) const
{
	return !(*this == item);
}


/*! \brief Reinitializes the BEntry to be a copy of the argument

	\return
	- A reference to the copy
*/
BEntry&
BEntry::operator=(const BEntry &item)
{
	if (this == &item)
		return *this;

	Unset();
	if (item.fCStatus == B_OK) {
		fDirFd = _kern_dup(item.fDirFd);
		if (fDirFd >= 0)
			fCStatus = set_name(item.fName);
		else
			fCStatus = fDirFd;

		if (fCStatus != B_OK)
			Unset();
	}

	return *this;
}


/*! Reserved for future use. */
void BEntry::_PennyEntry1(){}
void BEntry::_PennyEntry2(){}
void BEntry::_PennyEntry3(){}
void BEntry::_PennyEntry4(){}
void BEntry::_PennyEntry5(){}
void BEntry::_PennyEntry6(){}


/*! \brief Updates the BEntry with the data from the stat structure according to the mask.
*/
status_t
BEntry::set_stat(struct stat &st, uint32 what)
{
	if (fCStatus != B_OK)
		return B_FILE_ERROR;

	return _kern_write_stat(fDirFd, fName, false, &st, sizeof(struct stat),
		what);
}


/*! Sets the Entry to point to the entry specified by the path \a path relative
	to the given directory. If \a traverse is \c true and the given entry is a
	symlink, the object is recursively set to point to the entry pointed to by
	the symlink.

	If \a path is an absolute path, \a dirFD is ignored.
	If \a dirFD is -1, path is considered relative to the current directory
	(unless it is an absolute path, that is).

	The ownership of the file descriptor \a dirFD is transferred to the
	function, regardless of whether it succeeds or fails. The caller must not
	close the FD afterwards.

	\param dirFD File descriptor of a directory relative to which path is to
		   be considered. May be -1, when the current directory shall be
		   considered.
	\param path Pointer to a path relative to the given directory.
	\param traverse If \c true and the given entry is a symlink, the object is
		   recursively set to point to the entry linked to by the symlink.
	\return
	- B_OK - Success
	- "error code" - Failure
*/
status_t
BEntry::set(int dirFD, const char *path, bool traverse)
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
					char *cwd = getcwd(tmpPath, B_PATH_NAME_LENGTH);
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
			size_t bufferSize = B_PATH_NAME_LENGTH;
			error = _kern_read_link(dirFD, leafName, tmpPath, &bufferSize);
			if (error < 0)
				return error;
			path = tmpPath;
			// next round...
		}
	}

	// set close on exec flag on dir FD
	fcntl(dirFD, F_SETFD, FD_CLOEXEC);

	// set the result
	status_t error = set_name(leafName);
	if (error != B_OK)
		return error;
	fdCloser.Detach();
	fDirFd = dirFD;
	return B_OK;
}


/*! \brief Handles string allocation, deallocation, and copying for the entry's leaf name.

	\return
	- B_OK - Success
	- "error code" - Failure
*/
status_t
BEntry::set_name(const char *name)
{
	if (name == NULL)
		return B_BAD_VALUE;

	free(fName);

	fName = strdup(name);
	if (!fName)
		return B_NO_MEMORY;

	return B_OK;
}


/*!	\brief Renames the entry referred to by this object to the location
		   specified by \a target.

	If an entry exists at the target location, the method fails, unless
	\a clobber is \c true, in which case that entry is overwritten (doesn't
	work for non-empty directories, though).

	If the operation was successful, this entry is made a clone of the
	supplied one and the supplied one is uninitialized.

	\param target The entry specifying the target location.
	\param clobber If \c true, the an entry existing at the target location
		   will be overwritten.
	\return \c B_OK, if everything went fine, another error code otherwise.
*/
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


/*! Debugging function, dumps the given entry to stdout. This function is not part of
	the R5 implementation, and thus calls to it will mean you can't link with the
	R5 Storage Kit.

	\param name	Pointer to a string to be printed along with the dump for identification
				purposes.
*/
void
BEntry::Dump(const char *name)
{
	if (name != NULL) {
		printf("------------------------------------------------------------\n");
		printf("%s\n", name);
		printf("------------------------------------------------------------\n");
	}

	printf("fCStatus == %" B_PRId32 "\n", fCStatus);

	struct stat st;
	if (fDirFd != -1
		&& _kern_read_stat(fDirFd, NULL, false, &st,
				sizeof(struct stat)) == B_OK) {
		printf("dir.device == %d\n", (int)st.st_dev);
		printf("dir.inode  == %lld\n", (long long)st.st_ino);
	} else {
		printf("dir == NullFd\n");
	}

	printf("leaf == '%s'\n", fName);
	printf("\n");

}


//	#pragma mark -


/*!	\brief Returns an entry_ref for a given path.
	\param path The path name referring to the entry
	\param ref The entry_ref structure to be filled in
	\return
	- \c B_OK - Everything went fine.
	- \c B_BAD_VALUE - \c NULL \a path or \a ref.
	- \c B_ENTRY_NOT_FOUND - A (non-leaf) path component does not exist.
	- \c B_NO_MEMORY - Insufficient memory for successful completion.
*/
status_t
get_ref_for_path(const char *path, entry_ref *ref)
{
	status_t error = (path && ref ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		BEntry entry(path);
		error = entry.InitCheck();
		if (error == B_OK)
			error = entry.GetRef(ref);
	}
	return error;
}


/*!	\brief Returns whether an entry is less than another.
	The components are compared in order \c device, \c directory, \c name.
	A \c NULL \c name is less than any non-null name.

	\return
	- true - a < b
	- false - a >= b
*/
bool
operator<(const entry_ref &a, const entry_ref &b)
{
	return a.device < b.device
		|| (a.device == b.device
			&& (a.directory < b.directory
				|| (a.directory == b.directory
					&& ((a.name == NULL && b.name != NULL)
						|| (a.name != NULL && b.name != NULL
							&& strcmp(a.name, b.name) < 0)))));
}
