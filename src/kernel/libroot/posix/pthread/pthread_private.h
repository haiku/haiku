#ifndef _PTHREAD_PRIVATE_H_
#define _PTHREAD_PRIVATE_H_
/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

#include <OS.h>

// The public *_t types are only pointers to these structures
// This way, we are completely free to change them, which might be
// necessary in the future (not only due to the incomplete implementation
// at this point).

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

#endif	/* _PTHREAD_PRIVATE_H_ */
