/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Copyright 2009-2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011-2012, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "ArchitectureX8664.h"

#include <new>

#include <String.h>

#include <AutoDeleter.h>

#include "CfaContext.h"
#include "CpuStateX8664.h"
#include "DisassembledCode.h"
#include "FunctionDebugInfo.h"
#include "InstructionInfo.h"
#include "NoOpStackFrameDebugInfo.h"
#include "RegisterMap.h"
#include "StackFrame.h"
#include "Statement.h"
#include "TeamMemory.h"
#include "X86AssemblyLanguage.h"

#include "disasm/DisassemblerX8664.h"


static const int32 kFromDwarfRegisters[] = {
	X86_64_REGISTER_RAX,
	X86_64_REGISTER_RDX,
	X86_64_REGISTER_RCX,
	X86_64_REGISTER_RBX,
	X86_64_REGISTER_RSI,
	X86_64_REGISTER_RDI,
	X86_64_REGISTER_RBP,
	X86_64_REGISTER_RSP,
	X86_64_REGISTER_R8,
	X86_64_REGISTER_R9,
	X86_64_REGISTER_R10,
	X86_64_REGISTER_R11,
	X86_64_REGISTER_R12,
	X86_64_REGISTER_R13,
	X86_64_REGISTER_R14,
	X86_64_REGISTER_R15,
	X86_64_REGISTER_RIP,
	-1, -1, -1, -1, -1, -1, -1, -1,	// xmm0-xmm7
	-1, -1, -1, -1, -1, -1, -1, -1,	// xmm8-xmm15
	-1, -1, -1, -1, -1, -1, -1, -1,	// st0-st7
	-1, -1, -1, -1, -1, -1, -1, -1,	// mm0-mm7
	-1,								// rflags
	X86_64_REGISTER_ES,
	X86_64_REGISTER_CS,
	X86_64_REGISTER_SS,
	X86_64_REGISTER_DS,
	X86_64_REGISTER_FS,
	X86_64_REGISTER_GS,
};

static const int32 kFromDwarfRegisterCount = sizeof(kFromDwarfRegisters) / 4;


// #pragma mark - ToDwarfRegisterMap


struct ArchitectureX8664::ToDwarfRegisterMap : RegisterMap {
	ToDwarfRegisterMap()
	{
		// init the index array from the reverse map
		memset(fIndices, -1, sizeof(fIndices));
		for (int32 i = 0; i < kFromDwarfRegisterCount; i++) {
			if (kFromDwarfRegisters[i] >= 0)
				fIndices[kFromDwarfRegisters[i]] = i;
		}
	}

	virtual int32 CountRegisters() const
	{
		return X86_64_REGISTER_COUNT;
	}

	virtual int32 MapRegisterIndex(int32 index) const
	{
		return index >= 0 && index < X86_64_REGISTER_COUNT ? fIndices[index] : -1;
	}

private:
	int32	fIndices[X86_64_REGISTER_COUNT];
};


// #pragma mark - FromDwarfRegisterMap


struct ArchitectureX8664::FromDwarfRegisterMap : RegisterMap {
	virtual int32 CountRegisters() const
	{
		return kFromDwarfRegisterCount;
	}

	virtual int32 MapRegisterIndex(int32 index) const
	{
		return index >= 0 && index < kFromDwarfRegisterCount
			? kFromDwarfRegisters[index] : -1;
	}
};


// #pragma mark - ArchitectureX8664


ArchitectureX8664::ArchitectureX8664(TeamMemory* teamMemory)
	:
	Architecture(teamMemory, 8, false),
	fAssemblyLanguage(NULL),
	fToDwarfRegisterMap(NULL),
	fFromDwarfRegisterMap(NULL)
{
}


ArchitectureX8664::~ArchitectureX8664()
{
	if (fToDwarfRegisterMap != NULL)
		fToDwarfRegisterMap->ReleaseReference();
	if (fFromDwarfRegisterMap != NULL)
		fFromDwarfRegisterMap->ReleaseReference();
	if (fAssemblyLanguage != NULL)
		fAssemblyLanguage->ReleaseReference();
}


