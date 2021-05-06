/*
 * Copyright 2021, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "NonBlockingProcessNode.h"

#include <unistd.h>

#include "AbstractProcess.h"
#include "Logger.h"


#define TIMEOUT_UNTIL_STARTED_SECS 10
#define TIMEOUT_UNTIL_STOPPED_SECS 10


NonBlockingProcessNode::NonBlockingProcessNode(AbstractProcess* process)
	:
	AbstractProcessNode(process)
{
}


NonBlockingProcessNode::~NonBlockingProcessNode()
{
}


/*! Considered to be protected from concurrent access by the ProcessCoordinator
*/

status_t
NonBlockingProcessNode::StartProcess()
{
	HDINFO("[Node<%s>] initiating non-blocking", Process()->Name());
	Process()->Run();
	return B_OK;
}


/*! Considered to be protected from concurrent access by the ProcessCoordinator
*/

status_t
NonBlockingProcessNode::StopProcess()
{
	Process()->SetListener(NULL);
	status_t stopResult = Process()->Stop();
	return stopResult;
}
