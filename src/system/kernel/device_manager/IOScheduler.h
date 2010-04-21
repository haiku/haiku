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
#include <Notifications.h>
#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h>

#include "dma_resources.h"
#include "IOCallback.h"
#include "IORequest.h"


// I/O scheduler notifications
#define IO_SCHEDULER_MONITOR			'_io_'
#define IO_SCHEDULER_ADDED				0x01
#define IO_SCHEDULER_REMOVED			0x02
#define IO_SCHEDULER_REQUEST_SCHEDULED	0x04
#define IO_SCHEDULER_REQUEST_FINISHED	0x08
#define IO_SCHEDULER_OPERATION_STARTED	0x10
#define IO_SCHEDULER_OPERATION_FINISHED	0x20


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

typedef DoublyLinkedList<IOScheduler> IOSchedulerList;


class IOSchedulerRoster {
public:
	static	void				Init();
	static	IOSchedulerRoster*	Default()	{ return &sDefaultInstance; }

			bool				Lock()	{ return mutex_lock(&fLock) == B_OK; }
			void				Unlock()	{ mutex_unlock(&fLock); }

			const IOSchedulerList& SchedulerList() const
									{ return fSchedulers; }
									// caller must keep the roster locked,
									// while accessing the list

			void				AddScheduler(IOScheduler* scheduler);
			void				RemoveScheduler(IOScheduler* scheduler);

			void				Notify(uint32 eventCode,
									const IOScheduler* scheduler,
									IORequest* request = NULL,
									IOOperation* operation = NULL);

			int32				NextID();

private:
								IOSchedulerRoster();
								~IOSchedulerRoster();

private:
			mutex				fLock;
			int32				fNextID;
			IOSchedulerList		fSchedulers;
			DefaultNotificationService fNotificationService;
			char				fEventBuffer[256];

	static	IOSchedulerRoster	sDefaultInstance;
};


#endif	// IO_SCHEDULER_H
