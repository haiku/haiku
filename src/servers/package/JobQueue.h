/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef JOB_QUEUE_H
#define JOB_QUEUE_H


#include <pthread.h>

#include "Job.h"


class JobQueue {
public:
								JobQueue();
								~JobQueue();

			status_t			Init();
			void				Close();

			bool				QueueJob(Job* job);
									// acquires a reference, if successful
			Job*				DequeueJob();
									// returns a reference

private:
			typedef DoublyLinkedList<Job> JobList;

private:
			pthread_mutex_t		fMutex;
			pthread_cond_t		fNewJobCondition;
			bool				fMutexInitialized;
			bool				fNewJobConditionInitialized;
			JobList				fJobs;
			bool				fClosed;
};


#endif	// JOB_QUEUE_H
