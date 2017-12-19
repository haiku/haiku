/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef STOPPABLE_H
#define STOPPABLE_H

class Stoppable {
public:
	virtual	bool				WasStopped() = 0;
};

#endif // STOPPABLE_H