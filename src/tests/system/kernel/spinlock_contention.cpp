/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <SupportDefs.h>

#include <syscalls.h>
#include <spinlock_contention.h>


#define panic printf


struct dummy_spinlock {
	vint32	lock;
	vint32	count_low;
	vint32	count_high;
};


struct dummy_smp_msg {
	dummy_smp_msg*	next;
};


static int				sNumCPUs = 2;
static bool				sICIEnabled = true;
static dummy_spinlock	cpu_msg_spinlock[SMP_MAX_CPUS];
static dummy_smp_msg*	smp_msgs[SMP_MAX_CPUS];
static dummy_spinlock	broadcast_msg_spinlock;
static dummy_smp_msg*	smp_broadcast_msgs;


bool
dummy_are_interrupts_enabled()
{
	return false;
}


void
dummy_acquire_spinlock_nocheck(dummy_spinlock* lock)
{
	if (sNumCPUs > 1) {
		if (dummy_are_interrupts_enabled())
			panic("acquire_spinlock_nocheck: attempt to acquire lock %p with interrupts enabled\n", lock);

		while (atomic_add(&lock->lock, 1) != 0) {
		}
	} else {
		if (dummy_are_interrupts_enabled())
			panic("acquire_spinlock_nocheck: attempt to acquire lock %p with interrupts enabled\n", lock);
		if (atomic_set((int32 *)lock, 1) != 0)
			panic("acquire_spinlock_nocheck: attempt to acquire lock %p twice on non-SMP system\n", lock);
	}
}


void
dummy_release_spinlock(dummy_spinlock* lock)
{
	if (sNumCPUs > 1) {
		if (dummy_are_interrupts_enabled())
			panic("release_spinlock: attempt to release lock %p with interrupts enabled\n", lock);

		{
			int32 count = atomic_set(&lock->lock, 0) - 1;
			if (count < 0) {
				panic("release_spinlock: lock %p was already released\n", lock);
			} else {
				// add to the total count -- deal with carry manually
				if ((uint32)atomic_add(&lock->count_low, count) + count
						< (uint32)count) {
					atomic_add(&lock->count_high, 1);
				}
			}
		}
	} else {
		if (dummy_are_interrupts_enabled())
			panic("release_spinlock: attempt to release lock %p with interrupts enabled\n", lock);
		if (atomic_set((int32 *)lock, 0) != 1)
			panic("release_spinlock: lock %p was already released\n", lock);
	}
}


struct dummy_smp_msg *
dummy_check_for_message(int currentCPU, int *source_mailbox)
{
	struct dummy_smp_msg *msg;

	if (!sICIEnabled)
		return NULL;

	dummy_acquire_spinlock_nocheck(&cpu_msg_spinlock[currentCPU]);
	msg = smp_msgs[currentCPU];
	if (msg != NULL) {
		printf("yeah\n");
		dummy_release_spinlock(&cpu_msg_spinlock[currentCPU]);
		*source_mailbox = 1;
	} else {
		// try getting one from the broadcast mailbox

		dummy_release_spinlock(&cpu_msg_spinlock[currentCPU]);
		dummy_acquire_spinlock_nocheck(&broadcast_msg_spinlock);

		msg = smp_broadcast_msgs;
		while (msg != NULL) {
			printf("yeah\n");
			msg = msg->next;
		}
		dummy_release_spinlock(&broadcast_msg_spinlock);
	}
	return msg;
}


int32
dummy_process_pending_ici(int32 currentCPU)
{
	int retval = 42;
	int sourceMailbox = 0;

	dummy_smp_msg* msg = dummy_check_for_message(currentCPU, &sourceMailbox);
	if (msg == NULL)
		return retval;

	switch ((addr_t)msg) {
		case 0:
			printf("foo\n");
			break;
		case 1:
			printf("foo\n");
			break;
		case 2:
			printf("foo\n");
			break;
	}

	return 9;
}


static void
test_spinlock(dummy_spinlock* lock)
{
	while (atomic_add(&lock->lock, -1) != 0)
		dummy_process_pending_ici(0);
}


static double
estimate_spinlock_tick_time()
{
	// time the spinlock
	int32 count = (INT_MAX >> 16) + 1;
	while (true) {
		bigtime_t startTime = system_time();

		dummy_spinlock lock;
		lock.lock = count;
		test_spinlock(&lock);

		bigtime_t totalTime = system_time() - startTime;
		double tickTime = (double)totalTime / count;

		if (totalTime > 1000000 || INT_MAX >> 2 < count)
			return tickTime;

		count <<= 1;
	}
}


static const char*
time_string(double timeInUsecs, char* buffer)
{
	static const char* const kUnits[] = { "us", "ms", "s ", NULL };

	double time = timeInUsecs;

	int32 i = 0;
	while (time > 1000 && kUnits[i + 1] != NULL) {
		time /= 1000;
		i++;
	}

	sprintf(buffer, "%.3f %s", time, kUnits[i]);

	return buffer;
}


int
main(int argc, char** argv)
{
	// get the initial contention info
	spinlock_contention_info startInfo;
	status_t error = _kern_generic_syscall(SPINLOCK_CONTENTION,
		GET_SPINLOCK_CONTENTION_INFO, &startInfo, sizeof(startInfo));
	if (error != B_OK) {
		fprintf(stderr, "Error: Failed to get spinlock contention info: %s\n",
			strerror(error));
		exit(1);
	}
	bigtime_t startTime = system_time();

	pid_t child = fork();
	if (child < 0) {
		fprintf(stderr, "Error: fork() failed: %s\n", strerror(errno));
		exit(1);
	}

	if (child == 0) {
		execvp(argv[1], argv + 1);
		fprintf(stderr, "Error: exec() failed: %s\n", strerror(errno));
		exit(1);
	} else {
		int status;
		wait(&status);
	}

	// get the final contention info
	spinlock_contention_info endInfo;
	error = _kern_generic_syscall(SPINLOCK_CONTENTION,
		GET_SPINLOCK_CONTENTION_INFO, &endInfo, sizeof(endInfo));
	if (error != B_OK) {
		fprintf(stderr, "Error: Failed to get spinlock contention info: %s\n",
			strerror(error));
		exit(1);
	}
	bigtime_t totalTime = system_time() - startTime;

	char buffer[128];
	printf("\ntotal run time: %s\n", time_string(totalTime, buffer));

	// estimate spinlock tick time
	printf("estimating time per spinlock tick...\n");
	double tickTime = estimate_spinlock_tick_time();
	printf("time per spinlock tick: %s\n",
		time_string(tickTime, buffer));

	// print results
	static const char* const kLockNames[] = { "thread", "team", NULL };
	uint64 lockCounts[] = {
		endInfo.thread_spinlock_counter - startInfo.thread_spinlock_counter,
		endInfo.team_spinlock_counter - startInfo.team_spinlock_counter
	};

	printf("\nlock             counter            time   wasted %% CPU\n");
	printf("-------------------------------------------------------\n");
	for (int32 i = 0; kLockNames[i] != NULL; i++) {
		double wastedUsecs = lockCounts[i] * tickTime;
		printf("%-10s  %12llu  %14s   %12.4f\n", kLockNames[i], lockCounts[i],
			time_string(wastedUsecs, buffer), wastedUsecs / totalTime * 100);
	}

	return 0;
}
