/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <usergroup.h>

#include <errno.h>
#include <sys/stat.h>

#include <new.h>

#include <kernel.h>
#include <syscalls.h>
#include <team.h>
#include <thread.h>
#include <thread_types.h>
#include <util/AutoLock.h>

#include <AutoDeleter.h>


// #pragma mark - Implementation Private


static bool
is_privileged(struct team* team)
{
	// currently only the root user is privileged
	return team->effective_uid == 0;
}


static status_t
common_setregid(gid_t rgid, gid_t egid, bool setAllIfPrivileged, bool kernel)
{
	struct team* team = thread_get_current_thread()->team;

	InterruptsSpinLocker _(team_spinlock);

	bool privileged = kernel || is_privileged(team);

	// real gid
	if (rgid == (gid_t)-1) {
		rgid = team->real_gid;
	} else {
		if (setAllIfPrivileged) {
			// setgid() semantics: If privileged set both, real, effective and
			// saved set-gid, otherwise set the effective gid.
			if (privileged) {
				team->saved_set_gid = rgid;
				team->real_gid = rgid;
				team->effective_gid = rgid;
				return B_OK;
			}

			// not privileged -- set only the effective gid
			egid = rgid;
			rgid = team->real_gid;
		} else {
			// setregid() semantics: set the real gid, if allowed to
			if (!privileged && rgid != team->real_gid)
				return EPERM;
		}
	}

	// effective gid
	if (egid == (gid_t)-1) {
		egid = team->effective_gid;
	} else {
		if (!privileged && egid != team->effective_gid
			&& egid != team->real_gid && egid != team->saved_set_gid) {
			return EPERM;
		}
	}

	// Getting here means all checks were successful -- set the gids.
	team->real_gid = rgid;
	team->effective_gid = egid;

	return B_OK;
}


static status_t
common_setreuid(uid_t ruid, uid_t euid, bool setAllIfPrivileged, bool kernel)
{
	struct team* team = thread_get_current_thread()->team;

	InterruptsSpinLocker _(team_spinlock);

	bool privileged = kernel || is_privileged(team);

	// real uid
	if (ruid == (uid_t)-1) {
		ruid = team->real_uid;
	} else {
		if (setAllIfPrivileged) {
			// setuid() semantics: If privileged set both, real, effective and
			// saved set-uid, otherwise set the effective uid.
			if (privileged) {
				team->saved_set_uid = ruid;
				team->real_uid = ruid;
				team->effective_uid = ruid;
				return B_OK;
			}

			// not privileged -- set only the effective uid
			euid = ruid;
			ruid = team->real_uid;
		} else {
			// setreuid() semantics: set the real uid, if allowed to
			// Note: We allow setting the real uid to the effective uid. This
			// is unspecified by the specs, but is common practice.
			if (!privileged && ruid != team->real_uid
				&& ruid != team->effective_uid) {
				return EPERM;
			}
		}
	}

	// effective uid
	if (euid == (uid_t)-1) {
		euid = team->effective_uid;
	} else {
		if (!privileged && euid != team->effective_uid
			&& euid != team->real_uid && euid != team->saved_set_uid) {
			return EPERM;
		}
	}

	// Getting here means all checks were successful -- set the uids.
	team->real_uid = ruid;
	team->effective_uid = euid;

	return B_OK;
}


// #pragma mark - Kernel Private


void
inherit_parent_user_and_group(struct team* team, struct team* parent)
{
	InterruptsSpinLocker _(team_spinlock);

	team->saved_set_uid = parent->saved_set_uid;
	team->real_uid = parent->real_uid;
	team->effective_uid = parent->effective_uid;
	team->saved_set_gid = parent->saved_set_gid;
	team->real_gid = parent->real_gid;
	team->effective_gid = parent->effective_gid;
}


status_t
update_set_id_user_and_group(struct team* team, const char* file)
{
	struct stat st;
	if (stat(file, &st) < 0)
		return errno;

	InterruptsSpinLocker _(team_spinlock);

	if ((st.st_mode & S_ISUID) != 0) {
		team->saved_set_uid = st.st_uid;
		team->effective_uid = st.st_uid;
	}

	if ((st.st_mode & S_ISGID) != 0) {
		team->saved_set_gid = st.st_gid;
		team->effective_gid = st.st_gid;
	}

	return B_OK;
}


gid_t
_kern_getgid(bool effective)
{
	struct team* team = thread_get_current_thread()->team;

	return effective ? team->effective_gid : team->real_gid;
}


uid_t
_kern_getuid(bool effective)
{
	struct team* team = thread_get_current_thread()->team;

	return effective ? team->effective_uid : team->real_uid;
}


ssize_t
_kern_getgroups(int groupSize, gid_t* groupList)
{
	// TODO: Implement proper supplementary group support!
	// For now only return the effective group.

	if (groupSize > 0)
		groupList[0] = getegid();

	return 1;
}


status_t
_kern_setregid(gid_t rgid, gid_t egid, bool setAllIfPrivileged)
{
	return common_setregid(rgid, egid, setAllIfPrivileged, true);
}


status_t
_kern_setreuid(uid_t ruid, uid_t euid, bool setAllIfPrivileged)
{
	return common_setreuid(ruid, euid, setAllIfPrivileged, true);
}


// #pragma mark - Syscalls


gid_t
_user_getgid(bool effective)
{
	struct team* team = thread_get_current_thread()->team;

	return effective ? team->effective_gid : team->real_gid;
}


uid_t
_user_getuid(bool effective)
{
	struct team* team = thread_get_current_thread()->team;

	return effective ? team->effective_uid : team->real_uid;
}


ssize_t
_user_getgroups(int groupSize, gid_t* userGroupList)
{
	gid_t* groupList = NULL;

	if (groupSize < 0)
		return B_BAD_VALUE;
	if (groupSize > NGROUPS_MAX + 1)
		groupSize = NGROUPS_MAX + 1;

	if (groupSize > 0) {
		if (userGroupList == NULL || !IS_USER_ADDRESS(userGroupList))
			return B_BAD_VALUE;

		groupList = new(nothrow) gid_t[groupSize];
		if (groupList == NULL)
			return B_NO_MEMORY;
	}

	ArrayDeleter<gid_t> _(groupList);

	ssize_t result = _kern_getgroups(groupSize, groupList);
	if (result < 0)
		return result;

	if (groupSize > 0) {
		if (user_memcpy(userGroupList, groupList, sizeof(gid_t) * result)
				!= B_OK) {
			return B_BAD_ADDRESS;
		}
	}

	return result;
}


status_t
_user_setregid(gid_t rgid, gid_t egid, bool setAllIfPrivileged)
{
	return common_setregid(rgid, egid, setAllIfPrivileged, false);
}


status_t
_user_setreuid(uid_t ruid, uid_t euid, bool setAllIfPrivileged)
{
	return common_setreuid(ruid, euid, setAllIfPrivileged, false);
}
