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

//#include <assert.h>

#include "arch-specific.h"

// How many iterations we spin waiting for a lock.
enum { SPIN_LIMIT = 100 };

// The values of a user-level lock.
enum { UNLOCKED = 0, LOCKED = 1 };

extern "C" {

#ifdef NEWOS

#include <stdio.h>
#include <syscalls.h>

sem_id create_sem(int32 count, const char *name);
status_t delete_sem(sem_id id);
status_t acquire_sem(sem_id id);
status_t release_sem(sem_id id);

int hoardGetThreadID (void)
{
  return find_thread(NULL);
}


void hoardLockInit (hoardLockType &lock)
{
 	lock.ben = 0;
	lock.sem = create_sem(1, "a hoard lock");
}


void hoardLock (hoardLockType &lock)
{
  if((atomic_add(&(lock.ben), 1)) >= 1) acquire_sem(lock.sem);
}


void hoardUnlock (hoardLockType &lock)
{
  if((atomic_add(&(lock.ben), -1)) > 1) release_sem(lock.sem);
}

int hoardGetPageSize (void)
{
  return 4096;
}

int hoardGetNumProcessors (void)
{
  // XXX return the real number of procs here
  return 1;
}

static region_id heap_region = -1;
static addr brk;

int
__heap_init()
{
	// XXX do something better here
	if (heap_region < 0) {
		heap_region = create_area("heap", (void **)&brk,
			B_ANY_ADDRESS, 4*1024*1024, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	}
	return 0;
}

class heap_init_hack_t
{
public:
	heap_init_hack_t(void) {
		__heap_init();
	}
} heap_init_hack;

void * hoardSbrk (long size)
{
	void *ret = (void *)brk;
	brk += size;
	return ret;
}

void hoardUnsbrk (void * ptr, long size)
{
  // NOT CURRENTLY IMPLEMENTED!
}

void hoardYield (void)
{
}

#elif defined(WIN32)

unsigned long hoardInterlockedExchange (unsigned long * oldval,
					unsigned long newval)
{
  return InterlockedExchange (reinterpret_cast<long *>(oldval), newval);
}

void hoardCreateThread (hoardThreadType& t,
			void *(*function) (void *),
			void * arg)
{
  t = CreateThread (0, 0, (LPTHREAD_START_ROUTINE) function, (LPVOID) arg, 0, 0);
}

void hoardJoinThread (hoardThreadType& t)
{
  WaitForSingleObject (t, INFINITE);
}

void hoardSetConcurrency (int)
{
}

int hoardGetThreadID (void) {
  // Threads from Windows 2000 are 4 apart,
  // so we shift them over to get an appropriate thread id.
  int tid = GetCurrentThreadId() >> 2;
  return tid;
}

void hoardLockInit (hoardLockType& mutex) {
  InterlockedExchange (&mutex, 0);
}

void hoardLock (hoardLockType& mutex) {
  // A yielding lock (with an initial spin).
  int i;
  while (1) {
    i = 0;
    while (i < SPIN_LIMIT) {

		if (mutex != LOCKED)
	     if (InterlockedExchange (&mutex, LOCKED) == UNLOCKED) {
			// We got the lock.
			return;
      }
      i++;
    }
    // Yield to other threads.
    Sleep (0);
  }
}

void hoardYield (void) {
  Sleep (0);
}

void hoardUnlock (hoardLockType& mutex) {
  InterlockedExchange (&mutex, UNLOCKED);
}

void * hoardSbrk (long size)
{

  void * ptr = HeapAlloc (GetProcessHeap(), 0, size);

  return (void *) ptr;
}


void hoardUnsbrk (void * ptr, long size)
{

	HeapFree (GetProcessHeap(), 0, ptr);

}


int hoardGetPageSize (void)
{
  SYSTEM_INFO infoReturn[1];
  GetSystemInfo (infoReturn);
  return (int) (infoReturn -> dwPageSize);
}


int hoardGetNumProcessors (void)
{
  static int numProcessors = 0;
  if (numProcessors == 0) {
    SYSTEM_INFO infoReturn[1];
    GetSystemInfo (infoReturn);
    numProcessors = (int) (infoReturn -> dwNumberOfProcessors);
  }
  return numProcessors;
}

#elif defined(__BEOS__)

#include <OS.h>
#include <unistd.h>

void hoardCreateThread (hoardThreadType &t,
			void *( *function)( void *),
			void *arg)
{
  t = spawn_thread((int32 (*)(void*))function, "some thread",
		   B_NORMAL_PRIORITY, arg);
  if (t >= B_OK) resume_thread(t);
  else debugger("spawn_thread() failed!");
}


void hoardJoinThread (hoardThreadType &t)
{
  status_t dummy;
  wait_for_thread(t, &dummy);
}


void hoardSetConcurrency (int)
{
}


int hoardGetThreadID (void)
{
  return find_thread(0);
}


void hoardLockInit (hoardLockType &lock)
{
  lock.ben = 0;
  lock.sem = create_sem(0, "a hoard lock");
}


void hoardLock (hoardLockType &lock)
{
  if((atomic_add(&(lock.ben), 1)) >= 1) acquire_sem(lock.sem);
}


void hoardUnlock (hoardLockType &lock)
{
  if((atomic_add(&(lock.ben), -1)) > 1) release_sem(lock.sem);
}

int hoardGetPageSize (void)
{
  return B_PAGE_SIZE;
}

int hoardGetNumProcessors (void)
{
  system_info si;
  status_t result = get_system_info(&si);
  assert (result == B_OK);
  return si.cpu_count;
}


void * hoardSbrk (long size)
{
  return sbrk(size + hoardHeap::ALIGNMENT - 1);
}


void hoardUnsbrk (void * ptr, long size)
{
  // NOT CURRENTLY IMPLEMENTED!
}

void hoardYield (void)
{
}

unsigned long hoardInterlockedExchange (unsigned long * oldval,
					unsigned long newval)
{
  // This *should* be made atomic.  It's never used in the BeOS
  // version, so this is included strictly for completeness.
  unsigned long o = *oldval;
  *oldval = newval;
  return o;
}


#else // UNIX


#if USE_SPROC
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ulocks.h>
#endif


void hoardCreateThread (hoardThreadType& t,
			void *(*function) (void *),
			void * arg)
{
#if USE_SPROC
  typedef void (*sprocFunction) (void *);
  t = sproc ((sprocFunction) function, PR_SADDR, arg);
#else
  pthread_attr_t attr;
  pthread_attr_init (&attr);
#if defined(_AIX)
  // Bound (kernel-level) threads.
  pthread_attr_setscope (&attr, PTHREAD_SCOPE_SYSTEM);
#endif
  pthread_create (&t, &attr, function, arg);
#endif
}

void hoardJoinThread (hoardThreadType& t)
{
#if USE_SPROC
  waitpid (t, 0, 0);
#else
  pthread_join (t, NULL);
#endif
}

#if defined(__linux)
// This extern declaration is required for Linux.
extern "C" void pthread_setconcurrency (int n);
#endif

void hoardSetConcurrency (int n)
{
#if USE_SPROC
  usconfig (CONF_INITUSERS, n);
#elif defined(__SVR4) // Solaris
  thr_setconcurrency (n);
#else
  pthread_setconcurrency (n);
#endif
}


#if defined(__SVR4) // Solaris

// Solaris's two-level threads model gives us an edge here;
// we can hash on the LWP's id. This helps us in two ways:
// (1) there are likely to be far fewer LWP's than threads,
// (2) if there's a one-to-one correspondence between LWP's
//     and the number of processors (usually the case), then
//     the number of heaps used will be the same as the number
//     of processors (the optimal case).
// Here we rely on an undocumented call in libthread.so, which
// turns out to be MUCH cheaper than the documented _lwp_self(). Go figure.

extern "C" unsigned int lwp_self(void);
#endif

int hoardGetThreadID (void) {
#if USE_SPROC
  // This hairiness has the same effect as calling getpid(),
  // but it's MUCH faster since it avoids making a system call
  // and just accesses the sproc-local data directly.
  int pid = (int) PRDA->sys_prda.prda_sys.t_pid;
  return pid;
#else
#if defined(__linux)
  // Consecutive thread id's in Linux are 1024 apart;
  // dividing off the 1024 gives us an appropriate thread id.
  return (int) pthread_self() >> 10; // (>> 10 = / 1024)
#endif
#if defined(__AIX)
  // Consecutive thread id's in AIX are 257 apart;
  // dividing off the 257 gives us an appropriate thread id.
  return (int) pthread_self() / 257;
#endif
#if defined(__SVR4)
  return (int) lwp_self();
#endif
  return (int) pthread_self();
#endif
}


// If we are using either Intel or SPARC,
// we use our own lock implementation
// (spin then yield). This is much cheaper than
// the ordinary mutex, at least on Linux and Solaris.

#if USER_LOCKS && (defined(i386) || defined(sparc) || defined(__sgi) || defined(ppc))

#include <sched.h>

#if defined(__sgi)
#include <mutex.h>
#endif


// Atomically:
//   retval = *oldval;
//   *oldval = newval;
//   return retval;

#if defined(sparc) && !defined(__GNUC__)
extern "C" unsigned long InterlockedExchange (unsigned long * oldval,
					      unsigned long newval);
#else
unsigned long InterlockedExchange (unsigned long * oldval,
						 unsigned long newval)
{
#if defined(sparc)
  asm volatile ("swap [%1],%0"
		:"=r" (newval)
		:"r" (oldval), "0" (newval)
		: "memory");

#endif
#if defined(i386)
  asm volatile ("xchgl %0, %1"
		: "=r" (newval)
		: "m" (*oldval), "0" (newval)
		: "memory");
#endif
#if defined(__sgi)
  newval = test_and_set (oldval, newval);
#endif
#if defined(ppc)
  int ret;
  asm volatile ("sync;"
		"0:    lwarx %0,0,%1 ;"
		"      xor. %0,%3,%0;"
		"      bne 1f;"
		"      stwcx. %2,0,%1;"
		"      bne- 0b;"
		"1:    sync"
	: "=&r"(ret)
	: "r"(oldval), "r"(newval), "r"(*oldval)
	: "cr0", "memory");
#endif
  return newval;
}
#endif

unsigned long hoardInterlockedExchange (unsigned long * oldval,
					unsigned long newval)
{
  return InterlockedExchange (oldval, newval);
}

void hoardLockInit (hoardLockType& mutex) {
  InterlockedExchange (&mutex, UNLOCKED);
}

#include <stdio.h>

void hoardLock (hoardLockType& mutex) {
  // A yielding lock (with an initial spin).
  int i;
  while (1) {
    i = 0;
    while (i < SPIN_LIMIT) {
      if (InterlockedExchange (&mutex, LOCKED) == UNLOCKED) {
	// We got the lock.
	return;
      }
      i++;
    }
    // The lock is still being held by someone else.
    // Give up our quantum.
    sched_yield ();
  }
}

void hoardUnlock (hoardLockType& mutex) {
  InterlockedExchange (&mutex, UNLOCKED);
}

#else

// use non-user-level locks.

#endif // USER_LOCKS

void hoardYield (void)
{
  sched_yield ();
}

#if 1 // !(defined(__sgi))
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#endif


void * hoardSbrk (long size) {
  // Use mmap for everything but SGI.
  // Apparently sproc's and mmap's don't cooperate.
#if 0 // defined(__sgi)
  // Protect calls to sbrk() with a lock.
#if USER_LOCKS
  static hoardLockType sbrkLock = 0;
#else
  static hoardLockType sbrkLock = PTHREAD_MUTEX_INITIALIZER;
#endif

  hoardLock (sbrkLock);
  // The "sizeof(double)" should equal hoardHeap::ALIGNMENT.
  // This is a workaround (compiler error in g++).
  void * moreMemory = sbrk (size + sizeof(double) - 1);
  hoardUnlock (sbrkLock);
  return moreMemory;
#else

#if defined(linux)
#if USER_LOCKS
  static hoardLockType sbrkLock = 0;
#else
  static hoardLockType sbrkLock = PTHREAD_MUTEX_INITIALIZER;
#endif
#endif
  // Use mmap to get memory from the backing store.
  static int fd = ::open ("/dev/zero", O_RDWR);
#if defined(linux)
  hoardLock (sbrkLock);

  // Map starting at this address to maximize the amount of memory
  // available to Hoard. Thanks to Jim Nance for this tip.
  char * ptr = (char *) mmap ((void *) 0x10000000, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE, fd, 0);
#else
  char * ptr = (char *) mmap (NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE, fd, 0);
#endif

  char * retPtr;
  if (ptr == (char *) MAP_FAILED) {
    retPtr = NULL;
  } else {
    retPtr = ptr;
  }
#if defined(linux)
  hoardUnlock (sbrkLock);
#endif
  return retPtr;
#endif
}


void hoardUnsbrk (void * ptr, long size)
{
#if defined(linux)
#if USER_LOCKS
  static hoardLockType sbrkLock = 0;
#else
  static hoardLockType sbrkLock = PTHREAD_MUTEX_INITIALIZER;
#endif
#endif
#if defined(linux)
  hoardLock (sbrkLock);
#endif
  int result = munmap ((char *) ptr, size);
  assert (result == 0);
#if defined(linux)
  hoardUnlock (sbrkLock);
#endif
}


int hoardGetPageSize (void)
{
  return (int) sysconf(_SC_PAGESIZE);
}


#if defined(linux)
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#endif

#if defined(__sgi)
#include <sys/types.h>
#include <sys/sysmp.h>
#include <sys/sysinfo.h>
#endif

int hoardGetNumProcessors (void)
{
#if !(defined(linux))
#if defined(__sgi)
  static int np = (int) sysmp(MP_NAPROCS);
  return np;
#else
  static int np = (int) sysconf(_SC_NPROCESSORS_ONLN);
  return np;
#endif
#else
  static int numProcessors = 0;

  if (numProcessors == 0) {
    // Ugly workaround.  Linux's sysconf indirectly calls malloc() (at
    // least on multiprocessors).  So we just read the info from the
    // proc file ourselves and count the occurrences of the word
    // "processor".

    // We only parse the first 32K of the CPU file.  By my estimates,
    // that should be more than enough for at least 64 processors.
    enum { MAX_PROCFILE_SIZE = 32768 };
    char line[MAX_PROCFILE_SIZE];
    int fd = open ("/proc/cpuinfo", O_RDONLY);
    assert (fd);
    read(fd, line, MAX_PROCFILE_SIZE);
    char * str = line;
    while (str) {
      str = strstr(str, "processor");
      if (str) {
	numProcessors++;
	str++;
      }
    }
    close (fd);
    assert (numProcessors > 0);
  }
  return numProcessors;
#endif
}

#endif // UNIX

}
