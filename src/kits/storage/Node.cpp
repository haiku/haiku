/*
 * Copyright 2002-2011, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Dauwalder
 *		Ingo Weinhold, bonefish@users.sf.net
 */


/*!
	\file Node.cpp
	BNode implementation.
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


/*! \brief Creates an uninitialized node_ref object.
*/
node_ref::node_ref()
		: device((dev_t)-1),
		  node((ino_t)-1)
{
}

// copy constructor
/*! \brief Creates a copy of the given node_ref object.
	\param ref the node_ref to be copied
*/
node_ref::node_ref(const node_ref &ref)
		: device((dev_t)-1),
		  node((ino_t)-1)
{
	*this = ref;
}

// ==
/*! \brief Tests whether this node_ref and the supplied one are equal.
	\param ref the node_ref to be compared with
	\return \c true, if the objects are equal, \c false otherwise
*/
bool
node_ref::operator==(const node_ref &ref) const
{
	return (device == ref.device && node == ref.node);
}

// !=
/*! \brief Tests whether this node_ref and the supplied one are not equal.
	\param ref the node_ref to be compared with
	\return \c false, if the objects are equal, \c true otherwise
*/
bool
node_ref::operator!=(const node_ref &ref) const
{
	return !(*this == ref);
}

// =
/*! \brief Makes this node ref a copy of the supplied one.
	\param ref the node_ref to be copied
	\return a reference to this object
*/
node_ref&
node_ref::operator=(const node_ref &ref)
{
	device = ref.device;
	node = ref.node;
	return *this;
}


//	#pragma mark - BNode


/*!	\brief Creates an uninitialized BNode object
*/
BNode::BNode()
	 : fFd(-1),
	   fAttrFd(-1),
	   fCStatus(B_NO_INIT)
{
}


/*!	\brief Creates a BNode object and initializes it to the specified
	entry_ref.
	\param ref the entry_ref referring to the entry
*/
BNode::BNode(const entry_ref *ref)
	 : fFd(-1),
	   fAttrFd(-1),
	   fCStatus(B_NO_INIT)
{
	SetTo(ref);
}


/*!	\brief Creates a BNode object and initializes it to the specified
	filesystem entry.
	\param entry the BEntry representing the entry
*/
BNode::BNode(const BEntry *entry)
	 : fFd(-1),
	   fAttrFd(-1),
	   fCStatus(B_NO_INIT)
{
	SetTo(entry);
}


/*!	\brief Creates a BNode object and initializes it to the entry referred
	to by the specified path.
	\param path the path referring to the entry
*/
BNode::BNode(const char *path)
	 : fFd(-1),
	   fAttrFd(-1),
	   fCStatus(B_NO_INIT)
{
	SetTo(path);
}


/*!	\brief Creates a BNode object and initializes it to the entry referred
	to by the specified path rooted in the specified directory.
	\param dir the BDirectory, relative to which the entry's path name is
		   given
	\param path the entry's path name relative to \a dir
*/
BNode::BNode(const BDirectory *dir, const char *path)
	 : fFd(-1),
	   fAttrFd(-1),
	   fCStatus(B_NO_INIT)
{
	SetTo(dir, path);
}


/*! \brief Creates a copy of the given BNode.
	\param node the BNode to be copied
*/
BNode::BNode(const BNode &node)
	 : fFd(-1),
	   fAttrFd(-1),
	   fCStatus(B_NO_INIT)
{
	*this = node;
}


/*!	\brief Frees all resources associated with the BNode.
*/
BNode::~BNode()
{
	Unset();
}


/*!	\brief Checks whether the object has been properly initialized or not.
	\return
	- \c B_OK, if the object has been properly initialized,
	- an error code, otherwise.
*/
status_t
BNode::InitCheck() const
{
	return fCStatus;
}


/*!	\fn status_t BNode::GetStat(struct stat *st) const
	\brief Fills in the given stat structure with \code stat() \endcode
		   information for this object.
	\param st a pointer to a stat structure to be filled in
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a st.
	- another error code, e.g., if the object wasn't properly initialized
*/


/*! \brief Reinitializes the object to the specified entry_ref.
	\param ref the entry_ref referring to the entry
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a ref.
	- \c B_ENTRY_NOT_FOUND: The entry could not be found.
	- \c B_BUSY: The entry is locked.
*/
status_t
BNode::SetTo(const entry_ref *ref)
{
	return _SetTo(ref, false);
}


