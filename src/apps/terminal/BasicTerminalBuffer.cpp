/*
 * Copyright 2013-2024, Haiku, Inc. All rights reserved.
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 *		Siarzhuk Zharski, zharik@gmx.li
 */

#include "BasicTerminalBuffer.h"

#include <alloca.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#include <algorithm>

#include <StackOrHeapArray.h>
#include <String.h>

#include "TermConst.h"
#include "TerminalCharClassifier.h"
#include "TerminalLine.h"


static const UTF8Char kSpaceChar(' ');

// Soft size limits for the terminal buffer. The constants defined in
// TermConst.h are rather for the Terminal in general (i.e. the GUI).
static const int32 kMinRowCount = 2;
static const int32 kMaxRowCount = 1024;
static const int32 kMinColumnCount = 4;
static const int32 kMaxColumnCount = 1024;


#define ALLOC_LINE_ON_STACK(width)	\
	((TerminalLine*)alloca(sizeof(TerminalLine)	\
		+ sizeof(TerminalCell) * ((width) - 1)))


static inline int32
restrict_value(int32 value, int32 min, int32 max)
{
	return value < min ? min : (value > max ? max : value);
}


// #pragma mark - private inline methods


inline int32
BasicTerminalBuffer::_LineIndex(int32 index) const
{
	return (index + fScreenOffset) % fHeight;
}


inline TerminalLine*
BasicTerminalBuffer::_LineAt(int32 index) const
{
	return fScreen[_LineIndex(index)];
}


inline TerminalLine*
BasicTerminalBuffer::_HistoryLineAt(int32 index, TerminalLine* lineBuffer) const
{
	if (index >= fHeight)
		return NULL;

	if (index < 0 && fHistory != NULL)
		return fHistory->GetTerminalLineAt(-index - 1, lineBuffer);

	return _LineAt(index + fHeight);
}


inline void
BasicTerminalBuffer::_Invalidate(int32 top, int32 bottom)
{
//debug_printf("%p->BasicTerminalBuffer::_Invalidate(%ld, %ld)\n", this, top, bottom);
	fDirtyInfo.ExtendDirtyRegion(top, bottom);

	if (!fDirtyInfo.messageSent) {
		NotifyListener();
		fDirtyInfo.messageSent = true;
	}
}


inline void
BasicTerminalBuffer::_CursorChanged()
{
	if (!fDirtyInfo.messageSent) {
		NotifyListener();
		fDirtyInfo.messageSent = true;
	}
}


// #pragma mark - public methods


BasicTerminalBuffer::BasicTerminalBuffer()
	:
	fWidth(0),
	fHeight(0),
	fScrollTop(0),
	fScrollBottom(0),
	fScreen(NULL),
	fScreenOffset(0),
	fHistory(NULL),
	fAttributes(),
	fSoftWrappedCursor(false),
	fOverwriteMode(false),
	fAlternateScreenActive(false),
	fOriginMode(false),
	fSavedOriginMode(false),
	fTabStops(NULL),
	fEncoding(M_UTF8),
	fCaptureFile(-1),
	fLast()
{
}


BasicTerminalBuffer::~BasicTerminalBuffer()
{
	delete fHistory;
	_FreeLines(fScreen, fHeight);
	delete[] fTabStops;

	if (fCaptureFile >= 0)
		close(fCaptureFile);
}


status_t
BasicTerminalBuffer::Init(int32 width, int32 height, int32 historySize)
{
	status_t error;

	fWidth = width;
	fHeight = height;

	fScrollTop = 0;
	fScrollBottom = fHeight - 1;

	fCursor.x = 0;
	fCursor.y = 0;
	fSoftWrappedCursor = false;

	fScreenOffset = 0;

	fOverwriteMode = true;
	fAlternateScreenActive = false;
	fOriginMode = fSavedOriginMode = false;

	fScreen = _AllocateLines(width, height);
	if (fScreen == NULL)
		return B_NO_MEMORY;

	if (historySize > 0) {
		fHistory = new(std::nothrow) HistoryBuffer;
		if (fHistory == NULL)
			return B_NO_MEMORY;

		error = fHistory->Init(width, historySize);
		if (error != B_OK)
			return error;
	}

	error = _ResetTabStops(fWidth);
	if (error != B_OK)
		return error;

	for (int32 i = 0; i < fHeight; i++)
		fScreen[i]->Clear();

	fDirtyInfo.Reset();

	return B_OK;
}


status_t
BasicTerminalBuffer::ResizeTo(int32 width, int32 height)
{
	return ResizeTo(width, height, fHistory != NULL ? fHistory->Capacity() : 0);
}


status_t
BasicTerminalBuffer::ResizeTo(int32 width, int32 height, int32 historyCapacity)
{
	if (height < kMinRowCount || height > kMaxRowCount
			|| width < kMinColumnCount || width > kMaxColumnCount) {
		return B_BAD_VALUE;
	}

	if (width == fWidth && height == fHeight)
		return SetHistoryCapacity(historyCapacity);

	if (fAlternateScreenActive)
		return _ResizeSimple(width, height, historyCapacity);

	return _ResizeRewrap(width, height, historyCapacity);
}


status_t
BasicTerminalBuffer::SetHistoryCapacity(int32 historyCapacity)
{
	return _ResizeHistory(fWidth, historyCapacity);
}


void
BasicTerminalBuffer::Clear(bool resetCursor)
{
	fSoftWrappedCursor = false;
	fScreenOffset = 0;
	_ClearLines(0, fHeight - 1);

	if (resetCursor)
		fCursor.SetTo(0, 0);

	if (fHistory != NULL)
		fHistory->Clear();

	fDirtyInfo.linesScrolled = 0;
	_Invalidate(0, fHeight - 1);
}


void
BasicTerminalBuffer::SynchronizeWith(const BasicTerminalBuffer* other,
	int32 offset, int32 dirtyTop, int32 dirtyBottom)
{
//debug_printf("BasicTerminalBuffer::SynchronizeWith(%p, %ld, %ld - %ld)\n",
//other, offset, dirtyTop, dirtyBottom);

	// intersect the visible region with the dirty region
	int32 first = 0;
	int32 last = fHeight - 1;
	dirtyTop -= offset;
	dirtyBottom -= offset;

	if (first > dirtyBottom || dirtyTop > last)
		return;

	if (first < dirtyTop)
		first = dirtyTop;
	if (last > dirtyBottom)
		last = dirtyBottom;

	// update the dirty lines
//debug_printf("  updating: %ld - %ld\n", first, last);
	for (int32 i = first; i <= last; i++) {
		TerminalLine* destLine = _LineAt(i);
		TerminalLine* sourceLine = other->_HistoryLineAt(i + offset, destLine);
		if (sourceLine != NULL) {
			if (sourceLine != destLine) {
				destLine->length = sourceLine->length;
				destLine->attributes = sourceLine->attributes;
				destLine->softBreak = sourceLine->softBreak;
				if (destLine->length > 0) {
					memcpy(destLine->cells, sourceLine->cells,
						fWidth * sizeof(TerminalCell));
				}
			} else {
				// The source line was a history line and has been copied
				// directly into destLine.
			}
		} else
			destLine->Clear(fAttributes, fWidth);
	}
}


