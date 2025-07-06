/*
 * Copyright 2023 Jules ALTIS. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "BFQInspiredIOScheduler.h"
#include <kernel.h>
#include <team.h>
#include <util/AutoLock.h>
#include <new>

//#define TRACE_BFQ_SCHEDULER
#ifdef TRACE_BFQ_SCHEDULER
#	define TRACE(x...) dprintf("BFQInspiredIOScheduler: " x)
#else
#	define TRACE(x...) ;
#endif

// Default budget for a process queue (can be tuned)
#define DEFAULT_PROCESS_BUDGET 16

static int32
scheduler_thread_entry(void* data)
{
	BFQInspiredIOScheduler* scheduler = (BFQInspiredIOScheduler*)data;
	IORequest* request;

	while (true) {
		request = scheduler->_SelectNextRequest();
		if (request == NULL) {
			// No request, or scheduler is terminating
			// _SelectNextRequest() handles waiting on condition variable
			// and checking fTerminating
			if (scheduler->fTerminating)
				break;
			continue;
		}
		scheduler->_DispatchOperation(request);
	}
	return 0;
}

BFQInspiredIOScheduler::BFQInspiredIOScheduler(DMAResource* dmaResource)
	:
	fDMAResource(dmaResource),
	fSchedulerThread(-1),
	fTerminating(false),
	fCallback(NULL),
	fCallbackCookie(NULL)
{
	mutex_init(&fLock, "bfq_inspired_scheduler_lock");
	fRequestCondition.Init(this, "bfq_request_condition");
}

BFQInspiredIOScheduler::~BFQInspiredIOScheduler()
{
	Shutdown();
	mutex_destroy(&fLock);
}

status_t
BFQInspiredIOScheduler::Init(const char* name)
{
	fTerminating = false;
	fSchedulerThread = spawn_kernel_thread(scheduler_thread_entry, name,
		B_NORMAL_PRIORITY, this);
	if (fSchedulerThread < 0)
		return fSchedulerThread;

	resume_thread(fSchedulerThread);
	return B_OK;
}

void
BFQInspiredIOScheduler::Shutdown()
{
	MutexLocker locker(fLock);
	if (fSchedulerThread < 0)
		return;

	fTerminating = true;
	locker.Unlock(); // Unlock before notifying to avoid scheduler thread blocking on lock

	fRequestCondition.NotifyAll(); // Wake up scheduler thread if it's waiting

	status_t exitValue;
	wait_for_thread(fSchedulerThread, &exitValue);
	fSchedulerThread = -1;

	// TODO: Clean up any pending requests in fProcessQueues
	// For now, we assume they are handled or flushed before shutdown.
	io_process_queue* queue = NULL;
	while ((queue = fProcessQueues.RemoveHead()) != NULL) {
		IORequest* request = NULL;
		while((request = queue->requests.RemoveHead()) != NULL) {
			// Potentially complete these with an error
			request->SetStatusAndNotify(B_CANCELED);
		}
		delete queue;
	}
}

status_t
BFQInspiredIOScheduler::ScheduleRequest(IORequest* request)
{
	if (fTerminating)
		return B_SHUTTING_DOWN;

	// TODO: Get actual team_id from the IORequest or associated context
	// For now, using current team as a placeholder. This needs to be fixed.
	team_id currentTeam = team_get_current_team_id();
	if (request->Team() < 0) {
		// Fallback or error if team_id is not set on IORequest
		// This is a critical piece for per-process queueing
		dprintf("BFQInspiredIOScheduler: IORequest %p has no team_id!\n", request);
		// Assign to a default/kernel queue or handle error
		currentTeam = 0; // Placeholder for a kernel/default queue
	} else {
		currentTeam = request->Team();
	}


	MutexLocker locker(fLock);

	io_process_queue* processQueue = _GetProcessQueue(currentTeam, true);
	if (processQueue == NULL)
		return B_NO_MEMORY;

	processQueue->requests.Add(request);
	fRequestCondition.Notify(); // Notify scheduler thread about new request

	TRACE("Scheduled request %p for team %" B_PRId32 "\n", request, currentTeam);
	return B_OK;
}

void
BFQInspiredIOScheduler::OperationCompleted(IOOperation* operation, status_t status,
	generic_size_t bytesTransferred)
{
	// This is called by the underlying driver/hardware when an IOOperation finishes.
	// We need to notify the original IORequest.
	// The IOSchedulerSimple directly calls operation->Parent()->Notify(status, bytesTransferred);
	// We should do something similar, or let the scheduler thread handle it.
	// For now, assuming the scheduler thread manages the IORequest lifecycle after dispatch.

	// The scheduler thread will call IORequest::SetStatusAndNotify
	// once the _DispatchOperation's callback is invoked.
	// This function might be called in interrupt context, so keep it minimal.

	// If the scheduler itself manages IOOperation objects directly (which it might
	// if it needs to re-queue/re-order them after they are created from IORequests),
	// then this is where it would update its internal state.
	// However, in this simpler model, _DispatchOperation uses the fCallback which
	// should be the original IORequest's completion mechanism.

	// If fCallback is set (meaning the driver itself is using this scheduler as a generic one)
	if (fCallback) {
		fCallback(fCallbackCookie, operation->Parent(), status, bytesTransferred);
	} else {
		// Default behavior: assume operation->Parent() is the IORequest
		// and it has a Notify mechanism.
		// This is tricky because IOOperation is generic.
		// The original IORequest is what needs to be notified.
		// The _DispatchOperation should have set up the callback correctly.
		// This function might be more about resource cleanup if the scheduler
		// wrapped IORequests in its own IOOperation structures.
		// For now, let's assume the callback set in _DispatchOperation handles this.
		TRACE("Operation %p completed for request %p, status %lx\n",
			operation, operation->Parent(), status);
	}
}

void
BFQInspiredIOScheduler::SetCallback(io_scheduler_callback callback, void* cookie)
{
	// This is if the scheduler is used more generically by a driver
	// that doesn't directly deal with VFS IORequests.
	MutexLocker locker(fLock);
	fCallback = callback;
	fCallbackCookie = cookie;
}


io_process_queue*
BFQInspiredIOScheduler::_GetProcessQueue(team_id team, bool create)
{
	// fLock must be held
	for (io_process_queue* pq = fProcessQueues.First(); pq != NULL; pq = fProcessQueues.GetNext(pq)) {
		if (pq->owner_team == team)
			return pq;
	}

	if (!create)
		return NULL;

	io_process_queue* newQueue = new(std::nothrow) io_process_queue;
	if (newQueue == NULL)
		return NULL;

	new(&newQueue->requests) DoublyLinkedList<IORequest>();
	newQueue->owner_team = team;
	newQueue->budget_remaining = DEFAULT_PROCESS_BUDGET;
	fProcessQueues.Add(newQueue);
	return newQueue;
}


IORequest*
BFQInspiredIOScheduler::_SelectNextRequest()
{
	MutexLocker locker(fLock);

	while (!fTerminating) {
		// Simple round-robin through process queues for now.
		// TODO: Implement actual BFQ-like budget and fairness logic.
		for (io_process_queue* pq = fProcessQueues.First(); pq != NULL; pq = fProcessQueues.GetNext(pq)) {
			if (!pq->requests.IsEmpty()) {
				if (pq->budget_remaining > 0) {
					pq->budget_remaining--;
					IORequest* request = pq->requests.RemoveHead();
					TRACE("Selected request %p from team %" B_PRId32 ", budget left %" B_PRIu32 "\n",
						request, pq->owner_team, pq->budget_remaining);
					return request;
				} else {
					// Budget exhausted, move to end of active list (or similar)
					// and reset budget for next round.
					// For now, just reset and skip.
					// TODO: This is a placeholder for proper budget handling.
					// A real BFQ would have a more complex way to pick the next queue.
					pq->budget_remaining = DEFAULT_PROCESS_BUDGET;
					// Potentially move this pq to the tail of fProcessQueues
					// to ensure other queues get a chance.
				}
			}
		}

		// If all queues had 0 budget or were empty, reset budgets and try again
		// This is a very naive way to give turns.
		bool foundActiveQueueWithBudget = false;
		for (io_process_queue* pq = fProcessQueues.First(); pq != NULL; pq = fProcessQueues.GetNext(pq)) {
			if (!pq->requests.IsEmpty()) {
				pq->budget_remaining = DEFAULT_PROCESS_BUDGET; // Replenish
				foundActiveQueueWithBudget = true;
			}
		}

		if (!foundActiveQueueWithBudget && !fTerminating) {
			TRACE("No active requests, waiting...\n");
			fRequestCondition.Wait();
			TRACE("Woken up.\n");
		}
		// Loop again to select after waking up or if budgets were reset
	}
	return NULL; // Terminating
}

void
BFQInspiredIOScheduler::_DispatchOperation(IOOperation* operation)
{
	// This is where the IOOperation (derived from an IORequest)
	// is actually sent to the hardware.
	// The IOSchedulerSimple uses:
	// fCallback(fCallbackCookie, operation->Parent(), B_OK, operation->Length());
	// status = fDMAResource->Execute(operation);
	// if (status != B_OK) {
	//    fCallback(fCallbackCookie, operation->Parent(), status, operation->Length());
	// }
	//
	// We need to ensure the original IORequest's completion is triggered.
	// The IORequest itself can be the callback cookie, and its Notify member the callback.
	// Or, if a generic callback is set for the scheduler, use that.

	IORequest* originalRequest = operation->Parent();
	TRACE("Dispatching operation %p for request %p (Team: %" B_PRId32 ", Offset: %lld, Length: %llu)\n",
		operation, originalRequest, originalRequest->Team(), operation->Offset(), operation->Length());

	if (fCallback != NULL) {
		// If a generic callback is set for the scheduler (e.g. by the driver)
		// TODO: This path needs careful thought. How does the driver's callback
		// relate to the IORequest's completion?
		// For now, let's assume this path is for drivers not using VFS IORequests.
		fCallback(fCallbackCookie, originalRequest, B_OK, 0); // Placeholder for "submitted"
		// The actual completion will be handled by OperationCompleted
	} else {
		// Default: The IORequest itself handles its completion.
		// The DMAResource::Execute will eventually call IORequest::OperationCompleted
		// which then calls IORequest::Notify.
		// We don't set a specific callback here; the IORequest is already set up.
	}

	status_t status = fDMAResource->Execute(operation);

	if (status != B_OK) {
		// If Execute fails immediately, notify completion with error.
		// The OperationCompleted method of the IORequest should handle this.
		// (or the generic callback if set)
		if (fCallback != NULL) {
			fCallback(fCallbackCookie, originalRequest, status, 0);
		} else {
			// This assumes IORequest::OperationCompleted can be called directly
			// if Execute fails. Or, more likely, IORequest has a way to signal
			// its own failure if Execute fails.
			// Let's assume the IORequest's own mechanism handles this.
			// Setting status directly on the request and notifying.
			originalRequest->SetStatusAndNotify(status);
		}
		TRACE("Dispatch failed for operation %p, status %lx\n", operation, status);
	} else {
		TRACE("Operation %p submitted to DMAResource\n", operation);
	}
}
