/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SOURCE_LOCATION_H
#define SOURCE_LOCATION_H

#include <SupportDefs.h>


class SourceLocation {
public:
	SourceLocation(uint32 line = 0, uint32 column = 0)
		:
		fLine(line),
		fColumn(column)
	{
	}

	SourceLocation(const SourceLocation& other)
		:
		fLine(other.fLine),
		fColumn(other.fColumn)
	{
	}

	SourceLocation& operator=(const SourceLocation& other)
	{
		fLine = other.fLine;
		fColumn = other.fColumn;
		return *this;
	}

	bool operator==(const SourceLocation& other) const
	{
		return fLine == other.fLine && fColumn == other.fColumn;
	}

	bool operator!=(const SourceLocation& other) const
	{
		return !(*this == other);
	}

	uint32 Line() const
	{
		return fLine;
	}

	uint32 Column() const
	{
		return fColumn;
	}

private:
	uint32	fLine;
	uint32	fColumn;
};


#endif	// SOURCE_LOCATION_H