status_t
ArchitectureX8664::Init()
{
	fAssemblyLanguage = new(std::nothrow) X86AssemblyLanguage;
	if (fAssemblyLanguage == NULL)
		return B_NO_MEMORY;

	try {
		_AddIntegerRegister(X86_64_REGISTER_RIP, "rip", B_UINT64_TYPE,
			REGISTER_TYPE_INSTRUCTION_POINTER, false);
		_AddIntegerRegister(X86_64_REGISTER_RSP, "rsp", B_UINT64_TYPE,
			REGISTER_TYPE_STACK_POINTER, true);
		_AddIntegerRegister(X86_64_REGISTER_RBP, "rbp", B_UINT64_TYPE,
			REGISTER_TYPE_GENERAL_PURPOSE, true);

		_AddIntegerRegister(X86_64_REGISTER_RAX, "rax", B_UINT64_TYPE,
			REGISTER_TYPE_GENERAL_PURPOSE, false);
		_AddIntegerRegister(X86_64_REGISTER_RBX, "rbx", B_UINT64_TYPE,
			REGISTER_TYPE_GENERAL_PURPOSE, true);
		_AddIntegerRegister(X86_64_REGISTER_RCX, "rcx", B_UINT64_TYPE,
			REGISTER_TYPE_GENERAL_PURPOSE, false);
		_AddIntegerRegister(X86_64_REGISTER_RDX, "rdx", B_UINT64_TYPE,
			REGISTER_TYPE_GENERAL_PURPOSE, false);

		_AddIntegerRegister(X86_64_REGISTER_RSI, "rsi", B_UINT64_TYPE,
			REGISTER_TYPE_GENERAL_PURPOSE, false);
		_AddIntegerRegister(X86_64_REGISTER_RDI, "rdi", B_UINT64_TYPE,
			REGISTER_TYPE_GENERAL_PURPOSE, false);

		_AddIntegerRegister(X86_64_REGISTER_R8, "r8", B_UINT64_TYPE,
			REGISTER_TYPE_GENERAL_PURPOSE, false);
		_AddIntegerRegister(X86_64_REGISTER_R9, "r9", B_UINT64_TYPE,
			REGISTER_TYPE_GENERAL_PURPOSE, false);
		_AddIntegerRegister(X86_64_REGISTER_R10, "r10", B_UINT64_TYPE,
			REGISTER_TYPE_GENERAL_PURPOSE, false);
		_AddIntegerRegister(X86_64_REGISTER_R11, "r11", B_UINT64_TYPE,
			REGISTER_TYPE_GENERAL_PURPOSE, false);
		_AddIntegerRegister(X86_64_REGISTER_R12, "r12", B_UINT64_TYPE,
			REGISTER_TYPE_GENERAL_PURPOSE, true);
		_AddIntegerRegister(X86_64_REGISTER_R13, "r13", B_UINT64_TYPE,
			REGISTER_TYPE_GENERAL_PURPOSE, true);
		_AddIntegerRegister(X86_64_REGISTER_R14, "r14", B_UINT64_TYPE,
			REGISTER_TYPE_GENERAL_PURPOSE, true);
		_AddIntegerRegister(X86_64_REGISTER_R15, "r15", B_UINT64_TYPE,
			REGISTER_TYPE_GENERAL_PURPOSE, true);

		_AddIntegerRegister(X86_64_REGISTER_CS, "cs", B_UINT16_TYPE,
			REGISTER_TYPE_SPECIAL_PURPOSE, true);
		_AddIntegerRegister(X86_64_REGISTER_DS, "ds", B_UINT16_TYPE,
			REGISTER_TYPE_SPECIAL_PURPOSE, true);
		_AddIntegerRegister(X86_64_REGISTER_ES, "es", B_UINT16_TYPE,
			REGISTER_TYPE_SPECIAL_PURPOSE, true);
		_AddIntegerRegister(X86_64_REGISTER_FS, "fs", B_UINT16_TYPE,
			REGISTER_TYPE_SPECIAL_PURPOSE, true);
		_AddIntegerRegister(X86_64_REGISTER_GS, "gs", B_UINT16_TYPE,
			REGISTER_TYPE_SPECIAL_PURPOSE, true);
		_AddIntegerRegister(X86_64_REGISTER_SS, "ss", B_UINT16_TYPE,
			REGISTER_TYPE_SPECIAL_PURPOSE, true);
	} catch (std::bad_alloc) {
		return B_NO_MEMORY;
	}

	fToDwarfRegisterMap = new(std::nothrow) ToDwarfRegisterMap;
	fFromDwarfRegisterMap = new(std::nothrow) FromDwarfRegisterMap;

	if (fToDwarfRegisterMap == NULL || fFromDwarfRegisterMap == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


int32
ArchitectureX8664::StackGrowthDirection() const
{
	return STACK_GROWTH_DIRECTION_NEGATIVE;
}


int32
ArchitectureX8664::CountRegisters() const
{
	return fRegisters.Count();
}


const Register*
ArchitectureX8664::Registers() const
{
	return fRegisters.Elements();
}


status_t
ArchitectureX8664::InitRegisterRules(CfaContext& context) const
{
	status_t error = Architecture::InitRegisterRules(context);
	if (error != B_OK)
		return error;

	// set up rule for RIP register
	context.RegisterRule(fToDwarfRegisterMap->MapRegisterIndex(
		X86_64_REGISTER_RIP))->SetToLocationOffset(0);

	return B_OK;
}


status_t
ArchitectureX8664::GetDwarfRegisterMaps(RegisterMap** _toDwarf,
	RegisterMap** _fromDwarf) const
{
	if (_toDwarf != NULL) {
		*_toDwarf = fToDwarfRegisterMap;
		fToDwarfRegisterMap->AcquireReference();
	}

	if (_fromDwarf != NULL) {
		*_fromDwarf = fFromDwarfRegisterMap;
		fFromDwarfRegisterMap->AcquireReference();
	}

	return B_OK;
}


status_t
ArchitectureX8664::CreateCpuState(CpuState*& _state)
{
	CpuStateX8664* state = new(std::nothrow) CpuStateX8664;
	if (state == NULL)
		return B_NO_MEMORY;

	_state = state;
	return B_OK;
}


status_t
ArchitectureX8664::CreateCpuState(const void* cpuStateData, size_t size,
	CpuState*& _state)
{
	if (size != sizeof(x86_64_debug_cpu_state))
		return B_BAD_VALUE;

	CpuStateX8664* state = new(std::nothrow) CpuStateX8664(
		*(const x86_64_debug_cpu_state*)cpuStateData);
	if (state == NULL)
		return B_NO_MEMORY;

	_state = state;
	return B_OK;
}


status_t
ArchitectureX8664::CreateStackFrame(Image* image, FunctionDebugInfo* function,
	CpuState* _cpuState, bool isTopFrame, StackFrame*& _frame,
	CpuState*& _previousCpuState)
{
	fprintf(stderr, "ArchitectureX8664::CreateStackFrame: TODO\n");
	return B_UNSUPPORTED;
}


void
ArchitectureX8664::UpdateStackFrameCpuState(const StackFrame* frame,
	Image* previousImage, FunctionDebugInfo* previousFunction,
	CpuState* previousCpuState)
{
	// This is not a top frame, so we want to offset rip to the previous
	// (calling) instruction.
	CpuStateX8664* cpuState = dynamic_cast<CpuStateX8664*>(previousCpuState);

	// get rip
	uint64 rip = cpuState->IntRegisterValue(X86_64_REGISTER_RIP);
	if (previousFunction == NULL || rip <= previousFunction->Address())
		return;
	target_addr_t functionAddress = previousFunction->Address();

	// allocate a buffer for the function code to disassemble
	size_t bufferSize = rip - functionAddress;
	void* buffer = malloc(bufferSize);
	if (buffer == NULL)
		return;
	MemoryDeleter bufferDeleter(buffer);

	// read the code
	ssize_t bytesRead = fTeamMemory->ReadMemory(functionAddress, buffer,
		bufferSize);
	if (bytesRead != (ssize_t)bufferSize)
		return;

	// disassemble to get the previous instruction
	DisassemblerX8664 disassembler;
	target_addr_t instructionAddress;
	target_size_t instructionSize;
	if (disassembler.Init(functionAddress, buffer, bufferSize) == B_OK
		&& disassembler.GetPreviousInstruction(rip, instructionAddress,
			instructionSize) == B_OK) {
		rip -= instructionSize;
		cpuState->SetIntRegister(X86_64_REGISTER_RIP, rip);
	}
}


status_t
ArchitectureX8664::ReadValueFromMemory(target_addr_t address, uint32 valueType,
	BVariant& _value) const
{
	uint8 buffer[64];
	size_t size = BVariant::SizeOfType(valueType);
	if (size == 0 || size > sizeof(buffer))
		return B_BAD_VALUE;

	ssize_t bytesRead = fTeamMemory->ReadMemory(address, buffer, size);
	if (bytesRead < 0)
		return bytesRead;
	if ((size_t)bytesRead != size)
		return B_ERROR;

	// TODO: We need to swap endianess, if the host is big endian!

	switch (valueType) {
		case B_INT8_TYPE:
			_value.SetTo(*(int8*)buffer);
			return B_OK;
		case B_UINT8_TYPE:
			_value.SetTo(*(uint8*)buffer);
			return B_OK;
		case B_INT16_TYPE:
			_value.SetTo(*(int16*)buffer);
			return B_OK;
		case B_UINT16_TYPE:
			_value.SetTo(*(uint16*)buffer);
			return B_OK;
		case B_INT32_TYPE:
			_value.SetTo(*(int32*)buffer);
			return B_OK;
		case B_UINT32_TYPE:
			_value.SetTo(*(uint32*)buffer);
			return B_OK;
		case B_INT64_TYPE:
			_value.SetTo(*(int64*)buffer);
			return B_OK;
		case B_UINT64_TYPE:
			_value.SetTo(*(uint64*)buffer);
			return B_OK;
		case B_FLOAT_TYPE:
			_value.SetTo(*(float*)buffer);
				// TODO: float on the host might work differently!
			return B_OK;
		case B_DOUBLE_TYPE:
			_value.SetTo(*(double*)buffer);
				// TODO: double on the host might work differently!
			return B_OK;
		default:
			return B_BAD_VALUE;
	}
}


status_t
ArchitectureX8664::ReadValueFromMemory(target_addr_t addressSpace,
	target_addr_t address, uint32 valueType, BVariant& _value) const
{
	// n/a on this architecture
	return B_BAD_VALUE;
}


status_t
ArchitectureX8664::DisassembleCode(FunctionDebugInfo* function,
	const void* buffer, size_t bufferSize, DisassembledCode*& _sourceCode)
{
	DisassembledCode* source = new(std::nothrow) DisassembledCode(
		fAssemblyLanguage);
	if (source == NULL)
		return B_NO_MEMORY;
	BReference<DisassembledCode> sourceReference(source, true);

	// init disassembler
	DisassemblerX8664 disassembler;
	status_t error = disassembler.Init(function->Address(), buffer, bufferSize);
	if (error != B_OK)
		return error;

	// add a function name line
	BString functionName(function->PrettyName());
	if (!source->AddCommentLine((functionName << ':').String()))
		return B_NO_MEMORY;

	// disassemble the instructions
	BString line;
	target_addr_t instructionAddress;
	target_size_t instructionSize;
	bool breakpointAllowed;
	while (disassembler.GetNextInstruction(line, instructionAddress,
				instructionSize, breakpointAllowed) == B_OK) {
// TODO: Respect breakpointAllowed!
		if (!source->AddInstructionLine(line, instructionAddress,
				instructionSize)) {
			return B_NO_MEMORY;
		}
	}

	_sourceCode = sourceReference.Detach();
	return B_OK;
}


status_t
ArchitectureX8664::GetStatement(FunctionDebugInfo* function,
	target_addr_t address, Statement*& _statement)
{
// TODO: This is not architecture dependent anymore!
	// get the instruction info
	InstructionInfo info;
	status_t error = GetInstructionInfo(address, info, NULL);
	if (error != B_OK)
		return error;

	// create a statement
	ContiguousStatement* statement = new(std::nothrow) ContiguousStatement(
		SourceLocation(-1), TargetAddressRange(info.Address(), info.Size()));
	if (statement == NULL)
		return B_NO_MEMORY;

	_statement = statement;
	return B_OK;
}


status_t
ArchitectureX8664::GetInstructionInfo(target_addr_t address,
	InstructionInfo& _info, CpuState* state)
{
	// read the code - maximum x86{-64} instruction size = 15 bytes
	uint8 buffer[16];
	ssize_t bytesRead = fTeamMemory->ReadMemory(address, buffer,
		sizeof(buffer));
	if (bytesRead < 0)
		return bytesRead;

	// init disassembler
	DisassemblerX8664 disassembler;
	status_t error = disassembler.Init(address, buffer, bytesRead);
	if (error != B_OK)
		return error;

	return disassembler.GetNextInstructionInfo(_info, state);
}


status_t
ArchitectureX8664::GetWatchpointDebugCapabilities(int32& _maxRegisterCount,
	int32& _maxBytesPerRegister, uint8& _watchpointCapabilityFlags)
{
	// Have 4 debug registers, 1 is required for breakpoint support, which
	// leaves 3 available for watchpoints.
	_maxRegisterCount = 3;
	_maxBytesPerRegister = 8;

	// x86 only supports write and read/write watchpoints.
	_watchpointCapabilityFlags = WATCHPOINT_CAPABILITY_FLAG_WRITE
		| WATCHPOINT_CAPABILITY_FLAG_READ_WRITE;

	return B_OK;
}


status_t
ArchitectureX8664::GetReturnAddressLocation(StackFrame* frame,
	target_size_t valueSize, ValueLocation*& _location) {
	return B_NOT_SUPPORTED;
}


void
ArchitectureX8664::_AddRegister(int32 index, const char* name,
	uint32 bitSize, uint32 valueType, register_type type, bool calleePreserved)
{
	if (!fRegisters.Add(Register(index, name, bitSize, valueType, type,
			calleePreserved))) {
		throw std::bad_alloc();
	}
}


void
ArchitectureX8664::_AddIntegerRegister(int32 index, const char* name,
	uint32 valueType, register_type type, bool calleePreserved)
{
	_AddRegister(index, name, 8 * BVariant::SizeOfType(valueType), valueType,
		type, calleePreserved);
}
