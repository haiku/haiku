//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file Entry.cpp
	BEntry and entry_ref implementations.
*/

#include <Entry.h>

#include <new>
#include <string.h>

#include <Directory.h>
#include <Path.h>
#include <SymLink.h>
#include "kernel_interface.h"
#include "storage_support.h"

#ifdef USE_OPENBEOS_NAMESPACE
using namespace OpenBeOS;
#endif

// SYMLINK_MAX is needed by B_SYMLINK_MAX
// I don't know, why it isn't defined.
#ifndef SYMLINK_MAX
#define SYMLINK_MAX (16)
#endif

//----------------------------------------------------------------------------
// struct entry_ref
//----------------------------------------------------------------------------

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
		 : device(-1),
		   directory(-1),
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
		 : device(dev), directory(dir), name(NULL)
{
	set_name(name);
}

/*! \brief Creates a copy of the given entry_ref.

	\param ref a reference to an entry_ref to copy
*/
entry_ref::entry_ref(const entry_ref &ref)
		 : device(ref.device),
		   directory(ref.directory),
		   name(NULL)
{
	set_name(ref.name);	
}

//! Destroys the object and frees the storage allocated for the leaf name, if necessary. 
entry_ref::~entry_ref()
{
	if (name != NULL)
		delete [] name;
}

/*! \brief Set the entry_ref's leaf name, freeing the storage allocated for any previous
	name and then making a copy of the new name.
	
	\param name pointer to a null-terminated string containing the new name for
	the entry. May be \c NULL.
*/
status_t entry_ref::set_name(const char *name)
{
	if (this->name != NULL) {
		delete [] this->name;
	}
	
	if (name == NULL) {
		this->name = NULL;
	} else {
		this->name = new(nothrow) char[strlen(name)+1];
		if (this->name == NULL)
			return B_NO_MEMORY;
		strcpy(this->name, name);
	}
	
	return B_OK;			
}

