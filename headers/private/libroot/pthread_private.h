/*
 * Copyright 2003-2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2007, Ryan Leavengood, leavengood@gmail.com.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _PTHREAD_PRIVATE_H_
#define _PTHREAD_PRIVATE_H_


#include <pthread.h>

#include <OS.h>


// _pthread_thread::flags values
#define THREAD_DETACHED				0x01
#define THREAD_DEAD					0x02
#define THREAD_CANCELED				0x04
#define THREAD_CANCEL_ENABLED		0x08
#define THREAD_CANCEL_ASYNCHRONOUS	0x10


struct thread_creation_attributes;

// The public *_t types are only pointers to these structures
// This way, we are completely free to change them, which might be
// necessary in the future (not only due to the incomplete implementation
// at this point).

typedef struct _pthread_condattr {
	bool		process_shared;
} pthread_condattr;

typedef struct _pthread_mutexattr {
	int32		type;
	bool		process_shared;
} pthread_mutexattr;

typedef struct _pthread_attr {
	int32		detach_state;
	int32		sched_priority;
	size_t		stack_size;
} pthread_attr;

typedef struct _pthread_rwlockattr {
	uint32_t	flags;
} pthread_rwlockattr;

typedef void (*pthread_key_destructor)(void *data);

struct pthread_key {
	vint32		sequence;
	pthread_key_destructor destructor;
};

struct pthread_key_data {
	vint32		sequence;
	void		*value;
};

#define PTHREAD_UNUSED_SEQUENCE	0

typedef struct _pthread_thread {
	thread_id	id;
	int32		flags;
	void		*(*entry)(void*);
	void		*entry_argument;
	void		*exit_value;
	struct pthread_key_data specific[PTHREAD_KEYS_MAX];
	struct __pthread_cleanup_handler *cleanup_handlers;
} pthread_thread;


#ifdef __cplusplus
extern "C" {
#endif

void __pthread_key_call_destructors(pthread_thread *thread);
void __pthread_destroy_thread(void);
pthread_thread *__allocate_pthread(void* (*entry)(void*), void *data);
void __init_pthread(pthread_thread* thread, void* (*entry)(void*), void* data);
status_t __pthread_init_creation_attributes(
	const pthread_attr_t* pthreadAttributes, pthread_t thread,
	status_t (*entryFunction)(void*, void*), void* argument1,
	void* argument2, const char* name,
	struct thread_creation_attributes* attributes);

#ifdef __cplusplus
}
#endif

#endif	/* _PTHREAD_PRIVATE_H_ */
