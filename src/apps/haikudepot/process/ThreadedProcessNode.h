/*
 * Copyright 2021, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef THREADED_PROCESS_NODE_H
#define THREADED_PROCESS_NODE_H


#include "AbstractProcessNode.h"


class AbstractProcess;


class ThreadedProcessNode : public AbstractProcessNode {
public:
								ThreadedProcessNode(AbstractProcess* process,
									int32 startTimeoutSeconds);
								ThreadedProcessNode(AbstractProcess* process);
	virtual						~ThreadedProcessNode();

	virtual	status_t			Start();
	virtual	status_t			RequestStop();

private:
	static	status_t			_StartProcess(void* cookie);

			thread_id			fWorker;
			int32				fStartTimeoutSeconds;
};


#endif // THREADED_PROCESS_NODE_H
