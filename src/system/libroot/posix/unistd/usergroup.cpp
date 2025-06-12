/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de. All rights reserved.
 * Copyright 2002-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <syscalls.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>

#include <errno_private.h>


template<typename T>
static inline T
set_errno_if_necessary(const T& result)
{
	if (result < 0) {
		__set_errno(result);
		return -1;
	}

	return result;
}


//	#pragma mark -


gid_t
getegid(void)
{
	gid_t egid = 0;
	_kern_getresgid(NULL, &egid, NULL);
	return egid;
}


uid_t
geteuid(void)
{
	uid_t euid = 0;
	_kern_getresuid(NULL, &euid, NULL);
	return euid;
}


gid_t
getgid(void)
{
	gid_t rgid = 0;
	_kern_getresgid(&rgid, NULL, NULL);
	return rgid;
}


uid_t
getuid(void)
{
	uid_t ruid = 0;
	_kern_getresuid(&ruid, NULL, NULL);
	return ruid;
}


int
getresgid(gid_t *rgid, gid_t *egid, gid_t *sgid)
{
	return set_errno_if_necessary(_kern_getresgid(rgid, egid, sgid));
}


int
getresuid(uid_t *ruid, uid_t *euid, uid_t *suid)
{
	return set_errno_if_necessary(_kern_getresuid(ruid, euid, suid));
}


int
setgid(gid_t gid)
{
	return set_errno_if_necessary(_kern_setresgid(gid, (gid_t)-1, (gid_t)-1, true));
}


int
setuid(uid_t uid)
{
	return set_errno_if_necessary(_kern_setresuid(uid, (uid_t)-1, (uid_t)-1, true));
}


int
setegid(gid_t gid)
{
	return set_errno_if_necessary(_kern_setresgid((gid_t)-1, gid, (gid_t)-1, false));
}


int
seteuid(uid_t uid)
{
	return set_errno_if_necessary(_kern_setresuid((uid_t)-1, uid, (uid_t)-1, false));
}


int
setregid(gid_t rgid, gid_t egid)
{
	return set_errno_if_necessary(_kern_setresgid(rgid, egid, (gid_t)-1, false));
}


int
setreuid(uid_t ruid, uid_t euid)
{
	return set_errno_if_necessary(_kern_setresuid(ruid, euid, (uid_t)-1, false));
}


int
setresgid(gid_t rgid, gid_t egid, gid_t sgid)
{
	return set_errno_if_necessary(_kern_setresgid(rgid, egid, sgid, false));
}


int
setresuid(uid_t ruid, uid_t euid, uid_t suid)
{
	return set_errno_if_necessary(_kern_setresuid(ruid, euid, suid, false));
}


int
getgroups(int groupCount, gid_t groupList[])
{
	return set_errno_if_necessary(_kern_getgroups(groupCount, groupList));
}


int
setgroups(int groupCount, const gid_t* groupList)
{
	return set_errno_if_necessary(_kern_setgroups(groupCount, groupList));
}

// NOTE: getgrouplist() and initgroups() are implemented in grp.cpp.
