/*
 * Copyright 2008-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_THREAD_DEFS_H
#define _SYSTEM_THREAD_DEFS_H


#include <pthread.h>

#include <OS.h>


/** Size of the stack given to teams in user space */
#define USER_STACK_GUARD_SIZE		(4 * B_PAGE_SIZE)		// 16 kB
#define MIN_USER_STACK_SIZE			(2 * B_PAGE_SIZE)		// 8 kB
#define MAX_USER_STACK_SIZE			(4096 * B_PAGE_SIZE)	// 16 MB
#define USER_MAIN_THREAD_STACK_SIZE	MAX_USER_STACK_SIZE
#define USER_STACK_SIZE				(64 * B_PAGE_SIZE)		// 256 kB


// The type of object a thread blocks on (thread::wait::type, set by
// thread_prepare_to_block()).
enum {
	THREAD_BLOCK_TYPE_SEMAPHORE				= 0,
	THREAD_BLOCK_TYPE_CONDITION_VARIABLE	= 1,
	THREAD_BLOCK_TYPE_SNOOZE				= 2,
	THREAD_BLOCK_TYPE_SIGNAL				= 3,
	THREAD_BLOCK_TYPE_MUTEX					= 4,
	THREAD_BLOCK_TYPE_RW_LOCK				= 5,
	THREAD_BLOCK_TYPE_USER					= 6,

	THREAD_BLOCK_TYPE_OTHER					= 9999,
};


#define THREAD_CREATION_FLAG_DEFER_SIGNALS	0x01
	// create the thread with signals deferred, i.e. with
	// user_thread::defer_signals set to 1


struct thread_creation_attributes {
	int32		(*entry)(void*, void*);
	const char*	name;
	int32		priority;
	void*		args1;
	void*		args2;
	void*		stack_address;
	size_t		stack_size;
	size_t		guard_size;
	pthread_t	pthread;
	uint32		flags;
};

#endif	/* _SYSTEM_THREAD_DEFS_H */
