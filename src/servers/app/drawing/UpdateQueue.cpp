// UpdateQueue.cpp

#include <new>
#include <stdio.h>
#include <string.h>

#include "HWInterface.h"

#include "UpdateQueue.h"

// constructor
UpdateQueue::UpdateQueue(HWInterface* interface)
	: fInterface(interface),
	  fUpdateRegion(),
	  fUpdateExecutor(-1),
	  fThreadControl(-1),
	  fStatus(B_ERROR)
	  
{
	fThreadControl = create_sem(0, "update queue control");
	if (fThreadControl >= B_OK)
		fStatus = B_OK;
	else
		fStatus = fThreadControl;
	if (fStatus == B_OK) {
		fUpdateExecutor = spawn_thread(_execute_updates_, "update queue runner",
									   B_NORMAL_PRIORITY, this);
		if (fUpdateExecutor >= B_OK) {
			fStatus = B_OK;
			resume_thread(fUpdateExecutor);
		} else
			fStatus = fUpdateExecutor;
	}
}

// destructor
UpdateQueue::~UpdateQueue()
{
	if (delete_sem(fThreadControl) == B_OK)
		wait_for_thread(fUpdateExecutor, &fUpdateExecutor);
}

// InitCheck
status_t
UpdateQueue::InitCheck()
{
	return fStatus;
}

// AddRect
void
UpdateQueue::AddRect(const BRect& rect)
{
//	Lock();
//printf("UpdateQueue::AddRect()\n");
	fUpdateRegion.Include(rect);
	_Reschedule();
//	Unlock();
}

// _execute_updates_
int32
UpdateQueue::_execute_updates_(void* cookie)
{
	UpdateQueue *gc = (UpdateQueue*)cookie;
	return gc->_ExecuteUpdates();
}

// _ExecuteUpdates
int32
UpdateQueue::_ExecuteUpdates()
{
	bool running = true;
	while (running) {
		status_t err = acquire_sem_etc(fThreadControl, 1, B_RELATIVE_TIMEOUT,
									   20000);
		switch (err) {
			case B_OK:
			case B_TIMED_OUT:
				// execute updates
				if (fInterface->LockWithTimeout(5000) >= B_OK) {
					int32 count = fUpdateRegion.CountRects();
//printf("%ld copy dirty region\n", find_thread(NULL));
					if (count > 0) {
printf("%ld rects\n", count);
						for (int32 i = 0; i < count; i++) {
							fInterface->CopyBackToFront(fUpdateRegion.RectAt(i));
						}
						fUpdateRegion.MakeEmpty();
					}
					fInterface->Unlock();
				}
				break;
			case B_BAD_SEM_ID:
				running = false;
				break;
			default:
printf("other error: %s\n", strerror(err));
running = false;
				snooze(20000);
				break;
		}
	}
	return 0;
}

// _Reschedule
//
// PRE: The object must be locked.
void
UpdateQueue::_Reschedule()
{
	if (fStatus == B_OK) {
		release_sem(fThreadControl);
	}
}

