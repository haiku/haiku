/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CPU_STATE_X86_H
#define CPU_STATE_X86_H

#include <debugger.h>

#include "CpuState.h"


typedef debug_cpu_state debug_cpu_state_x86;
	// TODO: Should be defined by <debugger.h>!


class CpuStateX86 : public CpuState {
public:
								CpuStateX86(const debug_cpu_state_x86& state);
	virtual						~CpuStateX86();

			const debug_cpu_state_x86 State() const { return fState; }

private:
			debug_cpu_state_x86	fState;
};


#endif	// CPU_STATE_X86_H