bool
BasicTerminalBuffer::IsFullWidthChar(int32 row, int32 column) const
{
	TerminalLine* lineBuffer = ALLOC_LINE_ON_STACK(fWidth);
	TerminalLine* line = _HistoryLineAt(row, lineBuffer);
	return line != NULL && column > 0 && column < line->length
		&& line->cells[column - 1].attributes.IsWidth();
}


int
BasicTerminalBuffer::GetChar(int32 row, int32 column, UTF8Char& character,
	Attributes& attributes) const
{
	TerminalLine* lineBuffer = ALLOC_LINE_ON_STACK(fWidth);
	TerminalLine* line = _HistoryLineAt(row, lineBuffer);
	if (line == NULL)
		return NO_CHAR;

	if (column < 0 || column >= line->length)
		return NO_CHAR;

	if (column > 0 && line->cells[column - 1].attributes.IsWidth())
		return IN_STRING;

	TerminalCell& cell = line->cells[column];
	character = cell.character;
	attributes = cell.attributes;
	return A_CHAR;
}


void
BasicTerminalBuffer::GetCellAttributes(int32 row, int32 column,
	Attributes& attributes, uint32& count) const
{
	count = 0;
	TerminalLine* lineBuffer = ALLOC_LINE_ON_STACK(fWidth);
	TerminalLine* line = _HistoryLineAt(row, lineBuffer);
	if (line == NULL || column < 0)
		return;

	int32 c = column;
	for (; c < fWidth; c++) {
		TerminalCell& cell = line->cells[c];
		if (c > column && attributes != cell.attributes)
			break;
		attributes = cell.attributes;
	}
	count = c - column;
}


int32
BasicTerminalBuffer::GetString(int32 row, int32 firstColumn, int32 lastColumn,
	char* buffer, Attributes& attributes) const
{
	TerminalLine* lineBuffer = ALLOC_LINE_ON_STACK(fWidth);
	TerminalLine* line = _HistoryLineAt(row, lineBuffer);
	if (line == NULL)
		return 0;

	if (lastColumn >= line->length)
		lastColumn = line->length - 1;

	int32 column = firstColumn;
	if (column <= lastColumn)
		attributes = line->cells[column].attributes;

	for (; column <= lastColumn; column++) {
		TerminalCell& cell = line->cells[column];
		if (cell.attributes != attributes)
			break;

		int32 bytes = cell.character.ByteCount();
		for (int32 i = 0; i < bytes; i++)
			*buffer++ = cell.character.bytes[i];
	}

	*buffer = '\0';

	return column - firstColumn;
}


void
BasicTerminalBuffer::GetStringFromRegion(BString& string, const TermPos& start,
	const TermPos& end) const
{
//debug_printf("BasicTerminalBuffer::GetStringFromRegion((%ld, %ld), (%ld, %ld))\n",
//start.x, start.y, end.x, end.y);
	if (start >= end)
		return;

	TermPos pos(start);

	if (IsFullWidthChar(pos.y, pos.x))
		pos.x--;

	// get all but the last line
	while (pos.y < end.y) {
		TerminalLine* line = _GetPartialLineString(string, pos.y, pos.x,
			fWidth);
		if (line != NULL && !line->softBreak)
			string.Append('\n', 1);
		pos.x = 0;
		pos.y++;
	}

	// get the last line, if not empty
	if (end.x > 0)
		_GetPartialLineString(string, end.y, pos.x, end.x);
}


bool
BasicTerminalBuffer::FindWord(const TermPos& pos,
	TerminalCharClassifier* classifier, bool findNonWords, TermPos& _start,
	TermPos& _end) const
{
	int32 x = pos.x;
	int32 y = pos.y;

	TerminalLine* lineBuffer = ALLOC_LINE_ON_STACK(fWidth);
	TerminalLine* line = _HistoryLineAt(y, lineBuffer);
	if (line == NULL || x < 0 || x >= fWidth)
		return false;

	if (x >= line->length) {
		// beyond the end of the line -- select all space
		if (!findNonWords)
			return false;

		_start.SetTo(line->length, y);
		_end.SetTo(fWidth, y);
		return true;
	}

	if (x > 0 && line->cells[x - 1].attributes.IsWidth())
		x--;

	// get the char type at the given position
	int type = classifier->Classify(line->cells[x].character);

	// check whether we are supposed to find words only
	if (type != CHAR_TYPE_WORD_CHAR && !findNonWords)
		return false;

	// find the beginning
	TermPos start(x, y);
	TermPos end(x + (line->cells[x].attributes.IsWidth()
				? FULL_WIDTH : HALF_WIDTH), y);
	for (;;) {
		TermPos previousPos = start;
		if (!_PreviousLinePos(lineBuffer, line, previousPos)
			|| classifier->Classify(line->cells[previousPos.x].character)
				!= type) {
			break;
		}

		start = previousPos;
	}

	// find the end
	line = _HistoryLineAt(end.y, lineBuffer);

	for (;;) {
		TermPos nextPos = end;
		if (!_NormalizeLinePos(lineBuffer, line, nextPos))
			break;

		if (classifier->Classify(line->cells[nextPos.x].character) != type)
			break;

		nextPos.x += line->cells[nextPos.x].attributes.IsWidth()
			? FULL_WIDTH : HALF_WIDTH;
		end = nextPos;
	}

	_start = start;
	_end = end;
	return true;
}


bool
BasicTerminalBuffer::PreviousLinePos(TermPos& pos) const
{
	TerminalLine* lineBuffer = ALLOC_LINE_ON_STACK(fWidth);
	TerminalLine* line = _HistoryLineAt(pos.y, lineBuffer);
	if (line == NULL || pos.x < 0 || pos.x >= fWidth)
		return false;

	return _PreviousLinePos(lineBuffer, line, pos);
}


bool
BasicTerminalBuffer::NextLinePos(TermPos& pos, bool normalize) const
{
	TerminalLine* lineBuffer = ALLOC_LINE_ON_STACK(fWidth);
	TerminalLine* line = _HistoryLineAt(pos.y, lineBuffer);
	if (line == NULL || pos.x < 0 || pos.x > fWidth)
		return false;

	if (!_NormalizeLinePos(lineBuffer, line, pos))
		return false;

	pos.x += line->cells[pos.x].attributes.IsWidth() ? FULL_WIDTH : HALF_WIDTH;
	return !normalize || _NormalizeLinePos(lineBuffer, line, pos);
}


