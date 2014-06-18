/*
 * Copyright 2002-2014 Haiku, Inc. All rights reserved.
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

	status_t GetStatBeOS(struct stat_beos* stat)
	{
		return fObject->_GetStat(stat);
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
	struct stat stat;
	if (GetStat(&stat) == B_OK)
		return S_ISREG(stat.st_mode);
	else
		return false;
}


// Returns whether or not the current node is a directory.
bool
BStatable::IsDirectory() const
{
	struct stat stat;
	if (GetStat(&stat) == B_OK)
		return S_ISDIR(stat.st_mode);
	else
		return false;
}


// Returns whether or not the current node is a symbolic link.
bool
BStatable::IsSymLink() const
{
	struct stat stat;
	if (GetStat(&stat) == B_OK)
		return S_ISLNK(stat.st_mode);
	else
		return false;
}


// Fills out ref with the node_ref of the node.
status_t
BStatable::GetNodeRef(node_ref* ref) const
{
	status_t result = (ref ? B_OK : B_BAD_VALUE);
	struct stat stat;

	if (result == B_OK)
		result = GetStat(&stat);

	if (result == B_OK) {
		ref->device  = stat.st_dev;
		ref->node = stat.st_ino;
	}

	return result;
}


// Fills out the node's UID into owner.
status_t
BStatable::GetOwner(uid_t* owner) const
{
	status_t result = (owner ? B_OK : B_BAD_VALUE);
	struct stat stat;

	if (result == B_OK)
		result = GetStat(&stat);

	if (result == B_OK)
		*owner = stat.st_uid;

	return result;
}


// Sets the node's UID to owner.
status_t
BStatable::SetOwner(uid_t owner)
{
	struct stat stat;
	stat.st_uid = owner;

	return set_stat(stat, B_STAT_UID);
}


// Fills out the node's GID into group.
status_t
BStatable::GetGroup(gid_t* group) const
{
	status_t result = (group ? B_OK : B_BAD_VALUE);
	struct stat stat;

	if (result == B_OK)
		result = GetStat(&stat);

	if (result == B_OK)
		*group = stat.st_gid;

	return result;
}


// Sets the node's GID to group.
status_t
BStatable::SetGroup(gid_t group)
{
	struct stat stat;
	stat.st_gid = group;

	return set_stat(stat, B_STAT_GID);
}


// Fills out permissions with the node's permissions.
status_t
BStatable::GetPermissions(mode_t* permissions) const
{
	status_t result = (permissions ? B_OK : B_BAD_VALUE);
	struct stat stat;

	if (result == B_OK)
		result = GetStat(&stat);

	if (result == B_OK)
		*permissions = (stat.st_mode & S_IUMSK);

	return result;
}


// Sets the node's permissions to permissions.
status_t
BStatable::SetPermissions(mode_t permissions)
{
	struct stat stat;
	// the FS should do the correct masking -- only the S_IUMSK part is
	// modifiable
	stat.st_mode = permissions;

	return set_stat(stat, B_STAT_MODE);
}


// Fills out the size of the node's data (not counting attributes) into size.
status_t
BStatable::GetSize(off_t* size) const
{
	status_t result = (size ? B_OK : B_BAD_VALUE);
	struct stat stat;

	if (result == B_OK)
		result = GetStat(&stat);

	if (result == B_OK)
		*size = stat.st_size;

	return result;
}


// Fills out mtime with the last modification time of the node.
status_t
BStatable::GetModificationTime(time_t* mtime) const
{
	status_t result = (mtime ? B_OK : B_BAD_VALUE);
	struct stat stat;

	if (result == B_OK)
		result = GetStat(&stat);

	if (result == B_OK)
		*mtime = stat.st_mtime;

	return result;
}


// Sets the node's last modification time to mtime.
status_t
BStatable::SetModificationTime(time_t mtime)
{
	struct stat stat;
	stat.st_mtime = mtime;

	return set_stat(stat, B_STAT_MODIFICATION_TIME);
}


// Fills out ctime with the creation time of the node
status_t
BStatable::GetCreationTime(time_t* ctime) const
{
	status_t result = (ctime ? B_OK : B_BAD_VALUE);
	struct stat stat;

	if (result == B_OK)
		result = GetStat(&stat);

	if (result == B_OK)
		*ctime = stat.st_crtime;

	return result;
}


// Sets the node's creation time to ctime.
status_t
BStatable::SetCreationTime(time_t ctime)
{
	struct stat stat;
	stat.st_crtime = ctime;

	return set_stat(stat, B_STAT_CREATION_TIME);
}


// Fills out atime with the access time of the node.
status_t
BStatable::GetAccessTime(time_t* atime) const
{
	status_t result = (atime ? B_OK : B_BAD_VALUE);
	struct stat stat;

	if (result == B_OK)
		result = GetStat(&stat);

	if (result == B_OK)
		*atime = stat.st_atime;

	return result;
}


// Sets the node's access time to atime.
status_t
BStatable::SetAccessTime(time_t atime)
{
	struct stat stat;
	stat.st_atime = atime;

	return set_stat(stat, B_STAT_ACCESS_TIME);
}


// Fills out vol with the the volume that the node lives on.
status_t
BStatable::GetVolume(BVolume* volume) const
{
	status_t result = (volume ? B_OK : B_BAD_VALUE);
	struct stat stat;
	if (result == B_OK)
		result = GetStat(&stat);

	if (result == B_OK)
		result = volume->SetTo(stat.st_dev);

	return result;
}


// _OhSoStatable1() -> GetStat()
extern "C" status_t
#if __GNUC__ == 2
_OhSoStatable1__9BStatable(const BStatable* self, struct stat* stat)
#else
_ZN9BStatable14_OhSoStatable1Ev(const BStatable* self, struct stat* stat)
#endif
{
	// No Perform() method -- we have to use the old GetStat() method instead.
	struct stat_beos oldStat;
	status_t result = BStatable::Private(self).GetStatBeOS(&oldStat);
	if (result != B_OK)
		return result;

	convert_from_stat_beos(&oldStat, stat);

	return B_OK;
}


void BStatable::_OhSoStatable2() {}
void BStatable::_OhSoStatable3() {}
