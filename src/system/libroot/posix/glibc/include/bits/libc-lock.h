/* libc-internal interface for mutex locks.  BeOS version.
   Copyright (C)  1998 Be Inc.
*/

#ifndef _BITS_LIBC_LOCK_H
#define _BITS_LIBC_LOCK_H 1

#include <Errors.h>
#include <OS.h>
#include <SupportDefs.h>


/* Helper definitions and prototypes.  */

/* Atomic operations.  */

extern char	_single_threaded;

static inline int
__compare_and_swap (volatile int32 *p, int oldval, int newval)
{
	int32 readval = atomic_test_and_set(p, newval, oldval);
	return (readval == oldval ? 1 : 0);
}


/* Mutex type.  */
typedef struct __libc_lock_t {
	volatile int32	count;
	sem_id			sem;
	thread_id		owner;
	int				owner_count;
} __libc_lock_t;

#define __LIBC_LOCK_INITIALIZER { 0, 0, 0, 0 }
#define _LIBC_LOCK_RECURSIVE_INITIALIZER { 0, 0, 0, 0 }

/* Type for object to ensure once-only execution.  */
typedef struct {
	int __initialized;
	__libc_lock_t __lock;
} __libc_once_t;

/* Define a lock variable NAME with storage class CLASS.  The lock must be
   initialized with __libc_lock_init before it can be used (or define it
   with __libc_lock_define_initialized, below).  Use `extern' for CLASS to
   declare a lock defined in another module.  In public structure
   definitions you must use a pointer to the lock structure (i.e., NAME
   begins with a `*'), because its storage size will not be known outside
   of libc.  */
#define __libc_lock_define(CLASS,NAME) \
	CLASS __libc_lock_t NAME;

#define __libc_lock_define_recursive(CLASS, NAME) \
	CLASS __libc_lock_t NAME;

/* Define an initialized lock variable NAME with storage class CLASS.  */
#define __libc_lock_define_initialized(CLASS,NAME) \
	CLASS __libc_lock_t NAME = __LIBC_LOCK_INITIALIZER;

/* Define an initialized recursive lock variable NAME with storage
   class CLASS.  */
#define __libc_lock_define_initialized_recursive(CLASS,NAME) \
  __libc_lock_define_initialized (CLASS, NAME)

/* Initialize the named lock variable, leaving it in a consistent, unlocked
   state.  */
#define __libc_lock_init(NAME) \
    (NAME).count = (NAME).sem = (NAME).owner_count = (NAME).owner = 0  \

/* Same as last but this time we initialize a recursive mutex.  */
#define __libc_lock_init_recursive(NAME) \
  __libc_lock_init(NAME)

/* Finalize the named lock variable, which must be locked.  It cannot be
   used again until __libc_lock_init is called again on it.  This must be
   called on a lock variable before the containing storage is reused.  */
#define __libc_lock_fini(NAME) \
  do { \
    if ((NAME).sem) \
      delete_sem((NAME).sem); \
  } while(0)

/* Finalize recursive named lock.  */
#define __libc_lock_fini_recursive(NAME) __libc_lock_fini(NAME)