int32
BasicTerminalBuffer::LineLength(int32 index) const
{
	TerminalLine* lineBuffer = ALLOC_LINE_ON_STACK(fWidth);
	TerminalLine* line = _HistoryLineAt(index, lineBuffer);
	return line != NULL ? line->length : 0;
}


void
BasicTerminalBuffer::GetLineColor(int32 index, Attributes& attr) const
{
	TerminalLine* lineBuffer = ALLOC_LINE_ON_STACK(fWidth);
	TerminalLine* line = _HistoryLineAt(index, lineBuffer);
	if (line != NULL)
		attr = line->attributes;
	else
		attr.Reset();
}


bool
BasicTerminalBuffer::Find(const char* _pattern, const TermPos& start,
	bool forward, bool caseSensitive, bool matchWord, TermPos& _matchStart,
	TermPos& _matchEnd) const
{
//debug_printf("BasicTerminalBuffer::Find(\"%s\", (%ld, %ld), forward: %d, case: %d, "
//"word: %d)\n", _pattern, start.x, start.y, forward, caseSensitive, matchWord);
	// normalize pos, so that _NextChar() and _PreviousChar() are happy
	TermPos pos(start);
	TerminalLine* lineBuffer = ALLOC_LINE_ON_STACK(fWidth);
	TerminalLine* line = _HistoryLineAt(pos.y, lineBuffer);
	if (line != NULL) {
		if (forward) {
			while (line != NULL && pos.x >= line->length && line->softBreak) {
				pos.x = 0;
				pos.y++;
				line = _HistoryLineAt(pos.y, lineBuffer);
			}
		} else {
			if (pos.x > line->length)
				pos.x = line->length;
		}
	}

	int32 patternByteLen = strlen(_pattern);

	// convert pattern to UTF8Char array
	BStackOrHeapArray<UTF8Char, 64> pattern(patternByteLen);
	if (!pattern.IsValid())
		return false;
	int32 patternLen = 0;
	while (*_pattern != '\0') {
		int32 charLen = UTF8Char::ByteCount(*_pattern);
		if (charLen > 0) {
			pattern[patternLen].SetTo(_pattern, charLen);

			// if not case sensitive, convert to lower case
			if (!caseSensitive && charLen == 1)
				pattern[patternLen] = pattern[patternLen].ToLower();

			patternLen++;
			_pattern += charLen;
		} else
			_pattern++;
	}
//debug_printf("  pattern byte len: %ld, pattern len: %ld\n", patternByteLen, patternLen);

	if (patternLen == 0)
		return false;

	// reverse pattern, if searching backward
	if (!forward) {
		for (int32 i = 0; i < patternLen / 2; i++)
			std::swap(pattern[i], pattern[patternLen - i - 1]);
	}

	// search loop
	int32 matchIndex = 0;
	TermPos matchStart;
	while (true) {
//debug_printf("    (%ld, %ld): matchIndex: %ld\n", pos.x, pos.y, matchIndex);
		TermPos previousPos(pos);
		UTF8Char c;
		if (!(forward ? _NextChar(pos, c) : _PreviousChar(pos, c)))
			return false;

		if (caseSensitive ? (c == pattern[matchIndex])
				: (c.ToLower() == pattern[matchIndex])) {
			if (matchIndex == 0)
				matchStart = previousPos;

			matchIndex++;

			if (matchIndex == patternLen) {
//debug_printf("      match!\n");
				// compute the match range
				TermPos matchEnd(pos);
				if (!forward)
					std::swap(matchStart, matchEnd);

				// check word match
				if (matchWord) {
					TermPos tempPos(matchStart);
					if ((_PreviousChar(tempPos, c) && !c.IsSpace())
						|| (_NextChar(tempPos = matchEnd, c) && !c.IsSpace())) {
//debug_printf("      but no word match!\n");
						continue;
					}
				}

				_matchStart = matchStart;
				_matchEnd = matchEnd;
//debug_printf("  -> (%ld, %ld) - (%ld, %ld)\n", matchStart.x, matchStart.y,
//matchEnd.x, matchEnd.y);
				return true;
			}
		} else if (matchIndex > 0) {
			// continue after the position where we started matching
			pos = matchStart;
			if (forward)
				_NextChar(pos, c);
			else
				_PreviousChar(pos, c);
			matchIndex = 0;
		}
	}
}


void
BasicTerminalBuffer::InsertChar(UTF8Char c)
{
//debug_printf("BasicTerminalBuffer::InsertChar('%.*s' (%d), %#lx)\n",
//(int)c.ByteCount(), c.bytes, c.bytes[0], attributes);
	fLast = c;
	int32 width = c.IsFullWidth() ? FULL_WIDTH : HALF_WIDTH;

	if (fSoftWrappedCursor || (fCursor.x + width) > fWidth)
		_SoftBreakLine();
	else
		_PadLineToCursor();

	fSoftWrappedCursor = false;

	if (!fOverwriteMode)
		_InsertGap(width);

	TerminalLine* line = _LineAt(fCursor.y);
	line->cells[fCursor.x].character = c;
	line->cells[fCursor.x].attributes = fAttributes;
	line->cells[fCursor.x].attributes.state |= (width == FULL_WIDTH ? A_WIDTH : 0);

	if (line->length < fCursor.x + width)
		line->length = fCursor.x + width;

	_Invalidate(fCursor.y, fCursor.y);

	fCursor.x += width;

// TODO: Deal correctly with full-width chars! We must take care not to
// overwrite half of a full-width char. This holds also for other methods.

	if (fCursor.x == fWidth) {
		fCursor.x -= width;
		fSoftWrappedCursor = true;
	}
}


void
BasicTerminalBuffer::FillScreen(UTF8Char c, Attributes &attributes)
{
	uint32 width = HALF_WIDTH;
	if (c.IsFullWidth()) {
		attributes |= A_WIDTH;
		width = FULL_WIDTH;
	}

	fSoftWrappedCursor = false;

	for (int32 y = 0; y < fHeight; y++) {
		TerminalLine *line = _LineAt(y);
		for (int32 x = 0; x < fWidth / (int32)width; x++) {
			line->cells[x].character = c;
			line->cells[x].attributes = attributes;
		}
		line->length = fWidth / width;
	}

	_Invalidate(0, fHeight - 1);
}


void
BasicTerminalBuffer::InsertCR()
{
	TerminalLine* line = _LineAt(fCursor.y);

	line->attributes = fAttributes;
	line->softBreak = false;
	fSoftWrappedCursor = false;
	fCursor.x = 0;
	_Invalidate(fCursor.y, fCursor.y);
	_CursorChanged();
}


