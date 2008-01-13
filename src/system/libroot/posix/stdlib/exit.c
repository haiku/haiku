/*
 * Copyright 2004, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author(s):
 *		Daniel Reinhold, danielre@users.sf.net
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <syscalls.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <limits.h>
#include <SupportDefs.h>


extern void _IO_cleanup(void);
extern void _thread_do_exit_notification(void);

struct exit_stack_info {
	void		(*exit_stack[ATEXIT_MAX])(void);
	int32		stack_size;
	sem_id		lock;
	vint32		lock_count;
	thread_id	lock_owner;
	size_t		recursion_count;
};


static struct exit_stack_info sExitStackInfo = { {}, 0, -1, 0, -1, 0 };


void
__init_exit_stack_lock()
{
	sExitStackInfo.lock = create_sem(0, "exit stack lock");
	if (sExitStackInfo.lock < 0)
		debugger("failed to create exit stack lock");
}


static void
_exit_stack_lock()
{
	thread_id self = find_thread(NULL);
	if (self != sExitStackInfo.lock_owner) {
		if (atomic_add(&sExitStackInfo.lock_count, 1) > 0) {
			while (acquire_sem(sExitStackInfo.lock) != B_OK)
				;
		}
		sExitStackInfo.lock_owner = self;
	}
	sExitStackInfo.recursion_count++;
}


static void
_exit_stack_unlock()
{
	if (sExitStackInfo.lock_owner != find_thread(NULL))
		debugger("exit stack lock not owned");

	if (sExitStackInfo.recursion_count-- == 1) {
		sExitStackInfo.lock_owner = -1;
		if (atomic_add(&sExitStackInfo.lock_count, -1) == 1)
			release_sem(sExitStackInfo.lock);
	}
}


void
_call_atexit_hooks_for_range(addr_t start, addr_t size)
{
	int32 index;
	int32 insertIndex = -1;

	_exit_stack_lock();
	for (index = sExitStackInfo.stack_size - 1; index >= 0; index--) {
		addr_t function = (addr_t)sExitStackInfo.exit_stack[index];
		if (function >= start && function < start + size) {
			(*sExitStackInfo.exit_stack[index])();
			sExitStackInfo.exit_stack[index] = NULL;
			insertIndex = index;
		}
	}

	if (insertIndex >= 0) {
		for (index = insertIndex + 1;
			index < sExitStackInfo.stack_size;
			index++) {
			if (sExitStackInfo.exit_stack[index] != NULL) {
				sExitStackInfo.exit_stack[insertIndex++]
					= sExitStackInfo.exit_stack[index];
			}
		}
		sExitStackInfo.stack_size = insertIndex;
	}

	_exit_stack_unlock();
}


void
abort()
{
	fprintf(stderr, "Abort\n");

	raise(SIGABRT);
	exit(EXIT_FAILURE);
}


int
atexit(void (*func)(void))
{
	// push the function pointer onto the exit stack
	int result = -1;
	_exit_stack_lock();

	if (sExitStackInfo.stack_size < ATEXIT_MAX) {
		sExitStackInfo.exit_stack[sExitStackInfo.stack_size++] = func;
		result = 0;
	}

	_exit_stack_unlock();
	return result;
}


void
exit(int status)
{
	// BeOS on exit notification for the main thread
	_thread_do_exit_notification();

	// unwind the exit stack, calling the registered functions
	_exit_stack_lock();
	while (--sExitStackInfo.stack_size >= 0)
		(*sExitStackInfo.exit_stack[sExitStackInfo.stack_size])();
	_exit_stack_unlock();

	// close all open files
	_IO_cleanup();

	// exit with status code
	_kern_exit_team(status);
}

