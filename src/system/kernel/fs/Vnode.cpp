/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Vnode.h"

#include <util/AutoLock.h>


vnode::Bucket vnode::sBuckets[kBucketCount];


vnode::Bucket::Bucket()
{
	mutex_init(&lock, "vnode bucket");
}


/*static*/ void
vnode::StaticInit()
{
	for (uint32 i = 0; i < kBucketCount; i++)
		new(&sBuckets[i]) Bucket;
}


void
vnode::_WaitForLock()
{
	LockWaiter waiter;
	waiter.thread = thread_get_current_thread();
	waiter.vnode = this;

	Bucket& bucket = _Bucket();
	MutexLocker bucketLocker(bucket.lock);

	if ((atomic_or(&fFlags, kFlagsWaitingLocker)
			& (kFlagsLocked | kFlagsWaitingLocker)) == 0) {
		// The lock holder dropped it in the meantime and no-one else was faster
		// than us, so it's ours now. Just mark the node locked and clear the
		// waiting flag again.
		atomic_or(&fFlags, kFlagsLocked);
		atomic_and(&fFlags, ~kFlagsWaitingLocker);
		return;
	}

	// prepare for waiting
	bucket.waiters.Add(&waiter);
	thread_prepare_to_block(waiter.thread, 0, THREAD_BLOCK_TYPE_OTHER,
		"vnode lock");

	// start waiting
	bucketLocker.Unlock();
	thread_block();
}


void
vnode::_WakeUpLocker()
{
	Bucket& bucket = _Bucket();
	MutexLocker bucketLocker(bucket.lock);

	// mark the node locked again
	atomic_or(&fFlags, kFlagsLocked);

	// get the first waiter from the list
	LockWaiter* waiter = NULL;
	bool onlyWaiter = true;
	for (LockWaiterList::Iterator it = bucket.waiters.GetIterator();
			LockWaiter* someWaiter = it.Next();) {
		if (someWaiter->vnode == this) {
			if (waiter != NULL) {
				onlyWaiter = false;
				break;
			}
			waiter = someWaiter;
			it.Remove();
		}
	}

	ASSERT(waiter != NULL);

	// if that's the only waiter, clear the flag
	if (onlyWaiter)
		atomic_and(&fFlags, ~kFlagsWaitingLocker);

	// and wake it up
	InterruptsSpinLocker threadLocker(gSchedulerLock);
	thread_unblock_locked(waiter->thread, B_OK);
}
