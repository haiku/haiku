//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//----------------------------------------------------------------------
/*!
	\file Node.cpp
	BNode implementation.
*/

#include <fs_attr.h> // for struct attr_info
#include <new>
#include <string.h>

#include <Node.h>
#include <Entry.h>

#include "kernel_interface.h"
#include "storage_support.h"

//----------------------------------------------------------------------
// node_ref
//----------------------------------------------------------------------

// constructor
/*! \brief Creates an uninitialized node_ref object.
*/
node_ref::node_ref()
		: device(-1),
		  node(-1)
{
}

// copy constructor
/*! \brief Creates a copy of the given node_ref object.
	\param ref the node_ref to be copied
*/
node_ref::node_ref(const node_ref &ref)
		: device(-1),
		  node(-1)
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

//----------------------------------------------------------------------
// BNode
//----------------------------------------------------------------------

/*!	\brief Creates an uninitialized BNode object
*/
BNode::BNode()
	 : fFd(StorageKit::NullFd),
	   fAttrFd(StorageKit::NullFd),
	   fCStatus(B_NO_INIT)
{
}

/*!	\brief Creates a BNode object and initializes it to the specified
	entry_ref.
	\param ref the entry_ref referring to the entry
*/
BNode::BNode(const entry_ref *ref)
	 : fFd(StorageKit::NullFd),
	   fAttrFd(StorageKit::NullFd),
	   fCStatus(B_NO_INIT)
{
	SetTo(ref);
}

/*!	\brief Creates a BNode object and initializes it to the specified
	filesystem entry.
	\param entry the BEntry representing the entry
*/
BNode::BNode(const BEntry *entry)
	 : fFd(StorageKit::NullFd),
	   fAttrFd(StorageKit::NullFd),
	   fCStatus(B_NO_INIT)
{
	SetTo(entry);
}

/*!	\brief Creates a BNode object and initializes it to the entry referred
	to by the specified path.
	\param path the path referring to the entry
*/
BNode::BNode(const char *path)
	 : fFd(StorageKit::NullFd),
	   fAttrFd(StorageKit::NullFd),
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
	 : fFd(StorageKit::NullFd),
	   fAttrFd(StorageKit::NullFd),
	   fCStatus(B_NO_INIT)
{
	SetTo(dir, path);
}

/*! \brief Creates a copy of the given BNode.
	\param node the BNode to be copied
*/
BNode::BNode(const BNode &node)
	 : fFd(StorageKit::NullFd),
	   fAttrFd(StorageKit::NullFd),
	   fCStatus(B_NO_INIT)
{
	*this = node;
}

/*!	\brief Frees all resources associated with this BNode.
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

/*! \brief Fills in the given stat structure with \code stat() \endcode
		   information for this object.
	\param st a pointer to a stat structure to be filled in
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a st.
	- another error code, e.g., if the object wasn't properly initialized
*/
status_t
BNode::GetStat(struct stat *st) const
{
	return (fCStatus != B_OK) ? fCStatus : StorageKit::get_stat(fFd, st) ;
}

/*! \brief Reinitializes the object to the specified entry_ref.
	\param ref the entry_ref referring to the entry
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a ref.
	- \c B_ENTRY_NOT_FOUND: The entry could not be found.
	- \c B_BUSY: The entry is locked.
	\todo Currently implemented using StorageKit::entry_ref_to_path().
		  Reimplement!
*/
status_t
BNode::SetTo(const entry_ref *ref)
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
	fCStatus = error;
	return error;
}

/*!	\brief Reinitializes the object to the specified filesystem entry.
	\param entry the BEntry representing the entry
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a entry.
	- \c B_ENTRY_NOT_FOUND: The entry could not be found.
	- \c B_BUSY: The entry is locked.
	\todo Implemented using SetTo(entry_ref*). Check, if necessary to
		  reimplement!
*/
status_t
BNode::SetTo(const BEntry *entry)
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
	fCStatus = error;
	return error;
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
	Unset();	
	if (path != NULL) 
		fCStatus = StorageKit::open(path, O_RDWR | O_NOTRAVERSE, fFd);
	return fCStatus;
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
	\todo Implemented using SetTo(BEntry*). Check, if necessary to reimplement!
*/
status_t
BNode::SetTo(const BDirectory *dir, const char *path)
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
	fCStatus = error;
	return error;
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
	\todo Currently unimplemented; requires new kernel.
*/
status_t
BNode::Lock()
{
	if (fCStatus != B_OK)
		return fCStatus;

	// This will have to wait for the new kenel
	return B_FILE_ERROR;

	// We'll need to keep lock around if the kernel function
	// doesn't just work on file descriptors
//	StorageKit::FileLock lock;
//	return StorageKit::lock(fFd, StorageKit::READ_WRITE, &lock);
}