/*!	\brief Reinitializes the object to the specified filesystem entry.
	\param entry the BEntry representing the entry
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a entry.
	- \c B_ENTRY_NOT_FOUND: The entry could not be found.
	- \c B_BUSY: The entry is locked.
*/
status_t
BNode::SetTo(const BEntry *entry)
{
	if (!entry) {
		Unset();
		return (fCStatus = B_BAD_VALUE);
	}
	return _SetTo(entry->fDirFd, entry->fName, false);
}


/*!	\brief Reinitializes the object to the entry referred to by the specified
		   path.
	\param path the path referring to the entry
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a path.
	- \c B_ENTRY_NOT_FOUND: The entry could not be found.
	- \c B_BUSY: The entry is locked.
*/
status_t
BNode::SetTo(const char *path)
{
	return _SetTo(-1, path, false);
}


/*! \brief Reinitializes the object to the entry referred to by the specified
	path rooted in the specified directory.
	\param dir the BDirectory, relative to which the entry's path name is
		   given
	\param path the entry's path name relative to \a dir
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a dir or \a path.
	- \c B_ENTRY_NOT_FOUND: The entry could not be found.
	- \c B_BUSY: The entry is locked.
*/
status_t
BNode::SetTo(const BDirectory *dir, const char *path)
{
	if (!dir || !path || BPrivate::Storage::is_absolute_path(path)) {
		Unset();
		return (fCStatus = B_BAD_VALUE);
	}
	return _SetTo(dir->fDirFd, path, false);
}


/*!	\brief Returns the object to an uninitialized state.
*/
void
BNode::Unset()
{
	close_fd();
	fCStatus = B_NO_INIT;
}


/*!	\brief Attains an exclusive lock on the data referred to by this node, so
	that it may not be modified by any other objects or methods.
	\return
	- \c B_OK: Everything went fine.
	- \c B_FILE_ERROR: The object is not initialized.
	- \c B_BUSY: The node is already locked.
*/
status_t
BNode::Lock()
{
	if (fCStatus != B_OK)
		return fCStatus;
	return _kern_lock_node(fFd);
}


/*!	\brief Unlocks the node.
	\return
	- \c B_OK: Everything went fine.
	- \c B_FILE_ERROR: The object is not initialized.
	- \c B_BAD_VALUE: The node is not locked.
*/
status_t
BNode::Unlock()
{
	if (fCStatus != B_OK)
		return fCStatus;
	return _kern_unlock_node(fFd);
}


/*!	\brief Immediately performs any pending disk actions on the node.
	\return
	- \c B_OK: Everything went fine.
	- an error code, if something went wrong.
*/
status_t
BNode::Sync()
{
	return (fCStatus != B_OK) ? B_FILE_ERROR : _kern_fsync(fFd);
}


/*!	\brief Writes data from a buffer to an attribute.
	Write the \a len bytes of data from \a buffer to
	the attribute specified by \a name after erasing any data
	that existed previously. The type specified by \a type \em is
	remembered, and may be queried with GetAttrInfo(). The value of
	\a offset is currently ignored.
	\param attr the name of the attribute
	\param type the type of the attribute
	\param offset the index at which to write the data (currently ignored)
	\param buffer the buffer containing the data to be written
	\param len the number of bytes to be written
	\return
	- the number of bytes actually written
	- \c B_BAD_VALUE: \c NULL \a attr or \a buffer
	- \c B_FILE_ERROR: The object is not initialized or the node it refers to
	  is read only.
	- \c B_NOT_ALLOWED: The node resides on a read only volume.
	- \c B_DEVICE_FULL: Insufficient disk space.
	- \c B_NO_MEMORY: Insufficient memory to complete the operation.
*/
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


/*!	\brief Reads data from an attribute into a buffer.
	Reads the data of the attribute given by \a name into
	the buffer specified by \a buffer with length specified
	by \a len. \a type and \a offset are currently ignored.
	\param attr the name of the attribute
	\param type the type of the attribute (currently ignored)
	\param offset the index from which to read the data (currently ignored)
	\param buffer the buffer for the data to be read
	\param len the number of bytes to be read
	\return
	- the number of bytes actually read
	- \c B_BAD_VALUE: \c NULL \a attr or \a buffer
	- \c B_FILE_ERROR: The object is not initialized.
	- \c B_ENTRY_NOT_FOUND: The node has no attribute \a attr.
*/
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


