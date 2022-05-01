/*
 * Copyright 2022, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PROCESS_LISTENER_H
#define PROCESS_LISTENER_H

#include <Referenceable.h>

/*! Clients are able to subclass from this 'interface' in order to accept
    call-backs when a process has exited; either through success or through
    failure.
 */

class ProcessListener {
public:
	virtual	void				ProcessChanged() = 0;
};

#endif // PROCESS_LISTENER_H
