/*
 * Copyright 2013, Haiku, Inc. All rights reserved.
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 *		Siarzhuk Zharski, zharik@gmx.li
 */
#ifndef TERMINAL_LINE_H
#define TERMINAL_LINE_H

#include <SupportDefs.h>

#include "TermConst.h"

#include "UTF8Char.h"


struct TerminalCell {
	UTF8Char		character;
	uint32			attributes;
};


struct TerminalLine {
	uint16			length;
	bool			softBreak;	// soft line break
	uint32			attributes;
	TerminalCell	cells[1];

	inline void Clear(uint32 attr = 0, size_t count = 0)
	{
		length = 0;
		attributes = attr;
		softBreak = false;
		for (size_t i = 0; i < count; i++)
			cells[i].attributes = attr;
	}
};


struct AttributesRun {
	uint32	attributes;
	uint16	offset;			// character offset
	uint16	length;			// length of the run in characters
};


struct HistoryLine {
	AttributesRun*	attributesRuns;
	uint16			attributesRunCount;	// number of attribute runs
	uint16			byteLength : 15;	// number of bytes in the line
	bool			softBreak : 1;		// soft line break;
	uint32			attributes;

	AttributesRun* AttributesRuns() const
	{
		return attributesRuns;
	}

	char* Chars() const
	{
		return (char*)(attributesRuns + attributesRunCount);
	}

	int32 BufferSize() const
	{
		return attributesRunCount * sizeof(AttributesRun) + byteLength;
	}
};


#endif	// TERMINAL_LINE_H
