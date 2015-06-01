/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Copyright 2009-2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011-2015, Rene Gollent, rene@gollent.com.
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
#include "ValueLocation.h"
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
	X86_64_REGISTER_ST0,
	X86_64_REGISTER_ST1,
	X86_64_REGISTER_ST2,
	X86_64_REGISTER_ST3,
	X86_64_REGISTER_ST4,
	X86_64_REGISTER_ST5,
	X86_64_REGISTER_ST6,
	X86_64_REGISTER_ST7,
	X86_64_REGISTER_MM0,
	X86_64_REGISTER_MM1,
	X86_64_REGISTER_MM2,
	X86_64_REGISTER_MM3,
	X86_64_REGISTER_MM4,
	X86_64_REGISTER_MM5,
	X86_64_REGISTER_MM6,
	X86_64_REGISTER_MM7,
	-1,								// rflags
	X86_64_REGISTER_ES,
	X86_64_REGISTER_CS,
	X86_64_REGISTER_SS,
	X86_64_REGISTER_DS,
	X86_64_REGISTER_FS,
	X86_64_REGISTER_GS,
};

static const int32 kFromDwarfRegisterCount = sizeof(kFromDwarfRegisters) / 4;
static const uint16 kFunctionPrologueSize = 4;


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

		_AddFPRegister(X86_64_REGISTER_ST0, "st0");
		_AddFPRegister(X86_64_REGISTER_ST1, "st1");
		_AddFPRegister(X86_64_REGISTER_ST2, "st2");
		_AddFPRegister(X86_64_REGISTER_ST3, "st3");
		_AddFPRegister(X86_64_REGISTER_ST4, "st4");
		_AddFPRegister(X86_64_REGISTER_ST5, "st5");
		_AddFPRegister(X86_64_REGISTER_ST6, "st6");
		_AddFPRegister(X86_64_REGISTER_ST7, "st7");

		_AddSIMDRegister(X86_64_REGISTER_MM0, "mm0", sizeof(uint64));
		_AddSIMDRegister(X86_64_REGISTER_MM1, "mm1", sizeof(uint64));
		_AddSIMDRegister(X86_64_REGISTER_MM2, "mm2", sizeof(uint64));
		_AddSIMDRegister(X86_64_REGISTER_MM3, "mm3", sizeof(uint64));
		_AddSIMDRegister(X86_64_REGISTER_MM4, "mm4", sizeof(uint64));
		_AddSIMDRegister(X86_64_REGISTER_MM5, "mm5", sizeof(uint64));
		_AddSIMDRegister(X86_64_REGISTER_MM6, "mm6", sizeof(uint64));
		_AddSIMDRegister(X86_64_REGISTER_MM7, "mm7", sizeof(uint64));

		_AddSIMDRegister(X86_64_REGISTER_XMM0, "xmm0",
			sizeof(x86_64_xmm_register));
		_AddSIMDRegister(X86_64_REGISTER_XMM1, "xmm1",
			sizeof(x86_64_xmm_register));
		_AddSIMDRegister(X86_64_REGISTER_XMM2, "xmm2",
			sizeof(x86_64_xmm_register));
		_AddSIMDRegister(X86_64_REGISTER_XMM3, "xmm3",
			sizeof(x86_64_xmm_register));
		_AddSIMDRegister(X86_64_REGISTER_XMM4, "xmm4",
			sizeof(x86_64_xmm_register));
		_AddSIMDRegister(X86_64_REGISTER_XMM5, "xmm5",
			sizeof(x86_64_xmm_register));
		_AddSIMDRegister(X86_64_REGISTER_XMM6, "xmm6",
			sizeof(x86_64_xmm_register));
		_AddSIMDRegister(X86_64_REGISTER_XMM7, "xmm7",
			sizeof(x86_64_xmm_register));
		_AddSIMDRegister(X86_64_REGISTER_XMM8, "xmm8",
			sizeof(x86_64_xmm_register));
		_AddSIMDRegister(X86_64_REGISTER_XMM9, "xmm9",
			sizeof(x86_64_xmm_register));
		_AddSIMDRegister(X86_64_REGISTER_XMM10, "xmm10",
			sizeof(x86_64_xmm_register));
		_AddSIMDRegister(X86_64_REGISTER_XMM11, "xmm11",
			sizeof(x86_64_xmm_register));
		_AddSIMDRegister(X86_64_REGISTER_XMM12, "xmm12",
			sizeof(x86_64_xmm_register));
		_AddSIMDRegister(X86_64_REGISTER_XMM13, "xmm13",
			sizeof(x86_64_xmm_register));
		_AddSIMDRegister(X86_64_REGISTER_XMM14, "xmm14",
			sizeof(x86_64_xmm_register));
		_AddSIMDRegister(X86_64_REGISTER_XMM15, "xmm15",
			sizeof(x86_64_xmm_register));

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
ArchitectureX8664::GetCpuFeatures(uint32& flags)
{
	// TODO: implement if/when it winds up being needed.
	flags = 0;
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
	CpuStateX8664* cpuState = dynamic_cast<CpuStateX8664*>(_cpuState);
	uint64 framePointer = cpuState->IntRegisterValue(X86_64_REGISTER_RBP);
	uint64 rip = cpuState->IntRegisterValue(X86_64_REGISTER_RIP);

	bool readStandardFrame = true;
	uint64 previousFramePointer = 0;
	uint64 returnAddress = 0;

	// check for syscall frames
	stack_frame_type frameType;
	bool hasPrologue = false;
	if (isTopFrame && cpuState->InterruptVector() == 99) {
		// The thread is performing a syscall. So this frame is not really the
		// top-most frame and we need to adjust the rip.
		frameType = STACK_FRAME_TYPE_SYSCALL;
		rip -= 2;
			// int 99, sysenter, and syscall all are 2 byte instructions

		// The syscall stubs are frameless, the return address is on top of the
		// stack.
		uint64 rsp = cpuState->IntRegisterValue(X86_64_REGISTER_RSP);
		uint64 address;
		if (fTeamMemory->ReadMemory(rsp, &address, 8) == 8) {
			returnAddress = address;
			previousFramePointer = framePointer;
			framePointer = 0;
			readStandardFrame = false;
		}
	} else {
		hasPrologue = _HasFunctionPrologue(function);
		if (hasPrologue)
			frameType = STACK_FRAME_TYPE_STANDARD;
		else
			frameType = STACK_FRAME_TYPE_FRAMELESS;
		// TODO: Handling for frameless functions. It's not trivial to find the
		// return address on the stack, though.

		// If the function is not frameless and we're at the top frame we need
		// to check whether the prologue has not been executed (completely) or
		// we're already after the epilogue.
		if (isTopFrame) {
			uint64 stack = 0;
			if (hasPrologue) {
				if (rip < function->Address() + kFunctionPrologueSize) {
					// The prologue has not been executed yet, i.e. there's no
					// stack frame yet. Get the return address from the stack.
					stack = cpuState->IntRegisterValue(X86_64_REGISTER_RSP);
					if (rip > function->Address()) {
						// The "push %rbp" has already been executed.
						stack += 8;
					}
				} else {
					// Not in the function prologue, but maybe after the
					// epilogue. The epilogue is a single "pop %rbp", so we
					// check whether the current instruction is already a
					// "ret".
					uint8 code[1];
					if (fTeamMemory->ReadMemory(rip, &code, 1) == 1
						&& code[0] == 0xc3) {
						stack = cpuState->IntRegisterValue(
							X86_64_REGISTER_RSP);
					}
				}
			} else {
				// Check if the instruction pointer is at a readable location.
				// If it isn't, then chances are we got here via a bogus
				// function pointer, and the prologue hasn't actually been
				// executed. In such a case, what we need is right at the top
				// of the stack.
				uint8 data[1];
				if (fTeamMemory->ReadMemory(rip, &data, 1) != 1)
					stack = cpuState->IntRegisterValue(X86_64_REGISTER_RSP);
			}

			if (stack != 0) {
				uint64 address;
				if (fTeamMemory->ReadMemory(stack, &address, 8) == 8) {
					returnAddress = address;
					previousFramePointer = framePointer;
					framePointer = 0;
					readStandardFrame = false;
					frameType = STACK_FRAME_TYPE_FRAMELESS;
				}
			}
		}
	}

	// create the stack frame
	StackFrameDebugInfo* stackFrameDebugInfo
		= new(std::nothrow) NoOpStackFrameDebugInfo;
	if (stackFrameDebugInfo == NULL)
		return B_NO_MEMORY;
	BReference<StackFrameDebugInfo> stackFrameDebugInfoReference(
		stackFrameDebugInfo, true);

	StackFrame* frame = new(std::nothrow) StackFrame(frameType, cpuState,
		framePointer, rip, stackFrameDebugInfo);
	if (frame == NULL)
		return B_NO_MEMORY;
	BReference<StackFrame> frameReference(frame, true);

	status_t error = frame->Init();
	if (error != B_OK)
		return error;

	// read the previous frame and return address, if this is a standard frame
	if (readStandardFrame) {
		uint64 frameData[2];
		if (framePointer != 0
			&& fTeamMemory->ReadMemory(framePointer, frameData, 16) == 16) {
			previousFramePointer = frameData[0];
			returnAddress = frameData[1];
		}
	}

	// create the CPU state, if we have any info
	CpuStateX8664* previousCpuState = NULL;
	if (returnAddress != 0) {
		// prepare the previous CPU state
		previousCpuState = new(std::nothrow) CpuStateX8664;
		if (previousCpuState == NULL)
			return B_NO_MEMORY;

		previousCpuState->SetIntRegister(X86_64_REGISTER_RBP,
			previousFramePointer);
		previousCpuState->SetIntRegister(X86_64_REGISTER_RIP, returnAddress);
		frame->SetPreviousCpuState(previousCpuState);
	}

	frame->SetReturnAddress(returnAddress);

	_frame = frameReference.Detach();
	_previousCpuState = previousCpuState;
	return B_OK;
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
ArchitectureX8664::ResolvePICFunctionAddress(target_addr_t instructionAddress,
	CpuState* state, target_addr_t& _targetAddress)
{
	target_addr_t previousIP = state->InstructionPointer();
	// if the function in question is position-independent, the call
	// will actually have taken us to its corresponding PLT slot.
	// in such a case, look at the disassembled jump to determine
	// where to find the actual function address.
	InstructionInfo info;
	if (GetInstructionInfo(instructionAddress, info, state) != B_OK)
		return B_BAD_VALUE;

	// x86-64 is likely to use a RIP-relative jump here
	// as such, set our instruction pointer to the address
	// after this instruction (where it would be during actual
	// execution), and recalculate the target address of the jump
	state->SetInstructionPointer(info.Address() + info.Size());
	status_t result = GetInstructionInfo(info.Address(), info, state);
	state->SetInstructionPointer(previousIP);
	if (result != B_OK)
		return result;

	target_addr_t subroutineAddress;
	ssize_t bytesRead = fTeamMemory->ReadMemory(info.TargetAddress(),
		&subroutineAddress, fAddressSize);

	if (bytesRead != fAddressSize)
		return B_BAD_VALUE;

	_targetAddress = subroutineAddress;
	return B_OK;
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
	target_size_t valueSize, ValueLocation*& _location)
{
	// for the calling conventions currently in use on Haiku,
	// the x86-64 rules for how values are returned are as follows:
	//
	// - 64 bit or smaller values are returned in RAX.
	// - > 64 bit values are returned on the stack.
	ValueLocation* location = new(std::nothrow) ValueLocation(
		IsBigEndian());
	if (location == NULL)
		return B_NO_MEMORY;
	BReference<ValueLocation> locationReference(location,
		true);

	if (valueSize <= 8) {
		ValuePieceLocation piece;
		piece.SetSize(valueSize);
		piece.SetToRegister(X86_64_REGISTER_RAX);
		if (!location->AddPiece(piece))
			return B_NO_MEMORY;
	} else {
		ValuePieceLocation piece;
		CpuStateX8664* state = dynamic_cast<CpuStateX8664*>(frame->GetCpuState());
		piece.SetToMemory(state->IntRegisterValue(X86_64_REGISTER_RAX));
		piece.SetSize(valueSize);
		if (!location->AddPiece(piece))
			return B_NO_MEMORY;
	}

	_location = locationReference.Detach();
	return B_OK;
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


void
ArchitectureX8664::_AddFPRegister(int32 index, const char* name)
{
	_AddRegister(index, name, 8 * BVariant::SizeOfType(B_DOUBLE_TYPE),
		B_DOUBLE_TYPE, REGISTER_TYPE_GENERAL_PURPOSE, true);
}


void
ArchitectureX8664::_AddSIMDRegister(int32 index, const char* name,
	uint32 byteSize)
{
	_AddRegister(index, name, byteSize * 8, B_RAW_TYPE,
		REGISTER_TYPE_GENERAL_PURPOSE, true);
}


bool
ArchitectureX8664::_HasFunctionPrologue(FunctionDebugInfo* function) const
{
	if (function == NULL)
		return false;

	// check whether the function has the typical prologue
	if (function->Size() < kFunctionPrologueSize)
		return false;

	uint8 buffer[kFunctionPrologueSize];
	if (fTeamMemory->ReadMemory(function->Address(), buffer,
			kFunctionPrologueSize) != kFunctionPrologueSize) {
		return false;
	}

	return buffer[0] == 0x55 && buffer[1] == 0x48 && buffer[2] == 0x89
		&& buffer[3] == 0xe5;
}