/*! \brief Compares the entry_ref with another entry_ref, returning true if they are equal.
	\return
	- \c true - The entry_refs are equal
	- \c false - The entry_refs are not equal
*/
bool
entry_ref::operator==(const entry_ref &ref) const
{
	return (device == ref.device
			&& directory == ref.directory
			&& (name == ref.name
				|| name != NULL && ref.name != NULL
					&& strcmp(name, ref.name) == 0));
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


//----------------------------------------------------------------------------
// BEntry
//----------------------------------------------------------------------------

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

//! Creates an uninitialized BEntry object.
/*!	Should be followed by a	call to one of the SetTo functions,
	or an assignment:
	- SetTo(const BDirectory*, const char*, bool)
	- SetTo(const entry_ref*, bool)
	- SetTo(const char*, bool)
	- operator=(const BEntry&)
*/
BEntry::BEntry()
	  : fDirFd(StorageKit::NullFd),
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
	  : fDirFd(StorageKit::NullFd),
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
	  : fDirFd(StorageKit::NullFd),
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
	  : fDirFd(StorageKit::NullFd),
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
	  : fDirFd(StorageKit::NullFd),
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

//! Returns true if the Entry exists in the filesytem, false otherwise.
/*! \return
		- \c true - The entry exists
		- \c false - The entry does not exist
*/
bool
BEntry::Exists() const
{
	if (fCStatus != B_OK)
		return false;

	// Attempt to find the entry in our current directory
	StorageKit::LongDirEntry entry;
	return StorageKit::find_dir(fDirFd, fName, &entry, sizeof(entry)) == B_OK;
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
		
 	entry_ref ref;
	status_t status = GetRef(&ref);
	if (status != B_OK)
		return status;
		
	return StorageKit::get_stat(ref, result);
}

/*! \brief Reinitializes the BEntry to the path or directory path combination,
	resolving symlinks if traverse is true
	
	\return
	- \c B_OK - Success
	- "error code" - Failure
		
	\todo Reimplement! Concatenating dir and leaf to an absolute path prevents
		  the user from accessing entries with longer absolute path.
		  R5 handles this without problems.
*/
status_t
BEntry::SetTo(const BDirectory *dir, const char *path, bool traverse)
{
	Unset();
	if (dir == NULL)
		return (fCStatus = B_BAD_VALUE);

	fCStatus = B_OK;
	if (StorageKit::is_absolute_path(path))	{
		SetTo(path, traverse);
	} else {
		if (dir->InitCheck() != B_OK)
			fCStatus = B_BAD_VALUE;
		// get the dir's path
		char rootPath[B_PATH_NAME_LENGTH + 1];
		if (fCStatus == B_OK) {
			fCStatus = StorageKit::dir_to_path(dir->get_fd(), rootPath,
											   B_PATH_NAME_LENGTH + 1);
		}
		// Concatenate our two path strings together
		if (fCStatus == B_OK && path) {
			// The concatenated strings must fit into our buffer.
			if (strlen(rootPath) + strlen(path) + 2 > B_PATH_NAME_LENGTH + 1)
				fCStatus = B_NAME_TOO_LONG;
			else {
				strcat(rootPath, "/");
				strcat(rootPath, path);
			}
		}
		// set the resulting path
		if (fCStatus == B_OK)
			SetTo(rootPath, traverse);
	}
	return fCStatus;
}
				  
/*! \brief Reinitializes the BEntry to the entry_ref, resolving symlinks if
	traverse is true

	\return
	- \c B_OK - Success
	- "error code" - Failure
		
	\todo Implemented using entry_ref_to_path(). Reimplement!
*/
status_t
BEntry::SetTo(const entry_ref *ref, bool traverse)
{
	Unset();
	if (ref == NULL) {
		return (fCStatus = B_BAD_VALUE);
	}

	char path[B_PATH_NAME_LENGTH + 1];

	fCStatus = StorageKit::entry_ref_to_path(ref, path,
											 B_PATH_NAME_LENGTH + 1);
	return (fCStatus == B_OK) ? SetTo(path, traverse) : fCStatus ;
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
	fCStatus = (path ? B_OK : B_BAD_VALUE);
	if (fCStatus == B_OK)
		fCStatus = StorageKit::check_path_name(path);
	if (fCStatus == B_OK) {
		// Get the path and leaf portions of the given path
		char *pathStr, *leafStr;
		pathStr = leafStr = NULL;
		fCStatus = StorageKit::split_path(path, pathStr, leafStr);
		if (fCStatus == B_OK) {
			// Open the directory
			StorageKit::FileDescriptor dirFd;
			fCStatus = StorageKit::open_dir(pathStr, dirFd);
			if (fCStatus == B_OK) {
				fCStatus = set(dirFd, leafStr, traverse);
				if (fCStatus != B_OK)
					StorageKit::close_dir(dirFd);		
			}
		}
		delete [] pathStr;
		delete [] leafStr;
	}
	return fCStatus;
}

/*! \brief Reinitializes the BEntry to an uninitialized BEntry object */
void
BEntry::Unset()
{
	// Close the directory
	if (fDirFd != StorageKit::NullFd) {
		StorageKit::close_dir(fDirFd);
	}
	
	// Free our leaf name
	if (fName != NULL) {
		delete [] fName;
	}

	fDirFd = StorageKit::NullFd;
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
	status_t error = StorageKit::get_stat(fDirFd, &st);
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
		
	entry_ref ref;
	status_t status;
	
	status = GetRef(&ref);
	if (status != B_OK)
		return status;
		
	return path->SetTo(&ref);
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
status_t BEntry::GetParent(BEntry *entry) const
{
	if (fCStatus != B_OK)
		return B_NO_INIT;
	
	if (entry == NULL)
		return B_BAD_VALUE;
		
	// Convert ourselves to a entry_ref and change the
	// leaf name to "."

	entry_ref ref;
	status_t status;

	status = GetRef(&ref);
	if (status == B_OK) {
	
		// Verify we aren't an entry representing "/"
		status = StorageKit::entry_ref_is_root_dir(ref) ? B_ENTRY_NOT_FOUND
														: B_OK ;
		if (status == B_OK) {

			status = ref.set_name(".");
			if (status == B_OK) {
			
				entry->SetTo(&ref);
				return entry->InitCheck();
				
			}
		}
	}
	
	// If we get this far, an error occured, so we Unset() the
	// argument as dictated by the BeBook
	entry->Unset();
	return status;
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
	if (fCStatus != B_OK)
		return B_NO_INIT;
		
	if (dir == NULL)
		return B_BAD_VALUE;
	
	entry_ref ref;
	status_t status;
	
	status = GetRef(&ref);
	if (status == B_OK) {
	
		// Verify we aren't an entry representing "/"
		status = StorageKit::entry_ref_is_root_dir(ref) ? B_ENTRY_NOT_FOUND : B_OK ;
		if (status == B_OK) {

			// Now point the entry_ref to the parent directory (instead of ourselves)
			status = ref.set_name(".");
			if (status == B_OK) {
				dir->SetTo(&ref);
				return dir->InitCheck();
			}
		}
	}
	
	// If we get this far, an error occured, so we Unset() the
	// argument as dictated by the BeBook
	dir->Unset();
	return status;
}

/*! \brief Gets the name of the entry's leaf.

	\c buffer must be pre-allocated and of sufficient
	length to hold the entire string. A length of \c B_FILE_NAME_LENGTH+1 is recommended.

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
	if (path == NULL)
		return B_BAD_VALUE;
	if (fCStatus != B_OK)
		return B_NO_INIT;
		
	status_t status = B_OK;
	// Convert the given path to an absolute path, if it isn't already.
	char fullPath[B_PATH_NAME_LENGTH + 1];
	if (!StorageKit::is_absolute_path(path)) {
		// Convert our directory to an absolute pathname
		status = StorageKit::dir_to_path(fDirFd, fullPath,
										 B_PATH_NAME_LENGTH + 1);
		if (status == B_OK) {
			// Concatenate our pathname to it
			strcat(fullPath, "/");
			strcat(fullPath, path);
			path = fullPath;
		}
	}
	// Check, whether the file does already exist, if clobber is false.
	if (status == B_OK && !clobber) {
		// We're not supposed to kill an already-existing file,
		// so we'll try to figure out if it exists by stat()ing it.
		StorageKit::Stat s;
		status = StorageKit::get_stat(path, &s);
		if (status == B_OK)
			status = B_FILE_EXISTS;
		else if (status == B_ENTRY_NOT_FOUND)
			status = B_OK;
	}
	// Turn ourselves into a pathname, rename ourselves
	if (status == B_OK) {
		BPath oldPath;
		status = GetPath(&oldPath);
		if (status == B_OK) {
			status = StorageKit::rename(oldPath.Path(), path);
			if (status == B_OK)
				status = SetTo(path, false);
		}
	}
	return status;
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
BEntry::MoveTo(BDirectory *dir, const char *path = NULL, bool clobber)
{
	if (fCStatus != B_OK)
		return B_NO_INIT;
	else if (dir == NULL)
		return B_BAD_VALUE;
	else if (dir->InitCheck() != B_OK)
		return B_BAD_VALUE;

	// NULL path simply means move without renaming
	if (path == NULL)
		path = fName;

	status_t status = B_OK;
	// Determine the absolute path of the target entry.
	if (!StorageKit::is_absolute_path(path)) {
		// Convert our directory to an absolute pathname
		char fullPath[B_PATH_NAME_LENGTH + 1];
		status = StorageKit::dir_to_path(dir->get_fd(), fullPath,
										 B_PATH_NAME_LENGTH + 1);
		// Concatenate our pathname to it
		if (status == B_OK) {
			strcat(fullPath, "/");
			strcat(fullPath, path);
			path = fullPath;
		}
	}
	// Now let rename do the dirty work
	if (status == B_OK)
		status = Rename(path, clobber);
	return status;
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
		
	BPath path;
	status_t status;
	
	status = GetPath(&path);
	if (status != B_OK)
		return status;
		
	return StorageKit::remove(path.Path());
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
		fCStatus = StorageKit::dup_dir(item.fDirFd, fDirFd);
		if (fCStatus == B_OK) {
			fCStatus = set_name(item.fName);
		}
	}
	
	return *this;
}

/*! Reserved for future use. */
void BEntry::_PennyEntry1(){}
/*! Reserved for future use. */
void BEntry::_PennyEntry2(){}
/*! Reserved for future use. */
void BEntry::_PennyEntry3(){}
/*! Reserved for future use. */
void BEntry::_PennyEntry4(){}
/*! Reserved for future use. */
void BEntry::_PennyEntry5(){}
/*! Reserved for future use. */
void BEntry::_PennyEntry6(){}

/*! \brief Updates the BEntry with the data from the stat structure according to the mask.
*/
status_t
BEntry::set_stat(struct stat &st, uint32 what)
{
	if (fCStatus != B_OK)
		return B_FILE_ERROR;
	
	BPath path;
	status_t status;
	
	status = GetPath(&path);
	if (status != B_OK)
		return status;
	
	return StorageKit::set_stat(path.Path(), st, what);
}

/*! Sets the Entry to point to the entry named by \c leaf in the given directory. If \c traverse
	is \c true and the given entry is a symlink, the object is recursively set to point to the
	entry pointed to by the symlink.
	
	\c leaf <b>must</b> be a leaf-name only (i.e. it must contain no '/' characters),
	otherwise this function will return \c B_BAD_VALUE. If \c B_OK is returned, 
	the caller is no longer responsible for closing the directory file descriptor
	with a call to StorageKit::close_dir.
	
	\param dirFd File descriptor of the directory in which the entry resides
	\param leaf Pointer to a string containing the entry's leaf name
	\param traverse If \c true and the given entry is a symlink, the object is recursively
	                set to point to the entry linked to by the symlink.
	\return
	- B_OK - Success
	- "error code" - Failure
	
*/
status_t
BEntry::set(StorageKit::FileDescriptor dirFd, const char *leaf, bool traverse)
{
	// Verify that path is valid
	status_t error = StorageKit::check_entry_name(leaf);
	if (error != B_OK)
		return error;
	// Check whether the entry is abstract or concrete.
	// We try traversing concrete entries only.
	StorageKit::LongDirEntry dirEntry;
	bool isConcrete = (StorageKit::find_dir(dirFd, leaf, &dirEntry,
											sizeof(dirEntry)) == B_OK);
	if (traverse && isConcrete) {
		// Though the link traversing strategy is iterative, we introduce
		// some recursion, since we are using BSymLink, which may be
		// (currently is) implemented using BEntry. Nevertheless this is
		// harmless, because BSymLink does, of course, not want to traverse
		// the link.

		// convert the dir FD into a BPath
		entry_ref ref;
		error = StorageKit::dir_to_self_entry_ref(dirFd, &ref);
		char dirPathname[B_PATH_NAME_LENGTH + 1];
		if (error == B_OK) {
			error = StorageKit::entry_ref_to_path(&ref, dirPathname,
												  sizeof(dirPathname));
		}
		BPath dirPath(dirPathname);
		if (error == B_OK)
			error = dirPath.InitCheck();
		BPath linkPath;
		if (error == B_OK)
			linkPath.SetTo(dirPath.Path(), leaf);
		if (error == B_OK) {
			// Here comes the link traversing loop: A BSymLink is created
			// from the dir and the leaf name, the link target is determined,
			// the target's dir and leaf name are got and so on.
			bool isLink = true;
			int32 linkLimit = B_MAX_SYMLINKS;
			while (error == B_OK && isLink && linkLimit > 0) {
				linkLimit--;
				// that's OK with any node, even if it's not a symlink
				BSymLink link(linkPath.Path());
				error = link.InitCheck();
				if (error == B_OK) {
					isLink = link.IsSymLink();
					if (isLink) {
						// get the path to the link target
						ssize_t linkSize = link.MakeLinkedPath(dirPath.Path(),
															   &linkPath);
						if (linkSize < 0)
							error = linkSize;
						// get the link target's dir path
						if (error == B_OK)
							error = linkPath.GetParent(&dirPath);
					}
				}
			}
			// set the new values
			if (error == B_OK) {
				if (isLink)
					error = B_LINK_LIMIT;
				else {
					StorageKit::FileDescriptor newDirFd = StorageKit::NullFd;
					error = StorageKit::open_dir(dirPath.Path(), newDirFd);
					if (error == B_OK) {
						// If we are successful, we are responsible for the
						// supplied FD. Thus we close it.
						StorageKit::close_dir(dirFd);
						dirFd = StorageKit::NullFd;
						fDirFd = newDirFd;
						// handle "/", which has a "" Leaf()
						if (linkPath == "/")
							set_name(".");
						else
							set_name(linkPath.Leaf());
					}
				}
			}
		}	// getting the dir path for the FD
	} else {
		// don't traverse: either the flag is not set or the entry is abstract
		fDirFd = dirFd;
		set_name(leaf);
	}
	return error;
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
	
	if (fName != NULL) {
		delete [] fName;
	}
	
	fName = new(nothrow) char[strlen(name)+1];
	if (fName == NULL)
		return B_NO_MEMORY;
		
	strcpy(fName, name);
	
	return B_OK;
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
	
	printf("fCStatus == %ld\n", fCStatus);
	
	StorageKit::LongDirEntry entry;
	if (fDirFd != -1
		&& StorageKit::find_dir(fDirFd, ".", &entry, sizeof(entry)) == B_OK) {
		printf("dir.device == %ld\n", entry.d_pdev);
		printf("dir.inode  == %lld\n", entry.d_pino);
	} else {
		printf("dir == NullFd\n");
	}
	
	printf("leaf == '%s'\n", fName);
	printf("\n");

}

// get_ref_for_path
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

// <
/*!	\brief Returns whether an entry is less than another.
	The components are compared in order \c device, \c directory, \c name.
	A \c NULL \c name is less than any non-null name.
	
	\return
	- true - a < b
	- false - a >= b
*/
bool
operator<(const entry_ref & a, const entry_ref & b)
{
	return (a.device < b.device
		|| (a.device == b.device
			&& (a.directory < b.directory
			|| (a.directory == b.directory
				&& (a.name == NULL && b.name != NULL
				|| (a.name != NULL && b.name != NULL
					&& strcmp(a.name, b.name) < 0))))));
}

