/*
 * Copyright 2009-2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011-2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef CPU_STATE_X86_H
#define CPU_STATE_X86_H

#include <bitset>

#include <debugger.h>

#include "CpuState.h"


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

	X86_REGISTER_ST0,
	X86_REGISTER_ST1,
	X86_REGISTER_ST2,
	X86_REGISTER_ST3,
	X86_REGISTER_ST4,
	X86_REGISTER_ST5,
	X86_REGISTER_ST6,
	X86_REGISTER_ST7,

	X86_FP_REGISTER_END,

	X86_REGISTER_MM0,
	X86_REGISTER_MM1,
	X86_REGISTER_MM2,
	X86_REGISTER_MM3,
	X86_REGISTER_MM4,
	X86_REGISTER_MM5,
	X86_REGISTER_MM6,
	X86_REGISTER_MM7,

	X86_MMX_REGISTER_END,

	X86_REGISTER_XMM0,
	X86_REGISTER_XMM1,
	X86_REGISTER_XMM2,
	X86_REGISTER_XMM3,
	X86_REGISTER_XMM4,
	X86_REGISTER_XMM5,
	X86_REGISTER_XMM6,
	X86_REGISTER_XMM7,

	X86_XMM_REGISTER_END,

	X86_REGISTER_COUNT
};


#define X86_INT_REGISTER_COUNT X86_INT_REGISTER_END
#define X86_FP_REGISTER_COUNT (X86_FP_REGISTER_END - X86_INT_REGISTER_END)
#define X86_MMX_REGISTER_COUNT (X86_MMX_REGISTER_END - X86_FP_REGISTER_END)
#define X86_XMM_REGISTER_COUNT (X86_XMM_REGISTER_END - X86_MMX_REGISTER_END)


class CpuStateX86 : public CpuState {
public:
								CpuStateX86();
								CpuStateX86(const x86_debug_cpu_state& state);
	virtual						~CpuStateX86();

	virtual	status_t			Clone(CpuState*& _clone) const;

	virtual	status_t			UpdateDebugState(void* state, size_t size)
									const;

	virtual	target_addr_t		InstructionPointer() const;
	virtual	void				SetInstructionPointer(target_addr_t address);

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
	typedef std::bitset<X86_REGISTER_COUNT> RegisterBitSet;

private:
			uint32				fIntRegisters[X86_INT_REGISTER_COUNT];
			double				fFloatRegisters[X86_FP_REGISTER_COUNT];
			x86_fp_register		fMMXRegisters[X86_MMX_REGISTER_COUNT];
			x86_xmm_register	fXMMRegisters[X86_XMM_REGISTER_COUNT];

			RegisterBitSet		fSetRegisters;
			uint32				fInterruptVector;
};


#endif	// CPU_STATE_X86_H
