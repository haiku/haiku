/*
 * Copyright 2008-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <usergroup.h>

#include <errno.h>
#include <limits.h>
#include <sys/stat.h>

#include <new>

#include <heap.h>
#include <kernel.h>
#include <syscalls.h>
#include <team.h>
#include <thread.h>
#include <thread_types.h>
#include <util/AutoLock.h>
#include <util/ThreadAutoLock.h>
#include <vfs.h>

#include <AutoDeleter.h>


// #pragma mark - Implementation Private


static bool
is_privileged(Team* team)
{
	// currently only the root user is privileged
	return team->effective_uid == 0;
}


static status_t
common_setresgid(gid_t rgid, gid_t egid, gid_t ssgid, bool setAllIfPrivileged, bool kernel)
{
	Team* team = thread_get_current_thread()->team;

	TeamLocker teamLocker(team);

	bool privileged = kernel || is_privileged(team);

	if (ssgid == (gid_t)-1 || !privileged)
		ssgid = team->saved_set_gid;

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
			// Note: We allow setting the real gid to the effective gid. This
			// is unspecified by the specs, but is common practice.
			if (!privileged && rgid != team->real_gid
				&& rgid != team->effective_gid) {
				return EPERM;
			}

			// Note: Also common practice is to set the saved set-gid when the
			// real gid is set.
			if (rgid != team->real_gid)
				ssgid = rgid;
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
	team->saved_set_gid = ssgid;

	return B_OK;
}


static status_t
common_setresuid(uid_t ruid, uid_t euid, uid_t ssuid, bool setAllIfPrivileged, bool kernel)
{
	Team* team = thread_get_current_thread()->team;

	TeamLocker teamLocker(team);

	bool privileged = kernel || is_privileged(team);

	if (ssuid == (uid_t)-1)
		ssuid = team->saved_set_uid;
	else {
		if (!privileged && ssuid != team->effective_uid
			&& ssuid != team->real_uid && ssuid != team->saved_set_uid) {
			return EPERM;
		}
	}

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

			// Note: Also common practice is to set the saved set-uid when the
			// real uid is set.
			if (ruid != team->real_uid)
				ssuid = ruid;
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
	team->saved_set_uid = ssuid;

	return B_OK;
}


static ssize_t
common_getgroups(int groupCount, gid_t* groupList, bool kernel)
{
	Team* team = thread_get_current_thread()->team;

	TeamLocker teamLocker(team);

	const gid_t* groups = NULL;
	int actualCount = 0;

	if (team->supplementary_groups != NULL) {
		groups = team->supplementary_groups->groups;
		actualCount = team->supplementary_groups->count;
	}

	// follow the specification and return always at least one group
	if (actualCount == 0) {
		groups = &team->effective_gid;
		actualCount = 1;
	}

	// if groupCount 0 is supplied, we only return the number of groups
	if (groupCount == 0)
		return actualCount;

	// check for sufficient space
	if (groupCount < actualCount)
		return B_BAD_VALUE;

	// copy
	if (kernel) {
		memcpy(groupList, groups, actualCount * sizeof(gid_t));
	} else {
		if (!IS_USER_ADDRESS(groupList)
			|| user_memcpy(groupList, groups,
					actualCount * sizeof(gid_t)) != B_OK) {
			return B_BAD_ADDRESS;
		}
	}

	return actualCount;
}


static status_t
common_setgroups(int groupCount, const gid_t* groupList, bool kernel)
{
	if (groupCount < 0 || groupCount > NGROUPS_MAX)
		return B_BAD_VALUE;

	BKernel::GroupsArray* newGroups = NULL;
	if (groupCount > 0) {
		newGroups = (BKernel::GroupsArray*)malloc(sizeof(BKernel::GroupsArray)
			+ (sizeof(gid_t) * groupCount));
		if (newGroups == NULL)
			return B_NO_MEMORY;
		new(newGroups) BKernel::GroupsArray;

		if (kernel) {
			memcpy(newGroups->groups, groupList, sizeof(gid_t) * groupCount);
		} else {
			if (!IS_USER_ADDRESS(groupList)
				|| user_memcpy(newGroups->groups, groupList, sizeof(gid_t) * groupCount) != B_OK) {
				free(newGroups);
				return B_BAD_ADDRESS;
			}
		}
		newGroups->count = groupCount;
	}

	Team* team = thread_get_current_thread()->team;
	TeamLocker teamLocker(team);

	BReference<BKernel::GroupsArray> previous = team->supplementary_groups;
		// so it will not be (potentially) destroyed until after we unlock
	team->supplementary_groups.SetTo(newGroups, true);

	teamLocker.Unlock();

	return B_OK;
}


// #pragma mark - Kernel Private


/*!	Copies the user and group information from \a parent to \a team.
	The caller must hold the lock to both \a team and \a parent.
*/
void
inherit_parent_user_and_group(Team* team, Team* parent)
{
	team->saved_set_uid = parent->saved_set_uid;
	team->real_uid = parent->real_uid;
	team->effective_uid = parent->effective_uid;
	team->saved_set_gid = parent->saved_set_gid;
	team->real_gid = parent->real_gid;
	team->effective_gid = parent->effective_gid;
	team->supplementary_groups = parent->supplementary_groups;
}


