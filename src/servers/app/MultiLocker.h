/* MultiLocker.h */
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

/* multiple-reader single-writer locking class */

#ifndef MULTI_LOCKER_H
#define MULTI_LOCKER_H

//#define TIMING 1

#include <OS.h>

const int32 LARGE_NUMBER = 100000;

class MultiLocker {
	public:
								MultiLocker(const char* semaphoreBaseName);
		virtual					~MultiLocker();
		
		status_t				InitCheck();
		
		//locking for reading or writing
		bool					ReadLock();
		bool					WriteLock();

		//unlocking after reading or writing
		bool					ReadUnlock();
		bool					WriteUnlock();

		//does the current thread hold a write lock ?
		bool					IsWriteLocked(uint32 *stack_base = NULL, thread_id *thread = NULL);
		//in DEBUG mode returns whether the lock is held
		//in non-debug mode returns true
		bool					IsReadLocked();

	private:
		//functions for managing the DEBUG reader array
		void					register_thread();
		void					unregister_thread();
		
		status_t				fInit;
		//readers adjust count and block on fReadSem when a writer
		//hold the lock
		int32					fReadCount;
		sem_id					fReadSem;
		//writers adjust the count and block on fWriteSem
		//when readers hold the lock
		int32					fWriteCount;
		sem_id 					fWriteSem;
		//writers must acquire fWriterLock when acquiring a write lock
		int32					fLockCount;
		sem_id					fWriterLock;
		int32					fWriterNest;
	
		thread_id				fWriterThread;
		uint32					fWriterStackBase;
				
		int32 *					fDebugArray;
		int32					fMaxThreads;

#if TIMING
		uint32 					rl_count;
		bigtime_t 				rl_time;
		uint32 					ru_count;
		bigtime_t 				ru_time;
		uint32					wl_count;
		bigtime_t				wl_time;
		uint32					wu_count;
		bigtime_t				wu_time;
		uint32					islock_count;
		bigtime_t				islock_time;
		uint32					reg_count;
		bigtime_t				reg_time;
		uint32					unreg_count;
		bigtime_t				unreg_time;
#endif
};

class AutoWriteLocker {
 public:
								AutoWriteLocker(MultiLocker* lock)
									: fLock(lock)
								{
									fLock->WriteLock();
								}
								~AutoWriteLocker()
								{
									fLock->WriteUnlock();
								}
 private:
	 	MultiLocker*			fLock;
};

class AutoReadLocker {
 public:
								AutoReadLocker(MultiLocker* lock)
									: fLock(lock)
								{
									fLock->ReadLock();
								}
								~AutoReadLocker()
								{
									fLock->ReadUnlock();
								}
 private:
	 	MultiLocker*			fLock;
};



#endif