void
BasicTerminalBuffer::InsertLF()
{
	fSoftWrappedCursor = false;

	// If we're at the end of the scroll region, scroll. Otherwise just advance
	// the cursor.
	if (fCursor.y == fScrollBottom) {
		_Scroll(fScrollTop, fScrollBottom, 1);
	} else {
		if (fCursor.y < fHeight - 1)
			fCursor.y++;
		_CursorChanged();
	}
}


void
BasicTerminalBuffer::InsertRI()
{
	fSoftWrappedCursor = false;

	// If we're at the beginning of the scroll region, scroll. Otherwise just
	// reverse the cursor.
	if (fCursor.y == fScrollTop) {
		_Scroll(fScrollTop, fScrollBottom, -1);
	} else {
		if (fCursor.y > 0)
			fCursor.y--;
		_CursorChanged();
	}
}


void
BasicTerminalBuffer::InsertTab()
{
	int32 x;

	fSoftWrappedCursor = false;

	// Find the next tab stop
	for (x = fCursor.x + 1; x < fWidth && !fTabStops[x]; x++)
		;
	// Ensure x stayx within the line bounds
	x = restrict_value(x, 0, fWidth - 1);

	if (x != fCursor.x) {
		TerminalLine* line = _LineAt(fCursor.y);
		for (int32 i = fCursor.x; i <= x; i++) {
			if (line->length <= i) {
				line->cells[i].character = ' ';
				line->cells[i].attributes = fAttributes;
			}
		}
		fCursor.x = x;
		if (line->length < fCursor.x)
			line->length = fCursor.x;
		_CursorChanged();
	}
}


void
BasicTerminalBuffer::InsertCursorBackTab(int32 numTabs)
{
	int32 x = fCursor.x - 1;

	fSoftWrappedCursor = false;

	// Find the next tab stop
	while (numTabs-- > 0)
		for (; x >=0 && !fTabStops[x]; x--)
			;
	// Ensure x stays within the line bounds
	x = restrict_value(x, 0, fWidth - 1);

	if (x != fCursor.x) {
		fCursor.x = x;
		_CursorChanged();
	}
}


void
BasicTerminalBuffer::InsertLines(int32 numLines)
{
	if (fCursor.y >= fScrollTop && fCursor.y < fScrollBottom) {
		fSoftWrappedCursor = false;
		_Scroll(fCursor.y, fScrollBottom, -numLines);
	}
}


void
BasicTerminalBuffer::SetInsertMode(int flag)
{
	fOverwriteMode = flag == MODE_OVER;
}


void
BasicTerminalBuffer::InsertSpace(int32 num)
{
// TODO: Deal with full-width chars!
	if (fCursor.x + num > fWidth)
		num = fWidth - fCursor.x;

	if (num > 0) {
		fSoftWrappedCursor = false;
		_PadLineToCursor();
		_InsertGap(num);

		TerminalLine* line = _LineAt(fCursor.y);
		for (int32 i = fCursor.x; i < fCursor.x + num; i++) {
			line->cells[i].character = kSpaceChar;
			line->cells[i].attributes = line->cells[fCursor.x - 1].attributes;
		}
		line->attributes = fAttributes;

		_Invalidate(fCursor.y, fCursor.y);
	}
}


void
BasicTerminalBuffer::EraseCharsFrom(int32 first, int32 numChars)
{
	TerminalLine* line = _LineAt(fCursor.y);

	int32 end = min_c(first + numChars, fWidth);
	for (int32 i = first; i < end; i++)
		line->cells[i].attributes = fAttributes;

	line->attributes = fAttributes;

	fSoftWrappedCursor = false;

	end = min_c(first + numChars, line->length);
	if (first > 0 && line->cells[first - 1].attributes.IsWidth())
		first--;
	if (end > 0 && line->cells[end - 1].attributes.IsWidth())
		end++;

	for (int32 i = first; i < end; i++) {
		line->cells[i].character = kSpaceChar;
		line->cells[i].attributes = fAttributes;
	}

	_Invalidate(fCursor.y, fCursor.y);
}


void
BasicTerminalBuffer::EraseAbove()
{
	// Clear the preceding lines.
	if (fCursor.y > 0)
		_ClearLines(0, fCursor.y - 1);

	fSoftWrappedCursor = false;

	// Delete the chars on the cursor line before (and including) the cursor.
	TerminalLine* line = _LineAt(fCursor.y);
	if (fCursor.x < line->length) {
		int32 to = fCursor.x;
		if (line->cells[fCursor.x].attributes.IsWidth())
			to++;
		for (int32 i = 0; i <= to; i++) {
			line->cells[i].attributes = fAttributes;
			line->cells[i].character = kSpaceChar;
		}
	} else
		line->Clear(fAttributes, fWidth);

	_Invalidate(fCursor.y, fCursor.y);
}


void
BasicTerminalBuffer::EraseBelow()
{
	fSoftWrappedCursor = false;

	// Clear the following lines.
	if (fCursor.y < fHeight - 1)
		_ClearLines(fCursor.y + 1, fHeight - 1);

	// Delete the chars on the cursor line after (and including) the cursor.
	DeleteColumns();
}


void
BasicTerminalBuffer::EraseAll()
{
	fSoftWrappedCursor = false;
	_Scroll(0, fHeight - 1, fHeight);
}


void
BasicTerminalBuffer::EraseScrollback()
{
	fSoftWrappedCursor = false;

	// Clear the history
	if (fHistory != NULL)
		fHistory->Clear();

	// Update the scrollbars
	_Invalidate(0, 0);
}


void
BasicTerminalBuffer::DeleteChars(int32 numChars)
{
	fSoftWrappedCursor = false;

	TerminalLine* line = _LineAt(fCursor.y);
	if (fCursor.x < line->length) {
		if (fCursor.x + numChars < line->length) {
			int32 left = line->length - fCursor.x - numChars;
			memmove(line->cells + fCursor.x, line->cells + fCursor.x + numChars,
				left * sizeof(TerminalCell));
			line->length = fCursor.x + left;
			// process BCE on freed tail cells
			for (int i = 0; i < numChars; i++)
				line->cells[fCursor.x + left + i].attributes = fAttributes;
		} else {
			// process BCE on freed tail cells
			for (int i = 0; i < line->length - fCursor.x; i++)
				line->cells[fCursor.x + i].attributes = fAttributes;
			// remove all remaining chars
			line->length = fCursor.x;
		}

		_Invalidate(fCursor.y, fCursor.y);
	}
}


void
BasicTerminalBuffer::DeleteColumnsFrom(int32 first)
{
	fSoftWrappedCursor = false;

	TerminalLine* line = _LineAt(fCursor.y);

	for (int32 i = first; i < fWidth; i++)
		line->cells[i].attributes = fAttributes;

	if (first <= line->length) {
		line->length = first;
		line->attributes = fAttributes;
	}
	_Invalidate(fCursor.y, fCursor.y);
}