status_t
update_set_id_user_and_group(Team* team, const char* file)
{
	struct stat st;
	status_t status = vfs_read_stat(-1, file, true, &st, false);
	if (status != B_OK)
		return status;

	TeamLocker teamLocker(team);

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


bool
is_in_group(Team* team, gid_t gid)
{
	TeamLocker teamLocker(team);

	if (team->effective_gid == gid)
		return true;

	if (team->supplementary_groups == NULL)
		return false;

	for (int i = 0; i < team->supplementary_groups->count; i++) {
		if (gid == team->supplementary_groups->groups[i])
			return true;
	}

	return false;
}


status_t
_kern_setresgid(gid_t rgid, gid_t egid, gid_t ssgid, bool setAllIfPrivileged)
{
	return common_setresgid(rgid, egid, ssgid, setAllIfPrivileged, true);
}


status_t
_kern_setresuid(uid_t ruid, uid_t euid, uid_t ssuid, bool setAllIfPrivileged)
{
	return common_setresuid(ruid, euid, ssuid, setAllIfPrivileged, true);
}


status_t
_kern_getresgid(gid_t *rgid, gid_t *egid, gid_t *ssgid)
{
	Team* team = thread_get_current_thread()->team;
	if (rgid != NULL)
		*rgid = team->real_gid;
	if (egid != NULL)
		*egid = team->effective_gid;
	if (ssgid != NULL)
		*ssgid = team->saved_set_gid;
	return B_OK;
}


status_t
_kern_getresuid(uid_t *ruid, uid_t *euid, gid_t *ssuid)
{
	Team* team = thread_get_current_thread()->team;
	if (ruid != NULL)
		*ruid = team->real_uid;
	if (euid != NULL)
		*euid = team->effective_uid;
	if (ssuid != NULL)
		*ssuid = team->saved_set_uid;
	return B_OK;
}


ssize_t
_kern_getgroups(int groupCount, gid_t* groupList)
{
	return common_getgroups(groupCount, groupList, true);
}


status_t
_kern_setgroups(int groupCount, const gid_t* groupList)
{
	return common_setgroups(groupCount, groupList, true);
}


// #pragma mark - Syscalls


status_t
_user_setresgid(gid_t rgid, gid_t egid, gid_t ssgid, bool setAllIfPrivileged)
{
	return common_setresgid(rgid, egid, ssgid, setAllIfPrivileged, false);
}


status_t
_user_setresuid(uid_t ruid, uid_t euid, uid_t ssuid, bool setAllIfPrivileged)
{
	return common_setresuid(ruid, euid, ssuid, setAllIfPrivileged, false);
}


status_t
_user_getresgid(gid_t *rgid, gid_t *egid, gid_t *ssgid)
{
	Team* team = thread_get_current_thread()->team;
	if (rgid != NULL) {
		if (!IS_USER_ADDRESS(rgid)
			|| user_memcpy(rgid, &team->real_gid, sizeof(uid_t)) != B_OK) {
			return B_BAD_ADDRESS;
		}
	}
	if (egid != NULL) {
		if (!IS_USER_ADDRESS(egid)
			|| user_memcpy(egid, &team->effective_gid, sizeof(uid_t)) != B_OK) {
			return B_BAD_ADDRESS;
		}
	}
	if (ssgid != NULL) {
		if (!IS_USER_ADDRESS(ssgid)
			|| user_memcpy(ssgid, &team->saved_set_gid, sizeof(uid_t)) != B_OK) {
			return B_BAD_ADDRESS;
		}
	}
	return B_OK;
}


status_t
_user_getresuid(uid_t *ruid, uid_t *euid, gid_t *ssuid)
{
	Team* team = thread_get_current_thread()->team;
	if (ruid != NULL) {
		if (!IS_USER_ADDRESS(ruid)
			|| user_memcpy(ruid, &team->real_uid, sizeof(uid_t)) != B_OK) {
			return B_BAD_ADDRESS;
		}
	}
	if (euid != NULL) {
		if (!IS_USER_ADDRESS(euid)
			|| user_memcpy(euid, &team->effective_uid, sizeof(uid_t)) != B_OK) {
			return B_BAD_ADDRESS;
		}
	}
	if (ssuid != NULL) {
		if (!IS_USER_ADDRESS(ssuid)
			|| user_memcpy(ssuid, &team->saved_set_uid, sizeof(uid_t)) != B_OK) {
			return B_BAD_ADDRESS;
		}
	}
	return B_OK;
}


ssize_t
_user_getgroups(int groupCount, gid_t* groupList)
{
	return common_getgroups(groupCount, groupList, false);
}


ssize_t
_user_setgroups(int groupCount, const gid_t* groupList)
{
	// check privilege
	Team* team = thread_get_current_thread()->team;
	if (!is_privileged(team))
		return EPERM;

	return common_setgroups(groupCount, groupList, false);
}
