/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "ArchitectureX86.h"

#include <new>

#include "CpuStateX86.h"


ArchitectureX86::ArchitectureX86(DebuggerInterface* debuggerInterface)
	:
	Architecture(debuggerInterface)
{
}


ArchitectureX86::~ArchitectureX86()
{
}


status_t
ArchitectureX86::CreateCpuState(const void* cpuStateData, size_t size,
	CpuState*& _state)
{
	if (size != sizeof(debug_cpu_state_x86))
		return B_BAD_VALUE;

	CpuStateX86* state = new(std::nothrow) CpuStateX86(
		*(const debug_cpu_state_x86*)cpuStateData);
	if (state == NULL)
		return B_NO_MEMORY;

	_state = state;
	return B_OK;
}
