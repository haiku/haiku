/*
 * Copyright 2002-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>
#include <syscalls.h>


sem_id
create_sem(int32 count, const char *name)
{
	return _kern_create_sem(count, name);
}


status_t
delete_sem(sem_id id)
{
	return _kern_delete_sem(id);
}


status_t
acquire_sem(sem_id id)
{
	return _kern_acquire_sem(id);
}


status_t
acquire_sem_etc(sem_id id, int32 count, uint32 flags, bigtime_t timeout)
{
	return _kern_acquire_sem_etc(id, count, flags, timeout);
}


// ToDo: the next two calls (switch_sem()) are not yet public API; no decision
//	has been made yet, so they may get changed or removed until R1

status_t
switch_sem(sem_id releaseSem, sem_id id)
{
	return _kern_switch_sem(releaseSem, id);
}


status_t
switch_sem_etc(sem_id releaseSem, sem_id id, int32 count, uint32 flags, bigtime_t timeout)
{
	return _kern_switch_sem_etc(releaseSem, id, count, flags, timeout);
}


status_t
release_sem(sem_id id)
{
	return _kern_release_sem(id);
}


status_t
release_sem_etc(sem_id id, int32 count, uint32 flags)
{
	return _kern_release_sem_etc(id, count, flags);
}


status_t
get_sem_count(sem_id sem, int32 *count)
{
	return _kern_get_sem_count(sem, count);
}


status_t
set_sem_owner(sem_id sem, team_id team)
{
	return _kern_set_sem_owner(sem, team);
}


status_t
_get_sem_info(sem_id sem, sem_info *info, size_t size)
{
	return _kern_get_sem_info(sem, info, size);
}


status_t
_get_next_sem_info(team_id team, int32 *cookie, sem_info *info, size_t size)
{
	return _kern_get_next_sem_info(team, cookie, info, size);
}

