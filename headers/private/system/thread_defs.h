/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_THREAD_DEFS_H
#define _SYSTEM_THREAD_DEFS_H

#include <OS.h>


#define THREAD_RETURN_EXIT			0x1
#define THREAD_RETURN_INTERRUPTED	0x2
#define THREAD_STOPPED				0x3
#define THREAD_CONTINUED			0x4

/** Size of the stack given to teams in user space */
#define USER_STACK_GUARD_PAGES		4								// 16 kB
#define USER_MAIN_THREAD_STACK_SIZE	(16 * 1024 * 1024 \
						- USER_STACK_GUARD_PAGES * B_PAGE_SIZE)		// 16 MB
#define USER_STACK_SIZE				(256 * 1024	\
						- USER_STACK_GUARD_PAGES * B_PAGE_SIZE)		// 256 kB
#define MIN_USER_STACK_SIZE			(4 * 1024)						// 4 KB
#define MAX_USER_STACK_SIZE			(16 * 1024 * 1024 \
						- USER_STACK_GUARD_PAGES * B_PAGE_SIZE)		// 16 MB


// The type of object a thread blocks on (thread::wait::type, set by
// thread_prepare_to_block()).
enum {
	THREAD_BLOCK_TYPE_SEMAPHORE				= 0,
	THREAD_BLOCK_TYPE_CONDITION_VARIABLE	= 1,
	THREAD_BLOCK_TYPE_SNOOZE				= 2,
	THREAD_BLOCK_TYPE_SIGNAL				= 3,
	THREAD_BLOCK_TYPE_MUTEX					= 4,
	THREAD_BLOCK_TYPE_RW_LOCK				= 5,

	THREAD_BLOCK_TYPE_OTHER					= 9999,
	THREAD_BLOCK_TYPE_USER_BASE				= 10000
};


struct thread_creation_attributes {
	int32 (*entry)(thread_func, void *);
	const char*	name;
	int32		priority;
	void*		args1;
	void*		args2;
	void*		stack_address;
	size_t		stack_size;

	// when calling from kernel only
	team_id		team;
	thread_id	thread;
};

#endif	/* _SYSTEM_THREAD_DEFS_H */
