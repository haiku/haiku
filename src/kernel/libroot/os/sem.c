/* 
** Copyright 2002, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <OS.h>
#include "syscalls.h"


sem_id
create_sem(int32 count, const char *name)
{
	return sys_create_sem(count, name);
}


status_t
delete_sem(sem_id id)
{
	return sys_delete_sem(id);
}


status_t
acquire_sem(sem_id id)
{
	return sys_acquire_sem(id);
}


status_t
acquire_sem_etc(sem_id id, int32 count, uint32 flags, bigtime_t timeout)
{
	return sys_acquire_sem_etc(id, count, flags, timeout);
}


status_t
release_sem(sem_id id)
{
	return sys_release_sem(id);
}


status_t
release_sem_etc(sem_id id, int32 count, uint32 flags)
{
	return sys_release_sem_etc(id, count, flags);
}


status_t
get_sem_count(sem_id sem, int32 *count)
{
	return sys_get_sem_count(sem, count);
}


status_t
set_sem_owner(sem_id sem, team_id team)
{
	return sys_set_sem_owner(sem, team);
}


status_t
_get_sem_info(sem_id sem, sem_info *info, size_t size)
{
	return sys_get_sem_info(sem, info, size);
}


status_t
_get_next_sem_info(team_id team, int32 *cookie, sem_info *info, size_t size)
{
	return sys_get_next_sem_info(team, cookie, info, size);
}