void
BasicTerminalBuffer::DeleteLines(int32 numLines)
{
	if (fCursor.y >= fScrollTop && fCursor.y <= fScrollBottom) {
		fSoftWrappedCursor = false;
		_Scroll(fCursor.y, fScrollBottom, numLines);
	}
}


void
BasicTerminalBuffer::SaveCursor()
{
	fSavedCursors.push(fCursor);
}


void
BasicTerminalBuffer::RestoreCursor()
{
	if (fSavedCursors.size() == 0)
		return;

	_SetCursor(fSavedCursors.top().x, fSavedCursors.top().y, true);
	fSavedCursors.pop();
}


void
BasicTerminalBuffer::SetScrollRegion(int32 top, int32 bottom)
{
	fScrollTop = restrict_value(top, 0, fHeight - 1);
	fScrollBottom = restrict_value(bottom, fScrollTop, fHeight - 1);

	// also sets the cursor position
	_SetCursor(0, 0, false);
}


void
BasicTerminalBuffer::SetOriginMode(bool enabled)
{
	fOriginMode = enabled;
	_SetCursor(0, 0, false);
}


void
BasicTerminalBuffer::SaveOriginMode()
{
	fSavedOriginMode = fOriginMode;
}


void
BasicTerminalBuffer::RestoreOriginMode()
{
	fOriginMode = fSavedOriginMode;
}


void
BasicTerminalBuffer::SetTabStop(int32 x)
{
	x = restrict_value(x, 0, fWidth - 1);
	fTabStops[x] = true;
}


void
BasicTerminalBuffer::ClearTabStop(int32 x)
{
	x = restrict_value(x, 0, fWidth - 1);
	fTabStops[x] = false;
}


void
BasicTerminalBuffer::ClearAllTabStops()
{
	for (int32 i = 0; i < fWidth; i++)
		fTabStops[i] = false;
}


void
BasicTerminalBuffer::NotifyListener()
{
	// Implemented by derived classes.
}


// #pragma mark - private methods


void
BasicTerminalBuffer::_SetCursor(int32 x, int32 y, bool absolute)
{
//debug_printf("BasicTerminalBuffer::_SetCursor(%d, %d)\n", x, y);
	fSoftWrappedCursor = false;

	x = restrict_value(x, 0, fWidth - 1);
	if (fOriginMode && !absolute) {
		y += fScrollTop;
		y = restrict_value(y, fScrollTop, fScrollBottom);
	} else {
		y = restrict_value(y, 0, fHeight - 1);
	}

	if (x != fCursor.x || y != fCursor.y) {
		fCursor.x = x;
		fCursor.y = y;
		_CursorChanged();
	}
}


void
BasicTerminalBuffer::_InvalidateAll()
{
	fDirtyInfo.invalidateAll = true;

	if (!fDirtyInfo.messageSent) {
		NotifyListener();
		fDirtyInfo.messageSent = true;
	}
}


/* static */ TerminalLine**
BasicTerminalBuffer::_AllocateLines(int32 width, int32 count)
{
	TerminalLine** lines = (TerminalLine**)malloc(sizeof(TerminalLine*) * count);
	if (lines == NULL)
		return NULL;

	for (int32 i = 0; i < count; i++) {
		const int32 size = sizeof(TerminalLine)
			+ sizeof(TerminalCell) * (width - 1);
		lines[i] = (TerminalLine*)malloc(size);
		if (lines[i] == NULL) {
			_FreeLines(lines, i);
			return NULL;
		}
		lines[i]->Clear(width);
	}

	return lines;
}


/* static */ void
BasicTerminalBuffer::_FreeLines(TerminalLine** lines, int32 count)
{
	if (lines != NULL) {
		for (int32 i = 0; i < count; i++)
			free(lines[i]);

		free(lines);
	}
}


void
BasicTerminalBuffer::_ClearLines(int32 first, int32 last)
{
	int32 firstCleared = -1;
	int32 lastCleared = -1;

	for (int32 i = first; i <= last; i++) {
		TerminalLine* line = _LineAt(i);
		if (line->length > 0) {
			if (firstCleared == -1)
				firstCleared = i;
			lastCleared = i;
		}

		line->Clear(fAttributes, fWidth);
	}

	if (firstCleared >= 0)
		_Invalidate(firstCleared, lastCleared);
}


status_t
BasicTerminalBuffer::_ResizeHistory(int32 width, int32 historyCapacity)
{
	if (width == fWidth && historyCapacity == HistoryCapacity())
		return B_OK;

	if (historyCapacity <= 0) {
		// new history capacity is 0 -- delete the old history object
		delete fHistory;
		fHistory = NULL;

		return B_OK;
	}

	HistoryBuffer* history = new(std::nothrow) HistoryBuffer;
	if (history == NULL)
		return B_NO_MEMORY;

	status_t error = history->Init(width, historyCapacity);
	if (error != B_OK) {
		delete history;
		return error;
	}

	// Transfer the lines from the old history to the new one.
	if (fHistory != NULL) {
		int32 historySize = min_c(HistorySize(), historyCapacity);
		TerminalLine* lineBuffer = ALLOC_LINE_ON_STACK(fWidth);
		for (int32 i = historySize - 1; i >= 0; i--) {
			TerminalLine* line = fHistory->GetTerminalLineAt(i, lineBuffer);
			if (line->length > width)
				_TruncateLine(line, width);
			history->AddLine(line);
		}
	}

	delete fHistory;
	fHistory = history;

	return B_OK;
}


status_t
BasicTerminalBuffer::_ResizeSimple(int32 width, int32 height,
	int32 historyCapacity)
{
//debug_printf("BasicTerminalBuffer::_ResizeSimple(): (%ld, %ld) -> "
//"(%ld, %ld)\n", fWidth, fHeight, width, height);
	if (width == fWidth && height == fHeight)
		return B_OK;

	if (width != fWidth || historyCapacity != HistoryCapacity()) {
		status_t error = _ResizeHistory(width, historyCapacity);
		if (error != B_OK)
			return error;
	}

	TerminalLine** lines = _AllocateLines(width, height);
	if (lines == NULL)
		return B_NO_MEMORY;
		// NOTE: If width or history capacity changed, the object will be in
		// an invalid state, since the history will already use the new values.

	int32 endLine = min_c(fHeight, height);
	int32 firstLine = 0;

	if (height < fHeight) {
		if (endLine <= fCursor.y) {
			endLine = fCursor.y + 1;
			firstLine = endLine - height;
		}

		// push the first lines to the history
		if (fHistory != NULL) {
			for (int32 i = 0; i < firstLine; i++) {
				TerminalLine* line = _LineAt(i);
				if (width < fWidth)
					_TruncateLine(line, width);
				fHistory->AddLine(line);
			}
		}
	}

	// copy the lines we keep
	for (int32 i = firstLine; i < endLine; i++) {
		TerminalLine* sourceLine = _LineAt(i);
		TerminalLine* destLine = lines[i - firstLine];
		if (width < fWidth)
			_TruncateLine(sourceLine, width);
		memcpy(destLine, sourceLine, (int32)sizeof(TerminalLine)
			+ (sourceLine->length - 1) * (int32)sizeof(TerminalCell));
	}

	// clear the remaining lines
	for (int32 i = endLine - firstLine; i < height; i++)
		lines[i]->Clear(fAttributes, width);

	_FreeLines(fScreen, fHeight);
	fScreen = lines;

	if (fWidth != width) {
		status_t error = _ResetTabStops(width);
		if (error != B_OK)
			return error;
	}

	fWidth = width;
	fHeight = height;

	fScrollTop = 0;
	fScrollBottom = fHeight - 1;
	fOriginMode = fSavedOriginMode = false;

	fScreenOffset = 0;

	if (fCursor.x > width)
		fCursor.x = width;
	fCursor.y -= firstLine;
	fSoftWrappedCursor = false;

	return B_OK;
}


