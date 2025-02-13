/*
 * Copyright 2024, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <OS.h>
#include <malloc.h>
#include <pthread.h>
#include <sys/param.h>

#include <errno_private.h>
#include <libroot_private.h>
#include <shared/locks.h>
#include <system/tls.h>

#include "PagesAllocator.h"


/* generic stuff */
#if B_PAGE_SIZE == 4096
#define _MAX_PAGE_SHIFT 12
#endif

extern char* __progname;

#define MAP_CONCEAL			(0)
#define __MAP_NOREPLACE		(0)

#define DEF_STRONG(X)
#define	DEF_WEAK(x)


/* entropy routines, wrapped for malloc */

static uint32_t
malloc_arc4random()
{
	// TODO: Improve this?
	const uintptr_t address = (uintptr_t)&address;
	uint32_t random = (uint32_t)address * 1103515245;
	random += (uint32_t)system_time();
	return random;
}
#define arc4random malloc_arc4random


static void
malloc_arc4random_buf(void *_buf, size_t nbytes)
{
	uint8* buf = (uint8*)_buf;
	while (nbytes > 0) {
		uint32_t value = malloc_arc4random();
		const size_t copy = (nbytes > 4) ? 4 : nbytes;
		memcpy(buf, &value, copy);
		nbytes -= copy;
		buf += copy;
	}
}
#define arc4random_buf malloc_arc4random_buf


static uint32_t
malloc_arc4random_uniform(uint32_t upper_bound)
{
	return (malloc_arc4random() % upper_bound);
}
#define arc4random_uniform malloc_arc4random_uniform


/* memory routines, wrapped for malloc */

static inline void
malloc_explicit_bzero(void* buf, size_t len)
{
	memset(buf, 0, len);
}
#define explicit_bzero malloc_explicit_bzero


static int
malloc_mprotect(void* address, size_t length, int protection)
{
	/* do nothing */
	return 0;
}
#define mprotect malloc_mprotect


static int
mimmutable(void* address, size_t length)
{
	/* do nothing */
	return 0;
}


/* memory mapping */


static void*
malloc_mmap(void* address, size_t length, int protection, int flags, int fd, off_t offset)
{
	status_t status;
	length = (length + (B_PAGE_SIZE - 1)) & ~(B_PAGE_SIZE - 1);;

	status = __allocate_pages(&address, length, flags);
	if (status != B_OK) {
		__set_errno(status);
		return MAP_FAILED;
	}
	return address;
}
#define mmap malloc_mmap


static int
malloc_munmap(void* address, size_t length)
{
	status_t status = __free_pages(address, length);
	if (status < 0) {
		errno = status;
		return -1;
	}
	return 0;
}
#define munmap malloc_munmap


/* public methods */

void*
memalign(size_t align, size_t len)
{
	void* result = NULL;
	int status;

	if (align < sizeof(void*))
		align = sizeof(void*);

	status = posix_memalign(&result, align, len);
	if (status != 0) {
		errno = status;
		return NULL;
	}
	return result;
}


void*
valloc(size_t size)
{
	return memalign(B_PAGE_SIZE, size);
}


/* malloc implementation */

#define _MALLOC_MUTEXES 32
static mutex sMallocMutexes[_MALLOC_MUTEXES];
static pthread_once_t sThreadedMallocInitOnce = PTHREAD_ONCE_INIT;

static int32 sNextMallocThreadID = 1;

static u_int mopts_nmutexes();
static void _malloc_init(int from_rthreads);


static inline void
_MALLOC_LOCK(int32 index)
{
	mutex_lock(&sMallocMutexes[index]);
}


static inline void
_MALLOC_UNLOCK(int32 index)
{
	mutex_unlock(&sMallocMutexes[index]);
}


static void
init_threaded_malloc()
{
	u_int i;
	for (i = 2; i < _MALLOC_MUTEXES; i++)
		mutex_init(&sMallocMutexes[i], "heap mutex");

	_MALLOC_LOCK(0);
	_malloc_init(1);
	_MALLOC_UNLOCK(0);
}


status_t
__init_heap()
{
	tls_set(TLS_MALLOC_SLOT, (void*)0);
	__init_pages_allocator();
	mutex_init(&sMallocMutexes[0], "heap mutex");
	mutex_init(&sMallocMutexes[1], "heap mutex");
	_malloc_init(0);
	return B_OK;
}


static int32
get_thread_malloc_id()
{
	int32 result = (int32)(intptr_t)tls_get(TLS_MALLOC_SLOT);
	if (result == -1) {
		// thread has never called malloc() before; assign it an ID.
		result = atomic_add(&sNextMallocThreadID, 1);
		tls_set(TLS_MALLOC_SLOT, (void*)(intptr_t)result);
	}
	return result;
}


void
__heap_thread_init()
{
	pthread_once(&sThreadedMallocInitOnce, &init_threaded_malloc);
	tls_set(TLS_MALLOC_SLOT, (void*)(intptr_t)-1);
}


void
__heap_thread_exit()
{
	const int32 id = (int32)(intptr_t)tls_get(TLS_MALLOC_SLOT);
	if (id != -1 && id == (sNextMallocThreadID - 1)) {
		// Try to "de-allocate" this thread's ID.
		atomic_test_and_set(&sNextMallocThreadID, id, id + 1);
	}
}


void
__heap_before_fork()
{
	u_int i;
	u_int nmutexes = mopts_nmutexes();
	for (i = 0; i < nmutexes; i++)
		_MALLOC_LOCK(i);

	__pages_allocator_before_fork();
}


void
__heap_after_fork_child()
{
	u_int i;
	u_int nmutexes = mopts_nmutexes();
	for (i = 0; i < nmutexes; i++)
		mutex_init(&sMallocMutexes[i], "heap mutex");

	__pages_allocator_after_fork(0);
}


void
__heap_after_fork_parent()
{
	u_int i;
	u_int nmutexes = mopts_nmutexes();
	for (i = 0; i < nmutexes; i++)
		_MALLOC_UNLOCK(i);

	__pages_allocator_after_fork(1);
}


void
__heap_terminate_after()
{
}
