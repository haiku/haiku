///-*-C++-*-//////////////////////////////////////////////////////////////////
//
// Hoard: A Fast, Scalable, and Memory-Efficient Allocator
//        for Shared-Memory Multiprocessors
// Contact author: Emery Berger, http://www.cs.utexas.edu/users/emery
//
// Copyright (c) 1998-2000, The University of Texas at Austin.
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Library General Public License as
// published by the Free Software Foundation, http://www.fsf.org.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _ARCH_SPECIFIC_H_
#define _ARCH_SPECIFIC_H_

#include "config.h"

#if defined(NEWOS)

#include <syscalls.h>
#include <assert.h>

#else

// Wrap architecture-specific functions.

#if defined(WIN32) || defined(__WIN32__) || defined(_WIN32)

// Windows

#ifndef WIN32
#define WIN32 1
#endif
#include <windows.h>
#include <process.h>
typedef LONG	hoardLockType;
typedef HANDLE 	hoardThreadType;

#elif defined(__BEOS__)

// BeOS

#include <OS.h>


#elif USE_SPROC

// SGI's SPROC library

#include <sys/types.h>
#include <sys/prctl.h>

#else

// Generic UNIX

#if defined(__SVR4) // Solaris
#include <thread.h>
#endif

#include <pthread.h>
#include <unistd.h>

#endif
#endif

#if defined(NEWOS)

typedef struct {
  int32 ben;
  sem_id sem;
} hoardLockType;
typedef thread_id		hoardThreadType;
inline void * operator new(size_t, void *_P)
	{return (_P); }

#else

#ifndef WIN32
#if defined(__BEOS__)
typedef struct {
  int32 ben;
  sem_id sem;
} hoardLockType;
#elif USER_LOCKS && (defined(i386) || defined(sparc) || defined(__sgi))
typedef unsigned long	hoardLockType;
#else
typedef pthread_mutex_t	hoardLockType;
#endif

#if USE_SPROC
typedef pid_t			hoardThreadType;
#elif defined(__BEOS__)
typedef thread_id               hoardThreadType;
#else
typedef pthread_t		hoardThreadType;
#endif

#endif
#endif

extern "C" {

  ///// Thread-related wrappers.

  void	hoardCreateThread (hoardThreadType& t,
			   void *(*function) (void *),
			   void * arg);
  void	hoardJoinThread (hoardThreadType& t);
  void  hoardSetConcurrency (int n);

  // Return a thread identifier appropriate for hashing:
  // if the system doesn't produce consecutive thread id's,
  // some hackery may be necessary.
  int	hoardGetThreadID (void);

  ///// Lock-related wrappers.

#if !defined(WIN32) && !defined(__BEOS__) && !USER_LOCKS

  // Define the lock operations inline to save a little overhead.

  inline void hoardLockInit (hoardLockType& lock) {
    pthread_mutex_init (&lock, NULL);
  }

  inline void hoardLock (hoardLockType& lock) {
    pthread_mutex_lock (&lock);
  }

  inline void hoardUnlock (hoardLockType& lock) {
    pthread_mutex_unlock (&lock);
  }
#else
  void	hoardLockInit (hoardLockType& lock);
  void	hoardLock (hoardLockType& lock);
  void	hoardUnlock (hoardLockType& lock);

#endif

  ///// Memory-related wrapper.

  int	hoardGetPageSize (void);
  void *	hoardSbrk (long size);
  void	hoardUnsbrk (void * ptr, long size);

  ///// Other.

  void  hoardYield (void);
  int	hoardGetNumProcessors (void);
  unsigned long hoardInterlockedExchange (unsigned long * oldval,
					  unsigned long newval);
}

#endif // _ARCH_SPECIFIC_H_

