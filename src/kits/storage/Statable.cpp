//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file Statable.cpp
	BStatable implementation.
*/

#include <Statable.h>
#include <Node.h>
#include <Volume.h>

#include <posix/sys/stat.h>

#include "fsproto.h"
#include "kernel_interface.h"

/*!	\fn status_t GetStat(struct stat *st) const
	\brief Returns the stat stucture for the node.
	\param st the stat structure to be filled in.
	\return
	- \c B_OK: Worked fine
	- \c B_NO_MEMORY: Could not allocate the memory for the call.
	- \c B_BAD_VALUE: The current node does not exist.
	- \c B_NOT_ALLOWED: Read only node or volume.
*/

/*!	\brief Returns if the current node is a file.
	\return \c true, if the BNode is properly initialized and is a file,
			\c false otherwise.
*/
bool
BStatable::IsFile() const
{
	struct stat statData;
	if ( GetStat(&statData) == B_OK )
		return S_ISREG(statData.st_mode);
	else 
		return false;
}

/*!	\brief Returns if the current node is a directory.
	\return \c true, if the BNode is properly initialized and is a file,
			\c false otherwise.
*/
bool
BStatable::IsDirectory() const
{
	struct stat statData;
	if ( GetStat(&statData) == B_OK )
		return S_ISDIR(statData.st_mode);
	else 
		return false;
}

/*!	\brief Returns if the current node is a symbolic link.
	\return \c true, if the BNode is properly initialized and is a symlink,
			\c false otherwise.
*/
bool
BStatable::IsSymLink() const
{
	struct stat statData;
	if ( GetStat(&statData) == B_OK )
		return S_ISLNK(statData.st_mode);
	else 
		return false;
}
	
/*!	\brief Returns a node_ref for the current node.
	\param ref the node_ref structure to be filled in
	\see GetStat() for return codes
*/
status_t 
BStatable::GetNodeRef(node_ref *ref) const
{
	status_t error = (ref ? B_OK : B_BAD_VALUE);
	struct stat statData;
	if (error == B_OK)
		error = GetStat(&statData);
	if (error == B_OK) {
		ref->device  = statData.st_dev;
		ref->node = statData.st_ino;
	}
	return error;
}
	
/*!	\brief Returns the owner of the node.
	\param owner a pointer to a uid_t variable to be set to the result
	\see GetStat() for return codes
*/
status_t 
BStatable::GetOwner(uid_t *owner) const
{
	status_t error = (owner ? B_OK : B_BAD_VALUE);
	struct stat statData;
	if (error == B_OK)
		error = GetStat(&statData);
	if (error == B_OK)
		*owner = statData.st_uid;
	return error;
}

/*!	\brief Sets the owner of the node.
	\param owner the new owner
	\see GetStat() for return codes
*/
status_t 
BStatable::SetOwner(uid_t owner)
{
	status_t error;
	struct stat statData;
	error = GetStat(&statData);
	if (error == B_OK) {
		statData.st_uid = owner;
		error = set_stat(statData, WSTAT_UID);
	}
	return error;
}
	
/*!	\brief Returns the group owner of the node.
	\param group a pointer to a gid_t variable to be set to the result
	\see GetStat() for return codes
*/
status_t
BStatable::GetGroup(gid_t *group) const
{
	status_t error = (group ? B_OK : B_BAD_VALUE);
	struct stat statData;
	if (error == B_OK)
		error = GetStat(&statData);
	if (error == B_OK)
		*group = statData.st_gid;
	return error;
}

/*!	\brief Sets the group owner of the node.
	\param group the new group
	\see GetStat() for return codes
*/
status_t
BStatable::SetGroup(gid_t group)
{
	status_t error;
	struct stat statData;
	error = GetStat(&statData);
	if (error == B_OK) {
		statData.st_gid = group;
		error = set_stat(statData, WSTAT_GID);
	}
	return error;
}
	
/*!	\brief Returns the permissions of the node.
	\param perms a pointer to a mode_t variable to be set to the result
	\see GetStat() for return codes
*/
status_t
BStatable::GetPermissions(mode_t *perms) const
{
	status_t error = (perms ? B_OK : B_BAD_VALUE);
	struct stat statData;
	if (error == B_OK)
		error = GetStat(&statData);
	if (error == B_OK)
		*perms = (statData.st_mode & S_IUMSK);
	return error;
}

