//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file Directory.cpp
	BDirectory implementation.	
*/

#include <fs_info.h>
#include <string.h>

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Path.h>
#include <SymLink.h>
#include "storage_support.h"
#include "kernel_interface.h"

#ifdef USE_OPENBEOS_NAMESPACE
namespace OpenBeOS {
#endif

// constructor
//! Creates an uninitialized BDirectory object.
BDirectory::BDirectory()
		  : BNode(),
			BEntryList(),
			fDirFd(StorageKit::NullFd)
{
}

// copy constructor
//! Creates a copy of the supplied BDirectory.
/*!	\param dir the BDirectory object to be copied
*/
BDirectory::BDirectory(const BDirectory &dir)
		  : BNode(),
			BEntryList(),
			fDirFd(StorageKit::NullFd)
{
	*this = dir;
}

// constructor
/*! \brief Creates a BDirectory and initializes it to the directory referred
	to by the supplied entry_ref.
	\param ref the entry_ref referring to the directory
*/
BDirectory::BDirectory(const entry_ref *ref)
		  : BNode(),
			BEntryList(),
			fDirFd(StorageKit::NullFd)
{
	SetTo(ref);
}

// constructor
/*! \brief Creates a BDirectory and initializes it to the directory referred
	to by the supplied node_ref.
	\param nref the node_ref referring to the directory
*/
BDirectory::BDirectory(const node_ref *nref)
		  : BNode(),
			BEntryList(),
			fDirFd(StorageKit::NullFd)
{
	SetTo(nref);
}

// constructor
/*! \brief Creates a BDirectory and initializes it to the directory referred
	to by the supplied BEntry.
	\param entry the BEntry referring to the directory
*/
BDirectory::BDirectory(const BEntry *entry)
		  : BNode(),
			BEntryList(),
			fDirFd(StorageKit::NullFd)
{
	SetTo(entry);
}

// constructor
/*! \brief Creates a BDirectory and initializes it to the directory referred
	to by the supplied path name.
	\param path the directory's path name 
*/
BDirectory::BDirectory(const char *path)
		  : BNode(),
			BEntryList(),
			fDirFd(StorageKit::NullFd)
{
	SetTo(path);
}

// constructor
/*! \brief Creates a BDirectory and initializes it to the directory referred
	to by the supplied path name relative to the specified BDirectory.
	\param dir the BDirectory, relative to which the directory's path name is
		   given
	\param path the directory's path name relative to \a dir
*/
BDirectory::BDirectory(const BDirectory *dir, const char *path)
		  : BNode(),
			BEntryList(),
			fDirFd(StorageKit::NullFd)
{
	SetTo(dir, path);
}

// destructor
//! Frees all allocated resources.
/*! If the BDirectory is properly initialized, the directory's file descriptor
	is closed.
*/
BDirectory::~BDirectory()
{
	// Also called by the BNode destructor, but we rather try to avoid
	// problems with calling virtual functions in the base class destructor.
	// Depending on the compiler implementation an object may be degraded to
	// an object of the base class after the destructor of the derived class
	// has been executed.
	close_fd();
}

// SetTo
/*! \brief Re-initializes the BDirectory to the directory referred to by the
	supplied entry_ref.
	\param ref the entry_ref referring to the directory
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a ref.
	- \c B_ENTRY_NOT_FOUND: Directory not found.
	- \c B_PERMISSION_DENIED: Directory permissions didn't allow operation.
	- \c B_NO_MEMORY: Insufficient memory for operation.
	- \c B_LINK_LIMIT: Indicates a cyclic loop within the file system.
	- \c B_BUSY: A node was busy.
	- \c B_FILE_ERROR: A general file error.
	- \c B_NO_MORE_FDS: The application has run out of file descriptors.
	\todo Currently implemented using StorageKit::entry_ref_to_path().
		  Reimplement!
*/
status_t
BDirectory::SetTo(const entry_ref *ref)
{
	Unset();	
	char path[B_PATH_NAME_LENGTH + 1];
	status_t error = (ref ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		error = StorageKit::entry_ref_to_path(ref, path,
											  B_PATH_NAME_LENGTH + 1);
	}
	if (error == B_OK)
		error = SetTo(path);
	set_status(error);
	return error;
}

// SetTo
/*! \brief Re-initializes the BDirectory to the directory referred to by the
	supplied node_ref.
	\param nref the node_ref referring to the directory
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a nref.
	- \c B_ENTRY_NOT_FOUND: Directory not found.
	- \c B_PERMISSION_DENIED: Directory permissions didn't allow operation.
	- \c B_NO_MEMORY: Insufficient memory for operation.
	- \c B_LINK_LIMIT: Indicates a cyclic loop within the file system.
	- \c B_BUSY: A node was busy.
	- \c B_FILE_ERROR: A general file error.
	- \c B_NO_MORE_FDS: The application has run out of file descriptors.
*/
status_t
BDirectory::SetTo(const node_ref *nref)
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

// SetTo
/*! \brief Re-initializes the BDirectory to the directory referred to by the
	supplied BEntry.
	\param entry the BEntry referring to the directory
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a entry.
	- \c B_ENTRY_NOT_FOUND: Directory not found.
	- \c B_PERMISSION_DENIED: Directory permissions didn't allow operation.
	- \c B_NO_MEMORY: Insufficient memory for operation.
	- \c B_LINK_LIMIT: Indicates a cyclic loop within the file system.
	- \c B_BUSY: A node was busy.
	- \c B_FILE_ERROR: A general file error.
	- \c B_NO_MORE_FDS: The application has run out of file descriptors.
	\todo Implemented using SetTo(entry_ref*). Check, if necessary to
		  reimplement!
*/
status_t
BDirectory::SetTo(const BEntry *entry)
{
	Unset();	
	entry_ref ref;
	status_t error = (entry ? B_OK : B_BAD_VALUE);
	if (error == B_OK && entry->InitCheck() != B_OK)
		error = B_BAD_VALUE;
	if (error == B_OK)
		error = entry->GetRef(&ref);
	if (error == B_OK)
		error = SetTo(&ref);
	set_status(error);
	return error;
}

// SetTo
/*! \brief Re-initializes the BDirectory to the directory referred to by the
	supplied path name.
	\param path the directory's path name 
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a path.
	- \c B_ENTRY_NOT_FOUND: Directory not found.
	- \c B_PERMISSION_DENIED: Directory permissions didn't allow operation.
	- \c B_NO_MEMORY: Insufficient memory for operation.
	- \c B_NAME_TOO_LONG: The supplied path name (\a path) is too long.
	- \c B_LINK_LIMIT: Indicates a cyclic loop within the file system.
	- \c B_BUSY: A node was busy.
	- \c B_FILE_ERROR: A general file error.
	- \c B_NO_MORE_FDS: The application has run out of file descriptors.
	- \c B_NOT_A_DIRECTORY: \a path includes a non-directory.
*/
status_t
BDirectory::SetTo(const char *path)
{
	Unset();	
	status_t result = (path ? B_OK : B_BAD_VALUE);
	StorageKit::FileDescriptor newDirFd = StorageKit::NullFd;
	if (result == B_OK)
		result = StorageKit::open_dir(path, newDirFd);
	if (result == B_OK) {
		// We have to take care that BNode doesn't stick to a symbolic link.
		// open_dir() does always traverse those. Therefore we open the FD for
		// BNode (without the O_NOTRAVERSE flag).
		StorageKit::FileDescriptor fd = StorageKit::NullFd;
		result = StorageKit::open(path, O_RDWR, fd);
		if (result == B_OK) {
			result = set_fd(fd);
			if (result != B_OK)
				StorageKit::close(fd);
		}
		if (result == B_OK)
			fDirFd = newDirFd;
		else
			StorageKit::close_dir(newDirFd);
	}
	// finally set the BNode status
	set_status(result);
	return result;
}

// SetTo
/*! \brief Re-initializes the BDirectory to the directory referred to by the
	supplied path name relative to the specified BDirectory.
	\param dir the BDirectory, relative to which the directory's path name is
		   given
	\param path the directory's path name relative to \a dir
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a dir or \a path, or \a path is absolute.
	- \c B_ENTRY_NOT_FOUND: Directory not found.
	- \c B_PERMISSION_DENIED: Directory permissions didn't allow operation.
	- \c B_NO_MEMORY: Insufficient memory for operation.
	- \c B_NAME_TOO_LONG: The supplied path name (\a path) is too long.
	- \c B_LINK_LIMIT: Indicates a cyclic loop within the file system.
	- \c B_BUSY: A node was busy.
	- \c B_FILE_ERROR: A general file error.
	- \c B_NO_MORE_FDS: The application has run out of file descriptors.
	- \c B_NOT_A_DIRECTORY: \a path includes a non-directory.
	\todo Implemented using SetTo(BEntry*). Check, if necessary to reimplement!
*/
status_t
BDirectory::SetTo(const BDirectory *dir, const char *path)
{
	Unset();
	status_t error = (dir && path ? B_OK : B_BAD_VALUE);
	if (error == B_OK && StorageKit::is_absolute_path(path))
		error = B_BAD_VALUE;
	BEntry entry;
	if (error == B_OK)
		error = entry.SetTo(dir, path);
	if (error == B_OK)
		error = SetTo(&entry);
	set_status(error);
	return error;
}

// GetEntry
//! Returns a BEntry referring to the directory represented by this object.
/*!	If the initialization of \a entry fails, it is Unset().
	\param entry a pointer to the entry that shall be set to refer to the
		   directory
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a entry.
	- \c B_ENTRY_NOT_FOUND: Directory not found.
	- \c B_PERMISSION_DENIED: Directory permissions didn't allow operation.
	- \c B_NO_MEMORY: Insufficient memory for operation.
	- \c B_LINK_LIMIT: Indicates a cyclic loop within the file system.
	- \c B_BUSY: A node was busy.
	- \c B_FILE_ERROR: A general file error.
	- \c B_NO_MORE_FDS: The application has run out of file descriptors.
	\todo Implemented using StorageKit::dir_to_self_entry_ref(). Check, if
		  there is a better alternative.
*/
status_t
BDirectory::GetEntry(BEntry *entry) const
{
	status_t error = (entry ? B_OK : B_BAD_VALUE);
	if (entry)
		entry->Unset();
	if (error == B_OK && InitCheck() != B_OK)
		error = B_NO_INIT;
	entry_ref ref;
	if (error == B_OK)
		error = StorageKit::dir_to_self_entry_ref(fDirFd, &ref);
	if (error == B_OK)
		error = entry->SetTo(&ref);
	return error;
}

// IsRootDirectory
/*!	\brief Returns whether the directory represented by this BDirectory is a
	root directory of a volume.
	\return
	- \c true, if the BDirectory is properly initialized and represents a
	  root directory of some volume,
	- \c false, otherwise.
*/
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

// FindEntry
/*! \brief Finds an entry referred to by a path relative to the directory
	represented by this BDirectory.
	\a path may be absolute. If the BDirectory is not properly initialized,
	the entry is search relative to the current directory.
	If the entry couldn't be found, \a entry is Unset().
	\param path the entry's path name. May be relative to this directory or
		   absolute.
	\param entry a pointer to a BEntry to be initialized with the found entry
	\param traverse specifies whether to follow it, if the found entry
		   is a symbolic link.
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a path or \a entry.
	- \c B_ENTRY_NOT_FOUND: Entry not found.
	- \c B_PERMISSION_DENIED: Directory permissions didn't allow operation.
	- \c B_NO_MEMORY: Insufficient memory for operation.
	- \c B_NAME_TOO_LONG: The supplied path name (\a path) is too long.
	- \c B_LINK_LIMIT: Indicates a cyclic loop within the file system.
	- \c B_BUSY: A node was busy.
	- \c B_FILE_ERROR: A general file error.
	- \c B_NO_MORE_FDS: The application has run out of file descriptors.
	- \c B_NOT_A_DIRECTORY: \a path includes a non-directory.
	\note The functionality of this method differs from the one of
		  BEntry::SetTo(BDirectory *, const char *, bool) in that the
		  latter doesn't require the entry to be existent, whereas this
		  function does.
*/
status_t
BDirectory::FindEntry(const char *path, BEntry *entry, bool traverse) const
{
	status_t error = (path && entry ? B_OK : B_BAD_VALUE);
	if (entry)
		entry->Unset();
	if (error == B_OK) {
		// init a potentially abstract entry
		if (InitCheck() == B_OK)
			error = entry->SetTo(this, path, traverse);
		else
			error = entry->SetTo(path, traverse);
		// fail, if entry is abstract
		if (error == B_OK && !entry->Exists()) {
			error = B_ENTRY_NOT_FOUND;
			entry->Unset();
		}
	}
	return error;
}

// Contains
/*!	\brief Returns whether this directory or any of its subdirectories
	at any level contain the entry referred to by the supplied path name.
	Only entries that match the node flavor specified by \a nodeFlags are
	considered.
	If the BDirectory is not properly initialized, the method returns \c true,
	if the entry exists and its kind does match. A non-absolute path is
	considered relative to the current directory.
	\param path the entry's path name. May be relative to this directory or
		   absolute.
	\param nodeFlags Any of the following:
		   - \c B_FILE_NODE: The entry must be a file.
		   - \c B_DIRECTORY_NODE: The entry must be a directory.
		   - \c B_SYMLINK_NODE: The entry must be a symbolic link.
		   - \c B_ANY_NODE: The entry may be of any kind.
	\return
	- \c true, if the entry exists, its kind does match \nodeFlags and the
	  BDirectory is either not properly initialized or it does contain the
	  entry at any level,
	- \c false, otherwise
*/
bool
BDirectory::Contains(const char *path, int32 nodeFlags) const
{
	bool result = true;
	if (path) {
		BEntry entry;
		if (InitCheck() == B_OK && !StorageKit::is_absolute_path(path))
			entry.SetTo(this, path);
		else
			entry.SetTo(path);
		result = Contains(&entry, nodeFlags);
	} else {
		// R5 behavior
		result = (InitCheck() == B_OK);
	}
	return result;
}

// Contains
/*!	\brief Returns whether this directory or any of its subdirectories
	at any level contain the entry referred to by the supplied BEntry.
	Only entries that match the node flavor specified by \a nodeFlags are
	considered.
	\param entry a BEntry referring to the entry
	\param nodeFlags Any of the following:
		   - \c B_FILE_NODE: The entry must be a file.
		   - \c B_DIRECTORY_NODE: The entry must be a directory.
		   - \c B_SYMLINK_NODE: The entry must be a symbolic link.
		   - \c B_ANY_NODE: The entry may be of any kind.
	\return
	- \c true, if the BDirectory is properly initialized and the entry of the
	  matching kind could be found,
	- \c false, otherwise
*/
bool
BDirectory::Contains(const BEntry *entry, int32 nodeFlags) const
{
	bool result = (entry);
	// check, if the entry exists at all
	if (result)
		result = entry->Exists();
	// test the node kind
	if (result) {
		switch (nodeFlags) {
			case B_FILE_NODE:
				result = entry->IsFile();
				break;
			case B_DIRECTORY_NODE:
				result = entry->IsDirectory();
				break;
			case B_SYMLINK_NODE:
				result = entry->IsSymLink();
				break;
			case B_ANY_NODE:
				break;
			default:
				result = false;
				break;
		}
	}
	// If the directory is initialized, get the canonical paths of the dir and
	// the entry and check, if the latter is a prefix of the first one.
	if (result && InitCheck() == B_OK) {
		char dirPath[B_PATH_NAME_LENGTH + 1];
		char entryPath[B_PATH_NAME_LENGTH + 1];
		result = (StorageKit::dir_to_path(fDirFd, dirPath,
										  B_PATH_NAME_LENGTH + 1) == B_OK);
		entry_ref ref;
		if (result)
			result = (entry->GetRef(&ref) == B_OK);
		if (result) {
			result = (StorageKit::entry_ref_to_path(&ref, entryPath,
													B_PATH_NAME_LENGTH + 1)
					  == B_OK);
		}
		if (result)
			result = !strncmp(dirPath, entryPath, strlen(dirPath));
	}
	return result;
}

// GetStatFor
/*!	\brief Returns the stat structure of the entry referred to by the supplied
	path name.
	\param path the entry's path name. May be relative to this directory or
		   absolute, or \c NULL to get the directories stat info.
	\param st a pointer to the stat structure to be filled in by this function
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a st.
	- \c B_ENTRY_NOT_FOUND: Entry not found.
	- \c B_PERMISSION_DENIED: Directory permissions didn't allow operation.
	- \c B_NO_MEMORY: Insufficient memory for operation.
	- \c B_NAME_TOO_LONG: The supplied path name (\a path) is too long.
	- \c B_LINK_LIMIT: Indicates a cyclic loop within the file system.
	- \c B_BUSY: A node was busy.
	- \c B_FILE_ERROR: A general file error.
	- \c B_NO_MORE_FDS: The application has run out of file descriptors.
	- \c B_NOT_A_DIRECTORY: \a path includes a non-directory.
*/
status_t
BDirectory::GetStatFor(const char *path, struct stat *st) const
{
	status_t error = (st ? B_OK : B_BAD_VALUE);
	if (error == B_OK && InitCheck() != B_OK)
		error = B_NO_INIT;
	if (error == B_OK) {
		if (path) {
			if (strlen(path) == 0)
				error = B_ENTRY_NOT_FOUND;
			else {
				BEntry entry(this, path);
				error = entry.InitCheck();
				if (error == B_OK)
					error = entry.GetStat(st);
			}
		} else
			error = GetStat(st);
	}
	return error;
}

// GetNextEntry
//! Returns the BDirectory's next entry as a BEntry.
/*!	Unlike GetNextDirents() this method ignores the entries "." and "..".
	\param entry a pointer to a BEntry to be initialized to the found entry
	\param traverse specifies whether to follow it, if the found entry
		   is a symbolic link.
	\note The iterator used by this method is the same one used by
		  GetNextRef(), GetNextDirents(), Rewind() and CountEntries().
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a entry.
	- \c B_ENTRY_NOT_FOUND: No more entries found.
	- \c B_PERMISSION_DENIED: Directory permissions didn't allow operation.
	- \c B_NO_MEMORY: Insufficient memory for operation.
	- \c B_LINK_LIMIT: Indicates a cyclic loop within the file system.
	- \c B_BUSY: A node was busy.
	- \c B_FILE_ERROR: A general file error.
	- \c B_NO_MORE_FDS: The application has run out of file descriptors.
*/
status_t
BDirectory::GetNextEntry(BEntry *entry, bool traverse)
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

// GetNextRef
//! Returns the BDirectory's next entry as an entry_ref.
/*!	Unlike GetNextDirents() this method ignores the entries "." and "..".
	\param ref a pointer to an entry_ref to be filled in with the data of the
		   found entry
	\param traverse specifies whether to follow it, if the found entry
		   is a symbolic link.
	\note The iterator used be this method is the same one used by
		  GetNextEntry(), GetNextDirents(), Rewind() and CountEntries().
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a ref.
	- \c B_ENTRY_NOT_FOUND: No more entries found.
	- \c B_PERMISSION_DENIED: Directory permissions didn't allow operation.
	- \c B_NO_MEMORY: Insufficient memory for operation.
	- \c B_LINK_LIMIT: Indicates a cyclic loop within the file system.
	- \c B_BUSY: A node was busy.
	- \c B_FILE_ERROR: A general file error.
	- \c B_NO_MORE_FDS: The application has run out of file descriptors.
*/
status_t
BDirectory::GetNextRef(entry_ref *ref)
{
	status_t error = (ref ? B_OK : B_BAD_VALUE);
	if (error == B_OK && InitCheck() != B_OK)
		error = B_FILE_ERROR;
	if (error == B_OK) {
		StorageKit::LongDirEntry entry;
		bool next = true;
		while (error == B_OK && next) {
			if (StorageKit::read_dir(fDirFd, &entry, sizeof(entry), 1) != 1)
				error = B_ENTRY_NOT_FOUND;
			if (error == B_OK) {
				next = (!strcmp(entry.d_name, ".")
						|| !strcmp(entry.d_name, ".."));
			}
		}
		if (error == B_OK)
			*ref = entry_ref(entry.d_pdev, entry.d_pino, entry.d_name);
	}
	return error;
}

// GetNextDirents
//! Returns the BDirectory's next entries as dirent structures.
/*!	Unlike GetNextEntry() and GetNextRef(), this method returns also
	the entries "." and "..".
	\param buf a pointer to a buffer to be filled with dirent structures of
		   the found entries
	\param count the maximal number of entries to be returned.
	\note The iterator used by this method is the same one used by
		  GetNextEntry(), GetNextRef(), Rewind() and CountEntries().
	\return
	- The number of dirent structures stored in the buffer, 0 when there are
	  no more entries to be returned.
	- \c B_BAD_VALUE: \c NULL \a buf.
	- \c B_PERMISSION_DENIED: Directory permissions didn't allow operation.
	- \c B_NO_MEMORY: Insufficient memory for operation.
	- \c B_NAME_TOO_LONG: The entry's name is too long for the buffer.
	- \c B_LINK_LIMIT: Indicates a cyclic loop within the file system.
	- \c B_BUSY: A node was busy.
	- \c B_FILE_ERROR: A general file error.
	- \c B_NO_MORE_FDS: The application has run out of file descriptors.
*/
int32
BDirectory::GetNextDirents(dirent *buf, size_t bufSize, int32 count)
{
	int32 result = (buf ? B_OK : B_BAD_VALUE);
	if (result == B_OK && InitCheck() != B_OK)
		result = B_FILE_ERROR;
	if (result == B_OK)
		result = StorageKit::read_dir(fDirFd, buf, bufSize, count);
	return result;
}

// Rewind
//!	Rewinds the directory iterator.
/*!	\return
	- \c B_OK: Everything went fine.
	- \c B_PERMISSION_DENIED: Directory permissions didn't allow operation.
	- \c B_NO_MEMORY: Insufficient memory for operation.
	- \c B_LINK_LIMIT: Indicates a cyclic loop within the file system.
	- \c B_BUSY: A node was busy.
	- \c B_FILE_ERROR: A general file error.
	- \c B_NO_MORE_FDS: The application has run out of file descriptors.
	\see GetNextEntry(), GetNextRef(), GetNextDirents(), CountEntries()
*/
status_t
BDirectory::Rewind()
{
	status_t error = B_OK;
	if (error == B_OK && InitCheck() != B_OK)
		error = B_FILE_ERROR;
	if (error == B_OK)
		error = StorageKit::rewind_dir(fDirFd);
	return error;
}

// CountEntries
//!	Returns the number of entries in this directory.
/*!	CountEntries() uses the directory iterator also used by GetNextEntry(),
	GetNextRef() and GetNextDirents(). It does a Rewind(), iterates through
	the entries and Rewind()s again. The entries "." and ".." are not counted.
	\return
	- the number of entries in the directory (not counting "." and "..").
	- \c B_PERMISSION_DENIED: Directory permissions didn't allow operation.
	- \c B_NO_MEMORY: Insufficient memory for operation.
	- \c B_LINK_LIMIT: Indicates a cyclic loop within the file system.
	- \c B_BUSY: A node was busy.
	- \c B_FILE_ERROR: A general file error.
	- \c B_NO_MORE_FDS: The application has run out of file descriptors.
	\see GetNextEntry(), GetNextRef(), GetNextDirents(), Rewind()
*/
int32
BDirectory::CountEntries()
{
	status_t error = Rewind();
	int32 count = 0;
	if (error == B_OK) {
		StorageKit::LongDirEntry entry;
		while (error == B_OK) {
			if (StorageKit::read_dir(fDirFd, &entry, sizeof(entry), 1) != 1)
				error = B_ENTRY_NOT_FOUND;
			if (error == B_OK
				&& strcmp(entry.d_name, ".") && strcmp(entry.d_name, "..")) {
				count++;
			}
		}
		if (error == B_ENTRY_NOT_FOUND)
			error = B_OK;
	}
	Rewind();
	return (error == B_OK ? count : error);
}

// CreateDirectory
//! Creates a new directory.
/*! If an entry with the supplied name does already exist, the method fails.
	\param path the new directory's path name. May be relative to this
		   directory or absolute.
	\param dir a pointer to a BDirectory to be initialized to the newly
		   created directory. May be \c NULL.
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a path.
	- \c B_ENTRY_NOT_FOUND: \a path does not refer to a possible entry.
	- \c B_PERMISSION_DENIED: Directory permissions didn't allow operation.
	- \c B_NO_MEMORY: Insufficient memory for operation.
	- \c B_LINK_LIMIT: Indicates a cyclic loop within the file system.
	- \c B_BUSY: A node was busy.
	- \c B_FILE_ERROR: A general file error.
	- \c B_FILE_EXISTS: An entry with that name does already exist.
	- \c B_NO_MORE_FDS: The application has run out of file descriptors.
*/
status_t
BDirectory::CreateDirectory(const char *path, BDirectory *dir)
{
	status_t error = (path ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		// get the actual (absolute) path using BEntry's help
		BEntry entry;
		if (InitCheck() == B_OK && !StorageKit::is_absolute_path(path))
			entry.SetTo(this, path);
		else
			entry.SetTo(path);
		error = entry.InitCheck();
		BPath realPath;
		if (error == B_OK)
			error = entry.GetPath(&realPath);
		if (error == B_OK)
			error = StorageKit::create_dir(realPath.Path());
		if (error == B_OK && dir)
			error = dir->SetTo(realPath.Path());
	}
	return error;
}

// CreateFile
//! Creates a new file.
/*!	If a file with the supplied name does already exist, the method fails,
	unless it is passed \c false to \a failIfExists -- in that case the file
	is truncated to zero size. The new BFile will operate in \c B_READ_WRITE
	mode.
	\param path the new file's path name. May be relative to this
		   directory or absolute.
	\param file a pointer to a BFile to be initialized to the newly
		   created file. May be \c NULL.
	\param failIfExists \c true, if the method should fail when the file
		   already exists, \c false otherwise
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a path.
	- \c B_ENTRY_NOT_FOUND: \a path does not refer to a possible entry.
	- \c B_PERMISSION_DENIED: Directory permissions didn't allow operation.
	- \c B_NO_MEMORY: Insufficient memory for operation.
	- \c B_LINK_LIMIT: Indicates a cyclic loop within the file system.
	- \c B_BUSY: A node was busy.
	- \c B_FILE_ERROR: A general file error.
	- \c B_FILE_EXISTS: A file with that name does already exist and
	  \c true has been passed for \a failIfExists.
	- \c B_IS_A_DIRECTORY: A directory with the supplied name does already
	  exist.
	- \c B_NO_MORE_FDS: The application has run out of file descriptors.
*/
status_t
BDirectory::CreateFile(const char *path, BFile *file, bool failIfExists)
{
	status_t error (path ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		// Let BFile do the dirty job.
		uint32 openMode = B_READ_WRITE | B_CREATE_FILE
						  | (failIfExists ? B_FAIL_IF_EXISTS : 0);
		BFile tmpFile;
		BFile *realFile = (file ? file : &tmpFile);
		if (InitCheck() == B_OK && !StorageKit::is_absolute_path(path))
			error = realFile->SetTo(this, path, openMode);
		else
			error = realFile->SetTo(path, openMode);
		if (error != B_OK)
			realFile->Unset();
	}
	return error;
}

// CreateSymLink
//! Creates a new symbolic link.
/*! If an entry with the supplied name does already exist, the method fails.
	\param path the new symbolic link's path name. May be relative to this
		   directory or absolute.
	\param linkToPath the path the symbolic link shall point to.
	\param dir a pointer to a BSymLink to be initialized to the newly
		   created symbolic link. May be \c NULL.
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a path or \a linkToPath.
	- \c B_ENTRY_NOT_FOUND: \a path does not refer to a possible entry.
	- \c B_PERMISSION_DENIED: Directory permissions didn't allow operation.
	- \c B_NO_MEMORY: Insufficient memory for operation.
	- \c B_LINK_LIMIT: Indicates a cyclic loop within the file system.
	- \c B_BUSY: A node was busy.
	- \c B_FILE_ERROR: A general file error.
	- \c B_FILE_EXISTS: An entry with that name does already exist.
	- \c B_NO_MORE_FDS: The application has run out of file descriptors.
*/
status_t
BDirectory::CreateSymLink(const char *path, const char *linkToPath,
						  BSymLink *link)
{
	status_t error = (path && linkToPath ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		// get the actual (absolute) path using BEntry's help
		BEntry entry;
		if (InitCheck() == B_OK && !StorageKit::is_absolute_path(path))
			entry.SetTo(this, path);
		else
			entry.SetTo(path);
		error = entry.InitCheck();
		BPath realPath;
		if (error == B_OK)
			error = entry.GetPath(&realPath);
		if (error == B_OK)
			error = StorageKit::create_link(realPath.Path(), linkToPath);
		if (error == B_OK && link)
			error = link->SetTo(realPath.Path());
	}
	return error;
}

// =
//! Assigns another BDirectory to this BDirectory.
/*!	If the other BDirectory is uninitialized, this one will be too. Otherwise
	it will refer to the same directory, unless an error occurs.
	\param dir the original BDirectory
	\return a reference to this BDirectory
*/
BDirectory &
BDirectory::operator=(const BDirectory &dir)
{
	if (&dir != this) {	// no need to assign us to ourselves
		Unset();
		if (dir.InitCheck() == B_OK) {
			*((BNode*)this) = dir;
			if (InitCheck() == B_OK) {
				// duplicate the file descriptor
				status_t status = StorageKit::dup_dir(dir.fDirFd, fDirFd);
				if (status != B_OK)
					Unset();
				set_status(status);
			}
		}
	}
	return *this;
}


// FBC
void BDirectory::_ReservedDirectory1() {}
void BDirectory::_ReservedDirectory2() {}
void BDirectory::_ReservedDirectory3() {}
void BDirectory::_ReservedDirectory4() {}
void BDirectory::_ReservedDirectory5() {}
void BDirectory::_ReservedDirectory6() {}

// close_fd
//! Closes the BDirectory's file descriptor.
void
BDirectory::close_fd()
{
	if (fDirFd != StorageKit::NullFd) {
		StorageKit::close_dir(fDirFd);
		fDirFd = StorageKit::NullFd;
	}
	BNode::close_fd();
}

//! Returns the BDirectory's file descriptor.
/*!	To be used instead of accessing the BDirectory's private \c fDirFd member
	directly.
	\return the file descriptor, or -1, if not properly initialized.
*/
StorageKit::FileDescriptor
BDirectory::get_fd() const
{
	return fDirFd;
}


// C functions

// create_directory
//! Creates all missing directories along a given path.
/*!	\param path the directory path name.
	\param mode a permission specification, which shall be used for the
		   newly created directories.
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a path.
	- \c B_ENTRY_NOT_FOUND: \a path does not refer to a possible entry.
	- \c B_PERMISSION_DENIED: Directory permissions didn't allow operation.
	- \c B_NO_MEMORY: Insufficient memory for operation.
	- \c B_LINK_LIMIT: Indicates a cyclic loop within the file system.
	- \c B_BUSY: A node was busy.
	- \c B_FILE_ERROR: A general file error.
	- \c B_NOT_A_DIRECTORY: An entry other than a directory with that name does
	  already exist.
	- \c B_NO_MORE_FDS: The application has run out of file descriptors.
	\todo Check for efficency.
*/
status_t
create_directory(const char *path, mode_t mode)
{
	// That's the strategy: We start with the first component of the supplied
	// path, create a BPath object from it and successively add the following
	// components. Each time we get a new path, we check, if the entry it
	// refers to exists and is a directory. If it doesn't exist, then we try
	// to create it. This goes on, until we're done with the input path or
	// an error occurs.
	status_t error = (path ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		BPath dirPath;
		char *component;
		int32 nextComponent;
		do {
			// get the next path component
			error = StorageKit::parse_first_path_component(path, component,
														   nextComponent);
			if (error == B_OK) {
				// append it to the BPath
				if (dirPath.InitCheck() == B_NO_INIT)	// first component
					error = dirPath.SetTo(component);
				else
					error = dirPath.Append(component);
				delete[] component;
				path += nextComponent;
				// create a BEntry from the BPath
				BEntry entry;
				if (error == B_OK)
					error = entry.SetTo(dirPath.Path(), true);
				// check, if it exists
				if (error == B_OK) {
					if (entry.Exists()) {
						// yep, it exists
						if (!entry.IsDirectory())	// but is no directory
							error = B_NOT_A_DIRECTORY;
					} else	// it doesn't exists -- create it
						error = StorageKit::create_dir(dirPath.Path(), mode);
				}
			}
		} while (error == B_OK && nextComponent != 0);
	}
	return error;
}


#ifdef USE_OPENBEOS_NAMESPACE
};		// namespace OpenBeOS
#endif
