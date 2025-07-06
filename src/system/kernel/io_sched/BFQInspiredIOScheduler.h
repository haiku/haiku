/*
 * Copyright 2023 Jules ALTIS. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_KERNEL_IO_SCHED_BFQ_INSPIRED_IO_SCHEDULER_H
#define _SYSTEM_KERNEL_IO_SCHED_BFQ_INSPIRED_IO_SCHEDULER_H

#include <kernel.h>
#include <util/DoublyLinkedList.h>
#include <IORequest.h>
#include <IOScheduler.h>

// Forward declaration
class IORequest;

// Per-process queue structure
struct io_process_queue {
	DoublyLinkedListLink<io_process_queue> link;
	DoublyLinkedList<IORequest> requests;
	team_id owner_team;
	uint32 budget_remaining;
	// Other stats like weight, priority can be added here
};

class BFQInspiredIOScheduler : public IOScheduler {
public:
								BFQInspiredIOScheduler(DMAResource* dmaResource);
	virtual						~BFQInspiredIOScheduler();

	virtual status_t			Init(const char* name);
	virtual void				Shutdown();

	virtual status_t			ScheduleRequest(IORequest* request);
	virtual void				OperationCompleted(IOOperation* operation,
									status_t status,
									generic_size_t bytesTransferred);

	virtual void				SetCallback(io_scheduler_callback callback,
									void* cookie);

private:
			IORequest*			_SelectNextRequest();
			io_process_queue*	_GetProcessQueue(team_id team, bool create);
			void				_DispatchOperation(IOOperation* operation);

private:
			DMAResource*		fDMAResource;
			DoublyLinkedList<io_process_queue> fProcessQueues;
			mutex				fLock;
			ConditionVariable	fRequestCondition;
			thread_id			fSchedulerThread;
			bool				fTerminating;

			io_scheduler_callback fCallback;
			void*				fCallbackCookie;

			// TODO: Add structures for managing active queues, budgets, etc.
			// For example, a run queue of process queues that have pending requests.
};

#endif // _SYSTEM_KERNEL_IO_SCHED_BFQ_INSPIRED_IO_SCHEDULER_H
