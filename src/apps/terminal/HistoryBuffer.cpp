/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "HistoryBuffer.h"

#include <new>

#include <OS.h>

#include "TermConst.h"


HistoryBuffer::HistoryBuffer()
	:
	fLines(NULL),
	fWidth(0),
	fCapacity(0),
	fNextLine(0),
	fSize(0),
	fBuffer(NULL),
	fBufferSize(0),
	fBufferAllocationOffset(0)
{
}


HistoryBuffer::~HistoryBuffer()
{
	delete[] fLines;
	delete[] fBuffer;
}


status_t
HistoryBuffer::Init(int32 width, int32 capacity)
{
	if (width <= 0 || capacity <= 0)
		return B_BAD_VALUE;

	int32 bufferSize = (width + 4) * capacity;

	if (capacity > 0) {
		fLines = new(std::nothrow) HistoryLine[capacity];
		fBuffer = new(std::nothrow) uint8[bufferSize];

		if (fLines == NULL || fBuffer == NULL)
			return B_NO_MEMORY;
	}

	fWidth = width;
	fCapacity = capacity;
	fNextLine = 0;
	fSize = 0;
	fBufferSize = bufferSize;
	fBufferAllocationOffset = 0;

	return B_OK;
}


void
HistoryBuffer::Clear()
{
	fNextLine = 0;
	fSize = 0;
	fBufferAllocationOffset = 0;
}


TerminalLine*
HistoryBuffer::GetTerminalLineAt(int32 index, TerminalLine* buffer) const
{
	HistoryLine* line = LineAt(index);
	if (line == NULL)
		return NULL;

	int32 charCount = 0;
	const char* chars = line->Chars();
	buffer->length = 0;
	uint32 attributes = 0;
	AttributesRun* attributesRun = line->AttributesRuns();
	int32 attributesRunCount = line->attributesRunCount;
	int32 nextAttributesAt = attributesRunCount > 0
		? attributesRun->offset : INT_MAX;

	for (int32 i = 0; i < line->byteLength;) {
		// get attributes
		if (charCount == nextAttributesAt) {
			if (attributesRunCount > 0) {
				attributes = attributesRun->attributes;
				nextAttributesAt = attributesRun->offset
					+ attributesRun->length;
				attributesRun++;
				attributesRunCount--;
			} else {
				attributes = 0;
				nextAttributesAt = INT_MAX;
			}
		}

		// copy character
		TerminalCell& cell = buffer->cells[charCount++];
		int32 charLength = UTF8Char::ByteCount(chars[i]);
		cell.character.SetTo(chars + i, charLength);
		i += charLength;

		// set attributes
		cell.attributes = attributes;

		// full width char?
		if (cell.character.IsFullWidth()) {
			cell.attributes |= A_WIDTH;
			charCount++;
		}
	}

	buffer->length = charCount;
	buffer->softBreak = line->softBreak;
	buffer->attributes = line->attributes;

	return buffer;
}


