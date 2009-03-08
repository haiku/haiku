/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef THREAD_H
#define THREAD_H

#include <OS.h>


struct thread;


static inline struct thread*
get_current_thread()
{
	return (struct thread*)find_thread(NULL);
}


static inline thread_id
get_thread_id(struct thread* thread)
{
	return (thread_id)thread;
}


#endif	// THREAD_H
