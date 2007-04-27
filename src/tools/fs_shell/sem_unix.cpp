#include "fssh_os.h"

#include <new>

#include "fssh_errors.h"
#include "fssh_kernel_export.h"
#include "fssh_string.h"


static const int sSemaphoreCount = 1024;
static fssh_sem_info* sSemaphores[sSemaphoreCount];


fssh_sem_id
fssh_create_sem(int32_t count, const char *name)
{
	// find a free slot
	for (int i = 0; i < sSemaphoreCount; i++) {
		if (!sSemaphores[i]) {
			sSemaphores[i] = new fssh_sem_info;
			sSemaphores[i]->sem = i;
			sSemaphores[i]->team = 1;
			fssh_strcpy(sSemaphores[i]->name, name);
			sSemaphores[i]->count = count;
			sSemaphores[i]->latest_holder = -1;
			return i;
		}
	}

	return FSSH_B_NO_MORE_SEMS;
}


fssh_status_t
fssh_delete_sem(fssh_sem_id id)
{
	if (id < 0 || id >= sSemaphoreCount || !sSemaphores[id])
		return FSSH_B_BAD_SEM_ID;

	delete sSemaphores[id];
	sSemaphores[id] = NULL;
	return 0;
}


fssh_status_t
fssh_acquire_sem(fssh_sem_id id)
{
	return fssh_acquire_sem_etc(id, 1, 0, 0);
}


fssh_status_t
fssh_acquire_sem_etc(fssh_sem_id id, int32_t count, uint32_t flags,
	fssh_bigtime_t timeout)
{
	if (id < 0 || id >= sSemaphoreCount || !sSemaphores[id])
		return FSSH_B_BAD_SEM_ID;
	if (count < 0)
		return FSSH_B_BAD_VALUE;

	if (sSemaphores[id]->count >= count) {
		sSemaphores[id]->count -= count;
		return FSSH_B_OK;
	}

	// can't acquire the sem with that count at the moment
	if (!flags & (FSSH_B_RELATIVE_TIMEOUT | FSSH_B_ABSOLUTE_TIMEOUT)) {
		fssh_panic("blocking on semaphore %d without timeout!\n", (int)id);
		return FSSH_B_BAD_VALUE;
	}

	// simulate timeout
	if (flags & FSSH_B_RELATIVE_TIMEOUT) {
		if (timeout == 0)
			return FSSH_B_WOULD_BLOCK;
		fssh_snooze(timeout);
	} else
		fssh_snooze_until(timeout, FSSH_B_SYSTEM_TIMEBASE);

	return FSSH_B_TIMED_OUT;
}


fssh_status_t
fssh_release_sem(fssh_sem_id id)
{
	return fssh_release_sem_etc(id, 1, 0);
}


fssh_status_t
fssh_release_sem_etc(fssh_sem_id id, int32_t count, uint32_t flags)
{
	if (id < 0 || id >= sSemaphoreCount || !sSemaphores[id])
		return FSSH_B_BAD_SEM_ID;
	if (count < 0)
		return FSSH_B_BAD_VALUE;

	sSemaphores[id]->count += count;
	return FSSH_B_OK;
}


fssh_status_t
fssh_get_sem_count(fssh_sem_id id, int32_t *threadCount)
{
	if (id < 0 || id >= sSemaphoreCount || !sSemaphores[id])
		return FSSH_B_BAD_SEM_ID;
	if (!threadCount)
		return FSSH_B_BAD_VALUE;

	*threadCount = sSemaphores[id]->count;
	return FSSH_B_OK;
}


fssh_status_t
fssh_set_sem_owner(fssh_sem_id id, fssh_team_id team)
{
	if (id < 0 || id >= sSemaphoreCount || !sSemaphores[id])
		return FSSH_B_BAD_SEM_ID;
	if (team != 1)
		return FSSH_B_BAD_VALUE;

	sSemaphores[id]->team = team;
	return FSSH_B_OK;
}


fssh_status_t
_fssh_get_sem_info(fssh_sem_id id, struct fssh_sem_info *info,
	fssh_size_t infoSize)
{
	if (id < 0 || id >= sSemaphoreCount || !sSemaphores[id])
		return FSSH_B_BAD_SEM_ID;
	if (!info)
		return FSSH_B_BAD_VALUE;

	fssh_memcpy(info, sSemaphores[id], sizeof(fssh_sem_info));

	return FSSH_B_OK;
}


fssh_status_t
_fssh_get_next_sem_info(fssh_team_id team, int32_t *cookie,
	struct fssh_sem_info *info, fssh_size_t infoSize)
{
	if (team != 0 || team != 1)
		return FSSH_B_BAD_TEAM_ID;

	for (int i = *cookie; i < sSemaphoreCount; i++) {
		if (sSemaphores[i]) {
			*cookie = i + 1;
			return _fssh_get_sem_info(i, info, infoSize);
		}
	}

	return FSSH_B_ENTRY_NOT_FOUND;
}

