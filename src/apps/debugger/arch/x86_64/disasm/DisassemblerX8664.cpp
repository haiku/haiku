/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Copyright 2009-2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2008, François Revol, revol@free.fr
 * Distributed under the terms of the MIT License.
 */

#include "DisassemblerX8664.h"

#include <new>

#include "udis86.h"

#include <OS.h>


#include "CpuStateX8664.h"
#include "InstructionInfo.h"


static uint8 RegisterNumberFromUdisIndex(int32 udisIndex)
{
	switch (udisIndex) {
		case UD_R_RIP: return X86_64_REGISTER_RIP;
		case UD_R_RSP: return X86_64_REGISTER_RSP;
		case UD_R_RBP: return X86_64_REGISTER_RBP;

		case UD_R_RAX: return X86_64_REGISTER_RAX;
		case UD_R_RBX: return X86_64_REGISTER_RBX;
		case UD_R_RCX: return X86_64_REGISTER_RCX;
		case UD_R_RDX: return X86_64_REGISTER_RDX;

		case UD_R_RSI: return X86_64_REGISTER_RSI;
		case UD_R_RDI: return X86_64_REGISTER_RDI;

		case UD_R_R8: return X86_64_REGISTER_R8;
		case UD_R_R9: return X86_64_REGISTER_R9;
		case UD_R_R10: return X86_64_REGISTER_R10;
		case UD_R_R11: return X86_64_REGISTER_R11;
		case UD_R_R12: return X86_64_REGISTER_R12;
		case UD_R_R13: return X86_64_REGISTER_R13;
		case UD_R_R14: return X86_64_REGISTER_R14;
		case UD_R_R15: return X86_64_REGISTER_R15;

		case UD_R_CS: return X86_64_REGISTER_CS;
		case UD_R_DS: return X86_64_REGISTER_DS;
		case UD_R_ES: return X86_64_REGISTER_ES;
		case UD_R_FS: return X86_64_REGISTER_FS;
		case UD_R_GS: return X86_64_REGISTER_GS;
		case UD_R_SS: return X86_64_REGISTER_SS;
	}

	return X86_64_INT_REGISTER_END;
}


struct DisassemblerX8664::UdisData : ud_t {
};


DisassemblerX8664::DisassemblerX8664()
	:
	fAddress(0),
	fCode(NULL),
	fCodeSize(0),
	fUdisData(NULL)
{
}


DisassemblerX8664::~DisassemblerX8664()
{
	delete fUdisData;
}


status_t
DisassemblerX8664::Init(target_addr_t address, const void* code, size_t codeSize)
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
	ud_set_mode(fUdisData, 64);
	ud_set_pc(fUdisData, (uint64_t)fAddress);
	ud_set_syntax(fUdisData, UD_SYN_ATT);
	ud_set_vendor(fUdisData, UD_VENDOR_INTEL);
		// TODO: Set the correct vendor!

	return B_OK;
}


status_t
DisassemblerX8664::GetNextInstruction(BString& line, target_addr_t& _address,
	target_size_t& _size, bool& _breakpointAllowed)
{
	unsigned int size = ud_disassemble(fUdisData);
	if (size < 1)
		return B_ENTRY_NOT_FOUND;

	uint64 address = ud_insn_off(fUdisData);

	char buffer[256];
	snprintf(buffer, sizeof(buffer), "0x%08" B_PRIx64 ": %16.16s  %s", address,
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
DisassemblerX8664::GetPreviousInstruction(target_addr_t nextAddress,
	target_addr_t& _address, target_size_t& _size)
{
	if (nextAddress < fAddress || nextAddress > fAddress + fCodeSize)
		return B_BAD_VALUE;

	// loop until hitting the last instruction
	while (true) {
		target_size_t size = ud_disassemble(fUdisData);
		if (size < 1)
			return B_ENTRY_NOT_FOUND;

		target_addr_t address = ud_insn_off(fUdisData);
		if (address + size == nextAddress) {
			_address = address;
			_size = size;
			return B_OK;
		}
	}
}


status_t
DisassemblerX8664::GetNextInstructionInfo(InstructionInfo& _info,
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
DisassemblerX8664::GetInstructionTargetAddress(CpuState* state) const
{
	if (fUdisData->mnemonic != UD_Icall && fUdisData->mnemonic != UD_Ijmp)
		return 0;

	CpuStateX8664* x64State = dynamic_cast<CpuStateX8664*>(state);
	if (x64State == NULL)
		return 0;

	target_addr_t targetAddress = 0;
	switch (fUdisData->operand[0].type) {
		case UD_OP_REG:
		{
			targetAddress = x64State->IntRegisterValue(
				RegisterNumberFromUdisIndex(fUdisData->operand[0].base));
			targetAddress += fUdisData->operand[0].offset;
		}
		break;
		case UD_OP_MEM:
		{
			targetAddress = x64State->IntRegisterValue(
				RegisterNumberFromUdisIndex(fUdisData->operand[0].base));
			targetAddress += x64State->IntRegisterValue(
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
