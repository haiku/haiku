/*
 * Copyright 2018-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "ProcessNode.h"

#include <unistd.h>

#include "AbstractProcess.h"
#include "Logger.h"


#define SPIN_UNTIL_STARTED_DELAY_MI 250 * 1000
	// quarter of a second

#define TIMEOUT_UNTIL_STARTED_SECS 10
#define TIMEOUT_UNTIL_STOPPED_SECS 10


ProcessNode::ProcessNode(AbstractProcess* process)
	:
	fWorker(B_BAD_THREAD_ID),
	fProcess(process)
{
}


ProcessNode::~ProcessNode()
{
	if (fProcess != NULL)
		delete fProcess;
}


AbstractProcess*
ProcessNode::Process() const
{
	return fProcess;
}


/*! This method will spin-lock the thread until the process is in one of the
    states defined by the mask.
 */

status_t
ProcessNode::_SpinUntilProcessState(
	uint32 desiredStatesMask, uint32 timeoutSeconds)
{
	uint32 start = real_time_clock();

	while (true) {
		if ((Process()->ProcessState() & desiredStatesMask) != 0)
			return B_OK;

		usleep(SPIN_UNTIL_STARTED_DELAY_MI);

		if (real_time_clock() - start > timeoutSeconds) {
			HDERROR("[Node<%s>] timeout waiting for process state",
				Process()->Name());
			return B_ERROR;
		}
	}
}


/*! Considered to be protected from concurrent access by the ProcessCoordinator
*/

status_t
ProcessNode::StartProcess()
{
	if (fWorker != B_BAD_THREAD_ID)
		return B_BUSY;

	HDINFO("[Node<%s>] initiating", Process()->Name());

	fWorker = spawn_thread(&_StartProcess, Process()->Name(),
		B_NORMAL_PRIORITY, Process());

	if (fWorker >= 0) {
		resume_thread(fWorker);
		return _SpinUntilProcessState(PROCESS_RUNNING | PROCESS_COMPLETE,
			TIMEOUT_UNTIL_STARTED_SECS);
	}

	return B_ERROR;
}


/*! Considered to be protected from concurrent access by the ProcessCoordinator
*/

status_t
ProcessNode::StopProcess()
{
	Process()->SetListener(NULL);
	status_t stopResult = Process()->Stop();
	status_t waitResult = _SpinUntilProcessState(PROCESS_COMPLETE,
		TIMEOUT_UNTIL_STOPPED_SECS);

	// if the thread is still running then it will be necessary to tear it
	// down.

	if (waitResult != B_OK) {
		HDINFO("[%s] process did not stop within timeout - will be stopped "
			"uncleanly", Process()->Name());
		kill_thread(fWorker);
	}

	if (stopResult != B_OK)
		return stopResult;

	if (waitResult != B_OK)
		return waitResult;

	return B_OK;
}


/*! This method is the initial function that is invoked on starting a new
	thread.  It will start a process that is part of the bulk-load.
 */

/*static*/ status_t
ProcessNode::_StartProcess(void* cookie)
{
	AbstractProcess* process = static_cast<AbstractProcess*>(cookie);

	HDINFO("[Node<%s>] starting process", process->Name());

	process->Run();
	return B_OK;
}


void
ProcessNode::AddPredecessor(ProcessNode *node)
{
	fPredecessorNodes.AddItem(node);
	node->_AddSuccessor(this);
}


int32
ProcessNode::CountPredecessors() const
{
	return fPredecessorNodes.CountItems();
}


ProcessNode*
ProcessNode::PredecessorAt(int32 index) const
{
	return fPredecessorNodes.ItemAt(index);
}


bool
ProcessNode::AllPredecessorsComplete() const
{
	for (int32 i = 0; i < CountPredecessors(); i++) {
		if (PredecessorAt(i)->Process()->ProcessState() != PROCESS_COMPLETE)
			return false;
	}

	return true;
}


void
ProcessNode::_AddSuccessor(ProcessNode* node)
{
	fSuccessorNodes.AddItem(node);
}


int32
ProcessNode::CountSuccessors() const
{
	return fSuccessorNodes.CountItems();
}


ProcessNode*
ProcessNode::SuccessorAt(int32 index) const
{
	return fSuccessorNodes.ItemAt(index);
}

