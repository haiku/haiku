/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "TerminalBuffer.h"

#include <stdlib.h>
#include <string.h>

#include <algorithm>

#include <Autolock.h>
#include <Message.h>
#include <String.h>

#include "CodeConv.h"
#include "TermConst.h"
#include "TerminalCharClassifier.h"


static const UTF8Char kSpaceChar(' ');


static inline int32
restrict_value(int32 value, int32 min, int32 max)
{
	return value < min ? min : (value > max ? max : value);
}


// #pragma mark - private inline methods


inline int32
TerminalBuffer::_LineIndex(int32 index) const
{
	return (index + fScreenOffset) % fHistoryCapacity;
}


inline TerminalBuffer::Line*
TerminalBuffer::_LineAt(int32 index) const
{
	return fHistory[_LineIndex(index)];
}


inline TerminalBuffer::Line*
TerminalBuffer::_HistoryLineAt(int32 index) const
{
	if (index >= fHeight || index < -fHistorySize)
		return NULL;

	return _LineAt(index + fHistoryCapacity);
}


inline void
TerminalBuffer::_Invalidate(int32 top, int32 bottom)
{
//debug_printf("TerminalBuffer::_Invalidate(%ld, %ld)\n", top, bottom);
	if (top < fDirtyInfo.dirtyTop)
		fDirtyInfo.dirtyTop = top;
	if (bottom > fDirtyInfo.dirtyBottom)
		fDirtyInfo.dirtyBottom = bottom;

	if (!fDirtyInfo.messageSent) {
//debug_printf("TerminalBuffer::_Invalidate(): sending message\n");
		fListener.SendMessage(MSG_TERMINAL_BUFFER_CHANGED);
		fDirtyInfo.messageSent = true;
	}
}


inline void
TerminalBuffer::_CursorChanged()
{
	if (!fDirtyInfo.messageSent) {
		fListener.SendMessage(MSG_TERMINAL_BUFFER_CHANGED);
		fDirtyInfo.messageSent = true;
	}
}


// #pragma mark - public methods


TerminalBuffer::TerminalBuffer()
	:
	BLocker("terminal buffer"),
	fHistory(NULL),
	fEncoding(M_UTF8)
{
}


TerminalBuffer::~TerminalBuffer()
{
	_FreeLines(fHistory, fHistoryCapacity);
}


status_t
TerminalBuffer::Init(int32 width, int32 height, int32 historySize)
{
	if (Sem() < 0)
		return Sem();

	if (historySize < 2 * height)
		historySize = 2 * height;

	fWidth = width;
	fHeight = height;

	fScrollTop = 0;
	fScrollBottom = fHeight - 1;

	fCursor.x = 0;
	fCursor.y = 0;

	fOverwriteMode = true;

	fHistoryCapacity = historySize;
	fHistorySize = 0;

	fHistory = _AllocateLines(width, historySize);
	if (fHistory == NULL)
		return B_NO_MEMORY;

	_ClearLines(0, fHeight - 1);

	fDirtyInfo.Reset();

	return B_OK;
}


void
TerminalBuffer::SetListener(BMessenger listener)
{
	fListener = listener;
}


int
TerminalBuffer::Encoding() const
{
	return fEncoding;
}


void
TerminalBuffer::SetEncoding(int encoding)
{
	fEncoding = encoding;
}


