/*
 * Copyright 2021-2022, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "ThreadedProcessNode.h"

#include <unistd.h>

#include "AbstractProcess.h"
#include "Logger.h"
#include "ProcessListener.h"


#define TIMEOUT_UNTIL_STARTED_SECS_DEFAULT 10
#define TIMEOUT_UNTIL_STOPPED_SECS_DEFAULT 10


ThreadedProcessNode::ThreadedProcessNode(AbstractProcess* process,
		int32 startTimeoutSeconds)
	:
	AbstractProcessNode(process),
	fWorker(B_BAD_THREAD_ID),
	fStartTimeoutSeconds(startTimeoutSeconds)
{
}


ThreadedProcessNode::ThreadedProcessNode(AbstractProcess* process)
	:
	AbstractProcessNode(process),
	fWorker(B_BAD_THREAD_ID),
	fStartTimeoutSeconds(TIMEOUT_UNTIL_STARTED_SECS_DEFAULT)
{
}


ThreadedProcessNode::~ThreadedProcessNode()
{
	if (IsRunning()) {
		HDFATAL("the process node is being deleted while the thread is"
			"still running");
	}
}


bool
ThreadedProcessNode::IsRunning()
{
	if (!AbstractProcessNode::IsRunning()) {
		AutoLocker<BLocker> locker(fLock);
		if (fWorker == B_BAD_THREAD_ID)
			return false;
		thread_info ti;
		status_t status = get_thread_info(fWorker, &ti);
		if (status != B_OK)
			// implies that the thread has stopped
			return false;
		HDTRACE("[Node<%s>] thread still running...", Process()->Name());
	}
	return true;
}


/*! Considered to be protected from concurrent access by the ProcessCoordinator
*/

status_t
ThreadedProcessNode::Start()
{
	AutoLocker<BLocker> locker(fLock);
	if (fWorker != B_BAD_THREAD_ID)
		return B_BUSY;

	HDINFO("[Node<%s>] initiating threaded", Process()->Name());

	fWorker = spawn_thread(&_RunProcessThreadEntry, Process()->Name(),
		B_NORMAL_PRIORITY, this);

	if (fWorker >= 0) {
		resume_thread(fWorker);
		return _SpinUntilProcessState(PROCESS_RUNNING | PROCESS_COMPLETE,
			fStartTimeoutSeconds);
	}

	return B_ERROR;
}


status_t
ThreadedProcessNode::RequestStop()
{
	return Process()->Stop();
}


void
ThreadedProcessNode::_RunProcessStart()
{
	if (fListener != NULL) {
		if (on_exit_thread(&_RunProcessThreadExit, this) != B_OK) {
			HDFATAL("unable to setup 'on exit' for thread");
		}
	}

	AbstractProcess* process = Process();

	if (process == NULL)
		HDFATAL("the process node must have a process defined");

	bigtime_t start = system_time();
	HDINFO("[Node<%s>] starting process in thread", process->Name());
	process->Run();
	HDINFO("[Node<%s>] finished process in thread %f seconds", process->Name(),
		(system_time() - start) / 1000000.0);
}


/*! This method is the initial function that is invoked on starting a new
	thread.  It will start a process that is part of the bulk-load.
 */

/*static*/ status_t
ThreadedProcessNode::_RunProcessThreadEntry(void* cookie)
{
	static_cast<ThreadedProcessNode*>(cookie)->_RunProcessStart();
	return B_OK;
}


void
ThreadedProcessNode::_RunProcessExit()
{
	AutoLocker<BLocker> locker(fLock);
	fWorker = B_BAD_THREAD_ID;
	HDTRACE("[Node<%s>] compute complete", Process()->Name());
	if (fListener != NULL)
		fListener->ProcessChanged();
}


/*static*/ void
ThreadedProcessNode::_RunProcessThreadExit(void* cookie)
{
	static_cast<ThreadedProcessNode*>(cookie)->_RunProcessExit();
}