/*!	\brief Deletes the attribute given by \a name.
	\param name the name of the attribute
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a name
	- \c B_FILE_ERROR: The object is not initialized or the node it refers to
	  is read only.
	- \c B_ENTRY_NOT_FOUND: The node has no attribute \a name.
	- \c B_NOT_ALLOWED: The node resides on a read only volume.
*/
status_t
BNode::RemoveAttr(const char *name)
{
	return fCStatus != B_OK ? B_FILE_ERROR : _kern_remove_attr(fFd, name);
}


/*!	\brief Moves the attribute given by \a oldname to \a newname.
	If \a newname already exists, the current data is clobbered.
	\param oldname the name of the attribute to be renamed
	\param newname the new name for the attribute
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a oldname or \a newname
	- \c B_FILE_ERROR: The object is not initialized or the node it refers to
	  is read only.
	- \c B_ENTRY_NOT_FOUND: The node has no attribute \a oldname.
	- \c B_NOT_ALLOWED: The node resides on a read only volume.
*/
status_t
BNode::RenameAttr(const char *oldname, const char *newname)
{
	if (fCStatus != B_OK)
		return B_FILE_ERROR;

	return _kern_rename_attr(fFd, oldname, fFd, newname);
}


/*!	\brief Fills in the pre-allocated attr_info struct pointed to by \a info
	with useful information about the attribute specified by \a name.
	\param name the name of the attribute
	\param info the attr_info structure to be filled in
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a name
	- \c B_FILE_ERROR: The object is not initialized.
	- \c B_ENTRY_NOT_FOUND: The node has no attribute \a name.
*/
status_t
BNode::GetAttrInfo(const char *name, struct attr_info *info) const
{
	if (fCStatus != B_OK)
		return B_FILE_ERROR;
	if (!name || !info)
		return B_BAD_VALUE;

	return fs_stat_attr(fFd, name, info) < 0 ? errno : B_OK ;
}


/*!	\brief Returns the next attribute in the node's list of attributes.
	Every BNode maintains a pointer to its list of attributes.
	GetNextAttrName() retrieves the name of the attribute that the pointer is
	currently pointing to, and then bumps the pointer to the next attribute.
	The name is copied into the buffer, which should be at least
	B_ATTR_NAME_LENGTH characters long. The copied name is NULL-terminated.
	When you've asked for every name in the list, GetNextAttrName()
	returns \c B_ENTRY_NOT_FOUND.
	\param buffer the buffer the name of the next attribute shall be stored in
		   (must be at least \c B_ATTR_NAME_LENGTH bytes long)
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a buffer.
	- \c B_FILE_ERROR: The object is not initialized.
	- \c B_ENTRY_NOT_FOUND: There are no more attributes, the last attribute
	  name has already been returned.
*/
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


/*! \brief Resets the object's attribute pointer to the first attribute in the
	list.
	\return
	- \c B_OK: Everything went fine.
	- \c B_FILE_ERROR: Some error occured.
*/
status_t
BNode::RewindAttrs()
{
	if (InitAttrDir() != B_OK)
		return B_FILE_ERROR;

	return _kern_rewind_dir(fAttrFd);
}


/*!	Writes the specified string to the specified attribute, clobbering any
	previous data.
	\param name the name of the attribute
	\param data the BString to be written to the attribute
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a name or \a data
	- \c B_FILE_ERROR: The object is not initialized or the node it refers to
	  is read only.
	- \c B_NOT_ALLOWED: The node resides on a read only volume.
	- \c B_DEVICE_FULL: Insufficient disk space.
	- \c B_NO_MEMORY: Insufficient memory to complete the operation.
*/
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


/*!	\brief Reads the data of the specified attribute into the pre-allocated
		   \a result.
	\param name the name of the attribute
	\param result the BString to be set to the value of the attribute
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a name or \a result
	- \c B_FILE_ERROR: The object is not initialized.
	- \c B_ENTRY_NOT_FOUND: The node has no attribute \a attr.
*/
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


/*!	\brief Reinitializes the object as a copy of the \a node.
	\param node the BNode to be copied
	\return a reference to this BNode object.
*/
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


/*!	Tests whether this and the supplied BNode object are equal.
	Two BNode objects are said to be equal if they're set to the same node,
	or if they're both \c B_NO_INIT.
	\param node the BNode to be compared with
	\return \c true, if the BNode objects are equal, \c false otherwise
*/
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


