/*
 * Copyright 2023, Jérôme Duval, jerome.duval@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <sched.h>

#include <pthread_private.h>
#include <syscalls.h>


int
pthread_setaffinity_np(pthread_t thread, size_t cpusetsize, const cpuset_t* mask)
{
	return _kern_set_thread_affinity(thread->id, mask, cpusetsize);
}


int
pthread_getaffinity_np(pthread_t thread, size_t cpusetsize, cpuset_t* mask)
{
	status_t status = _kern_get_thread_affinity(thread->id, mask, cpusetsize);
	if (status != B_OK) {
		if (status == B_BAD_THREAD_ID)
			return ESRCH;

		return status;
	}

	return 0;
}


int
sched_setaffinity(pid_t id, size_t cpusetsize, const cpuset_t* mask)
{
	status_t status = _kern_set_thread_affinity(id, mask, cpusetsize);
	if (status != B_OK) {
		if (status == B_BAD_THREAD_ID)
			return ESRCH;

		return status;
	}

	return 0;
}


int
sched_getaffinity(pid_t id, size_t cpusetsize, cpuset_t* mask)
{
	status_t status = _kern_get_thread_affinity(id, mask, cpusetsize);
	if (status != B_OK) {
		if (status == B_BAD_THREAD_ID)
			return ESRCH;

		return status;
	}

	return 0;
}
