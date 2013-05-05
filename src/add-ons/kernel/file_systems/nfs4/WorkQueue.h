/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef WORKQUEUE_H
#define WORKQUEUE_H


#include <io_requests.h>
#include <lock.h>
#include <SupportDefs.h>
#include <util/DoublyLinkedList.h>

#include "Delegation.h"
#include "Inode.h"


enum JobType {
	DelegationRecall,
	IORequest
};

struct DelegationRecallArgs {
	Delegation*		fDelegation;
	bool			fTruncate;
};

struct IORequestArgs {
	io_request*		fRequest;
	Inode*			fInode;
};

struct WorkQueueEntry : public DoublyLinkedListLinkImpl<WorkQueueEntry> {
	JobType			fType;
	void*			fArguments;
};

class WorkQueue {
public:
						WorkQueue();
						~WorkQueue();

	inline	status_t	InitStatus();

			status_t	EnqueueJob(JobType type, void* args);

protected:
	static	status_t	LaunchWorkingThread(void* object);
			status_t	WorkingThread();

			void		DequeueJob();

			void		JobRecall(DelegationRecallArgs* args);
			void		JobIO(IORequestArgs* args);

private:
			status_t	fInitError;

			sem_id		fQueueSemaphore;
			mutex		fQueueLock;
			DoublyLinkedList<WorkQueueEntry>	fQueue;

			sem_id		fThreadCancel;
			thread_id	fThread;	
};


inline status_t
WorkQueue::InitStatus()
{
	return fInitError;
}


extern WorkQueue*		gWorkQueue;


#endif	// WORKQUEUE_H

