/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_USERGROUP_H
#define _KERNEL_USERGROUP_H


#include <unistd.h>

#include <SupportDefs.h>


namespace BKernel {
	struct Team;
}

using BKernel::Team;


#ifdef __cplusplus
extern "C" {
#endif

// kernel private functions

void		inherit_parent_user_and_group(Team* team, Team* parent);
status_t	update_set_id_user_and_group(Team* team, const char* file);

// syscalls

gid_t		_user_getgid(bool effective);
uid_t		_user_getuid(bool effective);
status_t	_user_setregid(gid_t rgid, gid_t egid, bool setAllIfPrivileged);
status_t	_user_setreuid(uid_t ruid, uid_t euid, bool setAllIfPrivileged);
ssize_t		_user_getgroups(int groupCount, gid_t* groupList);
ssize_t		_user_setgroups(int groupCount, const gid_t* groupList);

#ifdef __cplusplus
}	// extern "C"
#endif


#endif	// _KERNEL_USERGROUP_H
