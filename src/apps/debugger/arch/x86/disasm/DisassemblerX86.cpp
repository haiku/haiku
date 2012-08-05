/*
 * Copyright 2009-2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2008, François Revol, revol@free.fr
 * Distributed under the terms of the MIT License.
 */

#include "DisassemblerX86.h"

#include <new>

#include "udis86.h"

#include <OS.h>


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
