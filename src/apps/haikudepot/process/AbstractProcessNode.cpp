/*
 * Copyright 2021, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "AbstractProcessNode.h"

#include <unistd.h>

#include "AbstractProcess.h"
#include "Logger.h"


#define SPIN_UNTIL_STARTED_DELAY_MI 250 * 1000
	// quarter of a second

#define TIMEOUT_UNTIL_STARTED_SECS 10
#define TIMEOUT_UNTIL_STOPPED_SECS 10


AbstractProcessNode::AbstractProcessNode(AbstractProcess* process)
	:
	fProcess(process)
{
}


AbstractProcessNode::~AbstractProcessNode()
{
	delete fProcess;
}


AbstractProcess*
AbstractProcessNode::Process() const
{
	return fProcess;
}


/*! This method will spin-lock the thread until the process is in one of the
    states defined by the mask.
 */

status_t
AbstractProcessNode::_SpinUntilProcessState(
	uint32 desiredStatesMask, uint32 timeoutSeconds)
{
	bigtime_t start = system_time();
	bigtime_t timeoutMicroSeconds = timeoutSeconds * 1000 * 1000;

	while (true) {
		if ((Process()->ProcessState() & desiredStatesMask) != 0)
			return B_OK;

		usleep(SPIN_UNTIL_STARTED_DELAY_MI);

		if (system_time() - start > timeoutMicroSeconds) {
			HDERROR("[Node<%s>] timeout waiting for process state",
				Process()->Name());
			return B_ERROR;
		}
	}
}


void
AbstractProcessNode::AddPredecessor(AbstractProcessNode *node)
{
	fPredecessorNodes.AddItem(node);
	node->_AddSuccessor(this);
}


int32
AbstractProcessNode::CountPredecessors() const
{
	return fPredecessorNodes.CountItems();
}


AbstractProcessNode*
AbstractProcessNode::PredecessorAt(int32 index) const
{
	return fPredecessorNodes.ItemAt(index);
}


bool
AbstractProcessNode::AllPredecessorsComplete() const
{
	for (int32 i = 0; i < CountPredecessors(); i++) {
		if (PredecessorAt(i)->Process()->ProcessState() != PROCESS_COMPLETE)
			return false;
	}

	return true;
}


void
AbstractProcessNode::_AddSuccessor(AbstractProcessNode* node)
{
	fSuccessorNodes.AddItem(node);
}


int32
AbstractProcessNode::CountSuccessors() const
{
	return fSuccessorNodes.CountItems();
}


AbstractProcessNode*
AbstractProcessNode::SuccessorAt(int32 index) const
{
	return fSuccessorNodes.ItemAt(index);
}