status_t
TerminalBuffer::ResizeTo(int32 width, int32 height)
{
	BAutolock _(this);

	if (height < MIN_ROWS || height > MAX_ROWS || width < MIN_COLS
			|| width > MAX_COLS) {
		return B_BAD_VALUE;
	}

//debug_printf("TerminalBuffer::ResizeTo(): (%ld, %ld) -> (%ld, %ld)\n",
//fWidth, fHeight, width, height);

	if (width != fWidth) {
		// The width changes. We have to allocate a new line array and re-wrap
		// all lines.
		Line** history = _AllocateLines(width, fHistoryCapacity);
		if (history == NULL)
			return B_NO_MEMORY;

		int32 totalLines = fHistorySize + fHeight;
		int32 historyOffset = fHistoryCapacity - fHistorySize;
			// base for _LineAt() invocations to iterate through the history

		// re-wrap
		TermPos cursor;
		int32 destIndex = 0;
		int32 sourceIndex = 0;
		int32 sourceX = 0;
		int32 destTotalLines = 0;
		int32 destScreenOffset = 0;
		int32 maxDestTotalLines = INT_MAX;
		bool newDestLine = true;
		while (sourceIndex < totalLines) {
			Line* sourceLine = _LineAt(historyOffset + sourceIndex);
			Line* destLine = history[destIndex];

			if (newDestLine) {
				destLine->Clear();
				newDestLine = false;
			}

			int32 sourceLeft = sourceLine->length - sourceX;
			int32 destLeft = width - destLine->length;
//debug_printf("    source: %ld, left: %ld, dest: %ld, left: %ld\n",
//sourceIndex, sourceLeft, destIndex, destLeft);

			if (sourceIndex == fHistorySize && sourceX == 0) {
				destScreenOffset = destTotalLines;
				if (destLeft == 0 && sourceLeft > 0)
					destScreenOffset++;
//debug_printf("      destScreenOffset: %ld\n", destScreenOffset);
			}

			int32 toCopy = min_c(sourceLeft, destLeft);
			if (toCopy > 0) {
				// If the last cell to copy is the first cell of a
				// full-width char, don't copy it yet.
				if (sourceX + toCopy > 0 && IS_WIDTH(
						sourceLine->cells[sourceX + toCopy - 1].attributes)) {
//debug_printf("      -> last char is full-width -- don't copy it\n");
					toCopy--;
				}

				// translate the cursor position
				if (fCursor.y + fHistorySize == sourceIndex
					&& fCursor.x >= sourceX
					&& (fCursor.x < sourceX + toCopy
						|| sourceX + sourceLeft <= fCursor.x)) {
					cursor.x = destLine->length + fCursor.x - sourceX;
					cursor.y = destTotalLines;

					if (cursor.x >= width) {
						// The cursor was in free space after the official end
						// of line.
						cursor.x = width - 1;
					}
//debug_printf("      cursor: (%ld, %ld)\n", cursor.x, cursor.y);

					// don't allow the cursor to get out of screen
					maxDestTotalLines = cursor.y + fHeight;
				}

				memcpy(destLine->cells + destLine->length,
					sourceLine->cells + sourceX, toCopy * sizeof(Cell));
				destLine->length += toCopy;
			}

			bool nextDestLine = false;
			if (toCopy == sourceLeft) {
				if (!sourceLine->softBreak)
					nextDestLine = true;
				sourceIndex++;
				sourceX = 0;
			} else {
				destLine->softBreak = true;
				nextDestLine = true;
				sourceX += toCopy;
			}

			if (nextDestLine) {
				destIndex = (destIndex + 1) % fHistoryCapacity;
				destTotalLines++;
				newDestLine = true;
				if (destTotalLines == maxDestTotalLines)
					break;
			}
		}

		// If the last source line had a soft break, the last dest line
		// won't have been counted yet.
		if (!newDestLine) {
			destIndex = (destIndex + 1) % fHistoryCapacity;
			destTotalLines++;
		}

//debug_printf("  total lines: %ld -> %ld\n", totalLines, destTotalLines);

		int32 tempHeight = destTotalLines - destScreenOffset;
		cursor.y -= destScreenOffset;

		// Re-wrapping might have produced more lines than we have room for.
		if (destTotalLines > fHistoryCapacity)
			destTotalLines = fHistoryCapacity;

		// Update the values
//debug_printf("  cursor: (%ld, %ld) -> (%ld, %ld)\n", fCursor.x, fCursor.y,
//cursor.x, cursor.y);
		fCursor.x = cursor.x;
		fCursor.y = cursor.y;
//debug_printf("  screen offset: %ld -> %ld\n", fScreenOffset,
//destScreenOffset % fHistoryCapacity);
		fScreenOffset = destScreenOffset % fHistoryCapacity;
//debug_printf("  history size: %ld -> %ld\n", fHistorySize, destTotalLines - fHeight);
		fHistorySize = destTotalLines - tempHeight;
//debug_printf("  height %ld -> %ld\n", fHeight, tempHeight);
		fHeight = tempHeight;
		fWidth = width;

		_FreeLines(fHistory, fHistoryCapacity);
		fHistory = history;
	}

	if (height == fHeight)
		return B_OK;

	// The height changes. We just add/remove lines at the end of the screen.

	if (height < fHeight) {
		// The screen shrinks. We just drop the lines at the end of the screen,
		// but we keep the cursor on screen at all costs.
		if (fCursor.y >= height) {
			int32 toShift = fCursor.y - height + 1;
			fScreenOffset = (fScreenOffset + fHistoryCapacity + toShift)
				% fHistoryCapacity;
			fHistorySize += toShift;
			fCursor.y -= toShift;
		}
	} else {
		// The screen grows. We add empty lines at the end of the current
		// screen.
		if (fHistorySize + height > fHistoryCapacity)
			fHistorySize = fHistoryCapacity - height;

		for (int32 i = fHeight; i < height; i++)
			_LineAt(i)->Clear();
	}

//debug_printf("  cursor: -> (%ld, %ld)\n", fCursor.x, fCursor.y);
//debug_printf("  screen offset: -> %ld\n", fScreenOffset);
//debug_printf("  history size: -> %ld\n", fHistorySize);

	fHeight = height;

	// reset scroll range to keep it simple
	fScrollTop = 0;
	fScrollBottom = fHeight - 1;

	return B_OK;
}


