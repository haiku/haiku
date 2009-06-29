/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef INSTRUCTION_INFO_H
#define INSTRUCTION_INFO_H

#include <String.h>

#include "Types.h"


enum instruction_type {
	INSTRUCTION_TYPE_SUBROUTINE_CALL,
	INSTRUCTION_TYPE_OTHER
};


class InstructionInfo {
public:
								InstructionInfo();
								InstructionInfo(target_addr_t address,
									target_size_t size, instruction_type type,
									bool breakpointAllowed,
									const BString& disassembledLine);

			bool				SetTo(target_addr_t address, target_size_t size,
									instruction_type type,
									bool breakpointAllowed,
									const BString& disassembledLine);

			target_addr_t		Address() const		{ return fAddress; }
			target_size_t		Size() const		{ return fSize; }
			instruction_type	Type() const		{ return fType; }
			bool				IsBreakpointAllowed() const
									{ return fBreakpointAllowed; }
			const char*			DisassembledLine() const
									{ return fDisassembledLine.String(); }


private:
			target_addr_t		fAddress;
			target_size_t		fSize;
			instruction_type	fType;
			bool				fBreakpointAllowed;
			BString				fDisassembledLine;
};


#endif	// INSTRUCTION_INFO_H
