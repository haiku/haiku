/*
 * Copyright 2021, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef NON_BLOCKING_PROCESS_NODE_H
#define NON_BLOCKING_PROCESS_NODE_H


#include "AbstractProcessNode.h"


class AbstractProcess;


class NonBlockingProcessNode : public AbstractProcessNode {
public:
								NonBlockingProcessNode(
									AbstractProcess* process);
	virtual						~NonBlockingProcessNode();

	virtual	status_t			StartProcess();
	virtual	status_t			StopProcess();
};


#endif // NON_BLOCKING_PROCESS_NODE_H
