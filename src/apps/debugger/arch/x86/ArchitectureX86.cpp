/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "ArchitectureX86.h"

#include <new>

#include <String.h>

#include <AutoDeleter.h>

#include "CpuStateX86.h"
#include "DisassembledCode.h"
#include "FunctionDebugInfo.h"
#include "InstructionInfo.h"
#include "StackFrame.h"
#include "Statement.h"
#include "TeamMemory.h"

#include "disasm/DisassemblerX86.h"


ArchitectureX86::ArchitectureX86(TeamMemory* teamMemory)
	:
	Architecture(teamMemory)
{
}


ArchitectureX86::~ArchitectureX86()
{
}


status_t
ArchitectureX86::Init()
{
	try {
		_AddIntegerRegister(X86_REGISTER_EIP, "eip", 32,
			REGISTER_TYPE_INSTRUCTION_POINTER);
		_AddIntegerRegister(X86_REGISTER_ESP, "esp", 32,
			REGISTER_TYPE_STACK_POINTER);
		_AddIntegerRegister(X86_REGISTER_EBP, "ebp", 32,
			REGISTER_TYPE_GENERAL_PURPOSE);

		_AddIntegerRegister(X86_REGISTER_EAX, "eax", 32,
			REGISTER_TYPE_GENERAL_PURPOSE);
		_AddIntegerRegister(X86_REGISTER_EBX, "ebx", 32,
			REGISTER_TYPE_GENERAL_PURPOSE);
		_AddIntegerRegister(X86_REGISTER_ECX, "ecx", 32,
			REGISTER_TYPE_GENERAL_PURPOSE);
		_AddIntegerRegister(X86_REGISTER_EDX, "edx", 32,
			REGISTER_TYPE_GENERAL_PURPOSE);

		_AddIntegerRegister(X86_REGISTER_ESI, "esi", 32,
			REGISTER_TYPE_GENERAL_PURPOSE);
		_AddIntegerRegister(X86_REGISTER_EDI, "edi", 32,
			REGISTER_TYPE_GENERAL_PURPOSE);

		_AddIntegerRegister(X86_REGISTER_CS, "cs", 16,
			REGISTER_TYPE_SPECIAL_PURPOSE);
		_AddIntegerRegister(X86_REGISTER_DS, "ds", 16,
			REGISTER_TYPE_SPECIAL_PURPOSE);
		_AddIntegerRegister(X86_REGISTER_ES, "es", 16,
			REGISTER_TYPE_SPECIAL_PURPOSE);
		_AddIntegerRegister(X86_REGISTER_FS, "fs", 16,
			REGISTER_TYPE_SPECIAL_PURPOSE);
		_AddIntegerRegister(X86_REGISTER_GS, "gs", 16,
			REGISTER_TYPE_SPECIAL_PURPOSE);
		_AddIntegerRegister(X86_REGISTER_SS, "ss", 16,
			REGISTER_TYPE_SPECIAL_PURPOSE);
	} catch (std::bad_alloc) {
		return B_NO_MEMORY;
	}

	return B_OK;
}


int32
ArchitectureX86::CountRegisters() const
{
	return fRegisters.Count();
}


const Register*
ArchitectureX86::Registers() const
{
	return fRegisters.Elements();
}


status_t
ArchitectureX86::CreateCpuState(const void* cpuStateData, size_t size,
	CpuState*& _state)
{
	if (size != sizeof(debug_cpu_state_x86))
		return B_BAD_VALUE;

	CpuStateX86* state = new(std::nothrow) CpuStateX86(
		*(const debug_cpu_state_x86*)cpuStateData);
	if (state == NULL)
		return B_NO_MEMORY;

	_state = state;
	return B_OK;
}