status_t
BasicTerminalBuffer::_ResizeRewrap(int32 width, int32 height,
	int32 historyCapacity)
{
//debug_printf("BasicTerminalBuffer::_ResizeRewrap(): (%ld, %ld, history: %ld) -> "
//"(%ld, %ld, history: %ld)\n", fWidth, fHeight, HistoryCapacity(), width, height,
//historyCapacity);

	// The width stays the same. _ResizeSimple() does exactly what we need.
	if (width == fWidth)
		return _ResizeSimple(width, height, historyCapacity);

	// The width changes. We have to allocate a new line array, a new history
	// and re-wrap all lines.

	TerminalLine** screen = _AllocateLines(width, height);
	if (screen == NULL)
		return B_NO_MEMORY;

	HistoryBuffer* history = NULL;

	if (historyCapacity > 0) {
		history = new(std::nothrow) HistoryBuffer;
		if (history == NULL) {
			_FreeLines(screen, height);
			return B_NO_MEMORY;
		}

		status_t error = history->Init(width, historyCapacity);
		if (error != B_OK) {
			_FreeLines(screen, height);
			delete history;
			return error;
		}
	}

	int32 historySize = HistorySize();
	int32 totalLines = historySize + fHeight;

	// re-wrap
	TerminalLine* lineBuffer = ALLOC_LINE_ON_STACK(fWidth);
	TermPos cursor;
	int32 destIndex = 0;
	int32 sourceIndex = 0;
	int32 sourceX = 0;
	int32 destTotalLines = 0;
	int32 destScreenOffset = 0;
	int32 maxDestTotalLines = INT_MAX;
	bool newDestLine = true;
	bool cursorSeen = false;
	TerminalLine* sourceLine = _HistoryLineAt(-historySize, lineBuffer);

	while (sourceIndex < totalLines) {
		TerminalLine* destLine = screen[destIndex];

		if (newDestLine) {
			// Clear a new dest line before using it. If we're about to
			// overwrite an previously written line, we push it to the
			// history first, though.
			if (history != NULL && destTotalLines >= height)
				history->AddLine(screen[destIndex]);
			destLine->Clear(fAttributes, width);
			newDestLine = false;
		}

		int32 sourceLeft = sourceLine->length - sourceX;
		int32 destLeft = width - destLine->length;
//debug_printf("    source: %ld, left: %ld, dest: %ld, left: %ld\n",
//sourceIndex, sourceLeft, destIndex, destLeft);

		if (sourceIndex == historySize && sourceX == 0) {
			destScreenOffset = destTotalLines;
			if (destLeft == 0 && sourceLeft > 0)
				destScreenOffset++;
			maxDestTotalLines = destScreenOffset + height;
//debug_printf("      destScreenOffset: %ld\n", destScreenOffset);
		}

		int32 toCopy = min_c(sourceLeft, destLeft);
		// If the last cell to copy is the first cell of a
		// full-width char, don't copy it yet.
		if (toCopy > 0 && sourceLine->cells[sourceX + toCopy - 1].attributes.IsWidth()) {
//debug_printf("      -> last char is full-width -- don't copy it\n");
			toCopy--;
		}

		// translate the cursor position
		if (fCursor.y + historySize == sourceIndex
			&& fCursor.x >= sourceX
			&& (fCursor.x < sourceX + toCopy
				|| (destLeft >= sourceLeft
					&& sourceX + sourceLeft <= fCursor.x))) {
			cursor.x = destLine->length + fCursor.x - sourceX;
			cursor.y = destTotalLines;

			if (cursor.x >= width) {
				// The cursor was in free space after the official end
				// of line.
				cursor.x = width - 1;
			}
//debug_printf("      cursor: (%ld, %ld)\n", cursor.x, cursor.y);

			cursorSeen = true;
		}

		if (toCopy > 0) {
			memcpy(destLine->cells + destLine->length,
				sourceLine->cells + sourceX, toCopy * sizeof(TerminalCell));
			destLine->length += toCopy;
		}

		destLine->attributes = sourceLine->attributes;

		bool nextDestLine = false;
		if (toCopy == sourceLeft) {
			if (!sourceLine->softBreak)
				nextDestLine = true;
			sourceIndex++;
			sourceX = 0;
			sourceLine = _HistoryLineAt(sourceIndex - historySize,
				lineBuffer);
		} else {
			destLine->softBreak = true;
			nextDestLine = true;
			sourceX += toCopy;
		}

		if (nextDestLine) {
			destIndex = (destIndex + 1) % height;
			destTotalLines++;
			newDestLine = true;
			if (cursorSeen && destTotalLines >= maxDestTotalLines)
				break;
		}
	}

	// If the last source line had a soft break, the last dest line
	// won't have been counted yet.
	if (!newDestLine) {
		destIndex = (destIndex + 1) % height;
		destTotalLines++;
	}

//debug_printf("  total lines: %ld -> %ld\n", totalLines, destTotalLines);

	if (destTotalLines - destScreenOffset > height)
		destScreenOffset = destTotalLines - height;

	cursor.y -= destScreenOffset;

	// When there are less lines (starting with the screen offset) than
	// there's room in the screen, clear the remaining screen lines.
	for (int32 i = destTotalLines; i < destScreenOffset + height; i++) {
		// Move the line we're going to clear to the history, if that's a
		// line we've written earlier.
		TerminalLine* line = screen[i % height];
		if (history != NULL && i >= height)
			history->AddLine(line);
		line->Clear(fAttributes, width);
	}

	// Update the values
	_FreeLines(fScreen, fHeight);
	delete fHistory;

	fScreen = screen;
	fHistory = history;

	if (fWidth != width) {
		status_t error = _ResetTabStops(width);
		if (error != B_OK)
			return error;
	}

//debug_printf("  cursor: (%ld, %ld) -> (%ld, %ld)\n", fCursor.x, fCursor.y,
//cursor.x, cursor.y);
	fCursor.x = cursor.x;
	fCursor.y = cursor.y;
	fSoftWrappedCursor = false;
//debug_printf("  screen offset: %ld -> %ld\n", fScreenOffset, destScreenOffset % height);
	fScreenOffset = destScreenOffset % height;
//debug_printf("  height %ld -> %ld\n", fHeight, height);
//debug_printf("  width %ld -> %ld\n", fWidth, width);
	fHeight = height;
	fWidth = width;

	fScrollTop = 0;
	fScrollBottom = fHeight - 1;
	fOriginMode = fSavedOriginMode = false;

	return B_OK;
}


