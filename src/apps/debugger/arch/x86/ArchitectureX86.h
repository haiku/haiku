/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ARCHITECTURE_X86_H
#define ARCHITECTURE_X86_H

#include "Architecture.h"


class ArchitectureX86 : public Architecture {
public:
								ArchitectureX86(
									DebuggerInterface* debuggerInterface);
	virtual						~ArchitectureX86();

	virtual	status_t			CreateCpuState(const void* cpuStateData,
									size_t size, CpuState*& _state);
};


#endif	// ARCHITECTURE_X86_H
