/*
 * Copyright 2009-2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2008, François Revol, revol@free.fr
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */

#include "DisassemblerX86.h"

#include <new>

#include "Zycore/Format.h"
#include "Zydis/Zydis.h"

#include <OS.h>


#include "CpuStateX86.h"
#include "InstructionInfo.h"


void
CpuStateToZydisRegContext(CpuStateX86* state, ZydisRegisterContext* context)
{
	context->values[ZYDIS_REGISTER_EAX] = state->IntRegisterValue(X86_REGISTER_EAX);
	context->values[ZYDIS_REGISTER_ESP] = state->IntRegisterValue(X86_REGISTER_ESP);
	context->values[ZYDIS_REGISTER_EIP] = state->IntRegisterValue(X86_REGISTER_EIP);
	// context->values[ZYDIS_REGISTER_RFLAGS] = eflags;
	context->values[ZYDIS_REGISTER_ECX] = state->IntRegisterValue(X86_REGISTER_ECX);
	context->values[ZYDIS_REGISTER_EDX] = state->IntRegisterValue(X86_REGISTER_EDX);
	context->values[ZYDIS_REGISTER_EBX] = state->IntRegisterValue(X86_REGISTER_EBX);
	context->values[ZYDIS_REGISTER_EBP] = state->IntRegisterValue(X86_REGISTER_EBP);
	context->values[ZYDIS_REGISTER_ESI] = state->IntRegisterValue(X86_REGISTER_ESI);
	context->values[ZYDIS_REGISTER_EDI] = state->IntRegisterValue(X86_REGISTER_EDI);
}


struct DisassemblerX86::ZydisData {
	ZydisDecoder decoder ;
	ZydisFormatter formatter;
	ZyanUSize offset;
};


DisassemblerX86::DisassemblerX86()
	:
	fAddress(0),
	fCode(NULL),
	fCodeSize(0),
	fZydisData(NULL)
{
}


DisassemblerX86::~DisassemblerX86()
{
	delete fZydisData;
}


status_t
DisassemblerX86::Init(target_addr_t address, const void* code, size_t codeSize)
{
	// unset old data
	delete fZydisData;
	fZydisData = NULL;

	// set new data
	fZydisData = new(std::nothrow) ZydisData;
	if (fZydisData == NULL)
		return B_NO_MEMORY;

	fAddress = address;
	fCode = (const uint8*)code;
	fCodeSize = codeSize;

	// init zydis
	fZydisData->offset = 0;
	ZydisDecoderInit(&fZydisData->decoder, ZYDIS_MACHINE_MODE_LEGACY_32, ZYDIS_STACK_WIDTH_32);
	ZydisFormatterInit(&fZydisData->formatter, ZYDIS_FORMATTER_STYLE_ATT);
	ZydisFormatterSetProperty(&fZydisData->formatter, ZYDIS_FORMATTER_PROP_FORCE_SIZE, ZYAN_TRUE);
	ZydisFormatterSetProperty(&fZydisData->formatter, ZYDIS_FORMATTER_PROP_HEX_UPPERCASE,
		ZYAN_FALSE);
	ZydisFormatterSetProperty(&fZydisData->formatter, ZYDIS_FORMATTER_PROP_ADDR_PADDING_ABSOLUTE,
		ZYDIS_PADDING_DISABLED);
	ZydisFormatterSetProperty(&fZydisData->formatter, ZYDIS_FORMATTER_PROP_ADDR_PADDING_RELATIVE,
		ZYDIS_PADDING_DISABLED);
	ZydisFormatterSetProperty(&fZydisData->formatter, ZYDIS_FORMATTER_PROP_DISP_PADDING,
		ZYDIS_PADDING_DISABLED);
	ZydisFormatterSetProperty(&fZydisData->formatter, ZYDIS_FORMATTER_PROP_IMM_PADDING,
		ZYDIS_PADDING_DISABLED);
		// TODO: Set the correct vendor!

	return B_OK;
}


