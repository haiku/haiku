/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "CpuStateX86.h"


CpuStateX86::CpuStateX86(const debug_cpu_state_x86& state)
	:
	fState(state)
{
}


CpuStateX86::~CpuStateX86()
{
}
