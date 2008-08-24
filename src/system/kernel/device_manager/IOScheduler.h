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
									status_t status, size_t transferredBytes);
									// called by the driver when the operation
									// has been completed successfully or failed
									// for some reason

private:
			void				_Finisher();
			bool				_FinisherWorkPending();
			IOOperation*		_GetOperation(bool wait);
			IORequest*			_GetNextUnscheduledRequest(bool wait);
			bool				_PrepareRequestOperations(IORequest* request,
									IOOperationList& operations,
									int32& operationsPrepared);
			status_t			_Scheduler();
	static	status_t			_SchedulerThread(void* self);
			status_t			_RequestNotifier();
	static	status_t			_RequestNotifierThread(void* self);

	static	status_t			_IOCallbackWrapper(void* data,
									io_operation* operation);

private:
			DMAResource*		fDMAResource;
			spinlock			fFinisherLock;
			mutex				fLock;
			thread_id			fSchedulerThread;
			thread_id			fRequestNotifierThread;
			io_callback			fIOCallback;
			void*				fIOCallbackData;
			IORequestList		fUnscheduledRequests;
			IORequestList		fFinishedRequests;
			ConditionVariable	fNewRequestCondition;
			ConditionVariable	fFinishedOperationCondition;
			ConditionVariable	fFinishedRequestCondition;
			IOOperationList		fUnusedOperations;
			IOOperationList		fCompletedOperations;
			bool				fWaiting;
};

#endif	// IO_SCHEDULER_H