void
HistoryBuffer::AddLine(const TerminalLine* line)
{
//debug_printf("HistoryBuffer::AddLine(%p): length: %d\n", line, line->length);
	// determine the amount of memory we need for the line
	uint32 attributes = 0;
	int32 attributesRuns = 0;
	int32 byteLength = 0;
	for (int32 i = 0; i < line->length; i++) {
		const TerminalCell& cell = line->cells[i];
		byteLength += cell.character.ByteCount();
		if ((cell.attributes & CHAR_ATTRIBUTES) != attributes) {
			attributes = cell.attributes & CHAR_ATTRIBUTES;
			if (attributes != 0)
				attributesRuns++;
		}
		if (IS_WIDTH(cell.attributes))
			i++;
	}

//debug_printf("  attributesRuns: %ld, byteLength: %ld\n", attributesRuns, byteLength);

	// allocate and translate the line
	HistoryLine* historyLine = _AllocateLine(attributesRuns, byteLength);

	attributes = 0;
	AttributesRun* attributesRun = historyLine->AttributesRuns();

	char* chars = historyLine->Chars();
	for (int32 i = 0; i < line->length; i++) {
		const TerminalCell& cell = line->cells[i];

		// copy char
		int32 charLength = cell.character.ByteCount();
		memcpy(chars, cell.character.bytes, charLength);
		chars += charLength;

		// deal with attributes
		if ((cell.attributes & CHAR_ATTRIBUTES) != attributes) {
			// terminate the previous attributes run
			if (attributes != 0) {
				attributesRun->length = i - attributesRun->offset;
				attributesRun++;
			}

			attributes = cell.attributes & CHAR_ATTRIBUTES;

			// init the new one
			if (attributes != 0) {
				attributesRun->attributes = attributes;
				attributesRun->offset = i;
			}
		}

		if (IS_WIDTH(cell.attributes))
			i++;
	}

	// set the last attributes run's length
	if (attributes != 0)
		attributesRun->length = line->length - attributesRun->offset;

	historyLine->softBreak = line->softBreak;
	historyLine->attributes = line->attributes;
//debug_printf("  line: \"%.*s\", history size now: %ld\n", historyLine->byteLength, historyLine->Chars(), fSize);
}


void
HistoryBuffer::AddEmptyLines(int32 count)
{
	if (count <= 0)
		return;

	if (count > fCapacity)
		count = fCapacity;

	if (count + fSize > fCapacity)
		DropLines(count + fSize - fCapacity);

	// All lines use the same buffer address, since they don't use any memory.
	AttributesRun* attributesRun
		= (AttributesRun*)(fBuffer + fBufferAllocationOffset);

	for (int32 i = 0; i < count; i++) {
		HistoryLine* line = &fLines[fNextLine];
		fNextLine = (fNextLine + 1) % fCapacity;
		line->attributesRuns = attributesRun;
		line->attributesRunCount = 0;
		line->byteLength = 0;
		line->softBreak = false;
	}

	fSize += count;
}


void
HistoryBuffer::DropLines(int32 count)
{
	if (count <= 0)
		return;

	if (count < fSize) {
		fSize -= count;
	} else {
		fSize = 0;
		fNextLine = 0;
		fBufferAllocationOffset = 0;
	}
}


HistoryLine*
HistoryBuffer::_AllocateLine(int32 attributesRuns, int32 byteLength)
{
	// we need at least one spare line slot
	int32 toDrop = 0;
	if (fSize == fCapacity)
		toDrop = 1;

	int32 bytesNeeded = attributesRuns * sizeof(AttributesRun) + byteLength;

	if (fBufferAllocationOffset + bytesNeeded > fBufferSize) {
		// drop all lines after the allocation index
		for (; toDrop < fSize; toDrop++) {
			HistoryLine* line = _LineAt(fSize - toDrop - 1);
			int32 offset = (uint8*)line->AttributesRuns() - fBuffer;
			if (offset < fBufferAllocationOffset)
				break;
		}

		fBufferAllocationOffset = 0;
	}

	// drop all lines interfering
	int32 nextOffset = (fBufferAllocationOffset + bytesNeeded + 1) & ~1;
	for (; toDrop < fSize; toDrop++) {
		HistoryLine* line = _LineAt(fSize - toDrop - 1);
		int32 offset = (uint8*)line->AttributesRuns() - fBuffer;
		if (offset + line->BufferSize() <= fBufferAllocationOffset
				|| offset >= nextOffset) {
			break;
		}
	}

	DropLines(toDrop);

	// init the line
	HistoryLine* line = &fLines[fNextLine];
	fNextLine = (fNextLine + 1) % fCapacity;
	fSize++;
	line->attributesRuns = (AttributesRun*)(fBuffer + fBufferAllocationOffset);
	line->attributesRunCount = attributesRuns;
	line->byteLength = byteLength;

	fBufferAllocationOffset = (fBufferAllocationOffset + bytesNeeded + 1) & ~1;
		// DropLines() may have changed fBufferAllocationOffset, so don't use
		// nextOffset.

	return line;
}
