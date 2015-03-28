/* libc-internal interface for mutex locks.  NPTL version.
   Copyright (C) 1996-2015 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation; either version 2.1 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If
   not, see <http://www.gnu.org/licenses/>.  */

#ifndef _BITS_LIBC_LOCK_H
#define _BITS_LIBC_LOCK_H 1

#include <pthread.h>
#define __need_NULL
#include <stddef.h>

#define __libc_maybe_call(func, args, else) func args
#define __pthread_mutex_init pthread_mutex_init
#define __pthread_mutex_destroy pthread_mutex_destroy
#define __pthread_mutexattr_init pthread_mutexattr_init
#define __pthread_mutexattr_destroy pthread_mutexattr_destroy
#define __pthread_mutexattr_settype pthread_mutexattr_settype
#define __pthread_mutex_lock pthread_mutex_lock
#define __pthread_mutex_unlock pthread_mutex_unlock
#define PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP	PTHREAD_RECURSIVE_MUTEX_INITIALIZER
#define PTHREAD_MUTEX_RECURSIVE_NP	PTHREAD_MUTEX_RECURSIVE

/* Mutex type.  */
typedef struct { pthread_mutex_t mutex; } __libc_lock_recursive_t;

/* Define a lock variable NAME with storage class CLASS.  The lock must be
   initialized with __libc_lock_init before it can be used (or define it
   with __libc_lock_define_initialized, below).  Use `extern' for CLASS to
   declare a lock defined in another module.  In public structure
   definitions you must use a pointer to the lock structure (i.e., NAME
   begins with a `*'), because its storage size will not be known outside
   of libc.  */
#define __libc_lock_define_recursive(CLASS,NAME) \
  CLASS __libc_lock_recursive_t NAME;

/* Define an initialized recursive lock variable NAME with storage
   class CLASS.  */
# define __libc_lock_define_initialized_recursive(CLASS,NAME) \
  CLASS __libc_lock_recursive_t NAME = _LIBC_LOCK_RECURSIVE_INITIALIZER;
# define _LIBC_LOCK_RECURSIVE_INITIALIZER \
  {PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP}

/* Initialize a recursive mutex.  */
# define __libc_lock_init_recursive(NAME) \
  do {									      \
    pthread_mutexattr_t __attr;					      \
	__pthread_mutexattr_init (&__attr);				      \
	__pthread_mutexattr_settype (&__attr, PTHREAD_MUTEX_RECURSIVE_NP);    \
	__pthread_mutex_init (&(NAME).mutex, &__attr);			      \
	__pthread_mutexattr_destroy (&__attr);				      \
  } while (0)

/* Finalize recursive named lock.  */
# define __libc_lock_fini_recursive(NAME) \
  __libc_maybe_call (__pthread_mutex_destroy, (&(NAME).mutex), 0)

/* Lock the recursive named lock variable.  */
# define __libc_lock_lock_recursive(NAME) \
  __libc_maybe_call (__pthread_mutex_lock, (&(NAME).mutex), 0)

/* Try to lock the recursive named lock variable.  */
# define __libc_lock_trylock_recursive(NAME) \
  __libc_maybe_call (__pthread_mutex_trylock, (&(NAME).mutex), 0)

/* Unlock the recursive named lock variable.  */
# define __libc_lock_unlock_recursive(NAME) \
  __libc_maybe_call (__pthread_mutex_unlock, (&(NAME).mutex), 0)

/* Start critical region with cleanup.  */
#define __libc_cleanup_region_start(DOIT, FCT, ARG) \

/* End critical region with cleanup.  */
#define __libc_cleanup_region_end(DOIT) \

/* Sometimes we have to exit the block in the middle.  */
#define __libc_cleanup_end(DOIT) \

/* Create thread-specific key.  */
#define __libc_key_create(KEY, DESTRUCTOR) \
  1

/* Get thread-specific data.  */
#define __libc_getspecific(KEY) \
  0

/* Set thread-specific data.  */
#define __libc_setspecific(KEY, VALUE) \
  0


/* Register handlers to execute before and after `fork'.  */
#define __libc_atfork(PREPARE, PARENT, CHILD) \
  0

#endif	/* bits/libc-lock.h */
