/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef IO_SCHEDULER_H
#define IO_SCHEDULER_H

#include <KernelExport.h>

#include <condition_variable.h>
#include <lock.h>
#include <util/DoublyLinkedList.h>

#include "dma_resources.h"
#include "io_requests.h"


class IOCallback {
public:
	virtual	status_t			DoIO(IOOperation* operation);
};

typedef status_t (*io_callback)(void* data, io_operation* operation);


class IOScheduler {
public:
								IOScheduler(DMAResource* resource);
								~IOScheduler();

			status_t			Init(const char* name);

			void				SetCallback(IOCallback& callback);
			void				SetCallback(io_callback callback, void* data);

			status_t			ScheduleRequest(IORequest* request);

			void				AbortRequest(IORequest* request,
									status_t status = B_CANCELED);
			void				OperationCompleted(IOOperation* operation,
									status_t status);
									// called by the driver when the operation
									// has been completed successfully or failed
									// for some reason

private:
			void				_Finisher();
			bool				_FinisherWorkPending();
			IOOperation*		_GetOperation();
			IORequest*			_GetNextUnscheduledRequest();
			status_t			_Scheduler();
	static	status_t			_SchedulerThread(void* self);


private:
			DMAResource*		fDMAResource;
			spinlock			fFinisherLock;
			mutex				fLock;
			thread_id			fThread;
			io_callback			fIOCallback;
			void*				fIOCallbackData;
			IORequestList		fUnscheduledRequests;
			ConditionVariable	fNewRequestCondition;
			ConditionVariable	fFinishedOperationCondition;
			IOOperationList		fUnusedOperations;
			IOOperationList		fCompletedOperations;
			bool				fWaiting;
};

#endif	// IO_SCHEDULER_H
