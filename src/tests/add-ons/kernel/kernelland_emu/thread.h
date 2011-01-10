/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef THREAD_H
#define THREAD_H

#include <OS.h>


namespace BKernel {
	struct Thread;
}

using BKernel::Thread;


static inline Thread*
get_current_thread()
{
	return (Thread*)(addr_t)find_thread(NULL);
}


static inline thread_id
get_thread_id(Thread* thread)
{
	return (thread_id)(addr_t)thread;
}


#endif	// THREAD_H
