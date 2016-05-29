/*
 * Copyright 2009-2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011-2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */

#include "CpuStateX86.h"

#include <new>

#include <string.h>

#include "Register.h"


CpuStateX86::CpuStateX86()
	:
	fSetRegisters(),
	fInterruptVector(0)
{
}


CpuStateX86::CpuStateX86(const x86_debug_cpu_state& state)
	:
	fSetRegisters(),
	fInterruptVector(0)
{
	SetIntRegister(X86_REGISTER_EIP, state.eip);
	SetIntRegister(X86_REGISTER_ESP, state.user_esp);
	SetIntRegister(X86_REGISTER_EBP, state.ebp);
	SetIntRegister(X86_REGISTER_EAX, state.eax);
	SetIntRegister(X86_REGISTER_EBX, state.ebx);
	SetIntRegister(X86_REGISTER_ECX, state.ecx);
	SetIntRegister(X86_REGISTER_EDX, state.edx);
	SetIntRegister(X86_REGISTER_ESI, state.esi);
	SetIntRegister(X86_REGISTER_EDI, state.edi);
	SetIntRegister(X86_REGISTER_CS, state.cs);
	SetIntRegister(X86_REGISTER_DS, state.ds);
	SetIntRegister(X86_REGISTER_ES, state.es);
	SetIntRegister(X86_REGISTER_FS, state.fs);
	SetIntRegister(X86_REGISTER_GS, state.gs);
	SetIntRegister(X86_REGISTER_SS, state.user_ss);

	const x86_extended_registers& extended = state.extended_registers;
	SetFloatRegister(X86_REGISTER_ST0,
		(double)(*(long double*)(extended.fp_registers[0].value)));
	SetFloatRegister(X86_REGISTER_ST1,
		(double)(*(long double*)(extended.fp_registers[1].value)));
	SetFloatRegister(X86_REGISTER_ST2,
		(double)(*(long double*)(extended.fp_registers[2].value)));
	SetFloatRegister(X86_REGISTER_ST3,
		(double)(*(long double*)(extended.fp_registers[3].value)));
	SetFloatRegister(X86_REGISTER_ST4,
		(double)(*(long double*)(extended.fp_registers[4].value)));
	SetFloatRegister(X86_REGISTER_ST5,
		(double)(*(long double*)(extended.fp_registers[5].value)));
	SetFloatRegister(X86_REGISTER_ST6,
		(double)(*(long double*)(extended.fp_registers[6].value)));
	SetFloatRegister(X86_REGISTER_ST7,
		(double)(*(long double*)(extended.fp_registers[7].value)));

	SetMMXRegister(X86_REGISTER_MM0, extended.mmx_registers[0].value);
	SetMMXRegister(X86_REGISTER_MM1, extended.mmx_registers[1].value);
	SetMMXRegister(X86_REGISTER_MM2, extended.mmx_registers[2].value);
	SetMMXRegister(X86_REGISTER_MM3, extended.mmx_registers[3].value);
	SetMMXRegister(X86_REGISTER_MM4, extended.mmx_registers[4].value);
	SetMMXRegister(X86_REGISTER_MM5, extended.mmx_registers[5].value);
	SetMMXRegister(X86_REGISTER_MM6, extended.mmx_registers[6].value);
	SetMMXRegister(X86_REGISTER_MM7, extended.mmx_registers[7].value);

	SetXMMRegister(X86_REGISTER_XMM0, extended.xmm_registers[0].value);
	SetXMMRegister(X86_REGISTER_XMM1, extended.xmm_registers[1].value);
	SetXMMRegister(X86_REGISTER_XMM2, extended.xmm_registers[2].value);
	SetXMMRegister(X86_REGISTER_XMM3, extended.xmm_registers[3].value);
	SetXMMRegister(X86_REGISTER_XMM4, extended.xmm_registers[4].value);
	SetXMMRegister(X86_REGISTER_XMM5, extended.xmm_registers[5].value);
	SetXMMRegister(X86_REGISTER_XMM6, extended.xmm_registers[6].value);
	SetXMMRegister(X86_REGISTER_XMM7, extended.xmm_registers[7].value);

	fInterruptVector = state.vector;
}


CpuStateX86::~CpuStateX86()
{
}


