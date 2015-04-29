/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Copyright 2009-2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011-2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */

#include "CpuStateX8664.h"

#include <new>

#include <string.h>

#include "Register.h"


CpuStateX8664::CpuStateX8664()
	:
	fSetRegisters()
{
}


CpuStateX8664::CpuStateX8664(const x86_64_debug_cpu_state& state)
	:
	fSetRegisters(),
	fInterruptVector(0)
{
	SetIntRegister(X86_64_REGISTER_RIP, state.rip);
	SetIntRegister(X86_64_REGISTER_RSP, state.rsp);
	SetIntRegister(X86_64_REGISTER_RBP, state.rbp);
	SetIntRegister(X86_64_REGISTER_RAX, state.rax);
	SetIntRegister(X86_64_REGISTER_RBX, state.rbx);
	SetIntRegister(X86_64_REGISTER_RCX, state.rcx);
	SetIntRegister(X86_64_REGISTER_RDX, state.rdx);
	SetIntRegister(X86_64_REGISTER_RSI, state.rsi);
	SetIntRegister(X86_64_REGISTER_RDI, state.rdi);
	SetIntRegister(X86_64_REGISTER_R8, state.r8);
	SetIntRegister(X86_64_REGISTER_R9, state.r9);
	SetIntRegister(X86_64_REGISTER_R10, state.r10);
	SetIntRegister(X86_64_REGISTER_R11, state.r11);
	SetIntRegister(X86_64_REGISTER_R12, state.r12);
	SetIntRegister(X86_64_REGISTER_R13, state.r13);
	SetIntRegister(X86_64_REGISTER_R14, state.r14);
	SetIntRegister(X86_64_REGISTER_R15, state.r15);
	SetIntRegister(X86_64_REGISTER_CS, state.cs);
	SetIntRegister(X86_64_REGISTER_DS, state.ds);
	SetIntRegister(X86_64_REGISTER_ES, state.es);
	SetIntRegister(X86_64_REGISTER_FS, state.fs);
	SetIntRegister(X86_64_REGISTER_GS, state.gs);
	SetIntRegister(X86_64_REGISTER_SS, state.ss);

	const x86_64_extended_registers& extended = state.extended_registers;

	SetFloatRegister(X86_64_REGISTER_ST0,
		(double)(*(long double*)(extended.fp_registers[0].value)));
	SetFloatRegister(X86_64_REGISTER_ST1,
		(double)(*(long double*)(extended.fp_registers[1].value)));
	SetFloatRegister(X86_64_REGISTER_ST2,
		(double)(*(long double*)(extended.fp_registers[2].value)));
	SetFloatRegister(X86_64_REGISTER_ST3,
		(double)(*(long double*)(extended.fp_registers[3].value)));
	SetFloatRegister(X86_64_REGISTER_ST4,
		(double)(*(long double*)(extended.fp_registers[4].value)));
	SetFloatRegister(X86_64_REGISTER_ST5,
		(double)(*(long double*)(extended.fp_registers[5].value)));
	SetFloatRegister(X86_64_REGISTER_ST6,
		(double)(*(long double*)(extended.fp_registers[6].value)));
	SetFloatRegister(X86_64_REGISTER_ST7,
		(double)(*(long double*)(extended.fp_registers[7].value)));

	SetMMXRegister(X86_64_REGISTER_MM0, extended.mmx_registers[0].value);
	SetMMXRegister(X86_64_REGISTER_MM1, extended.mmx_registers[1].value);
	SetMMXRegister(X86_64_REGISTER_MM2, extended.mmx_registers[2].value);
	SetMMXRegister(X86_64_REGISTER_MM3, extended.mmx_registers[3].value);
	SetMMXRegister(X86_64_REGISTER_MM4, extended.mmx_registers[4].value);
	SetMMXRegister(X86_64_REGISTER_MM5, extended.mmx_registers[5].value);
	SetMMXRegister(X86_64_REGISTER_MM6, extended.mmx_registers[6].value);
	SetMMXRegister(X86_64_REGISTER_MM7, extended.mmx_registers[7].value);

	SetXMMRegister(X86_64_REGISTER_XMM0, extended.xmm_registers[0].value);
	SetXMMRegister(X86_64_REGISTER_XMM1, extended.xmm_registers[1].value);
	SetXMMRegister(X86_64_REGISTER_XMM2, extended.xmm_registers[2].value);
	SetXMMRegister(X86_64_REGISTER_XMM3, extended.xmm_registers[3].value);
	SetXMMRegister(X86_64_REGISTER_XMM4, extended.xmm_registers[4].value);
	SetXMMRegister(X86_64_REGISTER_XMM5, extended.xmm_registers[5].value);
	SetXMMRegister(X86_64_REGISTER_XMM6, extended.xmm_registers[6].value);
	SetXMMRegister(X86_64_REGISTER_XMM7, extended.xmm_registers[7].value);
	SetXMMRegister(X86_64_REGISTER_XMM8, extended.xmm_registers[8].value);
	SetXMMRegister(X86_64_REGISTER_XMM9, extended.xmm_registers[9].value);
	SetXMMRegister(X86_64_REGISTER_XMM10, extended.xmm_registers[10].value);
	SetXMMRegister(X86_64_REGISTER_XMM11, extended.xmm_registers[11].value);
	SetXMMRegister(X86_64_REGISTER_XMM12, extended.xmm_registers[12].value);
	SetXMMRegister(X86_64_REGISTER_XMM13, extended.xmm_registers[13].value);
	SetXMMRegister(X86_64_REGISTER_XMM14, extended.xmm_registers[14].value);
	SetXMMRegister(X86_64_REGISTER_XMM15, extended.xmm_registers[15].value);

	fInterruptVector = state.vector;
}


