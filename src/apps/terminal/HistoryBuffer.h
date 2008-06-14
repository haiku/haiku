/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef HISTORY_BUFFER_H
#define HISTORY_BUFFER_H

#include <SupportDefs.h>

#include "TerminalLine.h"


struct HistoryLine;
struct TerminalLine;


class HistoryBuffer {
public:
								HistoryBuffer();
								~HistoryBuffer();

			status_t			Init(int32 width, int32 capacity);

			void				Clear();

			int32				Width() const		{ return fWidth; }
			int32				Capacity() const	{ return fCapacity; }
			int32				Size() const		{ return fSize; }

	inline	HistoryLine*		LineAt(int32 index) const;
			TerminalLine*		GetTerminalLineAt(int32 index,
									TerminalLine* buffer) const;

			void				AddLine(const TerminalLine* line);
			void				AddEmptyLines(int32 count);
			void				DropLines(int32 count);

private:
			HistoryLine*		_AllocateLine(int32 attributesRuns,
									int32 byteLength);
	inline	HistoryLine*		_LineAt(int32 index) const;

private:
			HistoryLine*		fLines;
			int32				fWidth;
			int32				fCapacity;
			int32				fNextLine;
			int32				fSize;
			uint8*				fBuffer;
			int32				fBufferSize;
			int32				fBufferAllocationOffset;
};


inline HistoryLine*
HistoryBuffer::_LineAt(int32 index) const
{
	return &fLines[(fCapacity + fNextLine - index - 1) % fCapacity];
}


inline HistoryLine*
HistoryBuffer::LineAt(int32 index) const
{
	return index >= 0 && index < fSize ? _LineAt(index) : NULL;
}


#endif	// HISTORY_BUFFER_H