void
TerminalBuffer::Clear()
{
	BAutolock _(this);

	fHistorySize = 0;
	fScreenOffset = 0;

	_ClearLines(0, fHeight - 1);

	fCursor.SetTo(0, 0);

	fDirtyInfo.linesScrolled = 0;
	_Invalidate(0, fHeight - 1);
}


bool
TerminalBuffer::IsFullWidthChar(int32 row, int32 column) const
{
	Line* line = _HistoryLineAt(row);
	return line != NULL && column > 0 && column < line->length
		&& (line->cells[column - 1].attributes & A_WIDTH) != 0;
}


int
TerminalBuffer::GetChar(int32 row, int32 column, UTF8Char& character,
	uint16& attributes) const
{
	Line* line = _HistoryLineAt(row);
	if (line == NULL)
		return NO_CHAR;

	if (column < 0 || column >= line->length)
		return NO_CHAR;

	if (column > 0 && (line->cells[column - 1].attributes & A_WIDTH) != 0)
		return IN_STRING;

	Cell& cell = line->cells[column];
	character = cell.character;
	attributes = cell.attributes;
	return A_CHAR;
}


int32
TerminalBuffer::GetString(int32 row, int32 firstColumn, int32 lastColumn,
	char* buffer, uint16& attributes) const
{
	Line* line = _HistoryLineAt(row);
	if (line == NULL)
		return NO_CHAR;

	if (lastColumn >= line->length)
		lastColumn = line->length - 1;

	int32 column = firstColumn;
	if (column <= lastColumn)
		attributes = line->cells[column].attributes;

	for (; column <= lastColumn; column++) {
		Cell& cell = line->cells[column];
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
TerminalBuffer::GetStringFromRegion(BString& string, const TermPos& start,
	const TermPos& end) const
{
//debug_printf("TerminalBuffer::GetStringFromRegion((%ld, %ld), (%ld, %ld))\n",
//start.x, start.y, end.x, end.y);
	if (start >= end)
		return;

	TermPos pos(start);

	if (IsFullWidthChar(pos.y, pos.x))
		pos.x--;

	// get all but the last line
	while (pos.y < end.y) {
		if (_GetPartialLineString(string, pos.y, pos.x, fWidth))
			string.Append('\n', 1);
		pos.x = 0;
		pos.y++;
	}

	// get the last line, if not empty
	if (end.x > 0)
		_GetPartialLineString(string, end.y, pos.x, end.x);
}


bool
TerminalBuffer::FindWord(const TermPos& pos, TerminalCharClassifier* classifier,
	bool findNonWords, TermPos& _start, TermPos& _end) const
{
	int32 x = pos.x;
	int32 y = pos.y;

	Line* line = _HistoryLineAt(y);
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

	if (x > 0 && IS_WIDTH(line->cells[x - 1].attributes))
		x--;

	// get the char type at the given position
	int type = classifier->Classify(line->cells[x].character.bytes);

	// check whether we are supposed to find words only
	if (type != CHAR_TYPE_WORD_CHAR && !findNonWords)
		return false;

	// find the beginning
	TermPos start(x, y);
	TermPos end(x + (IS_WIDTH(line->cells[x].attributes) ? 2 : 1), y);
	while (true) {
		if (--x < 0) {
			// Hit the beginning of the line -- continue at the end of the
			// previous line, if it soft-breaks.
			y--;
			if ((line = _HistoryLineAt(y)) == NULL || !line->softBreak
					|| line->length == 0) {
				break;
			}
			x = line->length - 1;
		}
		if (x > 0 && IS_WIDTH(line->cells[x - 1].attributes))
			x--;

		if (classifier->Classify(line->cells[x].character.bytes) != type)
			break;

		start.SetTo(x, y);
	}

	// find the end
	x = end.x;
	y = end.y;
	line = _HistoryLineAt(y);

	while (true) {
		if (x >= line->length) {
			// Hit the end of the line -- if it soft-breaks continue with the
			// next line.
			if (!line->softBreak)
				break;
			y++;
			x = 0;
			if ((line = _HistoryLineAt(y)) == NULL)
				break;
		}

		if (classifier->Classify(line->cells[x].character.bytes) != type)
			break;

		x += IS_WIDTH(line->cells[x].attributes) ? 2 : 1;
		end.SetTo(x, y);
	}

	_start = start;
	_end = end;
	return true;
}


int32
TerminalBuffer::LineLength(int32 index) const
{
	Line* line = _HistoryLineAt(index);
	return line != NULL ? line->length : 0;
}


bool
TerminalBuffer::Find(const char* _pattern, const TermPos& start, bool forward,
	bool caseSensitive, bool matchWord, TermPos& _matchStart,
	TermPos& _matchEnd) const
{
//debug_printf("TerminalBuffer::Find(\"%s\", (%ld, %ld), forward: %d, case: %d, "
//"word: %d)\n", _pattern, start.x, start.y, forward, caseSensitive, matchWord);
	// normalize pos, so that _NextChar() and _PreviousChar() are happy
	TermPos pos(start);
	Line* line = _HistoryLineAt(pos.y);
	if (line != NULL) {
		if (forward) {
			while (line != NULL && pos.x >= line->length && line->softBreak) {
				pos.x = 0;
				pos.y++;
				line = _HistoryLineAt(pos.y);
			}
		} else {
			if (pos.x > line->length)
				pos.x = line->length;
		}
	}

	int32 patternByteLen = strlen(_pattern);

	// convert pattern to UTF8Char array
	UTF8Char pattern[patternByteLen];
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
					if (_PreviousChar(tempPos, c) && !c.IsSpace()
						|| _NextChar(tempPos = matchEnd, c) && !c.IsSpace()) {
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
TerminalBuffer::InsertChar(UTF8Char c, uint32 attributes)
{
//debug_printf("TerminalBuffer::InsertChar('%.*s' (%d), %#lx)\n",
//(int)c.ByteCount(), c.bytes, c.bytes[0], attributes);
	int width = CodeConv::UTF8GetFontWidth(c.bytes);
	if (width == FULL_WIDTH)
		attributes |= A_WIDTH;

	if (fCursor.x + width > fWidth)
		_SoftBreakLine();
	else
		_PadLineToCursor();

	if (!fOverwriteMode)
		_InsertGap(width);

	Line* line = _LineAt(fCursor.y);
	line->cells[fCursor.x].character = c;
	line->cells[fCursor.x].attributes = attributes;

	if (line->length < fCursor.x + width)
		line->length = fCursor.x + width;

	_Invalidate(fCursor.y, fCursor.y);

	fCursor.x += width;

// TODO: Deal correctly with full-width chars! We must take care not to
// overwrite half of a full-width char. This holds also for other methods.

	if (fCursor.x == fWidth)
		_SoftBreakLine();
			// TODO: Handle a subsequent CR correctly!
}


void
TerminalBuffer::Insert(uchar* string, ushort attr)
{
// TODO: Remove! Use InsertChar instead!
	UTF8Char character((const char*)string, 4);
	InsertChar(character, attr);
}


void
TerminalBuffer::InsertCR()
{
	_LineAt(fCursor.y)->softBreak = false;
	fCursor.x = 0;
	_CursorChanged();
}


void
TerminalBuffer::InsertLF()
{
	// If we're at the end of the scroll region, scroll. Otherwise just advance
	// the cursor.
	if (fCursor.y == fScrollBottom) {
		_Scroll(fScrollTop, fScrollBottom, 1);
	} else {
		fCursor.y++;
		_CursorChanged();
	}
}


void
TerminalBuffer::InsertNewLine(int numLines)
{
	if (fCursor.y >= fScrollTop && fCursor.y < fScrollBottom)
		_Scroll(fCursor.y, fScrollBottom, -numLines);
}


void
TerminalBuffer::SetInsertMode(int flag)
{
	fOverwriteMode = flag == MODE_OVER;
}


void
TerminalBuffer::InsertSpace(int num)
{
// TODO: Deal with full-width chars!
	if (fCursor.x + num > fWidth)
		num = fWidth - fCursor.x;

	if (num > 0) {
		_PadLineToCursor();
		_InsertGap(num);

		Line* line = _LineAt(fCursor.y);
		for (int32 i = fCursor.x; i < fCursor.x + num; i++) {
			line->cells[i].character = kSpaceChar;
			line->cells[i].attributes = 0;
		}
	}
}


void
TerminalBuffer::EraseBelow()
{
	_Scroll(fCursor.y, fHeight - 1, fHeight);
}


void
TerminalBuffer::DeleteChar(int numChars)
{
	Line* line = _LineAt(fCursor.y);
	if (fCursor.x < line->length) {
		if (fCursor.x + numChars < line->length) {
			int32 left = line->length - fCursor.x - numChars;
			memmove(line->cells + fCursor.x, line->cells + fCursor.x + numChars,
				left * sizeof(Cell));
			line->length = fCursor.x + left;
		} else {
			// remove all remaining chars
			line->length = fCursor.x;
		}

		_Invalidate(fCursor.y, fCursor.y);
	}
}


void
TerminalBuffer::DeleteColumns()
{
	Line* line = _LineAt(fCursor.y);
	if (fCursor.x < line->length) {
		line->length = fCursor.x;
		_Invalidate(fCursor.y, fCursor.y);
	}
}


void
TerminalBuffer::DeleteLine(int numLines)
{
	if (fCursor.y >= fScrollTop && fCursor.y <= fScrollBottom)
		_Scroll(fCursor.y, fScrollBottom, numLines);
}


void
TerminalBuffer::SetCurPos(int x, int y)
{
//debug_printf("TerminalBuffer::SetCurPos(%d, %d)\n", x, y);
	x = restrict_value(x, 0, fWidth - 1);
	y = restrict_value(y, fScrollTop, fScrollBottom);
	if (x != fCursor.x || y != fCursor.y) {
		fCursor.x = x;
		fCursor.y = y;
		_CursorChanged();
	}
}


void
TerminalBuffer::SetCurX(int x)
{
	SetCurPos(x, fCursor.y);
}


void
TerminalBuffer::SetCurY(int y)
{
	SetCurPos(fCursor.x, y);
}


int
TerminalBuffer::GetCurX()
{
	return fCursor.x;
}


void
TerminalBuffer::SaveCursor()
{
	fSavedCursor = fCursor;
}


void
TerminalBuffer::RestoreCursor()
{
	SetCurPos(fSavedCursor.x, fSavedCursor.y);
}


void
TerminalBuffer::MoveCurRight(int num)
{
	SetCurPos(fCursor.x + num, fCursor.y);
}


void
TerminalBuffer::MoveCurLeft(int num)
{
	SetCurPos(fCursor.x - num, fCursor.y);
}


void
TerminalBuffer::MoveCurUp(int num)
{
	SetCurPos(fCursor.x, fCursor.y - num);
}


void
TerminalBuffer::MoveCurDown(int num)
{
	SetCurPos(fCursor.x, fCursor.y + num);
}


void
TerminalBuffer::ScrollRegion(int top, int bot, int dir, int numLines)
{
// TODO: Is only invoked with SCRDOWN and numLines = 1
	_Scroll(fScrollTop, fScrollBottom, -1);
}


void
TerminalBuffer::SetScrollRegion(int top, int bottom)
{
	fScrollTop = restrict_value(top, 0, fHeight - 1);
	fScrollBottom = restrict_value(bottom, fScrollTop, fHeight - 1);

	// also sets the cursor position
	SetCurPos(0, 0);
}


void
TerminalBuffer::SetTitle(const char* title)
{
	BMessage message(MSG_SET_TERMNAL_TITLE);
	message.AddString("title", title);
	fListener.SendMessage(&message);
}


void
TerminalBuffer::NotifyQuit(int32 reason)
{
	BMessage message(MSG_QUIT_TERMNAL);
	message.AddInt32("reason", reason);
	fListener.SendMessage(&message);
}


// #pragma mark - private methods


/* static */ TerminalBuffer::Line**
TerminalBuffer::_AllocateLines(int32 width, int32 count)
{
	Line** lines = (Line**)malloc(sizeof(Line*) * count);
	if (lines == NULL)
		return NULL;

	for (int32 i = 0; i < count; i++) {
		lines[i] = (Line*)malloc(sizeof(Line) + sizeof(Cell) * (width - 1));
		if (lines[i] == NULL) {
			_FreeLines(lines, i);
			return NULL;
		}
	}

	return lines;
}


/* static */ void
TerminalBuffer::_FreeLines(Line** lines, int32 count)
{
	if (lines != NULL) {
		for (int32 i = 0; i < count; i++)
			free(lines[i]);

		free(lines);
	}
}


void
TerminalBuffer::_ClearLines(int32 first, int32 last)
{
	int32 firstCleared = -1;
	int32 lastCleared = -1;

	for (int32 i = first; i <= last; i++) {
		Line* line = _LineAt(i);
		if (line->length > 0) {
			if (firstCleared == -1)
				firstCleared = i;
			lastCleared = i;

			line->Clear();
		}
	}

	if (firstCleared >= 0)
		_Invalidate(firstCleared, lastCleared);
}


void
TerminalBuffer::_Scroll(int32 top, int32 bottom, int32 numLines)
{
	if (numLines == 0)
		return;

	if (numLines > 0) {
		if (numLines > fHistoryCapacity - fHeight) {
			numLines = fHistoryCapacity - fHeight;
			// TODO: This is not quite correct -- we should replace the complete
			// history and screen with empty lines.
		}

		// scroll text up
		if (top == 0) {
			// The lines scrolled out of the screen range are transferred to
			// the history.

			// make room for numLines new lines
			if (fHistorySize + fHeight + numLines > fHistoryCapacity) {
				int32 toDrop = fHistorySize + fHeight + numLines
					- fHistoryCapacity;
				fHistorySize -= toDrop;
			}

			// clear numLines after the current screen
			for (int32 i = fHeight; i < fHeight + numLines; i++)
				_LineAt(i)->Clear();

			if (bottom < fHeight - 1) {
				// Only scroll part of the screen. Move the unscrolled lines to
				// their new location.
				for (int32 i = bottom + 1; i < fHeight; i++) {
					std::swap(fHistory[_LineIndex(i)],
						fHistory[_LineIndex(i) + numLines]);
				}
			}

			// all lines are in place -- offset the screen
			fScreenOffset = (fScreenOffset + numLines) % fHistoryCapacity;
			fHistorySize += numLines;

			// scroll/extend dirty range
			if (fDirtyInfo.dirtyTop != INT_MAX) {
				if (fDirtyInfo.dirtyTop <= bottom
					&& top <= fDirtyInfo.dirtyBottom) {
					// intersects with the scroll region -- extend
					if (top < fDirtyInfo.dirtyTop)
						fDirtyInfo.dirtyTop = top - numLines;
					else
						fDirtyInfo.dirtyTop -= numLines;
					if (bottom > fDirtyInfo.dirtyBottom)
						fDirtyInfo.dirtyBottom = bottom;
				} else if (fDirtyInfo.dirtyBottom < top) {
					// does not intersect with the scroll region -- just offset
					fDirtyInfo.dirtyTop -= numLines;
					fDirtyInfo.dirtyBottom -= numLines;
				}
			}

			fDirtyInfo.linesScrolled += numLines;
// TODO: The linesScrolled might be suboptimal when scrolling partially
// only, since we would scroll the whole visible area, including unscrolled
// lines, which invalidates them, too.

			// invalidate new empty lines
			_Invalidate(bottom + 1 - numLines, bottom);

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
				fHistory[lineToDrop]->Clear();
				std::swap(fHistory[lineToDrop], fHistory[lineToKeep]);
			}

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
			for (int32 i = bottom - numLines; i >= top; i--) {
				int32 lineToKeep = _LineIndex(i);
				int32 lineToDrop = _LineIndex(i + numLines);
				fHistory[lineToDrop]->Clear();
				std::swap(fHistory[lineToDrop], fHistory[lineToKeep]);
			}

			_Invalidate(top, bottom);
		}
	}
}


void
TerminalBuffer::_SoftBreakLine()
{
	Line* line = _LineAt(fCursor.y);
	line->length = fCursor.x;
	line->softBreak = true;

	fCursor.x = 0;
	if (fCursor.y == fScrollBottom)
		_Scroll(fScrollTop, fScrollBottom, 1);
	else
		fCursor.y++;
}


void
TerminalBuffer::_PadLineToCursor()
{
	Line* line = _LineAt(fCursor.y);
	if (line->length < fCursor.x) {
		for (int32 i = line->length; i < fCursor.x; i++) {
			line->cells[i].character = kSpaceChar;
			line->cells[i].attributes = 0;
				// TODO: Other attributes?
		}
	}
}


void
TerminalBuffer::_InsertGap(int32 width)
{
// ASSERT(fCursor.x + width <= fWidth)
	Line* line = _LineAt(fCursor.y);

	int32 toMove = min_c(line->length - fCursor.x, fWidth - fCursor.x - width);
	if (toMove > 0) {
		memmove(line->cells + fCursor.x + width,
			line->cells + fCursor.x, toMove * sizeof(Cell));
	}

	line->length += width;
}


/*!	\a endColumn is not inclusive.
*/
bool
TerminalBuffer::_GetPartialLineString(BString& string, int32 row,
	int32 startColumn, int32 endColumn) const
{
	Line* line = _HistoryLineAt(row);
	if (line == NULL)
		return false;

	if (endColumn > line->length)
		endColumn = line->length;

	for (int32 x = startColumn; x < endColumn; x++) {
		const Cell& cell = line->cells[x];
		string.Append(cell.character.bytes, cell.character.ByteCount());

		if (IS_WIDTH(cell.attributes))
			x++;
	}

	return true;
}


/*!	Decrement \a pos and return the char at that location.
*/
bool
TerminalBuffer::_PreviousChar(TermPos& pos, UTF8Char& c) const
{
	pos.x--;

	Line* line = _HistoryLineAt(pos.y);

	while (true) {
		if (pos.x < 0) {
			pos.y--;
			line = _HistoryLineAt(pos.y);
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
TerminalBuffer::_NextChar(TermPos& pos, UTF8Char& c) const
{
	Line* line = _HistoryLineAt(pos.y);
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
		line = _HistoryLineAt(pos.y);
	}

	return true;
}
