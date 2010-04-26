/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
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
#include "IOCallback.h"
#include "IORequest.h"


struct IORequestOwner : DoublyLinkedListLinkImpl<IORequestOwner> {
	team_id			team;
	thread_id		thread;
	int32			priority;
	IORequestList	requests;
	IORequestList	completed_requests;
	IOOperationList	operations;
	IORequestOwner*	hash_link;

			bool				IsActive() const
									{ return !requests.IsEmpty()
										|| !completed_requests.IsEmpty()
										|| !operations.IsEmpty(); }

			void				Dump() const;
};


class IOScheduler : public DoublyLinkedListLinkImpl<IOScheduler> {
public:
								IOScheduler(DMAResource* resource);
								~IOScheduler();

			status_t			Init(const char* name);
			status_t			InitCheck() const;

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

			const char*			Name() const	{ return fName; }

			int32				ID() const		{ return fID; }

			void				Dump() const;

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
			DMAResource*		fDMAResource;
			char*				fName;
			int32				fID;
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
	volatile bool				fTerminating;
};


#endif	// IO_SCHEDULER_H
