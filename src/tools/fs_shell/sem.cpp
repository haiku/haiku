/*
 * Copyright 2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */

#include "compatibility.h"

#include <string.h>

#include <OS.h>

#include "fssh_errors.h"
#include "fssh_os.h"


static void
copy_sem_info(fssh_sem_info* info, const sem_info* systemInfo)
{
	info->sem = systemInfo->sem;
	info->team = systemInfo->team;
	strcpy(info->name, systemInfo->name);
	info->count = systemInfo->count;
	info->latest_holder = systemInfo->latest_holder;
}


// #pragma mark -


fssh_sem_id
fssh_create_sem(int32_t count, const char *name)
{
	return create_sem(count, name);
}


fssh_status_t
fssh_delete_sem(fssh_sem_id id)
{
	return delete_sem(id);
}


fssh_status_t
fssh_acquire_sem(fssh_sem_id id)
{
	return acquire_sem(id);
}


fssh_status_t
fssh_acquire_sem_etc(fssh_sem_id id, int32_t count, uint32_t flags,
	fssh_bigtime_t timeout)
{
	return acquire_sem_etc(id, count, flags, timeout);
}


fssh_status_t
fssh_release_sem(fssh_sem_id id)
{
	return release_sem(id);
}


fssh_status_t
fssh_release_sem_etc(fssh_sem_id id, int32_t count, uint32_t flags)
{
	return release_sem_etc(id, count, flags);
}


fssh_status_t
fssh_get_sem_count(fssh_sem_id id, int32_t *threadCount)
{
	return get_sem_count(id, (int32*)threadCount);
}


fssh_status_t
fssh_set_sem_owner(fssh_sem_id id, fssh_team_id team)
{
	// return set_sem_owner(id, team);
	// The FS shell is the kernel and no other teams exist.
	return FSSH_B_OK;
}


fssh_status_t
_fssh_get_sem_info(fssh_sem_id id, struct fssh_sem_info *info,
	fssh_size_t infoSize)
{
	sem_info systemInfo;
	status_t result = get_sem_info(id, &systemInfo);
	if (result != B_OK)
		return result;

	copy_sem_info(info, &systemInfo);

	return FSSH_B_OK;
}


fssh_status_t
_fssh_get_next_sem_info(fssh_team_id team, int32_t *cookie,
	struct fssh_sem_info *info, fssh_size_t infoSize)
{
	sem_info systemInfo;
	status_t result = get_next_sem_info(team, (int32*)cookie, &systemInfo);
	if (result != B_OK)
		return result;

	copy_sem_info(info, &systemInfo);

	return FSSH_B_OK;
}

