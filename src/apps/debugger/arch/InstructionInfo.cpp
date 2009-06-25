/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "InstructionInfo.h"


InstructionInfo::InstructionInfo()
	:
	fAddress(0),
	fSize(0),
	fType(INSTRUCTION_TYPE_OTHER),
	fBreakpointAllowed(false),
	fDisassembledLine()
{
}


InstructionInfo::InstructionInfo(target_addr_t address, target_size_t size,
	instruction_type type, bool breakpointAllowed,
	const BString& disassembledLine)
	:
	fAddress(address),
	fSize(size),
	fType(type),
	fBreakpointAllowed(breakpointAllowed),
	fDisassembledLine(disassembledLine)
{
}


bool
InstructionInfo::SetTo(target_addr_t address, target_size_t size,
	instruction_type type, bool breakpointAllowed,
	const BString& disassembledLine)
{
	fAddress = address;
	fSize = size;
	fType = type;
	fBreakpointAllowed = breakpointAllowed;
	fDisassembledLine = disassembledLine;
	return disassembledLine.Length() == 0 || fDisassembledLine.Length() > 0;
}
