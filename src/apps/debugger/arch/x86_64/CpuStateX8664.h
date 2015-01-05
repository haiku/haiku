/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Copyright 2009-2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011-2013, Rene Gollent, rene@gollent.com.
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

	X86_64_REGISTER_ST0,
	X86_64_REGISTER_ST1,
	X86_64_REGISTER_ST2,
	X86_64_REGISTER_ST3,
	X86_64_REGISTER_ST4,
	X86_64_REGISTER_ST5,
	X86_64_REGISTER_ST6,
	X86_64_REGISTER_ST7,

	X86_64_FP_REGISTER_END,

	X86_64_REGISTER_MM0,
	X86_64_REGISTER_MM1,
	X86_64_REGISTER_MM2,
	X86_64_REGISTER_MM3,
	X86_64_REGISTER_MM4,
	X86_64_REGISTER_MM5,
	X86_64_REGISTER_MM6,
	X86_64_REGISTER_MM7,

	X86_64_MMX_REGISTER_END,

	X86_64_REGISTER_XMM0,
	X86_64_REGISTER_XMM1,
	X86_64_REGISTER_XMM2,
	X86_64_REGISTER_XMM3,
	X86_64_REGISTER_XMM4,
	X86_64_REGISTER_XMM5,
	X86_64_REGISTER_XMM6,
	X86_64_REGISTER_XMM7,
	X86_64_REGISTER_XMM8,
	X86_64_REGISTER_XMM9,
	X86_64_REGISTER_XMM10,
	X86_64_REGISTER_XMM11,
	X86_64_REGISTER_XMM12,
	X86_64_REGISTER_XMM13,
	X86_64_REGISTER_XMM14,
	X86_64_REGISTER_XMM15,

	X86_64_XMM_REGISTER_END,

	X86_64_REGISTER_COUNT
};


#define X86_64_INT_REGISTER_COUNT X86_64_INT_REGISTER_END
#define X86_64_FP_REGISTER_COUNT (X86_64_FP_REGISTER_END \
	- X86_64_INT_REGISTER_END)
#define X86_64_MMX_REGISTER_COUNT (X86_64_MMX_REGISTER_END \
	- X86_64_FP_REGISTER_END)
#define X86_64_XMM_REGISTER_COUNT (X86_64_XMM_REGISTER_END \
	- X86_64_MMX_REGISTER_END)


class CpuStateX8664 : public CpuState {
public:
								CpuStateX8664();
								CpuStateX8664(const x86_64_debug_cpu_state& state);
	virtual						~CpuStateX8664();

	virtual	status_t			Clone(CpuState*& _clone) const;

	virtual	status_t			UpdateDebugState(void* state, size_t size)
									const;

	virtual	target_addr_t		InstructionPointer() const;
	virtual void				SetInstructionPointer(target_addr_t address);

	virtual	target_addr_t		StackFramePointer() const;
	virtual	target_addr_t		StackPointer() const;
	virtual	bool				GetRegisterValue(const Register* reg,
									BVariant& _value) const;
	virtual	bool				SetRegisterValue(const Register* reg,
									const BVariant& value);

			uint64				InterruptVector() const
									{ return fInterruptVector; }

			bool				IsRegisterSet(int32 index) const;

			uint64				IntRegisterValue(int32 index) const;
			void				SetIntRegister(int32 index, uint64 value);

			double				FloatRegisterValue(int32 index) const;
			void				SetFloatRegister(int32 index, double value);

			const void*			MMXRegisterValue(int32 index) const;
			void				SetMMXRegister(int32 index,
									const uint8* value);

			const void*			XMMRegisterValue(int32 index) const;
			void				SetXMMRegister(int32 index,
									const uint8* value);

			void				UnsetRegister(int32 index);

private:
	typedef std::bitset<X86_64_REGISTER_COUNT> RegisterBitSet;

private:
			uint64				fIntRegisters[X86_64_INT_REGISTER_COUNT];
			double				fFloatRegisters[X86_64_FP_REGISTER_COUNT];
			x86_64_fp_register	fMMXRegisters[X86_64_MMX_REGISTER_COUNT];
			x86_64_xmm_register	fXMMRegisters[X86_64_XMM_REGISTER_COUNT];
			RegisterBitSet		fSetRegisters;
			uint64				fInterruptVector;
};


#endif	// CPU_STATE_X86_64_H
