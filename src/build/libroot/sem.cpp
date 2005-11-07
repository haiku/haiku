
#include <BeOSBuildCompatibility.h>

#include <string.h>

#include <OS.h>
#include <SupportDefs.h>

// We assume that everything is single-threaded, so we don't need real
// semaphores. Simple fakes are sufficient.

struct semaphore {
	char	name[B_OS_NAME_LENGTH];
	int32	count;
	bool	inUse;
};

static const int kSemaphoreCount = 1024;
static semaphore sSemaphores[kSemaphoreCount];


static bool
check_sem(sem_id id)
{
	if (id < 0 || id >= kSemaphoreCount)
		return false;
	return sSemaphores[id].inUse;
}

// create_sem
sem_id
create_sem(int32 count, const char *name)
{
	for (int i = 0; i < kSemaphoreCount; i++) {
		semaphore &sem = sSemaphores[i];
		if (!sem.inUse) {
			sem.inUse = true;
			sem.count = count;
			strlcpy(sem.name, name, sizeof(sem.name));
			return i;
		}
	}
	
	return B_NO_MORE_SEMS;
}

// delete_sem
status_t
delete_sem(sem_id id)
{
	if (!check_sem(id))
		return B_BAD_SEM_ID;

	sSemaphores[id].inUse = false;
	
	return B_OK;
}

// acquire_sem
status_t
acquire_sem(sem_id id)
{
	return acquire_sem_etc(id, 1, 0, 0);
}

// acquire_sem_etc
status_t
acquire_sem_etc(sem_id id, int32 count, uint32 flags, bigtime_t timeout)
{
	if (!check_sem(id))
		return B_BAD_SEM_ID;

	if (count <= 0)
		return B_BAD_VALUE;

	semaphore &sem = sSemaphores[id];
	if (sem.count >= count) {
		sem.count -= count;
		return B_OK;
	}

	if (timeout < 0)
		timeout = 0;

	bool noTimeout = false;		
	if (flags & B_RELATIVE_TIMEOUT) {
		// relative timeout: get the absolute time when to time out
		
		// special case: on timeout == 0 we return B_WOULD_BLOCK
		if (timeout == 0)
			return B_WOULD_BLOCK;
		
		bigtime_t currentTime = system_time();
		if (timeout > B_INFINITE_TIMEOUT || currentTime >= B_INFINITE_TIMEOUT - timeout) {
			noTimeout = true;
		} else {
			timeout += currentTime;
		}
	
	} else if (flags & B_ABSOLUTE_TIMEOUT) {
		// absolute timeout
	} else {
		// no timeout given
		noTimeout = true;
	}

	// no timeout?
	if (noTimeout) {
		debugger("Would block on a semaphore without timeout in a "
			"single-threaded context!");
		return B_ERROR;
	}

	// wait for the time out time
	status_t error = snooze_until(timeout, B_SYSTEM_TIMEBASE);
	if (error != B_OK)
		return error;
						
	return B_TIMED_OUT;
}

// release_sem
status_t
release_sem(sem_id id)
{
	return release_sem_etc(id, 1, 0);
}

// release_sem_etc
status_t
release_sem_etc(sem_id id, int32 count, uint32 flags)
{
	if (!check_sem(id))
		return B_BAD_SEM_ID;

	if (count <= 0)
		return B_BAD_VALUE;

	semaphore &sem = sSemaphores[id];
	sem.count += count;

	return B_OK;
}

// atomic_add
int32
atomic_add(vint32 *value, int32 addValue)
{
	int32 oldValue = *value;
	value += addValue;
	return oldValue;
}