/*!	\brief Sets the permissions of the node.
	\param perms the new permissions
	\see GetStat() for return codes
*/
status_t
BStatable::SetPermissions(mode_t perms)
{
	status_t error;
	struct stat statData;
	error = GetStat(&statData);
	if (error == B_OK) {
		statData.st_mode = (statData.st_mode & ~S_IUMSK) | (perms & S_IUMSK);
		error = set_stat(statData, WSTAT_MODE);
	}
	return error;
}

/*!	\brief Get the size of the node's data (not counting attributes).
	\param size a pointer to a variable to be set to the result
	\see GetStat() for return codes
*/
status_t
BStatable::GetSize(off_t *size) const
{
	status_t error = (size ? B_OK : B_BAD_VALUE);
	struct stat statData;
	if (error == B_OK)
		error = GetStat(&statData);
	if (error == B_OK)
		*size = statData.st_size;
	return error;
}

/*!	\brief Returns the last time the node was modified.
	\param mtime a pointer to a variable to be set to the result
	\see GetStat() for return codes
*/
status_t
BStatable::GetModificationTime(time_t *mtime) const
{
	status_t error = (mtime ? B_OK : B_BAD_VALUE);
	struct stat statData;
	if (error == B_OK)
		error = GetStat(&statData);
	if (error == B_OK)
		*mtime = statData.st_mtime;
	return error;
}

/*!	\brief Sets the last time the node was modified.
	\param mtime the new modification time
	\see GetStat() for return codes
*/
status_t
BStatable::SetModificationTime(time_t mtime)
{
	status_t error;
	struct stat statData;
	error = GetStat(&statData);
	if (error == B_OK) {
		statData.st_mtime = mtime;
		error = set_stat(statData, WSTAT_MTIME);
	}
	return error;
}

/*!	\brief Returns the time the node was created.
	\param ctime a pointer to a variable to be set to the result
	\see GetStat() for return codes
*/
status_t
BStatable::GetCreationTime(time_t *ctime) const
{
	status_t error = (ctime ? B_OK : B_BAD_VALUE);
	struct stat statData;
	if (error == B_OK)
		error = GetStat(&statData);
	if (error == B_OK)
		*ctime = statData.st_crtime;
	return error;
}

/*!	\brief Sets the time the node was created.
	\param ctime the new creation time
	\see GetStat() for return codes
*/
status_t
BStatable::SetCreationTime(time_t ctime)
{
	status_t error;
	struct stat statData;
	error = GetStat(&statData);
	if (error == B_OK) {
		statData.st_crtime = ctime;
		error = set_stat(statData, WSTAT_CRTIME);
	}
	return error;
}

/*!	\brief Returns the time the node was accessed.
	Not used.
	\see GetModificationTime()
	\see GetStat() for return codes
*/
status_t
BStatable::GetAccessTime(time_t *atime) const
{
	status_t error = (atime ? B_OK : B_BAD_VALUE);
	struct stat statData;
	if (error == B_OK)
		error = GetStat(&statData);
	if (error == B_OK)
		*atime = statData.st_atime;
	return error;
}

/*!	\brief Sets the time the node was accessed.
	Not used.
	\see GetModificationTime()
	\see GetStat() for return codes
*/
status_t
BStatable::SetAccessTime(time_t atime)
{
	status_t error;
	struct stat statData;
	error = GetStat(&statData);
	if (error == B_OK) {
		statData.st_atime = atime;
		error = set_stat(statData, WSTAT_ATIME);
	}
	return error;
}

/*!	\brief Returns the volume the node lives on.
	\param vol a pointer to a variable to be set to the result
	\see BVolume
	\see GetStat() for return codes
*/
status_t
BStatable::GetVolume(BVolume *vol) const
{
	status_t error = (vol ? B_OK : B_BAD_VALUE);
	struct stat statData;
	if (error == B_OK)
		error = GetStat(&statData);
	if (error == B_OK)
		error = vol->SetTo(statData.st_dev);
	return error;
}

void BStatable::_OhSoStatable1() {}
void BStatable::_OhSoStatable2() {}
void BStatable::_OhSoStatable3() {}