status_t
ArchitectureX86::CreateStackFrame(Image* image, FunctionDebugInfo* function,
	CpuState* _cpuState, bool isTopFrame, StackFrame*& _previousFrame,
	CpuState*& _previousCpuState)
{
	CpuStateX86* cpuState = dynamic_cast<CpuStateX86*>(_cpuState);

	uint32 framePointer = cpuState->IntRegisterValue(X86_REGISTER_EBP);
	uint32 eip = cpuState->IntRegisterValue(X86_REGISTER_EIP);

	bool readStandardFrame = true;
	uint32 previousFramePointer = 0;
	uint32 returnAddress = 0;

	// check for syscall frames
	stack_frame_type frameType;
	bool hasPrologue = false;
	if (isTopFrame && cpuState->InterruptVector() == 99) {
		// The thread is performing a syscall. So this frame is not really the
		// top-most frame and we need to adjust the eip.
		frameType = STACK_FRAME_TYPE_SYSCALL;
		eip -= 2;
			// int 99, sysenter, and syscall all are 2 byte instructions

		// The syscall stubs are frameless, the return address is on top of the
		// stack.
		uint32 esp = cpuState->IntRegisterValue(X86_REGISTER_ESP);
		uint32 address;
		if (fTeamMemory->ReadMemory(esp, &address, 4) == 4) {
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
		if (hasPrologue && isTopFrame) {
			uint32 stack = 0;
			if (eip < function->Address() + 3) {
				// The prologue has not been executed yet, i.e. there's no
				// stack frame yet. Get the return address from the stack.
				stack = cpuState->IntRegisterValue(X86_REGISTER_ESP);
				if (eip > function->Address()) {
					// The "push %ebp" has already been executed.
					stack += 4;
				}
			} else {
				// Not in the function prologue, but maybe after the epilogue.
				// The epilogue is a single "pop %ebp", so we check whether the
				// current instruction is already a "ret".
				uint8 code[1];
				if (fTeamMemory->ReadMemory(eip, &code, 1) == 1
					&& code[0] == 0xc3) {
					stack = cpuState->IntRegisterValue(X86_REGISTER_ESP);
				}
			}

			if (stack != 0) {
				uint32 address;
				if (fTeamMemory->ReadMemory(stack, &address, 4) == 4) {
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
	StackFrame* frame = new(std::nothrow) StackFrame(frameType, cpuState,
		framePointer, eip);
	if (frame == NULL)
		return B_NO_MEMORY;
	Reference<StackFrame> frameReference(frame, true);

	// read the previous frame and return address, if this is a standard frame
	if (readStandardFrame) {
		uint32 frameData[2];
		if (framePointer != 0
			&& fTeamMemory->ReadMemory(framePointer, frameData, 8) == 8) {
			previousFramePointer = frameData[0];
			returnAddress = frameData[1];
		}
	}

	// create the CPU state, if we have any info
	CpuStateX86* previousCpuState = NULL;
	if (returnAddress != 0) {
		// prepare the previous CPU state
		previousCpuState = new(std::nothrow) CpuStateX86;
		if (previousCpuState == NULL)
			return B_NO_MEMORY;

		previousCpuState->SetIntRegister(X86_REGISTER_EBP,
			previousFramePointer);
		previousCpuState->SetIntRegister(X86_REGISTER_EIP, returnAddress);
	}

	frame->SetReturnAddress(returnAddress);

	_previousFrame = frameReference.Detach();
	_previousCpuState = previousCpuState;
	return B_OK;
}


void
ArchitectureX86::UpdateStackFrameCpuState(const StackFrame* frame,
	Image* previousImage, FunctionDebugInfo* previousFunction,
	CpuState* previousCpuState)
{
	// This is not a top frame, so we want to offset eip to the previous
	// (calling) instruction.
	CpuStateX86* cpuState = dynamic_cast<CpuStateX86*>(previousCpuState);

	// get eip
	uint32 eip = cpuState->IntRegisterValue(X86_REGISTER_EIP);
	if (previousFunction == NULL || eip <= previousFunction->Address())
		return;
	target_addr_t functionAddress = previousFunction->Address();

	// allocate a buffer for the function code to disassemble
	size_t bufferSize = eip - functionAddress;
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
	DisassemblerX86 disassembler;
	target_addr_t instructionAddress;
	target_size_t instructionSize;
	if (disassembler.Init(functionAddress, buffer, bufferSize) == B_OK
		&& disassembler.GetPreviousInstruction(eip, instructionAddress,
			instructionSize) == B_OK) {
		eip -= instructionSize;
		cpuState->SetIntRegister(X86_REGISTER_EIP, eip);
	}
}


status_t
ArchitectureX86::DisassembleCode(FunctionDebugInfo* function,
	const void* buffer, size_t bufferSize, DisassembledCode*& _sourceCode)
{
	DisassembledCode* source = new(std::nothrow) DisassembledCode;
	if (source == NULL)
		return B_NO_MEMORY;
	Reference<DisassembledCode> sourceReference(source, true);

	// init disassembler
	DisassemblerX86 disassembler;
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
ArchitectureX86::GetStatement(FunctionDebugInfo* function,
	target_addr_t address, Statement*& _statement)
{
// TODO: This is not architecture dependent anymore!
	// get the instruction info
	InstructionInfo info;
	status_t error = GetInstructionInfo(address, info);
	if (error != B_OK)
		return error;

	// create a statement
	ContiguousStatement* statement = new(std::nothrow) ContiguousStatement(
		SourceLocation(0), TargetAddressRange(info.Address(), info.Size()));
	if (statement == NULL)
		return B_NO_MEMORY;

	_statement = statement;
	return B_OK;
}


status_t
ArchitectureX86::GetInstructionInfo(target_addr_t address,
	InstructionInfo& _info)
{
	// read the code
	uint8 buffer[16];
		// TODO: What's the maximum instruction size?
	ssize_t bytesRead = fTeamMemory->ReadMemory(address, buffer,
		sizeof(buffer));
	if (bytesRead < 0)
		return bytesRead;

	// init disassembler
	DisassemblerX86 disassembler;
	status_t error = disassembler.Init(address, buffer, bytesRead);
	if (error != B_OK)
		return error;

	// disassemble the instruction
	BString line;
	target_addr_t instructionAddress;
	target_size_t instructionSize;
	bool breakpointAllowed;
	error = disassembler.GetNextInstruction(line, instructionAddress,
		instructionSize, breakpointAllowed);
	if (error != B_OK)
		return error;

	instruction_type instructionType = INSTRUCTION_TYPE_OTHER;
	if (buffer[0] == 0xff) {
		// absolute call with r/m32
		instructionType = INSTRUCTION_TYPE_SUBROUTINE_CALL;
	} else if (buffer[0] == 0xe8 && instructionSize == 5) {
		// relative call with rel32 -- don't categorize the call with 0 as
		// subroutine call, since it is only used to get the address of the GOT
		if (buffer[1] != 0 || buffer[2] != 0 || buffer[3] != 0
			|| buffer[4] != 0) {
			instructionType = INSTRUCTION_TYPE_SUBROUTINE_CALL;
		}
	}

	if (!_info.SetTo(instructionAddress, instructionSize, instructionType,
			breakpointAllowed, line)) {
		return B_NO_MEMORY;
	}

	return B_OK;
}


void
ArchitectureX86::_AddRegister(int32 index, const char* name,
	register_format format, uint32 bitSize, register_type type)
{
	if (!fRegisters.Add(Register(index, name, format, bitSize, type)))
		throw std::bad_alloc();
}


void
ArchitectureX86::_AddIntegerRegister(int32 index, const char* name,
	uint32 bitSize, register_type type)
{
	_AddRegister(index, name, REGISTER_FORMAT_INTEGER, bitSize, type);
}


bool
ArchitectureX86::_HasFunctionPrologue(FunctionDebugInfo* function) const
{
	if (function == NULL)
		return false;

	// check whether the function has the typical prologue
	if (function->Size() < 3)
		return false;

	uint8 buffer[3];
	if (fTeamMemory->ReadMemory(function->Address(), buffer, 3) != 3)
		return false;

	return buffer[0] == 0x55 && buffer[1] == 0x89 && buffer[2] == 0xe5;
}
