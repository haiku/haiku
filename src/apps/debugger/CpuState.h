/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CPU_STATE_H
#define CPU_STATE_H

#include <OS.h>

#include <Referenceable.h>


class CpuState : public Referenceable {
public:
	virtual						~CpuState();
};


#endif	// CPU_STATE_H