status_t
BasicTerminalBuffer::_ResetTabStops(int32 width)
{
	if (fTabStops != NULL)
		delete[] fTabStops;

	fTabStops = new(std::nothrow) bool[width];
	if (fTabStops == NULL)
		return B_NO_MEMORY;

	for (int32 i = 0; i < width; i++)
		fTabStops[i] = (i % TAB_WIDTH) == 0;
	return B_OK;
}


void
BasicTerminalBuffer::_Scroll(int32 top, int32 bottom, int32 numLines)
{
	if (numLines == 0)
		return;

	if (numLines > 0) {
		// scroll text up
		if (top == 0) {
			// The lines scrolled out of the screen range are transferred to
			// the history.

			// add the lines to the history
			if (fHistory != NULL) {
				int32 toHistory = min_c(numLines, bottom - top + 1);
				for (int32 i = 0; i < toHistory; i++)
					fHistory->AddLine(_LineAt(i));

				if (toHistory < numLines)
					fHistory->AddEmptyLines(numLines - toHistory);
			}

			if (numLines >= bottom - top + 1) {
				// all lines are scrolled out of range -- just clear them
				_ClearLines(top, bottom);
			} else if (bottom == fHeight - 1) {
				// full screen scroll -- update the screen offset and clear new
				// lines
				fScreenOffset = (fScreenOffset + numLines) % fHeight;
				for (int32 i = bottom - numLines + 1; i <= bottom; i++)
					_LineAt(i)->Clear(fAttributes, fWidth);
			} else {
				// Partial screen scroll. We move the screen offset anyway, but
				// have to move the unscrolled lines to their new location.
				// TODO: It may be more efficient to actually move the scrolled
				// lines only (might depend on the number of scrolled/unscrolled
				// lines).
				for (int32 i = fHeight - 1; i > bottom; i--) {
					std::swap(fScreen[_LineIndex(i)],
						fScreen[_LineIndex(i + numLines)]);
				}

				// update the screen offset and clear the new lines
				fScreenOffset = (fScreenOffset + numLines) % fHeight;
				for (int32 i = bottom - numLines + 1; i <= bottom; i++)
					_LineAt(i)->Clear(fAttributes, fWidth);
			}

			// scroll/extend dirty range

			if (fDirtyInfo.dirtyTop != INT_MAX) {
				// If the top or bottom of the dirty region are above the
				// bottom of the scroll region, we have to scroll them up.
				if (fDirtyInfo.dirtyTop <= bottom) {
					fDirtyInfo.dirtyTop -= numLines;
					if (fDirtyInfo.dirtyBottom <= bottom)
						fDirtyInfo.dirtyBottom -= numLines;
				}

				// numLines above the bottom become dirty
				_Invalidate(bottom - numLines + 1, bottom);
			}

			fDirtyInfo.linesScrolled += numLines;

			// invalidate new empty lines
			_Invalidate(bottom + 1 - numLines, bottom);

			// In case only part of the screen was scrolled, we invalidate also
			// the lines below the scroll region. Those remain unchanged, but
			// we can't convey that they have not been scrolled via
			// TerminalBufferDirtyInfo. So we need to force the view to sync
			// them again.
			if (bottom < fHeight - 1)
				_Invalidate(bottom + 1, fHeight - 1);
		} else if (numLines >= bottom - top + 1) {
			// all lines are completely scrolled out of range -- just clear
			// them
			_ClearLines(top, bottom);
		} else {
			// partial scroll -- clear the lines scrolled out of range and move
			// the other ones
			for (int32 i = top + numLines; i <= bottom; i++) {
				int32 lineToDrop = _LineIndex(i - numLines);
				int32 lineToKeep = _LineIndex(i);
				fScreen[lineToDrop]->Clear(fAttributes, fWidth);
				std::swap(fScreen[lineToDrop], fScreen[lineToKeep]);
			}
			// clear any lines between the two swapped ranges above
			for (int32 i = bottom - numLines + 1; i < top + numLines; i++)
				_LineAt(i)->Clear(fAttributes, fWidth);

			_Invalidate(top, bottom);
		}
	} else {
		// scroll text down
		numLines = -numLines;

		if (numLines >= bottom - top + 1) {
			// all lines are completely scrolled out of range -- just clear
			// them
			_ClearLines(top, bottom);
		} else {
			// partial scroll -- clear the lines scrolled out of range and move
			// the other ones
// TODO: When scrolling the whole screen, we could just update fScreenOffset and
// clear the respective lines.
			for (int32 i = bottom - numLines; i >= top; i--) {
				int32 lineToKeep = _LineIndex(i);
				int32 lineToDrop = _LineIndex(i + numLines);
				fScreen[lineToDrop]->Clear(fAttributes, fWidth);
				std::swap(fScreen[lineToDrop], fScreen[lineToKeep]);
			}
			// clear any lines between the two swapped ranges above
			for (int32 i = bottom - numLines + 1; i < top + numLines; i++)
				_LineAt(i)->Clear(fAttributes, fWidth);

			_Invalidate(top, bottom);
		}
	}
}


void
BasicTerminalBuffer::_SoftBreakLine()
{
	TerminalLine* line = _LineAt(fCursor.y);
	line->softBreak = true;

	fCursor.x = 0;
	if (fCursor.y == fScrollBottom)
		_Scroll(fScrollTop, fScrollBottom, 1);
	else
		fCursor.y++;
}


void
BasicTerminalBuffer::_PadLineToCursor()
{
	TerminalLine* line = _LineAt(fCursor.y);
	if (line->length < fCursor.x)
		for (int32 i = line->length; i < fCursor.x; i++)
			line->cells[i].character = kSpaceChar;
}


/*static*/ void
BasicTerminalBuffer::_TruncateLine(TerminalLine* line, int32 length)
{
	if (line->length <= length)
		return;

	if (length > 0 && line->cells[length - 1].attributes.IsWidth())
		length--;

	line->length = length;
}


