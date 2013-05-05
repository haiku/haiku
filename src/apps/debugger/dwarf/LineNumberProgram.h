/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef LINE_NUMBER_PROGRAM_H
#define LINE_NUMBER_PROGRAM_H

#include "DataReader.h"
#include "Types.h"


class LineNumberProgram {
public:
	struct State;

public:
								LineNumberProgram(uint8 addressSize);
								~LineNumberProgram();

			status_t			Init(const void* program, size_t programSize,
									uint8 minInstructionLength,
									bool defaultIsStatement, int8 lineBase,
									uint8 lineRange, uint8 opcodeBase,
									const uint8* standardOpcodeLengths);

			bool				IsValid() const	{ return fProgram != NULL; }
			void				GetInitialState(State& state) const;
			bool				GetNextRow(State& state) const;

private:
			void				_SetToInitial(State& state) const;

private:
			const void*			fProgram;
			size_t				fProgramSize;
			uint8				fMinInstructionLength;
			bool				fDefaultIsStatement;
			int8				fLineBase;
			uint8				fLineRange;
			uint8				fOpcodeBase;
			uint8				fAddressSize;
			const uint8*		fStandardOpcodeLengths;
};


struct LineNumberProgram::State {
	target_addr_t	address;
	int32			file;
	int32			line;
	int32			column;
	bool			isStatement;
	bool			isBasicBlock;
	bool			isSequenceEnd;
	bool			isPrologueEnd;
	bool			isEpilogueBegin;
	uint32			instructionSet;
	uint32			discriminator;

	// when file is set to -1
	const char*		explicitFile;
	uint32			explicitFileDirIndex;

	DataReader		dataReader;
};


#endif	// LINE_NUMBER_PROGRAM_H
