/* MultiLocker.cpp */
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "MultiLocker.h"

#include <Debug.h>
#include <Errors.h>
#include <OS.h>

//#define TIMING 1
#define DEBUG 1


MultiLocker::MultiLocker(const char* semaphoreBaseName)
	:	fInit(B_NO_INIT),
		fReadCount(0),
		fReadSem(-1),
		fWriteCount(0),
		fWriteSem(-1),
		fLockCount(0),
		fWriterLock(-1),
		fWriterNest(0),
		fWriterThread(-1),
		fWriterStackBase(0),
		fDebugArray(NULL),
		fMaxThreads(0)
{
	//build the semaphores
	if (semaphoreBaseName) {
		char name[128];
		sprintf(name, "%s-%s", semaphoreBaseName, "ReadSem");
		fReadSem = create_sem(0, name);
		sprintf(name, "%s-%s", semaphoreBaseName, "WriteSem");
		fWriteSem = create_sem(0, name);
		sprintf(name, "%s-%s", semaphoreBaseName, "WriterLock");
		fWriterLock = create_sem(0, name);
	} else {
		fReadSem = create_sem(0, "MultiLocker_ReadSem");
		fWriteSem = create_sem(0, "MultiLocker_WriteSem");
		fWriterLock = create_sem(0, "MultiLocker_WriterLock");
	}

	if (fReadSem >= 0 && fWriteSem >=0 && fWriterLock >= 0)
		fInit = B_OK;
		
	#if DEBUG
		//we are in debug mode!
		//create the reader tracking list
		//the array needs to be large enough to hold all possible threads
		system_info sys;
		get_system_info(&sys);
		fMaxThreads = sys.max_threads;
		fDebugArray = (int32 *) malloc(fMaxThreads * sizeof(int32));
		for (int32 i = 0; i < fMaxThreads; i++) {
			fDebugArray[i] = 0;
		}
		
	#endif
	#if TIMING
		//initialize the counter variables
		rl_count = ru_count = wl_count = wu_count = islock_count = 0;
		rl_time = ru_time = wl_time = wu_time = islock_time = 0;
		#if DEBUG
			reg_count = unreg_count = 0;
			reg_time = unreg_time = 0;
		#endif
	#endif
}


MultiLocker::~MultiLocker()
{
	//become the writer
	if (!IsWriteLocked()) WriteLock();
	
	//set locker to be uninitialized
	fInit = B_NO_INIT;

	//delete the semaphores
	delete_sem(fReadSem);
	delete_sem(fWriteSem);
	delete_sem(fWriterLock);
	
	#if DEBUG
		//we are in debug mode!
		//clear and delete the reader tracking list
		free(fDebugArray);
	#endif
	#if TIMING	
		//let's produce some performance numbers
		printf("MultiLocker Statistics:\n"
				"Avg ReadLock: %lld\n"
				"Avg ReadUnlock: %lld\n"
				"Avg WriteLock: %lld\n"
				"Avg WriteUnlock: %lld\n"
				"Avg IsWriteLocked: %lld\n",
				rl_count > 0 ? rl_time / rl_count : 0,
				ru_count > 0 ? ru_time / ru_count : 0,
				wl_count > 0 ? wl_time / wl_count : 0,
				wu_count > 0 ? wu_time / wu_count : 0,
				islock_count > 0 ? islock_time / islock_count : 0
		);
		#if DEBUG
			printf(	"Avg register_thread: %lld\n"
					"Avg unregister_thread: %lld\n",
					reg_count > 0 ? reg_time / reg_count : 0,
					unreg_count > 0 ? unreg_time / unreg_count : 0
			);
		#endif
	#endif	
}

status_t 
MultiLocker::InitCheck()
{
	return fInit;
}

bool 
MultiLocker::ReadLock()
{
	#if TIMING
		bigtime_t start = system_time();
	#endif

	bool locked = false;	

	//the lock must be initialized
	if (fInit == B_OK) {
		if (IsWriteLocked()) {
			//the writer simply increments the nesting
			fWriterNest++;
			locked = true;
		} else {
			//increment and retrieve the current count of readers
			int32 current_count = atomic_add(&fReadCount, 1);	
			if (current_count < 0) {
				//a writer holds the lock so wait for fReadSem to be released
				locked = (acquire_sem_etc(fReadSem, 1, B_DO_NOT_RESCHEDULE,
										  B_INFINITE_TIMEOUT) == B_OK);
			} else locked = true;			
		#if DEBUG
			//register if we acquired the lock
			if (locked) register_thread();
		#endif
		}
		
	}
	
	#if TIMING
		bigtime_t end = system_time();
		rl_time += (end - start);
		rl_count++;
	#endif

	return locked;	
}

