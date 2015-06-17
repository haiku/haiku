/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "Worker.h"


static const bigtime_t kWorkerTimeout = 1000000;
	// One second until a worker thread quits without a job

static int32 sWorkerCount;


Worker::Worker(JobQueue& queue)
	:
	fJobQueue(queue)
{
	fThread = spawn_thread(&Worker::_Process, "worker", B_NORMAL_PRIORITY,
		this);
	if (fThread >= 0 && resume_thread(fThread) == B_OK)
		atomic_add(&sWorkerCount, 1);
}


Worker::~Worker()
{
}


status_t
Worker::Process()
{
	while (true) {
		BJob* job;
		status_t status = fJobQueue.Pop(Timeout(), false, &job);
		if (status != B_OK)
			return status;

		status = Run(job);
		if (status != B_OK) {
			// TODO: proper error reporting on failed job!
			debug_printf("Launching %s failed: %s\n", job->Title().String(),
				strerror(status));
		}
	}
}


bigtime_t
Worker::Timeout() const
{
	return kWorkerTimeout;
}


status_t
Worker::Run(BJob* job)
{
	return job->Run();
}


/*static*/ status_t
Worker::_Process(void* _self)
{
	Worker* self = (Worker*)_self;
	status_t status = self->Process();
	delete self;

	return status;
}


// #pragma mark -


MainWorker::MainWorker(JobQueue& queue)
	:
	Worker(queue)
{
	// TODO: keep track of workers, and quit them on destruction
	system_info info;
	if (get_system_info(&info) == B_OK)
		fCPUCount = info.cpu_count;
}


bigtime_t
MainWorker::Timeout() const
{
	return B_INFINITE_TIMEOUT;
}


status_t
MainWorker::Run(BJob* job)
{
	int32 count = atomic_get(&sWorkerCount);

	size_t jobCount = fJobQueue.CountJobs();
	if (jobCount > INT_MAX)
		jobCount = INT_MAX;

	if ((int32)jobCount > count && count < fCPUCount)
		new Worker(fJobQueue);

	return Worker::Run(job);
}
