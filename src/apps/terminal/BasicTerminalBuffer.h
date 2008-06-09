/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef BASIC_TERMINAL_BUFFER_H
#define BASIC_TERMINAL_BUFFER_H

#include <limits.h>

#include "TermPos.h"
#include "UTF8Char.h"


class BString;
class TerminalCharClassifier;


struct TerminalBufferDirtyInfo {
	int32	linesScrolled;			// number of lines added to the history
	int32	dirtyTop;				// dirty line range
	int32	dirtyBottom;			//
	bool	messageSent;			// listener has been notified

	bool IsDirtyRegionValid() const
	{
		return dirtyTop <= dirtyBottom;
	}

	void Reset()
	{
		linesScrolled = 0;
		dirtyTop = INT_MAX;
		dirtyBottom = INT_MIN;
		messageSent = false;
	}
};


class BasicTerminalBuffer {
public:
								BasicTerminalBuffer();
	virtual						~BasicTerminalBuffer();

			status_t			Init(int32 width, int32 height,
									int32 historySize);

			int32				Width() const		{ return fWidth; }
			int32				Height() const		{ return fHeight; }
			int32				HistorySize() const	{ return fHistorySize; }

			TerminalBufferDirtyInfo& DirtyInfo()	{ return fDirtyInfo; }

			status_t			ResizeTo(int32 width, int32 height);
			void				Clear();

			bool				IsFullWidthChar(int32 row, int32 column) const;
			int					GetChar(int32 row, int32 column,
									UTF8Char& character,
									uint16& attributes) const;
			int32				GetString(int32 row, int32 firstColumn,
									int32 lastColumn, char* buffer,
									uint16& attributes) const;
			void				GetStringFromRegion(BString& string,
									const TermPos& start,
									const TermPos& end) const;  
			bool				FindWord(const TermPos& pos,
									TerminalCharClassifier* classifier,
									bool findNonWords, TermPos& start,
									TermPos& end) const;
			int32				LineLength(int32 index) const;

			bool				Find(const char* pattern, const TermPos& start,
									bool forward, bool caseSensitive,
									bool matchWord, TermPos& matchStart,
									TermPos& matchEnd) const;

			// insert chars/lines
			void				InsertChar(UTF8Char c, uint32 attributes);
	inline	void				InsertChar(const char* c, int32 length,
									uint32 attributes);
			void				InsertCR();
			void				InsertLF();
			void				SetInsertMode(int flag);
			void				InsertSpace(int32 num);
			void				InsertLines(int32 numLines);
	
			// delete chars/lines
			void				EraseBelow();
			void				DeleteChars(int32 numChars);
			void				DeleteColumns();
			void				DeleteLines(int32 numLines);

			// get and set cursor position
			void				SetCursor(int32 x, int32 y);
	inline	void				SetCursorX(int32 x);
	inline	void				SetCursorY(int32 y);
	inline	TermPos				Cursor() const			{ return fCursor; }
			void				SaveCursor();
			void				RestoreCursor();

			// move cursor
	inline	void				MoveCursorRight(int32 num);
	inline	void				MoveCursorLeft(int32 num);
	inline	void				MoveCursorUp(int32 num);
	inline	void				MoveCursorDown(int32 num);

			// scroll region
	inline	void				ScrollBy(int32 numLines);
			void				SetScrollRegion(int32 top, int32 bot);

protected:
			struct Cell {
				UTF8Char		character;
				uint16			attributes;
			};

			struct Line {
				int16			length;
				bool			softBreak;	// soft line break
				Cell			cells[1];

				inline void Clear()
				{
					length = 0;
					softBreak = false;
				}
			};

	virtual	void				NotifyListener();

	inline	int32				_LineIndex(int32 index) const;
	inline	Line*				_LineAt(int32 index) const;
	inline	Line*				_HistoryLineAt(int32 index) const;

	inline	void				_Invalidate(int32 top, int32 bottom);
	inline	void				_CursorChanged();

	static	Line**				_AllocateLines(int32 width, int32 count);
	static	void				_FreeLines(Line** lines, int32 count);
			void				_ClearLines(int32 first, int32 last);

			void				_Scroll(int32 top, int32 bottom,
									int32 numLines);
			void				_SoftBreakLine();
			void				_PadLineToCursor();
			void				_InsertGap(int32 width);
			bool				_GetPartialLineString(BString& string,
									int32 row, int32 startColumn,
									int32 endColumn) const;
			bool				_PreviousChar(TermPos& pos, UTF8Char& c) const;
			bool				_NextChar(TermPos& pos, UTF8Char& c) const;

protected:
			// screen width/height
			int32				fWidth;
			int32				fHeight;

			// scroll region top/bottom
			int32				fScrollTop;		// first line to scroll
			int32				fScrollBottom;	// last line to scroll (incl.)

			// line buffers for the history (ring buffer)
			Line**				fHistory;
			int32				fHistoryCapacity;
			int32				fScreenOffset;	// index of screen line 0
			int32				fHistorySize;

			// cursor position (origin: (0, 0))
			TermPos				fCursor;
			TermPos				fSavedCursor;

			bool				fOverwriteMode;	// false for insert

			int					fEncoding;

			// listener/dirty region management
			TerminalBufferDirtyInfo fDirtyInfo;
};


void
BasicTerminalBuffer::InsertChar(const char* c, int32 length, uint32 attributes)
{
	return InsertChar(UTF8Char(c, length), attributes);
}


void
BasicTerminalBuffer::SetCursorX(int32 x)
{
	SetCursor(x, fCursor.y);
}


void
BasicTerminalBuffer::SetCursorY(int32 y)
{
	SetCursor(fCursor.x, y);
}


void
BasicTerminalBuffer::MoveCursorRight(int32 num)
{
	SetCursor(fCursor.x + num, fCursor.y);
}


void
BasicTerminalBuffer::MoveCursorLeft(int32 num)
{
	SetCursor(fCursor.x - num, fCursor.y);
}


void
BasicTerminalBuffer::MoveCursorUp(int32 num)
{
	SetCursor(fCursor.x, fCursor.y - num);
}


void
BasicTerminalBuffer::MoveCursorDown(int32 num)
{
	SetCursor(fCursor.x, fCursor.y + num);
}


void
BasicTerminalBuffer::ScrollBy(int32 numLines)
{
	_Scroll(fScrollTop, fScrollBottom, numLines);
}


#endif	// BASIC_TERMINAL_BUFFER_H
