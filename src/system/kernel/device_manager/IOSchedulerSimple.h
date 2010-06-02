/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef IO_SCHEDULER_SIMPLE_H
#define IO_SCHEDULER_SIMPLE_H


#include <KernelExport.h>

#include <condition_variable.h>
#include <lock.h>
#include <util/OpenHashTable.h>

#include "dma_resources.h"
#include "IOScheduler.h"


class IOSchedulerSimple : public IOScheduler {
public:
								IOSchedulerSimple(DMAResource* resource);
	virtual						~IOSchedulerSimple();

	virtual	status_t			Init(const char* name);

	virtual	status_t			ScheduleRequest(IORequest* request);

	virtual	void				AbortRequest(IORequest* request,
									status_t status = B_CANCELED);
	virtual	void				OperationCompleted(IOOperation* operation,
									status_t status,
									generic_size_t transferredBytes);
									// called by the driver when the operation
									// has been completed successfully or failed
									// for some reason

	virtual	void				Dump() const;

private:
			typedef DoublyLinkedList<IORequestOwner> RequestOwnerList;

			struct RequestOwnerHashDefinition;
			struct RequestOwnerHashTable;

			void				_Finisher();
			bool				_FinisherWorkPending();
			off_t				_ComputeRequestOwnerBandwidth(
									int32 priority) const;
			bool				_NextActiveRequestOwner(IORequestOwner*& owner,
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

private:
			spinlock			fFinisherLock;
			mutex				fLock;
			thread_id			fSchedulerThread;
			thread_id			fRequestNotifierThread;
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
			generic_size_t		fBlockSize;
			int32				fPendingOperations;
			off_t				fIterationBandwidth;
			off_t				fMinOwnerBandwidth;
			off_t				fMaxOwnerBandwidth;
	volatile bool				fTerminating;
};


#endif	// IO_SCHEDULER_SIMPLE_H
