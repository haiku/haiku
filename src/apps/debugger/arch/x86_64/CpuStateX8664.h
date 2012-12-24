/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Copyright 2009-2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef CPU_STATE_X86_64_H
#define CPU_STATE_X86_64_H

#include <bitset>

#include <debugger.h>

#include "CpuState.h"


enum {
	X86_64_REGISTER_RIP = 0,
	X86_64_REGISTER_RSP,
	X86_64_REGISTER_RBP,

	X86_64_REGISTER_RAX,
	X86_64_REGISTER_RBX,
	X86_64_REGISTER_RCX,
	X86_64_REGISTER_RDX,

	X86_64_REGISTER_RSI,
	X86_64_REGISTER_RDI,

	X86_64_REGISTER_R8,
	X86_64_REGISTER_R9,
	X86_64_REGISTER_R10,
	X86_64_REGISTER_R11,
	X86_64_REGISTER_R12,
	X86_64_REGISTER_R13,
	X86_64_REGISTER_R14,
	X86_64_REGISTER_R15,

	X86_64_REGISTER_CS,
	X86_64_REGISTER_DS,
	X86_64_REGISTER_ES,
	X86_64_REGISTER_FS,
	X86_64_REGISTER_GS,
	X86_64_REGISTER_SS,

	X86_64_INT_REGISTER_END,
	X86_64_REGISTER_COUNT
};


class CpuStateX8664 : public CpuState {
public:
								CpuStateX8664();
								CpuStateX8664(const x86_64_debug_cpu_state& state);
	virtual						~CpuStateX8664();

	virtual	target_addr_t		InstructionPointer() const;
	virtual	target_addr_t		StackFramePointer() const;
	virtual	target_addr_t		StackPointer() const;
	virtual	bool				GetRegisterValue(const Register* reg,
									BVariant& _value) const;
	virtual	bool				SetRegisterValue(const Register* reg,
									const BVariant& value);

			bool				IsRegisterSet(int32 index) const;
			uint64				IntRegisterValue(int32 index) const;
			void				SetIntRegister(int32 index, uint64 value);
			void				UnsetRegister(int32 index);

private:
	typedef std::bitset<X86_64_REGISTER_COUNT> RegisterBitSet;

private:
			uint64				fIntRegisters[X86_64_REGISTER_COUNT];
			RegisterBitSet		fSetRegisters;
};


#endif	// CPU_STATE_X86_64_H