bool 
MultiLocker::WriteLock()
{
	#if TIMING
		bigtime_t start = system_time();
	#endif

	bool locked = false;
	
	if (fInit == B_OK) {
		uint32 stack_base = 0;
		thread_id thread = -1;

		if (IsWriteLocked(&stack_base, &thread)) {
			//already the writer - increment the nesting count
			fWriterNest++;
			locked = true;
		} else {
			//new writer acquiring the lock
			if (atomic_add(&fLockCount, 1) >= 1) {
				//another writer in the lock - acquire the semaphore
				locked = (acquire_sem_etc(fWriterLock, 1, B_DO_NOT_RESCHEDULE,
						B_INFINITE_TIMEOUT) == B_OK);
			} else locked = true;
			
			if (locked) {
				//new holder of the lock

				//decrement fReadCount by a very large number
				//this will cause new readers to block on fReadSem
				int32 readers = atomic_add(&fReadCount, -LARGE_NUMBER);
				
				if (readers > 0) {				
					//readers hold the lock - acquire fWriteSem
					locked = (acquire_sem_etc(fWriteSem, readers, B_DO_NOT_RESCHEDULE,
							B_INFINITE_TIMEOUT) == B_OK);
					}
				if (locked) {
					ASSERT(fWriterThread == -1);
					//record thread information
					fWriterThread = thread;
					fWriterStackBase = stack_base;
				}
			}
		}
	}

	#if TIMING
		bigtime_t end = system_time();
		wl_time += (end - start);
		wl_count++;
	#endif
	
	return locked;
}

bool 
MultiLocker::ReadUnlock()
{
	#if TIMING
		bigtime_t start = system_time();
	#endif

	bool unlocked = false;

	if (IsWriteLocked()) {
		//writers simply decrement the nesting count
		fWriterNest--;
		unlocked = true;	
	} else {
		//decrement and retrieve the read counter
		int32 current_count = atomic_add(&fReadCount, -1);
		if (current_count < 0) {
			//a writer is waiting for the lock so release fWriteSem
			unlocked = (release_sem_etc(fWriteSem, 1,
										B_DO_NOT_RESCHEDULE) == B_OK);
		} else unlocked = true;
	
		#ifdef DEBUG
			//unregister if we released the lock
			if (unlocked) unregister_thread();
		#endif
	}
	
	#if TIMING
		bigtime_t end = system_time();
		ru_time += (end - start);
		ru_count++;
	#endif

	return unlocked;	
}

bool 
MultiLocker::WriteUnlock()
{
	#if TIMING
		bigtime_t start = system_time();
	#endif

	bool unlocked = false;

	if (IsWriteLocked()) {
		//if this is a nested lock simply decrement the nest count
		if (fWriterNest > 0) {
			fWriterNest--;
			unlocked = true;
		} else {
			//writer finally unlocking

			//increment fReadCount by a large number
			//this will let new readers acquire the read lock
			//retrieve the number of current waiters
			int32 readersWaiting = atomic_add(&fReadCount, LARGE_NUMBER) + LARGE_NUMBER;
			
			if (readersWaiting > 0) {
				//readers are waiting to acquire the lock
				unlocked = (release_sem_etc(fReadSem, readersWaiting,
						B_DO_NOT_RESCHEDULE) == B_OK);
			} else unlocked = true;
			
			if (unlocked) {
				//clear the information
				fWriterThread = -1;
				fWriterStackBase = 0;
				
				//decrement and retrieve the lock count
				if (atomic_add(&fLockCount, -1) > 1) {
					//other writers are waiting so release fWriterLock
					unlocked = (release_sem_etc(fWriterLock, 1,
							B_DO_NOT_RESCHEDULE) == B_OK);
				}
			}
		}
		
	} else debugger("Non-writer attempting to WriteUnlock()\n");

	#if TIMING
		bigtime_t end = system_time();
		wu_time += (end - start);
		wu_count++;
	#endif

	return unlocked;
}

/* this function demonstrates a nice method of determining if the current thread */
/* is the writer or not.  The method involves caching the index of the page in memory */
/* where the thread's stack is located.  Each time a new writer acquires the lock, */
/* its thread_id and stack_page are recorded.  IsWriteLocked gets the stack_page of the */
/* current thread and sees if it is a match.  If the stack_page matches you are guaranteed */
/* to have the matching thread.  If the stack page doesn't match the more traditional  */
/* find_thread(NULL) method of matching the thread_ids is used. */