status_t
CpuStateX86::Clone(CpuState*& _clone) const
{
	CpuStateX86* newState = new(std::nothrow) CpuStateX86();
	if (newState == NULL)
		return B_NO_MEMORY;


	memcpy(newState->fIntRegisters, fIntRegisters, sizeof(fIntRegisters));
	memcpy(newState->fFloatRegisters, fFloatRegisters,
		sizeof(fFloatRegisters));
	memcpy(newState->fMMXRegisters, fMMXRegisters, sizeof(fMMXRegisters));
	memcpy(newState->fXMMRegisters, fXMMRegisters, sizeof(fXMMRegisters));

	newState->fSetRegisters = fSetRegisters;
	newState->fInterruptVector = fInterruptVector;

	_clone = newState;

	return B_OK;
}


status_t
CpuStateX86::UpdateDebugState(void* state, size_t size) const
{
	if (size != sizeof(x86_debug_cpu_state))
		return B_BAD_VALUE;

	x86_debug_cpu_state* x86State = (x86_debug_cpu_state*)state;

	x86State->eip = InstructionPointer();
	x86State->user_esp = StackPointer();
	x86State->ebp = StackFramePointer();
	x86State->eax = IntRegisterValue(X86_REGISTER_EAX);
	x86State->ebx = IntRegisterValue(X86_REGISTER_EBX);
	x86State->ecx = IntRegisterValue(X86_REGISTER_ECX);
	x86State->edx = IntRegisterValue(X86_REGISTER_EDX);
	x86State->esi = IntRegisterValue(X86_REGISTER_ESI);
	x86State->edi = IntRegisterValue(X86_REGISTER_EDI);
	x86State->cs = IntRegisterValue(X86_REGISTER_CS);
	x86State->ds = IntRegisterValue(X86_REGISTER_DS);
	x86State->es = IntRegisterValue(X86_REGISTER_ES);
	x86State->fs = IntRegisterValue(X86_REGISTER_FS);
	x86State->gs = IntRegisterValue(X86_REGISTER_GS);
	x86State->user_ss = IntRegisterValue(X86_REGISTER_SS);
	x86State->vector = fInterruptVector;

	for (int32 i = 0; i < 8; i++) {
		*(long double*)(x86State->extended_registers.fp_registers[i].value)
			= (long double)FloatRegisterValue(X86_REGISTER_ST0 + i);

		if (IsRegisterSet(X86_REGISTER_MM0 + i)) {
			memcpy(&x86State->extended_registers.mmx_registers[i],
				&fMMXRegisters[i], sizeof(x86_fp_register));
		}

		if (IsRegisterSet(X86_REGISTER_XMM0 + i)) {
			memcpy(&x86State->extended_registers.xmm_registers[i],
				&fXMMRegisters[i], sizeof(x86_xmm_register));
		} else {
			memset(&x86State->extended_registers.xmm_registers[i],
				0, sizeof(x86_xmm_register));
		}
	}

	return B_OK;
}


target_addr_t
CpuStateX86::InstructionPointer() const
{
	return IsRegisterSet(X86_REGISTER_EIP)
		? IntRegisterValue(X86_REGISTER_EIP) : 0;
}


void
CpuStateX86::SetInstructionPointer(target_addr_t address)
{
	SetIntRegister(X86_REGISTER_EIP, (uint32)address);
}


target_addr_t
CpuStateX86::StackFramePointer() const
{
	return IsRegisterSet(X86_REGISTER_EBP)
		? IntRegisterValue(X86_REGISTER_EBP) : 0;
}


target_addr_t
CpuStateX86::StackPointer() const
{
	return IsRegisterSet(X86_REGISTER_ESP)
		? IntRegisterValue(X86_REGISTER_ESP) : 0;
}


bool
CpuStateX86::GetRegisterValue(const Register* reg, BVariant& _value) const
{
	int32 index = reg->Index();
	if (!IsRegisterSet(index))
		return false;

	if (index >= X86_XMM_REGISTER_END)
		return false;

	if (BVariant::TypeIsInteger(reg->ValueType())) {
		if (reg->BitSize() == 16)
			_value.SetTo((uint16)fIntRegisters[index]);
		else
			_value.SetTo(fIntRegisters[index]);
	} else if (BVariant::TypeIsFloat(reg->ValueType())) {
		index -= X86_REGISTER_ST0;
		if (reg->ValueType() == B_FLOAT_TYPE)
			_value.SetTo((float)fFloatRegisters[index]);
		else
			_value.SetTo(fFloatRegisters[index]);
	} else {
		if (index >= X86_REGISTER_MM0 && index < X86_REGISTER_XMM0) {
			index -= X86_REGISTER_MM0;
			_value.SetTo(fMMXRegisters[index].value);
		} else {
			index -= X86_REGISTER_XMM0;
			_value.SetTo(fXMMRegisters[index].value);
		}
	}

	return true;
}


