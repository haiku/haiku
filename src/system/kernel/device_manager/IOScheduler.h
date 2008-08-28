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
#include <util/OpenHashTable.h>

#include "dma_resources.h"
#include "io_requests.h"


class IOCallback {
public:
	virtual	status_t			DoIO(IOOperation* operation);
};

typedef status_t (*io_callback)(void* data, io_operation* operation);


struct IORequestOwner : DoublyLinkedListLinkImpl<IORequestOwner>,
		HashTableLink<IORequestOwner> {
	team_id			team;
	thread_id		thread;
	int32			priority;
	IORequestList	requests;
	IORequestList	completed_requests;
	IOOperationList	operations;

			bool				IsActive() const
									{ return !requests.IsEmpty()
										|| !completed_requests.IsEmpty()
										|| !operations.IsEmpty(); }

			void				Dump() const;
};


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

			void				Dump() const;

private:
			typedef DoublyLinkedList<IORequestOwner> RequestOwnerList;

			struct RequestOwnerHashDefinition;
			struct RequestOwnerHashTable;

			void				_Finisher();
			bool				_FinisherWorkPending();
			off_t				_ComputeRequestOwnerBandwidth(
									int32 priority) const;
			void				_NextActiveRequestOwner(IORequestOwner*& owner,
									off_t& quantum);
			bool				_PrepareRequestOperations(IORequest* request,
									IOOperationList& operations,
									int32& operationsPrepared);
			bool				_PrepareRequestOperations(IORequest* request,
									IOOperationList& operations,
									int32& operationsPrepared, off_t quantum,
									off_t& usedBandwidth);
			void				_SortOperations(IOOperationList& operations,
									off_t& lastOffset);
			status_t			_Scheduler();
	static	status_t			_SchedulerThread(void* self);
			status_t			_RequestNotifier();
	static	status_t			_RequestNotifierThread(void* self);

			void				_AddRequestOwner(IORequestOwner* owner);
			IORequestOwner*		_GetRequestOwner(team_id team, thread_id thread,
									bool allocate);

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
			IOOperation**		fOperationArray;
			IOOperationList		fUnusedOperations;
			IOOperationList		fCompletedOperations;
			IORequestOwner*		fAllocatedRequestOwners;
			int32				fAllocatedRequestOwnerCount;
			RequestOwnerList	fActiveRequestOwners;
			RequestOwnerList	fUnusedRequestOwners;
			RequestOwnerHashTable* fRequestOwners;
			size_t				fBlockSize;
			int32				fPendingOperations;
			off_t				fIterationBandwidth;
			off_t				fMinOwnerBandwidth;
			off_t				fMaxOwnerBandwidth;
};

#endif	// IO_SCHEDULER_H