status_t
DisassemblerX86::GetNextInstruction(BString& line, target_addr_t& _address,
	target_size_t& _size, bool& _breakpointAllowed)
{
	const uint8* buffer = fCode + fZydisData->offset;
	ZydisDecodedInstruction instruction;
	ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];
	if (!ZYAN_SUCCESS(ZydisDecoderDecodeFull(&fZydisData->decoder, buffer,
		fCodeSize - fZydisData->offset, &instruction, operands))) {
		return B_ENTRY_NOT_FOUND;
	}

	uint32 address = (uint32)(fAddress + fZydisData->offset);
	fZydisData->offset += instruction.length;

	char hexString[32];
	char* srcHex = hexString;
	for (ZyanUSize i = 0; i < instruction.length; i++) {
		sprintf(srcHex, "%02" PRIx8, buffer[i]);
		srcHex += 2;
	}

	char formatted[1024];
	if (ZYAN_SUCCESS(ZydisFormatterFormatInstruction(&fZydisData->formatter, &instruction,
		operands, instruction.operand_count_visible, formatted, sizeof(formatted), address,
		NULL))) {
		line.SetToFormat("0x%08" B_PRIx32 ": %16.16s  %s", address, hexString, formatted);
	} else {
		line.SetToFormat("0x%08" B_PRIx32 ": failed-to-format", address);
	}
		// TODO: Resolve symbols!

	_address = address;
	_size = instruction.length;
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
		const uint8* buffer = fCode + fZydisData->offset;
		ZydisDecodedInstruction instruction;
		if (!ZYAN_SUCCESS(ZydisDecoderDecodeInstruction(&fZydisData->decoder,
				(ZydisDecoderContext*)ZYAN_NULL, buffer, fCodeSize - fZydisData->offset,
				&instruction))) {
			return B_ENTRY_NOT_FOUND;
		}

		fZydisData->offset += instruction.length;
		target_addr_t address = fAddress + fZydisData->offset;
		if (address == nextAddress) {
			_address = address;
			_size = instruction.length;
			return B_OK;
		}
	}
}


status_t
DisassemblerX86::GetNextInstructionInfo(InstructionInfo& _info,
	CpuState* state)
{
	const uint8* buffer = fCode + fZydisData->offset;
	ZydisDecodedInstruction instruction;
	ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];
	if (!ZYAN_SUCCESS(ZydisDecoderDecodeFull(&fZydisData->decoder, buffer,
		fCodeSize - fZydisData->offset, &instruction, operands))) {
		return B_ENTRY_NOT_FOUND;
	}

	uint32 address = (uint32)(fAddress + fZydisData->offset);
	fZydisData->offset += instruction.length;

	char hexString[32];
	char* srcHex = hexString;
	for (ZyanUSize i = 0; i < instruction.length; i++) {
		sprintf(srcHex, "%02" PRIx8, buffer[i]);
		srcHex += 2;
	}

	instruction_type type = INSTRUCTION_TYPE_OTHER;
	target_addr_t targetAddress = 0;
	if (instruction.mnemonic == ZYDIS_MNEMONIC_CALL)
		type = INSTRUCTION_TYPE_SUBROUTINE_CALL;
	else if (instruction.mnemonic == ZYDIS_MNEMONIC_JMP)
		type = INSTRUCTION_TYPE_JUMP;
	if (state != NULL) {
		CpuStateX86* x86State = dynamic_cast<CpuStateX86*>(state);
		if (x86State != NULL) {
			ZydisRegisterContext registers;
			CpuStateToZydisRegContext(x86State, &registers);
			ZYAN_CHECK(ZydisCalcAbsoluteAddressEx(&instruction, operands,
				address, &registers, &targetAddress));
		}
	}

	char string[1024];
	int written = snprintf(string, sizeof(string), "0x%08" B_PRIx32 ": %16.16s  ", address,
		hexString);
	char* formatted = string + written;
	if (!ZYAN_SUCCESS(ZydisFormatterFormatInstruction(&fZydisData->formatter, &instruction,
		operands, instruction.operand_count_visible, formatted, sizeof(string) - written, address,
		NULL))) {
		snprintf(string, sizeof(string), "0x%08" B_PRIx32 ": failed-to-format", address);
	}

		// TODO: Resolve symbols!

	if (!_info.SetTo(address, targetAddress, instruction.length, type, true, string))
		return B_NO_MEMORY;

	return B_OK;
}

