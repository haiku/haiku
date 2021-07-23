/*
 * Copyright 2021, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "ThreadedProcessNode.h"

#include <unistd.h>

#include "AbstractProcess.h"
#include "Logger.h"


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
}


/*! Considered to be protected from concurrent access by the ProcessCoordinator
*/

status_t
ThreadedProcessNode::Start()
{
	if (fWorker != B_BAD_THREAD_ID)
		return B_BUSY;

	HDINFO("[Node<%s>] initiating threaded", Process()->Name());

	fWorker = spawn_thread(&_StartProcess, Process()->Name(),
		B_NORMAL_PRIORITY, Process());

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


/*! This method is the initial function that is invoked on starting a new
	thread.  It will start a process that is part of the bulk-load.
 */

/*static*/ status_t
ThreadedProcessNode::_StartProcess(void* cookie)
{
	AbstractProcess* process = static_cast<AbstractProcess*>(cookie);
	bigtime_t start = system_time();

	HDINFO("[Node<%s>] starting process in thread", process->Name());
	process->Run();
	HDINFO("[Node<%s>] finished process in thread %f seconds", process->Name(),
		(system_time() - start) / 1000000.0);

	return B_OK;
}
