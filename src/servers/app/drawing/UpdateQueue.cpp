/*
 * Copyright 2005-2008 Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "UpdateQueue.h"

#include <new>
#include <stdio.h>
#include <string.h>


//#define TRACE_UPDATE_QUEUE
#ifdef TRACE_UPDATE_QUEUE
#	include <FunctionTracer.h>
#	include <String.h>

	static int32 sFunctionDepth = -1;
#	define CALLED(x...)	FunctionTracer _ft("UpdateQueue", __FUNCTION__, \
							sFunctionDepth)
#	define TRACE(x...)	{ BString _to; \
							_to.Append(' ', (sFunctionDepth + 1) * 2); \
							printf("%s", _to.String()); printf(x); }
#else
#	define CALLED(x...)
#	define TRACE(x...)
#endif


// constructor
UpdateQueue::UpdateQueue(HWInterface* interface)
	:
	BLocker("AppServer_UpdateQueue"),
	fQuitting(false),
 	fInterface(interface),
	fUpdateRegion(),
	fUpdateExecutor(B_BAD_THREAD_ID),
	fRetraceSem(B_BAD_SEM_ID),
	fRefreshDuration(1000000 / 60)
{
	CALLED();
	TRACE("this: %p\n", this);
	TRACE("fInterface: %p\n", fInterface);
}

// destructor
UpdateQueue::~UpdateQueue()
{
	CALLED();

	Shutdown();
}

// FrameBufferChanged
void
UpdateQueue::FrameBufferChanged()
{
	CALLED();

	Init();
}

// Init
status_t
UpdateQueue::Init()
{
	CALLED();

	Shutdown();

	fRetraceSem = fInterface->RetraceSemaphore();
//	fRefreshDuration = fInterface->...

	TRACE("fRetraceSem: %ld, fRefreshDuration: %lld\n",
		fRetraceSem, fRefreshDuration);

	fQuitting = false;
	fUpdateExecutor = spawn_thread(_ExecuteUpdatesEntry, "update queue runner",
		B_REAL_TIME_PRIORITY, this);
	if (fUpdateExecutor < B_OK)
		return fUpdateExecutor;
		
	return resume_thread(fUpdateExecutor);
}

// Shutdown
void
UpdateQueue::Shutdown()
{
	CALLED();

	if (fUpdateExecutor < B_OK)
		return;
	fQuitting = true;
	status_t exitValue;
	wait_for_thread(fUpdateExecutor, &exitValue);
	fUpdateExecutor = B_BAD_THREAD_ID;
}

// AddRect
void
UpdateQueue::AddRect(const BRect& rect)
{
	if (!rect.IsValid())
		return;

	CALLED();

	if (Lock()) {
		fUpdateRegion.Include(rect);
		Unlock();
	}
}

// _ExecuteUpdatesEntry
int32
UpdateQueue::_ExecuteUpdatesEntry(void* cookie)
{
	UpdateQueue *gc = (UpdateQueue*)cookie;
	return gc->_ExecuteUpdates();
}

// _ExecuteUpdates
int32
UpdateQueue::_ExecuteUpdates()
{
	while (!fQuitting) {
		status_t err;
		if (fRetraceSem >= 0) {
			bigtime_t timeout = system_time() + fRefreshDuration * 2;
//			TRACE("acquire_sem_etc(%lld)\n", timeout);
			do {
				err = acquire_sem_etc(fRetraceSem, 1,
					B_ABSOLUTE_TIMEOUT | B_CAN_INTERRUPT, timeout);
			} while (err == B_INTERRUPTED && !fQuitting);
		} else {
			bigtime_t timeout = system_time() + fRefreshDuration;
//			TRACE("snooze_until(%lld)\n", timeout);
			do {
				err = snooze_until(timeout, B_SYSTEM_TIMEBASE);
			} while (err == B_INTERRUPTED && !fQuitting);
		}
		if (fQuitting)
			return B_OK;
		switch (err) {
			case B_OK:
			case B_TIMED_OUT:
				// execute updates
				if (fInterface->LockParallelAccess()) {
					if (Lock()) {
						int32 count = fUpdateRegion.CountRects();
						if (count > 0) {
							TRACE("CopyBackToFront() - rects: %ld\n", count);
							// NOTE: not using the BRegion version, since that
							// doesn't take care of leaving out and compositing
							// the cursor.
							for (int32 i = 0; i < count; i++)
								fInterface->CopyBackToFront(
									fUpdateRegion.RectAt(i));
							fUpdateRegion.MakeEmpty();
						}
						Unlock();
					}
					fInterface->UnlockParallelAccess();
				}
				break;
			default:
				return err;
		}
	}
	return B_OK;
}