/*!	Tests whether this and the supplied BNode object are not equal.
	Two BNode objects are said to be equal if they're set to the same node,
	or if they're both \c B_NO_INIT.
	\param node the BNode to be compared with
	\return \c false, if the BNode objects are equal, \c true otherwise
*/
bool
BNode::operator!=(const BNode &node) const
{
	return !(*this == node);
}


/*!	\brief Returns a POSIX file descriptor to the node this object refers to.
	Remember to call close() on the file descriptor when you're through with
	it.
	\return a valid file descriptor, or -1, if something went wrong.
*/
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


/*!	\brief Sets the node's file descriptor.
	Used by each implementation (i.e. BNode, BFile, BDirectory, etc.) to set
	the node's file descriptor. This allows each subclass to use the various
	file-type specific system calls for opening file descriptors.
	\param fd the file descriptor this BNode should be set to (may be -1)
	\return \c B_OK, if everything went fine, an error code otherwise.
	\note This method calls close_fd() to close previously opened FDs. Thus
		  derived classes should take care to first call set_fd() and set
		  class specific resources freed in their close_fd() version
		  thereafter.
*/
status_t
BNode::set_fd(int fd)
{
	if (fFd != -1)
		close_fd();
	fFd = fd;
	return B_OK;
}


/*!	\brief Closes the node's file descriptor(s).
	To be implemented by subclasses to close the file descriptor using the
	proper system call for the given file-type. This implementation calls
	_kern_close(fFd) and also _kern_close(fAttrDir) if necessary.
*/
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


/*! \brief Sets the BNode's status.
	To be used by derived classes instead of accessing the BNode's private
	\c fCStatus member directly.
	\param newStatus the new value for the status variable.
*/
void
BNode::set_status(status_t newStatus)
{
	fCStatus = newStatus;
}


/*!	\brief Initializes the BNode's file descriptor to the node referred to
		   by the given FD and path combo.

	\a path must either be \c NULL, an absolute or a relative path.
	In the first case, \a fd must not be \c NULL; the node it refers to will
	be opened. If absolute, \a fd is ignored. If relative and \a fd is >= 0,
	it will be reckoned off the directory identified by \a fd, otherwise off
	the current working directory.

	The method will first try to open the node with read and write permission.
	If that fails due to a read-only FS or because the user has no write
	permission for the node, it will re-try opening the node read-only.

	The \a fCStatus member will be set to the return value of this method.

	\param fd Either a directory FD or a value < 0. In the latter case \a path
		   must be specified.
	\param path Either \a NULL in which case \a fd must be given, absolute, or
		   relative to the directory specified by \a fd (if given) or to the
		   current working directory.
	\param traverse If the node identified by \a fd and \a path is a symlink
		   and \a traverse is \c true, the symlink will be resolved recursively.
	\return \c B_OK, if everything went fine, another error code otherwise.
*/
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


/*!	\brief Initializes the BNode's file descriptor to the node referred to
		   by the given entry_ref.

	The method will first try to open the node with read and write permission.
	If that fails due to a read-only FS or because the user has no write
	permission for the node, it will re-try opening the node read-only.

	The \a fCStatus member will be set to the return value of this method.

	\param ref An entry_ref identifying the node to be opened.
	\param traverse If the node identified by \a ref is a symlink
		   and \a traverse is \c true, the symlink will be resolved recursively.
	\return \c B_OK, if everything went fine, another error code otherwise.
*/
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


/*! \brief Modifies a certain setting for this node based on \a what and the
	corresponding value in \a st.
	Inherited from and called by BStatable.
	\param st a stat structure containing the value to be set
	\param what specifies what setting to be modified
	\return \c B_OK if everything went fine, an error code otherwise.
*/
status_t
BNode::set_stat(struct stat &st, uint32 what)
{
	if (fCStatus != B_OK)
		return B_FILE_ERROR;

	return _kern_write_stat(fFd, NULL, false, &st, sizeof(struct stat),
		what);
}


/*! \brief Verifies that the BNode has been properly initialized, and then
	(if necessary) opens the attribute directory on the node's file
	descriptor, storing it in fAttrDir.
	\return \c B_OK if everything went fine, an error code otherwise.
*/
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


/*!	\var BNode::fFd
	File descriptor for the given node.
*/

/*!	\var BNode::fAttrFd
	File descriptor for the attribute directory of the node. Initialized lazily.
*/

/*!	\var BNode::fCStatus
	The object's initialization status.
*/


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
