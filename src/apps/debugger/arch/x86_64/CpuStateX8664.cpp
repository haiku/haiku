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
	fSetRegisters()
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
	newState->fSetRegisters = fSetRegisters;

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

	if (index >= X86_64_INT_REGISTER_END)
		return false;

	if (reg->BitSize() == 16)
		_value.SetTo((uint16)fIntRegisters[index]);
	else
		_value.SetTo(fIntRegisters[index]);

	return true;
}


bool
CpuStateX8664::SetRegisterValue(const Register* reg, const BVariant& value)
{
	int32 index = reg->Index();
	if (index >= X86_64_INT_REGISTER_END)
		return false;

	fIntRegisters[index] = value.ToUInt64();
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


void
CpuStateX8664::UnsetRegister(int32 index)
{
	if (index < 0 || index >= X86_64_REGISTER_COUNT)
		return;

	fSetRegisters[index] = 0;
}