/* Lock the named lock variable.  */
#define __libc_lock_lock(NAME) \
  do { \
	if (!_single_threaded) { \
    	long err; \
    	long old = atomic_add(&(NAME).count, 1); \
    	if (old > 0) { \
			if ((NAME).sem == 0) { \
				sem_id __new_sem = create_sem (0, "libc:" #NAME);	      \
				if (!__compare_and_swap ((volatile int32 *)&(NAME).sem, 0, __new_sem))		      \
				/* We do not need the semaphore.  */				      \
					delete_sem (__new_sem);					      \
			} \
    	    do { \
    	      err = acquire_sem((NAME).sem); \
		    } while (err == B_INTERRUPTED); \
    	} \
	} \
  } while (0)


/* Lock the recursive named lock variable.  */
#define __libc_lock_lock_recursive(NAME) \
  do { \
	if (!_single_threaded) { \
    	thread_id owner = find_thread(NULL); \
    	long old, err = B_OK; \
    	if (owner == (NAME).owner) { \
    	  (NAME).owner_count++; \
    	  break; \
    	} \
    	old = atomic_add(&(NAME).count, 1); \
    	if (old > 0) { \
    	    if ((NAME).sem == 0) { \
				sem_id __new_sem = create_sem (0, "libc:" #NAME);	      \
				if (!__compare_and_swap ((volatile int32 *)&(NAME).sem, 0, __new_sem))		      \
				/* We do not need the semaphore.  */				      \
					delete_sem (__new_sem);					      \
			} \
    	    do { \
    	      err = acquire_sem((NAME).sem); \
		    } while (err == B_INTERRUPTED); \
		} \
    	if (err == B_OK) { \
    	  (NAME).owner = owner; \
    	  (NAME).owner_count = 1; \
		} \
	} \
  } while (0)

#if 0
/* Try to lock the named lock variable.  */
#define __libc_lock_trylock(NAME) \
	({									      \
		int __result = EBUSY;						      \
		status_t err; \
		if (!(NAME).sem) {							      \
			__libc_sem_id __new_sem = create_sem (1, "libc:" #NAME);	      \
			if (!__compare_and_swap (&(NAME).sem, 0, __new_sem))		      \
			/* We do not need the semaphore.  */				      \
				delete_sem (__new_sem);					      \
		}									      \
		do { \
    		err = acquire_sem_etc ((NAME).sem, 1, B_TIMEOUT, 0); \
			if (err == B_OK) { \
				NAME.__count = 1;						      \
				__result = 0;							      \
			}									      \
		} while (err == B_INTERRUPTED); \
	__result; })
#endif

#if 0
/* Try to lock the recursive named lock variable.  */
#define __libc_lock_trylock_recursive(NAME) \
	({									      \
		__libc_thread_id __me = find_thread (NULL);				      \
		int __result = EBUSY;						      \
		status_t err;							 \
		if (!(NAME).sem) {							      \
			__libc_sem_id __new_sem = create_sem (1, "libc:" #NAME);	      \
			if (!__compare_and_swap (&(NAME).sem, 0, __new_sem))		      \
			/* We do not need the semaphore.  */				      \
				delete_sem (__new_sem);					      \
		}									      \
		if ((NAME).__owner == __me) {\
			++(NAME).__count;							      \
			__result = 0;							      \
		} else \
			do { \
				err = acquire_sem_etc ((NAME).sem, 1, B_TIMEOUT, 0); \
				if (err == B_OK) {						  \
					(NAME).__owner = __me;					      \
					(NAME).__count = 1;						      \
					__result = 0;							      \
				}								      \
			} while (err == B_INTERRUPTED); \
    __result; })
#endif

/* Unlock the named lock variable.  */
#define __libc_lock_unlock(NAME) \
  do { \
	if (!_single_threaded) { \
    	if (atomic_add(&(NAME).count, -1) > 1) { \
    	    if ((NAME).sem == 0) { \
				sem_id __new_sem = create_sem (0, "libc:" #NAME);	      \
				if (!__compare_and_swap ((volatile int32 *)&(NAME).sem, 0, __new_sem))		      \
				/* We do not need the semaphore.  */				      \
					delete_sem (__new_sem);					      \
			} \
			release_sem((NAME).sem); \
		} \
	} \
  } while(0) 

/* Unlock the recursive named lock variable.  */
#define __libc_lock_unlock_recursive(NAME) \
  do { \
	if (!_single_threaded) { \
    	(NAME).owner_count--; \
    	if ((NAME).owner_count == 0) { \
    	  (NAME).owner = 0; \
    	  if (atomic_add(&(NAME).count, -1) > 1) { \
    	    if ((NAME).sem == 0) { \
				sem_id __new_sem = create_sem (0, "libc:" #NAME);	      \
				if (!__compare_and_swap ((volatile int32 *)&(NAME).sem, 0, __new_sem))		      \
				/* We do not need the semaphore.  */				      \
					delete_sem (__new_sem);					      \
			} \
    	    release_sem((NAME).sem); \
		  } \
		} \
	} \
  } while(0) 


/* Define once control variable.  */
#define __libc_once_define(CLASS, NAME) \
  CLASS __libc_once_t NAME = { 0, __LIBC_LOCK_INITIALIZER }

/* Call handler iff the first call.  */
#define __libc_once(ONCE_CONTROL, INIT_FUNCTION) \
  do {									      \
    if (! ONCE_CONTROL.__initialized)					      \
      {									      \
	__libc_lock_lock (ONCE_CONTROL.__lock);				      \
	if (! ONCE_CONTROL.__initialized)				      \
	  {								      \
	    /* Still not initialized, then call the function.  */	      \
	    INIT_FUNCTION ();						      \
	    ONCE_CONTROL.__initialized = 1;				      \
	  }								      \
	__libc_lock_unlock (ONCE_CONTROL.__lock);			      \
      }									      \
  } while (0)


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