/* This technique is very useful when dealing with a lock that is acquired in a nested fashion. */
/* It could be expanded to cache the information of the last thread in the lock, and then if */
/* the same thread returns while there is no one in the lock, it could save some time, if the */
/* same thread is likely to acquire the lock again and again. */
/* I should note another shortcut that could be implemented here */
/* If fWriterThread is set to -1 then there is no writer in the lock, and we could */
/* return from this function much faster.  However the function is currently set up */
/* so all of the stack_base and thread_id info is determined here.  WriteLock passes */
/* in some variables so that if the lock is not held it does not have to get the thread_id */
/* and stack base again.  Instead this function returns that information.  So this shortcut */
/* would only move this information gathering outside of this function, and I like it all */
/* contained. */

bool 
MultiLocker::IsWriteLocked(uint32 *the_stack_base, thread_id *the_thread)
{
	#if TIMING
		bigtime_t start = system_time();
	#endif

	//get a variable on the stack
	bool write_lock_holder = false;

	if (fInit == B_OK) {
		uint32 stack_base;
		thread_id thread = 0;	
		
		//determine which page in memory this stack represents
		//this is managed by taking the address of the item on the
		//stack and dividing it by the size of the memory pages
		//if it is the same as the cached stack_page, there is a match 
		stack_base = (uint32) &write_lock_holder/B_PAGE_SIZE;
		if (fWriterStackBase == stack_base) {
			write_lock_holder = true;
		} else {
			//as there was no stack_page match we resort to the
			//tried and true methods
			thread = find_thread(NULL);
			if (fWriterThread == thread) {
				write_lock_holder = true;
			}
		}
		
		//if someone wants this information, give it to them
		if (the_stack_base != NULL) {
			*the_stack_base = stack_base;
		}
		if (the_thread != NULL) {
			*the_thread = thread;
		}
	}

	#if TIMING
		bigtime_t end = system_time();
		islock_time += (end - start);
		islock_count++;
	#endif

	return write_lock_holder;
}

bool 
MultiLocker::IsReadLocked()
{
	//a properly initialized MultiLocker in non-debug always returns true
	bool locked = true;
	if (fInit == B_NO_INIT) locked = false;
	
	#if DEBUG
		//determine if the lock is actually held
		thread_id thread = find_thread(NULL);
		if (fDebugArray[thread % fMaxThreads] > 0) locked = true;
		else locked = false;
	#endif
	
	return locked;
}


/* these two functions manage the debug array for readers */
/* an array is created in the constructor large enough to hold */
/* an int32 for each of the maximum number of threads the system */
/* can have at one time. */
/* this array does not need to be locked because each running thread */
/* can be uniquely mapped to a slot in the array by performing: */
/* 		thread_id % max_threads */
/* each time ReadLock is called while in debug mode the thread_id	*/
/* is retrived in register_thread() and the count is adjusted in the */
/* array.  If register thread is ever called and the count is not 0 then */
/* an illegal, potentially deadlocking nested ReadLock occured */
/* unregister_thread clears the appropriate slot in the array */

/* this system could be expanded or retracted to include multiple arrays of information */
/* in all fairness for it's current use, fDebugArray could be an array of bools */

/* The disadvantage of this system for maintaining state is that it sucks up a ton of */
/* memory.  The other method (which would be slower), would involve an additional lock and */
/* traversing a list of cached information.  As this is only for a debug mode, the extra memory */
/* was not deemed to be a problem */

void 
MultiLocker::register_thread()
{
	#ifdef DEBUG	
		#if TIMING
			bigtime_t start = system_time();
		#endif

		thread_id thread = find_thread(NULL);
		
		ASSERT_WITH_MESSAGE(fDebugArray[thread%fMaxThreads] == 0,"Nested ReadLock!\n");
		fDebugArray[thread%fMaxThreads]++;
	
		#if TIMING
			bigtime_t end = system_time();
			reg_time += (end - start);
			reg_count++;
		#endif
	#else
		debugger("register_thread should never be called unless in DEBUG mode!\n");
	#endif
}

void 
MultiLocker::unregister_thread()
{
	#ifdef DEBUG	
		#if TIMING
			bigtime_t start = system_time();
		#endif

		thread_id thread = find_thread(NULL);
		
		ASSERT(fDebugArray[thread%fMaxThreads] == 1);
		fDebugArray[thread%fMaxThreads]--;
	
		#if TIMING
			bigtime_t end = system_time();
			unreg_time += (end - start);
			unreg_count++;
		#endif
	#else
		debugger("unregister_thread should never be called unless in DEBUG mode!\n");
	#endif

}



