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
#define USER_MAIN_THREAD_STACK_SIZE	(16 * 1024 * 1024)	// 16 MB
#define USER_STACK_SIZE				(256 * 1024)		// 256 kB
#define MIN_USER_STACK_SIZE			(4 * 1024)			// 4 KB
#define MAX_USER_STACK_SIZE			(16 * 1024 * 1024)	// 16 MB
#define USER_STACK_GUARD_PAGES		4					// 16 kB


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