void
BasicTerminalBuffer::_InsertGap(int32 width)
{
// ASSERT(fCursor.x + width <= fWidth)
	TerminalLine* line = _LineAt(fCursor.y);

	int32 toMove = min_c(line->length - fCursor.x, fWidth - fCursor.x - width);
	if (toMove > 0) {
		memmove(line->cells + fCursor.x + width,
			line->cells + fCursor.x, toMove * sizeof(TerminalCell));
	}

	line->length = min_c(line->length + width, fWidth);
}


/*!	\a endColumn is not inclusive.
*/
TerminalLine*
BasicTerminalBuffer::_GetPartialLineString(BString& string, int32 row,
	int32 startColumn, int32 endColumn) const
{
	TerminalLine* lineBuffer = ALLOC_LINE_ON_STACK(fWidth);
	TerminalLine* line = _HistoryLineAt(row, lineBuffer);
	if (line == NULL)
		return NULL;

	if (endColumn > line->length)
		endColumn = line->length;

	for (int32 x = startColumn; x < endColumn; x++) {
		const TerminalCell& cell = line->cells[x];
		string.Append(cell.character.bytes, cell.character.ByteCount());

		if (cell.attributes.IsWidth())
			x++;
	}

	return line;
}


/*!	Decrement \a pos and return the char at that location.
*/
bool
BasicTerminalBuffer::_PreviousChar(TermPos& pos, UTF8Char& c) const
{
	pos.x--;

	TerminalLine* lineBuffer = ALLOC_LINE_ON_STACK(fWidth);
	TerminalLine* line = _HistoryLineAt(pos.y, lineBuffer);

	while (true) {
		if (pos.x < 0) {
			pos.y--;
			line = _HistoryLineAt(pos.y, lineBuffer);
			if (line == NULL)
				return false;

			pos.x = line->length;
			if (line->softBreak) {
				pos.x--;
			} else {
				c = '\n';
				return true;
			}
		} else {
			c = line->cells[pos.x].character;
			return true;
		}
	}
}


/*!	Return the char at \a pos and increment it.
*/
bool
BasicTerminalBuffer::_NextChar(TermPos& pos, UTF8Char& c) const
{
	TerminalLine* lineBuffer = ALLOC_LINE_ON_STACK(fWidth);
	TerminalLine* line = _HistoryLineAt(pos.y, lineBuffer);
	if (line == NULL)
		return false;

	if (pos.x >= line->length) {
		c = '\n';
		pos.x = 0;
		pos.y++;
		return true;
	}

	c = line->cells[pos.x].character;

	pos.x++;
	while (line != NULL && pos.x >= line->length && line->softBreak) {
		pos.x = 0;
		pos.y++;
		line = _HistoryLineAt(pos.y, lineBuffer);
	}

	return true;
}


bool
BasicTerminalBuffer::_PreviousLinePos(TerminalLine* lineBuffer,
	TerminalLine*& line, TermPos& pos) const
{
	if (--pos.x < 0) {
		// Hit the beginning of the line -- continue at the end of the
		// previous line, if it soft-breaks.
		pos.y--;
		if ((line = _HistoryLineAt(pos.y, lineBuffer)) == NULL
				|| !line->softBreak || line->length == 0) {
			return false;
		}
		pos.x = line->length - 1;
	}
	if (pos.x > 0 && line->cells[pos.x - 1].attributes.IsWidth())
		pos.x--;

	return true;
}


bool
BasicTerminalBuffer::_NormalizeLinePos(TerminalLine* lineBuffer,
	TerminalLine*& line, TermPos& pos) const
{
	if (pos.x < line->length)
		return true;

	// Hit the end of the line -- if it soft-breaks continue with the
	// next line.
	if (!line->softBreak)
		return false;

	pos.y++;
	pos.x = 0;
	line = _HistoryLineAt(pos.y, lineBuffer);
	return line != NULL;
}


#ifdef USE_DEBUG_SNAPSHOTS

void
BasicTerminalBuffer::MakeLinesSnapshots(time_t timeStamp, const char* fileName)
{
	BString str("/var/log/");
	struct tm* ts = gmtime(&timeStamp);
	str << ts->tm_hour << ts->tm_min << ts->tm_sec;
	str << fileName;
	FILE* fileOut = fopen(str.String(), "w");

	bool dumpHistory = false;
	do {
		if (dumpHistory && fHistory == NULL) {
			fprintf(fileOut, "> History is empty <\n");
			break;
		}

		int countLines = dumpHistory ? fHistory->Size() : fHeight;
		fprintf(fileOut, "> %s lines dump begin <\n",
					dumpHistory ? "History" : "Terminal");

		TerminalLine* lineBuffer = ALLOC_LINE_ON_STACK(fWidth);
		for (int i = 0; i < countLines; i++) {
			TerminalLine* line = dumpHistory
				? fHistory->GetTerminalLineAt(i, lineBuffer)
					: fScreen[_LineIndex(i)];

			if (line == NULL) {
				fprintf(fileOut, "line: %d is NULL!!!\n", i);
				continue;
			}

			fprintf(fileOut, "%02" B_PRId16 ":%02" B_PRId16 ":%08" B_PRIx32 ":\n",
					i, line->length, line->attributes.state);
			for (int j = 0; j < line->length; j++)
				if (line->cells[j].character.bytes[0] != 0)
					fwrite(line->cells[j].character.bytes, 1,
						line->cells[j].character.ByteCount(), fileOut);

			fprintf(fileOut, "\n");
			for (int s = 28; s >= 0; s -= 4) {
				for (int j = 0; j < fWidth; j++)
					fprintf(fileOut, "%01" B_PRIx32,
						(line->cells[j].attributes.state >> s) & 0x0F);

				fprintf(fileOut, "\n");
			}

			fprintf(fileOut, "\n");
		}

		fprintf(fileOut, "> %s lines dump finished <\n",
					dumpHistory ? "History" : "Terminal");

		dumpHistory = !dumpHistory;
	} while (dumpHistory);

	fclose(fileOut);
}


void
BasicTerminalBuffer::StartStopDebugCapture()
{
	if (fCaptureFile >= 0) {
		close(fCaptureFile);
		fCaptureFile = -1;
		return;
	}

	time_t timeStamp = time(NULL);
	BString str("/var/log/");
	struct tm* ts = gmtime(&timeStamp);
	str << ts->tm_hour << ts->tm_min << ts->tm_sec;
	str << ".Capture.log";
	fCaptureFile = open(str.String(), O_CREAT | O_WRONLY,
			S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
}


void
BasicTerminalBuffer::CaptureChar(char ch)
{
	if (fCaptureFile >= 0)
		write(fCaptureFile, &ch, 1);
}


void
BasicTerminalBuffer::InsertLastChar()
{
	InsertChar(fLast);
}

#endif
