/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TERMINAL_BUFFER_H
#define TERMINAL_BUFFER_H

#include <limits.h>

#include <Locker.h>
#include <Messenger.h>

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


class TerminalBuffer : public BLocker {
public:
								TerminalBuffer();
								~TerminalBuffer();

			status_t			Init(int32 width, int32 height,
									int32 historySize);

			void				SetListener(BMessenger listener);

			int					Encoding() const;
			void				SetEncoding(int encoding);

			int32				Width() const		{ return fWidth; }
			int32				Height() const		{ return fHeight; }
			TermPos				Cursor() const		{ return fCursor; }
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

	// output character
			void				InsertChar(UTF8Char c, uint32 attributes);
			void				Insert(uchar* string, ushort attr);
			void				InsertCR();
			void				InsertLF();
			void				InsertNewLine(int numLines);
			void				SetInsertMode(int flag);
			void				InsertSpace(int num);
	
	// delete character
			void				EraseBelow();
			void				DeleteChar(int num);
			void				DeleteColumns();
			void				DeleteLine(int num);

	// get and set cursor position
// TODO: Inline most of these!
			void				SetCurPos(int x, int y);
			void				SetCurX(int x);
			void				SetCurY(int y);
			int					GetCurX();
			void				SaveCursor();
			void				RestoreCursor();

	// move cursor
			void				MoveCurRight(int num);
			void				MoveCurLeft(int num);
			void				MoveCurUp(int num);
			void				MoveCurDown(int num);

	// scroll region
			void				ScrollRegion(int top, int bot, int dir,
									int num);
			void				SetScrollRegion(int top, int bot);

	// other
			void				SetTitle(const char* title);
			void				NotifyQuit(int32 reason);

private:
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

private:
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
			BMessenger			fListener;
			TerminalBufferDirtyInfo fDirtyInfo;
};

#endif	// TERMINAL_BUFFER_H
