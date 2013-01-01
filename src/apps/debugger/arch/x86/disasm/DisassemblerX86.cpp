/*
 * Copyright 2009-2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2008, François Revol, revol@free.fr
 * Distributed under the terms of the MIT License.
 */

#include "DisassemblerX86.h"

#include <new>

#include "udis86.h"

#include <OS.h>

#include "CpuStateX86.h"
#include "InstructionInfo.h"


static uint8 RegisterNumberFromUdisIndex(int32 udisIndex)
{
	switch (udisIndex) {
		case UD_R_RIP: return X86_REGISTER_EIP;
		case UD_R_ESP: return X86_REGISTER_ESP;
		case UD_R_EBP: return X86_REGISTER_EBP;

		case UD_R_EAX: return X86_REGISTER_EAX;
		case UD_R_EBX: return X86_REGISTER_EBX;
		case UD_R_ECX: return X86_REGISTER_ECX;
		case UD_R_EDX: return X86_REGISTER_EDX;

		case UD_R_ESI: return X86_REGISTER_ESI;
		case UD_R_EDI: return X86_REGISTER_EDI;

		case UD_R_CS: return X86_REGISTER_CS;
		case UD_R_DS: return X86_REGISTER_DS;
		case UD_R_ES: return X86_REGISTER_ES;
		case UD_R_FS: return X86_REGISTER_FS;
		case UD_R_GS: return X86_REGISTER_GS;
		case UD_R_SS: return X86_REGISTER_SS;
	}

	return X86_INT_REGISTER_END;
}


struct DisassemblerX86::UdisData : ud_t {
};


DisassemblerX86::DisassemblerX86()
	:
	fAddress(0),
	fCode(NULL),
	fCodeSize(0),
	fUdisData(NULL)
{
}


DisassemblerX86::~DisassemblerX86()
{
	delete fUdisData;
}


status_t
DisassemblerX86::Init(target_addr_t address, const void* code, size_t codeSize)
{
	// unset old data
	delete fUdisData;
	fUdisData = NULL;

	// set new data
	fUdisData = new(std::nothrow) UdisData;
	if (fUdisData == NULL)
		return B_NO_MEMORY;

	fAddress = address;
	fCode = (const uint8*)code;
	fCodeSize = codeSize;

	// init udis
	ud_init(fUdisData);
	ud_set_input_buffer(fUdisData, (unsigned char*)fCode, fCodeSize);
	ud_set_mode(fUdisData, 32);
	ud_set_pc(fUdisData, (uint64_t)fAddress);
	ud_set_syntax(fUdisData, UD_SYN_ATT);
	ud_set_vendor(fUdisData, UD_VENDOR_INTEL);
		// TODO: Set the correct vendor!

	return B_OK;
}


status_t
DisassemblerX86::GetNextInstruction(BString& line, target_addr_t& _address,
	target_size_t& _size, bool& _breakpointAllowed)
{
	unsigned int size = ud_disassemble(fUdisData);
	if (size < 1)
		return B_ENTRY_NOT_FOUND;

	uint32 address = (uint32)ud_insn_off(fUdisData);

	char buffer[256];
	snprintf(buffer, sizeof(buffer), "0x%08" B_PRIx32 ": %16.16s  %s", address,
		ud_insn_hex(fUdisData), ud_insn_asm(fUdisData));
			// TODO: Resolve symbols!

	line = buffer;
	_address = address;
	_size = size;
	_breakpointAllowed = true;
		// TODO: Implement (rep!)!

	return B_OK;
}


status_t
DisassemblerX86::GetPreviousInstruction(target_addr_t nextAddress,
	target_addr_t& _address, target_size_t& _size)
{
	if (nextAddress < fAddress || nextAddress > fAddress + fCodeSize)
		return B_BAD_VALUE;

	// loop until hitting the last instruction
	while (true) {
		unsigned int size = ud_disassemble(fUdisData);
		if (size < 1)
			return B_ENTRY_NOT_FOUND;

		uint32 address = (uint32)ud_insn_off(fUdisData);
		if (address + size == nextAddress) {
			_address = address;
			_size = size;
			return B_OK;
		}
	}
}


status_t
DisassemblerX86::GetNextInstructionInfo(InstructionInfo& _info,
	CpuState* state)
{
	unsigned int size = ud_disassemble(fUdisData);
	if (size < 1)
		return B_ENTRY_NOT_FOUND;

	uint32 address = (uint32)ud_insn_off(fUdisData);

	instruction_type type = INSTRUCTION_TYPE_OTHER;
	target_addr_t targetAddress = 0;

	if (fUdisData->mnemonic == UD_Icall)
		type = INSTRUCTION_TYPE_SUBROUTINE_CALL;
	else if (fUdisData->mnemonic == UD_Ijmp)
		type = INSTRUCTION_TYPE_JUMP;
	if (state != NULL)
		targetAddress = GetInstructionTargetAddress(state);

	char buffer[256];
	snprintf(buffer, sizeof(buffer), "0x%08" B_PRIx32 ": %16.16s  %s", address,
		ud_insn_hex(fUdisData), ud_insn_asm(fUdisData));
			// TODO: Resolve symbols!

	if (!_info.SetTo(address, targetAddress, size, type, true, buffer))
		return B_NO_MEMORY;

	return B_OK;
}


target_addr_t
DisassemblerX86::GetInstructionTargetAddress(CpuState* state) const
{
	if (fUdisData->mnemonic != UD_Icall && fUdisData->mnemonic != UD_Ijmp)
		return 0;

	CpuStateX86* x86State = dynamic_cast<CpuStateX86*>(state);
	if (x86State == NULL)
		return 0;

	target_addr_t targetAddress = 0;
	switch (fUdisData->operand[0].type) {
		case UD_OP_REG:
		{
			targetAddress = x86State->IntRegisterValue(
				RegisterNumberFromUdisIndex(fUdisData->operand[0].base));
			targetAddress += fUdisData->operand[0].offset;
		}
		break;
		case UD_OP_MEM:
		{
			targetAddress = x86State->IntRegisterValue(
				RegisterNumberFromUdisIndex(fUdisData->operand[0].base));
			targetAddress += x86State->IntRegisterValue(
				RegisterNumberFromUdisIndex(fUdisData->operand[0].index))
				* fUdisData->operand[0].scale;
		}
		break;
		case UD_OP_JIMM:
		{
			targetAddress = ud_insn_off(fUdisData)
				+ fUdisData->operand[0].lval.sdword + ud_insn_len(fUdisData);
		}
		break;

		case UD_OP_IMM:
		case UD_OP_CONST:
		{
			targetAddress = fUdisData->operand[0].lval.udword;
		}
		break;

		default:
		break;
	}

	return targetAddress;
}
