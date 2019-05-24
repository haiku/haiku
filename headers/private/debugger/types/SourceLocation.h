/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SOURCE_LOCATION_H
#define SOURCE_LOCATION_H

#include <SupportDefs.h>


class SourceLocation {
public:
	SourceLocation(int32 line = 0, int32 column = 0)
		:
		fLine(line),
		fColumn(column)
	{
	}

	bool operator==(const SourceLocation& other) const
	{
		return fLine == other.fLine && fColumn == other.fColumn;
	}

	bool operator!=(const SourceLocation& other) const
	{
		return !(*this == other);
	}

	bool operator<(const SourceLocation& other) const
	{
		return fLine < other.fLine
			|| (fLine == other.fLine && fColumn < other.fColumn);
	}

	bool operator<=(const SourceLocation& other) const
	{
		return fLine < other.fLine
			|| (fLine == other.fLine && fColumn <= other.fColumn);
	}

	int32 Line() const
	{
		return fLine;
	}

	int32 Column() const
	{
		return fColumn;
	}

private:
	int32	fLine;
	int32	fColumn;
};


#endif	// SOURCE_LOCATION_H
