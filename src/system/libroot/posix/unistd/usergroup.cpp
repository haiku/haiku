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
	return _kern_getgid(true);
}


uid_t
geteuid(void)
{
	return _kern_getuid(true);
}


gid_t
getgid(void)
{
	return _kern_getgid(false);
}


uid_t
getuid(void)
{
	return _kern_getuid(false);
}


int
setgid(gid_t gid)
{
	return set_errno_if_necessary(_kern_setregid(gid, (gid_t)-1, true));
}


int
setuid(uid_t uid)
{
	return set_errno_if_necessary(_kern_setreuid(uid, (uid_t)-1, true));
}


int
setegid(gid_t gid)
{
	return set_errno_if_necessary(_kern_setregid((gid_t)-1, gid, false));
}


int
seteuid(uid_t uid)
{
	return set_errno_if_necessary(_kern_setreuid((uid_t)-1, uid, false));
}


int
setregid(gid_t rgid, gid_t egid)
{
	return set_errno_if_necessary(_kern_setregid(rgid, egid, false));
}


int
setreuid(uid_t ruid, uid_t euid)
{
	return set_errno_if_necessary(_kern_setreuid(ruid, euid, false));
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