CpuStateX8664::~CpuStateX8664()
{
}


status_t
CpuStateX8664::Clone(CpuState*& _clone) const
{
	CpuStateX8664* newState = new(std::nothrow) CpuStateX8664();
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
CpuStateX8664::UpdateDebugState(void* state, size_t size) const
{
	if (size != sizeof(x86_64_debug_cpu_state))
		return B_BAD_VALUE;

	x86_64_debug_cpu_state* x64State = (x86_64_debug_cpu_state*)state;

	x64State->rip = InstructionPointer();
	x64State->rsp = StackPointer();
	x64State->rbp = StackFramePointer();
	x64State->rax = IntRegisterValue(X86_64_REGISTER_RAX);
	x64State->rbx = IntRegisterValue(X86_64_REGISTER_RBX);
	x64State->rcx = IntRegisterValue(X86_64_REGISTER_RCX);
	x64State->rdx = IntRegisterValue(X86_64_REGISTER_RDX);
	x64State->rsi = IntRegisterValue(X86_64_REGISTER_RSI);
	x64State->rdi = IntRegisterValue(X86_64_REGISTER_RDI);
	x64State->r8 = IntRegisterValue(X86_64_REGISTER_R8);
	x64State->r9 = IntRegisterValue(X86_64_REGISTER_R9);
	x64State->r10 = IntRegisterValue(X86_64_REGISTER_R10);
	x64State->r11 = IntRegisterValue(X86_64_REGISTER_R11);
	x64State->r12 = IntRegisterValue(X86_64_REGISTER_R12);
	x64State->r13 = IntRegisterValue(X86_64_REGISTER_R13);
	x64State->r14 = IntRegisterValue(X86_64_REGISTER_R14);
	x64State->r15 = IntRegisterValue(X86_64_REGISTER_R15);
	x64State->cs = IntRegisterValue(X86_64_REGISTER_CS);
	x64State->ds = IntRegisterValue(X86_64_REGISTER_DS);
	x64State->es = IntRegisterValue(X86_64_REGISTER_ES);
	x64State->fs = IntRegisterValue(X86_64_REGISTER_FS);
	x64State->gs = IntRegisterValue(X86_64_REGISTER_GS);
	x64State->ss = IntRegisterValue(X86_64_REGISTER_SS);

	for (int32 i = 0; i < 8; i++) {
		*(long double*)(x64State->extended_registers.fp_registers[i].value)
			= (long double)FloatRegisterValue(X86_64_REGISTER_ST0 + i);

		if (IsRegisterSet(X86_64_REGISTER_MM0 + i)) {
			memcpy(&x64State->extended_registers.mmx_registers[i],
				&fMMXRegisters[i], sizeof(x86_64_fp_register));
		}
	}

	for (int32 i = 0; i < 16; i++) {
		if (IsRegisterSet(X86_64_REGISTER_XMM0 + i)) {
			memcpy(&x64State->extended_registers.xmm_registers[i],
				&fXMMRegisters[i], sizeof(x86_64_xmm_register));
		} else {
			memset(&x64State->extended_registers.xmm_registers[i],
				0, sizeof(x86_64_xmm_register));
		}
	}

	return B_OK;
}


target_addr_t
CpuStateX8664::InstructionPointer() const
{
	return IsRegisterSet(X86_64_REGISTER_RIP)
		? IntRegisterValue(X86_64_REGISTER_RIP) : 0;
}


void
CpuStateX8664::SetInstructionPointer(target_addr_t address)
{
	SetIntRegister(X86_64_REGISTER_RIP, address);
}


target_addr_t
CpuStateX8664::StackFramePointer() const
{
	return IsRegisterSet(X86_64_REGISTER_RBP)
		? IntRegisterValue(X86_64_REGISTER_RBP) : 0;
}


target_addr_t
CpuStateX8664::StackPointer() const
{
	return IsRegisterSet(X86_64_REGISTER_RSP)
		? IntRegisterValue(X86_64_REGISTER_RSP) : 0;
}


bool
CpuStateX8664::GetRegisterValue(const Register* reg, BVariant& _value) const
{
	int32 index = reg->Index();
	if (!IsRegisterSet(index))
		return false;

	if (index >= X86_64_XMM_REGISTER_END)
		return false;

	if (BVariant::TypeIsInteger(reg->ValueType())) {
		if (reg->BitSize() == 16)
			_value.SetTo((uint16)fIntRegisters[index]);
		else
			_value.SetTo(fIntRegisters[index]);
	} else if (BVariant::TypeIsFloat(reg->ValueType())) {
		index -= X86_64_REGISTER_ST0;
		if (reg->ValueType() == B_FLOAT_TYPE)
			_value.SetTo((float)fFloatRegisters[index]);
		else
			_value.SetTo(fFloatRegisters[index]);
	} else {
		if (index >= X86_64_REGISTER_MM0 && index < X86_64_REGISTER_XMM0) {
			index -= X86_64_REGISTER_MM0;
			_value.SetTo(fMMXRegisters[index].value);
		} else {
			index -= X86_64_REGISTER_XMM0;
			_value.SetTo(fXMMRegisters[index].value);
		}
	}

	return true;
}


bool
CpuStateX8664::SetRegisterValue(const Register* reg, const BVariant& value)
{
	int32 index = reg->Index();
	if (index >= X86_64_XMM_REGISTER_END)
		return false;

	if (index < X86_64_INT_REGISTER_END)
		fIntRegisters[index] = value.ToUInt64();
	else if (index >= X86_64_REGISTER_ST0 && index < X86_64_FP_REGISTER_END)
		fFloatRegisters[index - X86_64_REGISTER_ST0] = value.ToDouble();
	else if (index >= X86_64_REGISTER_MM0 && index < X86_64_MMX_REGISTER_END) {
		if (value.Size() > sizeof(int64))
			return false;
		memset(&fMMXRegisters[index - X86_64_REGISTER_MM0], 0,
			sizeof(x86_64_fp_register));
		memcpy(fMMXRegisters[index - X86_64_REGISTER_MM0].value,
			value.ToPointer(), value.Size());
	} else if (index >= X86_64_REGISTER_XMM0
			&& index < X86_64_XMM_REGISTER_END) {
		if (value.Size() > sizeof(x86_64_xmm_register))
			return false;

		memset(&fXMMRegisters[index - X86_64_REGISTER_XMM0], 0,
			sizeof(x86_64_xmm_register));
		memcpy(fXMMRegisters[index - X86_64_REGISTER_XMM0].value,
			value.ToPointer(), value.Size());
	} else
		return false;

	fSetRegisters[index] = 1;
	return true;
}


bool
CpuStateX8664::IsRegisterSet(int32 index) const
{
	return index >= 0 && index < X86_64_REGISTER_COUNT && fSetRegisters[index];
}


uint64
CpuStateX8664::IntRegisterValue(int32 index) const
{
	if (!IsRegisterSet(index) || index >= X86_64_INT_REGISTER_END)
		return 0;

	return fIntRegisters[index];
}


void
CpuStateX8664::SetIntRegister(int32 index, uint64 value)
{
	if (index < 0 || index >= X86_64_INT_REGISTER_END)
		return;

	fIntRegisters[index] = value;
	fSetRegisters[index] = 1;
}


double
CpuStateX8664::FloatRegisterValue(int32 index) const
{
	if (index < X86_64_REGISTER_ST0 || index >= X86_64_FP_REGISTER_END
		|| !IsRegisterSet(index)) {
		return 0.0;
	}

	return fFloatRegisters[index - X86_64_REGISTER_ST0];
}


void
CpuStateX8664::SetFloatRegister(int32 index, double value)
{
	if (index < X86_64_REGISTER_ST0 || index >= X86_64_FP_REGISTER_END)
		return;

	fFloatRegisters[index - X86_64_REGISTER_ST0] = value;
	fSetRegisters[index] = 1;
}


const void*
CpuStateX8664::MMXRegisterValue(int32 index) const
{
	if (index < X86_64_REGISTER_MM0 || index >= X86_64_MMX_REGISTER_END
		|| !IsRegisterSet(index)) {
		return 0;
	}

	return fMMXRegisters[index - X86_64_REGISTER_MM0].value;
}


void
CpuStateX8664::SetMMXRegister(int32 index, const uint8* value)
{
	if (index < X86_64_REGISTER_MM0 || index >= X86_64_MMX_REGISTER_END)
		return;

	memcpy(fMMXRegisters[index - X86_64_REGISTER_MM0].value, value,
		sizeof(uint64));
	fSetRegisters[index] = 1;
}


const void*
CpuStateX8664::XMMRegisterValue(int32 index) const
{
	if (index < X86_64_REGISTER_XMM0 || index >= X86_64_XMM_REGISTER_END
		|| !IsRegisterSet(index)) {
		return NULL;
	}

	return fXMMRegisters[index - X86_64_REGISTER_XMM0].value;
}


void
CpuStateX8664::SetXMMRegister(int32 index, const uint8* value)
{
	if (index < X86_64_REGISTER_XMM0 || index >= X86_64_XMM_REGISTER_END)
		return;

	memcpy(fXMMRegisters[index - X86_64_REGISTER_XMM0].value, value,
		sizeof(x86_64_xmm_register));
	fSetRegisters[index] = 1;
}


void
CpuStateX8664::UnsetRegister(int32 index)
{
	if (index < 0 || index >= X86_64_REGISTER_COUNT)
		return;

	fSetRegisters[index] = 0;
}