bool
CpuStateX86::SetRegisterValue(const Register* reg, const BVariant& value)
{
	int32 index = reg->Index();
	if (index >= X86_XMM_REGISTER_END)
		return false;

	if (index < X86_INT_REGISTER_END)
		fIntRegisters[index] = value.ToUInt32();
	else if (index >= X86_REGISTER_ST0 && index < X86_FP_REGISTER_END)
		fFloatRegisters[index - X86_REGISTER_ST0] = value.ToDouble();
	else if (index >= X86_REGISTER_MM0 && index < X86_MMX_REGISTER_END) {
		if (value.Size() > sizeof(int64))
			return false;
		memset(&fMMXRegisters[index - X86_REGISTER_MM0], 0,
			sizeof(x86_fp_register));
		memcpy(fMMXRegisters[index - X86_REGISTER_MM0].value,
			value.ToPointer(), value.Size());
	} else if (index >= X86_REGISTER_XMM0 && index < X86_XMM_REGISTER_END) {
		if (value.Size() > sizeof(x86_xmm_register))
			return false;

		memset(&fXMMRegisters[index - X86_REGISTER_XMM0], 0,
			sizeof(x86_xmm_register));
		memcpy(fXMMRegisters[index - X86_REGISTER_XMM0].value,
			value.ToPointer(), value.Size());
	} else
		return false;

	fSetRegisters[index] = 1;
	return true;
}


bool
CpuStateX86::IsRegisterSet(int32 index) const
{
	return index >= 0 && index < X86_REGISTER_COUNT && fSetRegisters[index];
}


uint32
CpuStateX86::IntRegisterValue(int32 index) const
{
	if (!IsRegisterSet(index) || index >= X86_INT_REGISTER_END)
		return 0;

	return fIntRegisters[index];
}


void
CpuStateX86::SetIntRegister(int32 index, uint32 value)
{
	if (index < 0 || index >= X86_INT_REGISTER_END)
		return;

	fIntRegisters[index] = value;
	fSetRegisters[index] = 1;
}


double
CpuStateX86::FloatRegisterValue(int32 index) const
{
	if (index < X86_REGISTER_ST0 || index >= X86_FP_REGISTER_END
		|| !IsRegisterSet(index)) {
		return 0.0;
	}

	return fFloatRegisters[index - X86_REGISTER_ST0];
}


void
CpuStateX86::SetFloatRegister(int32 index, double value)
{
	if (index < X86_REGISTER_ST0 || index >= X86_FP_REGISTER_END)
		return;

	fFloatRegisters[index - X86_REGISTER_ST0] = value;
	fSetRegisters[index] = 1;
}


const void*
CpuStateX86::MMXRegisterValue(int32 index) const
{
	if (index < X86_REGISTER_MM0 || index >= X86_MMX_REGISTER_END
		|| !IsRegisterSet(index)) {
		return 0;
	}

	return fMMXRegisters[index - X86_REGISTER_MM0].value;
}


void
CpuStateX86::SetMMXRegister(int32 index, const uint8* value)
{
	if (index < X86_REGISTER_MM0 || index >= X86_MMX_REGISTER_END)
		return;

	memcpy(fMMXRegisters[index - X86_REGISTER_MM0].value, value,
		sizeof(uint64));
	fSetRegisters[index] = 1;
}


const void*
CpuStateX86::XMMRegisterValue(int32 index) const
{
	if (index < X86_REGISTER_XMM0 || index >= X86_XMM_REGISTER_END
		|| !IsRegisterSet(index)) {
		return NULL;
	}

	return fXMMRegisters[index - X86_REGISTER_XMM0].value;
}


void
CpuStateX86::SetXMMRegister(int32 index, const uint8* value)
{
	if (index < X86_REGISTER_XMM0 || index >= X86_XMM_REGISTER_END)
		return;

	memcpy(fXMMRegisters[index - X86_REGISTER_XMM0].value, value,
		sizeof(x86_xmm_register));
	fSetRegisters[index] = 1;
}


void
CpuStateX86::UnsetRegister(int32 index)
{
	if (index < 0 || index >= X86_REGISTER_COUNT)
		return;

	fSetRegisters[index] = 0;
}
