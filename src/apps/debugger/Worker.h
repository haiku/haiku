/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef WORKER_H
#define WORKER_H

#include <Locker.h>

#include <ObjectList.h>
#include <Referenceable.h>
#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h>


class Job;
class Worker;


enum job_state {
	JOB_STATE_UNSCHEDULED,
	JOB_STATE_WAITING,
	JOB_STATE_ACTIVE,
	JOB_STATE_ABORTED,
	JOB_STATE_FAILED,
	JOB_STATE_SUCCEEDED
};

enum job_wait_status {
	JOB_DEPENDENCY_NOT_FOUND,
	JOB_DEPENDENCY_SUCCEEDED,
	JOB_DEPENDENCY_FAILED,
	JOB_DEPENDENCY_ABORTED,
	JOB_DEPENDENCY_ACTIVE
		// internal only
};


class JobKey {
public:
	virtual						~JobKey();

	virtual	uint32				HashValue() const = 0;

	virtual	bool				operator==(const JobKey& other) const = 0;
};


struct SimpleJobKey : public JobKey {
			void*				object;
			uint32				type;

public:
								SimpleJobKey(void* object, uint32 type);
								SimpleJobKey(const SimpleJobKey& other);

	virtual	uint32				HashValue() const;

	virtual	bool				operator==(const JobKey& other) const;

			SimpleJobKey&		operator=(const SimpleJobKey& other);
};


class JobListener {
public:
	virtual						~JobListener();

	virtual	void				JobDone(Job* job);
	virtual	void				JobFailed(Job* job);
	virtual	void				JobAborted(Job* job);
};


typedef DoublyLinkedList<Job> JobList;


class Job : public BReferenceable, public DoublyLinkedListLinkImpl<Job> {
public:
								Job();
	virtual						~Job();

	virtual	const JobKey&		Key() const = 0;
	virtual	status_t			Do() = 0;

			Worker*				GetWorker() const	{ return fWorker; }
			job_state			State() const		{ return fState; }

protected:
			job_wait_status		WaitFor(const JobKey& key);

private:
			friend class Worker;

private:
			void				SetWorker(Worker* worker);
			void				SetState(job_state state);

			Job*				Dependency() const	{ return fDependency; }
			void				SetDependency(Job* job);

			JobList&			DependentJobs()		{ return fDependentJobs; }

			job_wait_status		WaitStatus() const	{ return fWaitStatus; }
			void				SetWaitStatus(job_wait_status status);

			status_t			AddListener(JobListener* listener);
			void				RemoveListener(JobListener* listener);
			void				NotifyListeners();

private:
	typedef BObjectList<JobListener> ListenerList;

private:
			Worker*				fWorker;
			job_state			fState;
			Job*				fDependency;
			JobList				fDependentJobs;
			job_wait_status		fWaitStatus;
			ListenerList		fListeners;

public:
			Job*				fNext;
};


class Worker {
public:
								Worker();
								~Worker();

			status_t			Init();
			void				ShutDown();

			bool				Lock()		{ return fLock.Lock(); }
			void				Unlock()	{ fLock.Unlock(); }

			status_t			ScheduleJob(Job* job,
									JobListener* listener = NULL);
										// always takes over reference
			void				AbortJob(const JobKey& key);
			Job*				GetJob(const JobKey& key);

			status_t			AddListener(const JobKey& key,
									JobListener* listener);
			void				RemoveListener(const JobKey& key,
									JobListener* listener);

private:
			friend class Job;

			struct JobHashDefinition {
				typedef JobKey	KeyType;
				typedef	Job		ValueType;

				size_t HashKey(const JobKey& key) const
				{
					return key.HashValue();
				}

				size_t Hash(Job* value) const
				{
					return HashKey(value->Key());
				}

				bool Compare(const JobKey& key, Job* value) const
				{
					return value->Key() == key;
				}

				Job*& GetLink(Job* value) const
				{
					return value->fNext;
				}
			};

			typedef BOpenHashTable<JobHashDefinition> JobTable;

private:
			job_wait_status		WaitForJob(Job* waitingJob, const JobKey& key);

	static	status_t			_WorkerLoopEntry(void* data);
			status_t			_WorkerLoop();

			void				_ProcessJobs();
			void				_AbortJob(Job* job, bool removeFromTable);
			void				_FinishJob(Job* job);

private:
			BLocker				fLock;
			JobTable			fJobs;
			JobList				fUnscheduledJobs;
			JobList				fAbortedJobs;
			sem_id				fWorkToDoSem;
			thread_id			fWorkerThread;
	volatile bool				fTerminating;
};


#endif	// WORKER_H
