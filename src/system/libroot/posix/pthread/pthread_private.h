/*
 * Copyright 2003-2008, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2007, Ryan Leavengood, leavengood@gmail.com.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _PTHREAD_PRIVATE_H_
#define _PTHREAD_PRIVATE_H_


#include <pthread.h>

#include <OS.h>

// The public *_t types are only pointers to these structures
// This way, we are completely free to change them, which might be
// necessary in the future (not only due to the incomplete implementation
// at this point).

typedef struct _pthread_condattr {
	bool		process_shared;
} pthread_condattr;

typedef struct _pthread_cond {
	sem_id			sem;
	pthread_mutex_t	*mutex;
	int32			waiter_count;
	int32			event_counter;
	pthread_condattr attr;
} pthread_cond;

typedef struct _pthread_mutexattr {
	int32		type;
	bool		process_shared;
} pthread_mutexattr;

typedef struct _pthread_mutex {
	vint32		count;
	sem_id		sem;
	thread_id	owner;
	int32		owner_count;
	pthread_mutexattr attr;
} pthread_mutex;

typedef struct _pthread_attr {
	int32		detach_state;
	int32		sched_priority;
	size_t		stack_size;
} pthread_attr;

typedef void (*pthread_key_destructor)(void *data);

struct pthread_key {
	vint32		sequence;
	pthread_key_destructor destructor;
};

struct pthread_key_data {
	vint32		sequence;
	void		*value;
};

#define PTHREAD_KEYS_MAX		256
#define PTHREAD_UNUSED_SEQUENCE	0

// This structure is used internally only, it has no public equivalent
struct pthread_thread {
	void		*(*entry)(void*);
	void		*entry_argument;
	int			cancel_state;
	int			cancel_type;
	bool		cancelled;
	struct pthread_key_data specific[PTHREAD_KEYS_MAX];
	struct __pthread_cleanup_handler *cleanup_handlers;
	// TODO: move pthread keys in here, too
};


#ifdef __cplusplus
extern "C" {
#endif

void __pthread_key_call_destructors(struct pthread_thread *thread);
struct pthread_thread *__get_pthread(void);

#ifdef __cplusplus
}
#endif

#endif	/* _PTHREAD_PRIVATE_H_ */
