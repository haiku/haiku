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


// kernel private functions

void		inherit_parent_user_and_group(Team* team, Team* parent);
status_t	update_set_id_user_and_group(Team* team, const char* file);
bool		is_in_group(Team* team, gid_t gid);


#ifdef __cplusplus
extern "C" {
#endif

// syscalls

status_t	_user_getresgid(gid_t *rgid, gid_t *egid, gid_t *sgid);
status_t	_user_getresuid(uid_t *ruid, uid_t *euid, uid_t *suid);
status_t	_user_setresgid(gid_t rgid, gid_t egid, gid_t sgid, bool setAllIfPrivileged);
status_t	_user_setresuid(uid_t ruid, uid_t euid, uid_t suid, bool setAllIfPrivileged);
ssize_t		_user_getgroups(int groupCount, gid_t* groupList);
ssize_t		_user_setgroups(int groupCount, const gid_t* groupList);

#ifdef __cplusplus
}	// extern "C"
#endif


#endif	// _KERNEL_USERGROUP_H
