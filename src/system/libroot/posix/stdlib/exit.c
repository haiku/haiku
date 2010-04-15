/*
 * Copyright 2004-2009, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author(s):
 *		Daniel Reinhold, danielre@users.sf.net
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <SupportDefs.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <libroot_private.h>
#include <locks.h>
#include <runtime_loader.h>
#include <syscalls.h>


extern void _IO_cleanup(void);
extern void _thread_do_exit_work(void);

struct exit_stack_info {
	void			(*exit_stack[ATEXIT_MAX])(void);
	int32			stack_size;
	recursive_lock	lock;
};


static struct exit_stack_info sExitStackInfo
	= { {}, 0, RECURSIVE_LOCK_INITIALIZER("exit stack lock") };


static void inline
_exit_stack_lock()
{
	recursive_lock_lock(&sExitStackInfo.lock);
}


static void inline
_exit_stack_unlock()
{
	recursive_lock_unlock(&sExitStackInfo.lock);
}


// #pragma mark - private API


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


// #pragma mark - public API


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
	_thread_do_exit_work();

	// unwind the exit stack, calling the registered functions
	_exit_stack_lock();
	while (--sExitStackInfo.stack_size >= 0)
		(*sExitStackInfo.exit_stack[sExitStackInfo.stack_size])();
	_exit_stack_unlock();

	// close all open files
	_IO_cleanup();

	__gRuntimeLoader->call_termination_hooks();

	// exit with status code
	_kern_exit_team(status);
}

