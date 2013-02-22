/*
 * Copyright 2002-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Dauwalder
 *		Ingo Weinhold, bonefish@users.sf.net
 */


#include <Statable.h>

#include <sys/stat.h>

#include <compat/sys/stat.h>

#include <Node.h>
#include <NodeMonitor.h>
#include <Volume.h>


class BStatable::Private {
public:
	Private(const BStatable* object)
		:
		fObject(object)
	{
	}

	status_t GetStatBeOS(struct stat_beos* st)
	{
		return fObject->_GetStat(st);
	}

private:
	const BStatable*	fObject;
};


#if __GNUC__ > 3
BStatable::~BStatable()
{
}
#endif


// Returns whether or not the current node is a file.
bool
BStatable::IsFile() const
{
	struct stat statData;
	if (GetStat(&statData) == B_OK)
		return S_ISREG(statData.st_mode);
	else
		return false;
}


// Returns whether or not the current node is a directory.
bool
BStatable::IsDirectory() const
{
	struct stat statData;
	if (GetStat(&statData) == B_OK)
		return S_ISDIR(statData.st_mode);
	else
		return false;
}


// Returns whether or not the current node is a symbolic link.
bool
BStatable::IsSymLink() const
{
	struct stat statData;
	if (GetStat(&statData) == B_OK)
		return S_ISLNK(statData.st_mode);
	else
		return false;
}


// Fills out ref with the node_ref of the node.
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


// Fills out the node's UID into owner.
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


// Sets the node's UID to owner.
status_t
BStatable::SetOwner(uid_t owner)
{
	struct stat statData;
	statData.st_uid = owner;
	return set_stat(statData, B_STAT_UID);
}


// Fills out the node's GID into group.
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


// Sets the node's GID to group.
status_t
BStatable::SetGroup(gid_t group)
{
	struct stat statData;
	statData.st_gid = group;
	return set_stat(statData, B_STAT_GID);
}


// Fills out perms with the node's permissions.
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


// Sets the node's permissions to perms.
status_t
BStatable::SetPermissions(mode_t perms)
{
	struct stat statData;
	// the FS should do the correct masking -- only the S_IUMSK part is
	// modifiable
	statData.st_mode = perms;
	return set_stat(statData, B_STAT_MODE);
}


// Fills out the size of the node's data (not counting attributes) into size.
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


// Fills out mtime with the last modification time of the node.
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


// Sets the node's last modification time to mtime.
status_t
BStatable::SetModificationTime(time_t mtime)
{
	struct stat statData;
	statData.st_mtime = mtime;
	return set_stat(statData, B_STAT_MODIFICATION_TIME);
}


// Fills out ctime with the creation time of the node
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


// Sets the node's creation time to ctime.
status_t
BStatable::SetCreationTime(time_t ctime)
{
	struct stat statData;
	statData.st_crtime = ctime;
	return set_stat(statData, B_STAT_CREATION_TIME);
}


// Fills out atime with the access time of the node.
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


// Sets the node's access time to atime.
status_t
BStatable::SetAccessTime(time_t atime)
{
	struct stat statData;
	statData.st_atime = atime;
	return set_stat(statData, B_STAT_ACCESS_TIME);
}


// Fills out vol with the the volume that the node lives on.
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


// _OhSoStatable1() -> GetStat()
extern "C" status_t
#if __GNUC__ == 2
_OhSoStatable1__9BStatable(const BStatable *self, struct stat *st)
#else
_ZN9BStatable14_OhSoStatable1Ev(const BStatable *self, struct stat *st)
#endif
{
	// No Perform() method -- we have to use the old GetStat() method instead.
	struct stat_beos oldStat;
	status_t error = BStatable::Private(self).GetStatBeOS(&oldStat);
	if (error != B_OK)
		return error;

	convert_from_stat_beos(&oldStat, st);
	return B_OK;
}


void BStatable::_OhSoStatable2() {}
void BStatable::_OhSoStatable3() {}
