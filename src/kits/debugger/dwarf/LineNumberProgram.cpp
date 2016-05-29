/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "LineNumberProgram.h"

#include <algorithm>

#include <stdio.h>
#include <string.h>

#include "Dwarf.h"
#include "Tracing.h"


static const uint8 kLineNumberStandardOpcodeOperands[]
	= { 0, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1 };
static const uint32 kLineNumberStandardOpcodeCount = 12;


LineNumberProgram::LineNumberProgram(uint8 addressSize)
	:
	fProgram(NULL),
	fProgramSize(0),
	fMinInstructionLength(0),
	fDefaultIsStatement(0),
	fLineBase(0),
	fLineRange(0),
	fOpcodeBase(0),
	fAddressSize(addressSize),
	fStandardOpcodeLengths(NULL)
{
}


LineNumberProgram::~LineNumberProgram()
{
}


status_t
LineNumberProgram::Init(const void* program, size_t programSize,
	uint8 minInstructionLength, bool defaultIsStatement, int8 lineBase,
	uint8 lineRange, uint8 opcodeBase, const uint8* standardOpcodeLengths)
{
	// first check the operand counts for the standard opcodes
	uint8 standardOpcodeCount = std::min((uint32)opcodeBase - 1,
		kLineNumberStandardOpcodeCount);
	for (uint8 i = 0; i < standardOpcodeCount; i++) {
		if (standardOpcodeLengths[i] != kLineNumberStandardOpcodeOperands[i]) {
			WARNING("operand count for standard opcode %u does not what we "
				"expect\n", i + 1);
			return B_BAD_DATA;
		}
	}

	fProgram = program;
	fProgramSize = programSize;
	fMinInstructionLength = minInstructionLength;
	fDefaultIsStatement = defaultIsStatement;
	fLineBase = lineBase;
	fLineRange = lineRange;
	fOpcodeBase = opcodeBase;
	fStandardOpcodeLengths = standardOpcodeLengths;

	return B_OK;
}


void
LineNumberProgram::GetInitialState(State& state) const
{
	if (!IsValid())
		return;

	_SetToInitial(state);
	state.dataReader.SetTo(fProgram, fProgramSize, fAddressSize);
}


bool
LineNumberProgram::GetNextRow(State& state) const
{
	if (state.isSequenceEnd)
		_SetToInitial(state);

	DataReader& dataReader = state.dataReader;

	while (dataReader.BytesRemaining() > 0) {
		bool appendRow = false;
		uint8 opcode = dataReader.Read<uint8>(0);
		if (opcode >= fOpcodeBase) {
			// special opcode
			uint adjustedOpcode = opcode - fOpcodeBase;
			state.address += (adjustedOpcode / fLineRange)
				* fMinInstructionLength;
			state.line += adjustedOpcode % fLineRange + fLineBase;
			state.isBasicBlock = false;
			state.isPrologueEnd = false;
			state.isEpilogueBegin = false;
			state.discriminator = 0;
			appendRow = true;
		} else if (opcode > 0) {
			// standard opcode
			switch (opcode) {
				case DW_LNS_copy:
					state.isBasicBlock = false;
					state.isPrologueEnd = false;
					state.isEpilogueBegin = false;
					appendRow = true;
					state.discriminator = 0;
					break;
				case DW_LNS_advance_pc:
					state.address += dataReader.ReadUnsignedLEB128(0)
						* fMinInstructionLength;
					break;
				case DW_LNS_advance_line:
					state.line += dataReader.ReadSignedLEB128(0);
					break;
				case DW_LNS_set_file:
					state.file = dataReader.ReadUnsignedLEB128(0);
					break;
				case DW_LNS_set_column:
					state.column = dataReader.ReadUnsignedLEB128(0);
					break;
				case DW_LNS_negate_stmt:
					state.isStatement = !state.isStatement;
					break;
				case DW_LNS_set_basic_block:
					state.isBasicBlock = true;
					break;
				case DW_LNS_const_add_pc:
					state.address += ((255 - fOpcodeBase) / fLineRange)
						* fMinInstructionLength;
					break;
				case DW_LNS_fixed_advance_pc:
					state.address += dataReader.Read<uint16>(0);
					break;
				case DW_LNS_set_prologue_end:
					state.isPrologueEnd = true;
					break;
				case DW_LNS_set_epilogue_begin:
					state.isEpilogueBegin = true;
					break;
				case DW_LNS_set_isa:
					state.instructionSet = dataReader.ReadUnsignedLEB128(0);
					break;
				default:
					WARNING("unsupported standard opcode %u\n", opcode);
					for (int32 i = 0; i < fStandardOpcodeLengths[opcode - 1];
							i++) {
						dataReader.ReadUnsignedLEB128(0);
					}
			}
		} else {
			// extended opcode
			uint32 instructionLength = dataReader.ReadUnsignedLEB128(0);
			off_t instructionOffset = dataReader.Offset();
			uint8 extendedOpcode = dataReader.Read<uint8>(0);

			switch (extendedOpcode) {
				case DW_LNE_end_sequence:
					state.isSequenceEnd = true;
					appendRow = true;
					break;
				case DW_LNE_set_address:
					state.address = dataReader.ReadAddress(0);
					break;
				case DW_LNE_define_file:
				{
					state.explicitFile = dataReader.ReadString();
					state.explicitFileDirIndex
						= dataReader.ReadUnsignedLEB128(0);
					dataReader.ReadUnsignedLEB128(0);	// modification time
					dataReader.ReadUnsignedLEB128(0);	// file length
					state.file = -1;
					break;
				}
				case DW_LNE_set_discriminator:
				{
					state.discriminator = dataReader.ReadUnsignedLEB128(0);
					break;
				}
				default:
					WARNING("unsupported extended opcode: %u\n",
						extendedOpcode);
					break;
			}

			dataReader.Skip(instructionLength
				- (dataReader.Offset() - instructionOffset));
		}

		if (dataReader.HasOverflow())
			return false;

		if (appendRow)
			return true;
	}

	return false;
}


void
LineNumberProgram::_SetToInitial(State& state) const
{
	state.address = 0;
	state.file = 1;
	state.line = 1;
	state.column = 0;
	state.isStatement = fDefaultIsStatement;
	state.isBasicBlock = false;
	state.isSequenceEnd = false;
	state.isPrologueEnd = false;
	state.isEpilogueBegin = false;
	state.instructionSet = 0;
	state.discriminator = 0;
}