/*!	\brief Unlocks the node.
	\return
	- \c B_OK: Everything went fine.
	- \c B_FILE_ERROR: The object is not initialized.
	- \c B_BAD_VALUE: The node is not locked.
	\todo Currently unimplemented; requires new kernel.
*/
status_t
BNode::Unlock()
{
	if (fCStatus != B_OK)
		return fCStatus;
	// This will have to wait for the new kenel
	return B_FILE_ERROR;
}

/*!	\brief Immediately performs any pending disk actions on the node.
	\return
	- \c B_OK: Everything went fine.
	- an error code, if something went wrong.
*/
status_t
BNode::Sync()
{
	return (fCStatus != B_OK) ? B_FILE_ERROR : StorageKit::sync(fFd) ;
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
	else {
		ssize_t result = StorageKit::write_attr(fFd, attr, type, offset,
												buffer, len);
		return result;
	}
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
	else {
		ssize_t result = StorageKit::read_attr(fFd, attr, type, offset, buffer,
											   len);
		return result;
	}
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
	return (fCStatus != B_OK) ? B_FILE_ERROR
							  : StorageKit::remove_attr(fFd, name);
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
	return StorageKit::rename_attr(fFd, oldname, newname);
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
	return (fCStatus != B_OK) ? B_FILE_ERROR
							  : StorageKit::stat_attr(fFd, name, info);
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
	// B_BUFFER_NAME_LENGTH chars long, but NULLs
	// are not acceptable.
	if (buffer == NULL)
		return B_BAD_VALUE;	// /new R5 crashed when passed NULL
	if (InitAttrDir() != B_OK)
		return B_FILE_ERROR;
		
	StorageKit::LongDirEntry entry;
	status_t error = StorageKit::read_attr_dir(fAttrFd, entry);
	if (error == B_OK) {
		strncpy(buffer, entry.d_name, B_ATTR_NAME_LENGTH);
		return B_OK;
	}
	return error;
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
	StorageKit::rewind_attr_dir(fAttrFd);
	return B_OK;
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
		int32 len = data->Length();
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
	char *data = result->LockBuffer(info.size+1);
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
	result->UnlockBuffer(bytes+1);	
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
	fFd = StorageKit::dup(node.fFd);
	fCStatus = (fFd == StorageKit::NullFd) ? B_NO_INIT : B_OK ;
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
		// Check if they're identical
		StorageKit::Stat s1, s2;
		if (GetStat(&s1) != B_OK)
			return false;
		if (node.GetStat(&s2) != B_OK)
			return false;
		return (s1.st_dev == s2.st_dev && s1.st_ino == s2.st_ino);
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
	return StorageKit::dup(fFd);
}


/*! (currently unused) */
void BNode::_ReservedNode1() { }
void BNode::_ReservedNode2() { }
void BNode::_ReservedNode3() { }
void BNode::_ReservedNode4() { }
void BNode::_ReservedNode5() { }
void BNode::_ReservedNode6() { }

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
BNode::set_fd(StorageKit::FileDescriptor fd)
{
	if (fFd != -1)
		close_fd();
	fFd = fd;
	return B_OK;
}

/*!	\brief Closes the node's file descriptor(s).
	To be implemented by subclasses to close the file descriptor using the
	proper system call for the given file-type. This implementation calls
	StorageKit::close(fFd) and also StorageKit::close_attr_dir(fAttrDir)
	if necessary.
*/
void
BNode::close_fd()
{
	if (fAttrFd != StorageKit::NullFd)
	{
		StorageKit::close_attr_dir(fAttrFd);
		fAttrFd = StorageKit::NullFd;
	}	
	if (fFd != StorageKit::NullFd) {
		close(fFd);
		fFd = StorageKit::NullFd;
	}	
}

// set_status
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
	return StorageKit::set_stat(fFd, st, what);
}

/*! \brief Verifies that the BNode has been properly initialized, and then
	(if necessary) opens the attribute directory on the node's file
	descriptor, storing it in fAttrDir.
	\return \c B_OK if everything went fine, an error code otherwise.
*/
status_t
BNode::InitAttrDir()
{
	if (fCStatus == B_OK && fAttrFd == StorageKit::NullFd) 
		return StorageKit::open_attr_dir(fFd, fAttrFd);
	return fCStatus;	
}

/*!	\var BNode::fFd
	File descriptor for the given node.
*/

/*!	\var BNode::fAttrFd
	This appears to be passed to the attribute directory functions
	like a StorageKit::Dir would be, but it's actually a file descriptor.
	Best I can figure, the R5 syscall for reading attributes must've
	just taken a file descriptor. Depending on what our kernel ends up
	providing, this may or may not be replaced with an Dir*
*/

/*!	\var BNode::fCStatus
	The object's initialization status.
*/
