/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef CPU_STATE_X86_H
#define CPU_STATE_X86_H

#include <bitset>

#include <debugger.h>

#include "CpuState.h"


typedef debug_cpu_state debug_cpu_state_x86;
	// TODO: Should be defined by <debugger.h>!

enum {
	X86_REGISTER_EIP = 0,
	X86_REGISTER_ESP,
	X86_REGISTER_EBP,

	X86_REGISTER_EAX,
	X86_REGISTER_EBX,
	X86_REGISTER_ECX,
	X86_REGISTER_EDX,

	X86_REGISTER_ESI,
	X86_REGISTER_EDI,

	X86_REGISTER_CS,
	X86_REGISTER_DS,
	X86_REGISTER_ES,
	X86_REGISTER_FS,
	X86_REGISTER_GS,
	X86_REGISTER_SS,

	X86_INT_REGISTER_END,
	X86_REGISTER_COUNT
};


class CpuStateX86 : public CpuState {
public:
								CpuStateX86();
								CpuStateX86(const debug_cpu_state_x86& state);
	virtual						~CpuStateX86();

	virtual	target_addr_t		InstructionPointer() const;
	virtual	target_addr_t		StackFramePointer() const;
	virtual	target_addr_t		StackPointer() const;
	virtual	bool				GetRegisterValue(const Register* reg,
									BVariant& _value) const;
	virtual	bool				SetRegisterValue(const Register* reg,
									const BVariant& value);

			uint32				InterruptVector() const
									{ return fInterruptVector; }

			bool				IsRegisterSet(int32 index) const;
			uint32				IntRegisterValue(int32 index) const;
			void				SetIntRegister(int32 index, uint32 value);
			void				UnsetRegister(int32 index);

private:
	typedef std::bitset<X86_REGISTER_COUNT> RegisterBitSet;

private:
			uint32				fIntRegisters[X86_REGISTER_COUNT];
			RegisterBitSet		fSetRegisters;
			uint32				fInterruptVector;
};


#endif	// CPU_STATE_X86_H
