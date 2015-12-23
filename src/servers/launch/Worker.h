/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef WORKER_H
#define WORKER_H


#include <Job.h>
#include <JobQueue.h>


using namespace BSupportKit;
using BSupportKit::BPrivate::JobQueue;


class Worker {
public:
								Worker(JobQueue& queue);
	virtual						~Worker();

			status_t			Init();

protected:
	virtual	status_t			Process();
	virtual	bigtime_t			Timeout() const;
	virtual	const char*			Name() const;
	virtual	status_t			Run(BJob* job);

private:
	static	status_t			_Process(void* self);

protected:
			thread_id			fThread;
			JobQueue&			fJobQueue;
};


class MainWorker : public Worker {
public:
								MainWorker(JobQueue& queue);

protected:
	virtual	bigtime_t			Timeout() const;
	virtual	const char*			Name() const;
	virtual	status_t			Run(BJob* job);

private:
			int32				fMaxWorkerCount;
};


#endif // WORKER_H
