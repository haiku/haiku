/*
 * Copyright 2001-2008, Haiku, Inc.
 * Copyright 2003-2004 Kian Duffy, myob@users.sourceforge.net
 * Parts Copyright 1998-1999 Kazuho Okui and Takashi Murai. 
 * All rights reserved. Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Stefano Ceccherini <stefano.ceccherini@gmail.com>
 *		Kian Duffy, myob@users.sourceforge.net
 *		Y.Hayakawa, hida@sawada.riec.tohoku.ac.jp
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include "TermView.h"

#include <ctype.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>

#include <algorithm>
#include <new>

#include <Alert.h>
#include <Beep.h>
#include <Clipboard.h>
#include <Debug.h>
#include <Dragger.h>
#include <Input.h>
#include <Message.h>
#include <MessageRunner.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <PropertyInfo.h>
#include <Region.h>
#include <Roster.h>
#include <ScrollBar.h>
#include <String.h>
#include <Window.h>

#include "CodeConv.h"
#include "Shell.h"
#include "TermConst.h"
#include "TerminalCharClassifier.h"
#include "VTkeymap.h"


// defined VTKeyTbl.c
extern int function_keycode_table[];
extern char *function_key_char_table[];

const static rgb_color kTermColorTable[8] = {
	{ 40,  40,  40, 0},	// black
	{204,   0,   0, 0},	// red
	{ 78, 154,   6, 0},	// green
	{218, 168,   0, 0},	// yellow
	{ 51, 102, 152, 0},	// blue
	{115,  68, 123, 0},	// magenta
	{  6, 152, 154, 0},	// cyan
	{245, 245, 245, 0},	// white
};

#define ROWS_DEFAULT 25
#define COLUMNS_DEFAULT 80

// selection granularity
enum {
	SELECT_CHARS,
	SELECT_WORDS,
	SELECT_LINES
};


static property_info sPropList[] = { 
	{ "encoding", 
	{B_GET_PROPERTY, 0},
	{B_DIRECT_SPECIFIER, 0},
	"get terminal encoding"}, 
	{ "encoding",
	{B_SET_PROPERTY, 0},
	{B_DIRECT_SPECIFIER, 0},
	"set terminal encoding"}, 
	{ "tty",
	{B_GET_PROPERTY, 0},
	{B_DIRECT_SPECIFIER, 0},
	"get tty name."}, 
	{ 0  }	     
};


static const uint32 kUpdateSigWinch = 'Rwin';
static const uint32 kBlinkCursor = 'BlCr';
static const uint32 kAutoScroll = 'AScr';

static const bigtime_t kSyncUpdateGranularity = 100000;	// 0.1 s

static const int32 kCursorBlinkIntervals = 3;
static const int32 kCursorVisibleIntervals = 2;
static const bigtime_t kCursorBlinkInterval = 500000;

static const rgb_color kBlackColor = { 0, 0, 0, 255 };
static const rgb_color kWhiteColor = { 255, 255, 255, 255 };

static const char* kDefaultSpecialWordChars = ":@-./_~";


template<typename Type>
static inline Type
restrict_value(const Type& value, const Type& min, const Type& max)
{
	return value < min ? min : (value > max ? max : value);
}


class TermView::CharClassifier : public TerminalCharClassifier
{
public:
	CharClassifier(const char* specialWordChars)
		:
		fSpecialWordChars(specialWordChars)
	{
	}

	virtual int Classify(const char* character)
	{
		// TODO: Deal correctly with non-ASCII chars.
		char c = *character;
		if (UTF8Char::ByteCount(c) > 1)
			return CHAR_TYPE_WORD_CHAR;

		if (isspace(c))
			return CHAR_TYPE_SPACE;
		if (isalnum(c) || strchr(fSpecialWordChars, c) != NULL)
			return CHAR_TYPE_WORD_CHAR;

		return CHAR_TYPE_WORD_DELIMITER;
	}

private:
	const char*	fSpecialWordChars;
};


inline int32
TermView::_LineAt(float y)
{
	int32 location = int32(y + fScrollOffset);

	// Make sure negative offsets are rounded towards the lower neighbor, too.
	if (location < 0)
		location -= fFontHeight - 1;

	return location / fFontHeight;
}


inline float
TermView::_LineOffset(int32 index)
{
	return index * fFontHeight - fScrollOffset;
}


// convert view coordinates to terminal text buffer position
inline TermPos
TermView::_ConvertToTerminal(const BPoint &p)
{
	return TermPos(p.x >= 0 ? (int32)p.x / fFontWidth : -1, _LineAt(p.y));
}


// convert terminal text buffer position to view coordinates
inline BPoint
TermView::_ConvertFromTerminal(const TermPos &pos)
{
	return BPoint(fFontWidth * pos.x, _LineOffset(pos.y));
}


inline void
TermView::_InvalidateTextRect(int32 x1, int32 y1, int32 x2, int32 y2)
{
	BRect rect(x1 * fFontWidth, _LineOffset(y1),
	    (x2 + 1) * fFontWidth - 1, _LineOffset(y2 + 1) - 1);
//debug_printf("Invalidate((%f, %f) - (%f, %f))\n", rect.left, rect.top,
//rect.right, rect.bottom);
	Invalidate(rect);
}


TermView::TermView(BRect frame, int32 argc, const char **argv, int32 historySize)
	: BView(frame, "termview", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE | B_PULSE_NEEDED),
	fShell(NULL),
	fWinchRunner(NULL),
	fCursorBlinkRunner(NULL),
	fAutoScrollRunner(NULL),
	fCharClassifier(NULL),
	fFontWidth(0),
	fFontHeight(0),
	fFontAscent(0),
	fFrameResized(false),
	fLastActivityTime(0),
	fCursorState(0),
	fCursorHeight(0),
	fCursor(0, 0),
	fTermRows(ROWS_DEFAULT),
	fTermColumns(COLUMNS_DEFAULT),
	fEncoding(M_UTF8),
	fTextBuffer(NULL),
	fVisibleTextBuffer(NULL),
	fScrollBar(NULL),
	fTextForeColor(kBlackColor),
	fTextBackColor(kWhiteColor),
	fCursorForeColor(kWhiteColor),
	fCursorBackColor(kBlackColor),
	fSelectForeColor(kWhiteColor),
	fSelectBackColor(kBlackColor),
	fScrollOffset(0),
	fScrBufSize(historySize),
	fLastSyncTime(0),
	fScrolledSinceLastSync(0),
	fSyncRunner(NULL),
	fConsiderClockedSync(false),
	fSelStart(-1, -1),
	fSelEnd(-1, -1),
	fMouseTracking(false),
	fIMflag(false)	
{	
	_InitObject(argc, argv);
}


TermView::TermView(int rows, int columns, int32 argc, const char **argv, int32 historySize)
	: BView(BRect(0, 0, 0, 0), "termview", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE | B_PULSE_NEEDED),
	fShell(NULL),
	fWinchRunner(NULL),
	fCursorBlinkRunner(NULL),
	fAutoScrollRunner(NULL),
	fCharClassifier(NULL),
	fFontWidth(0),
	fFontHeight(0),
	fFontAscent(0),
	fFrameResized(false),
	fLastActivityTime(0),
	fCursorState(0),
	fCursorHeight(0),
	fCursor(0, 0),
	fTermRows(rows),
	fTermColumns(columns),
	fEncoding(M_UTF8),
	fTextBuffer(NULL),
	fVisibleTextBuffer(NULL),
	fScrollBar(NULL),
	fTextForeColor(kBlackColor),
	fTextBackColor(kWhiteColor),
	fCursorForeColor(kWhiteColor),
	fCursorBackColor(kBlackColor),
	fSelectForeColor(kWhiteColor),
	fSelectBackColor(kBlackColor),
	fScrollOffset(0),
	fScrBufSize(historySize),
	fLastSyncTime(0),
	fScrolledSinceLastSync(0),
	fSyncRunner(NULL),
	fConsiderClockedSync(false),
	fSelStart(-1, -1),
	fSelEnd(-1, -1),
	fMouseTracking(false),
	fIMflag(false)	
{	
	_InitObject(argc, argv);
	SetTermSize(fTermRows, fTermColumns, true);
	
	// TODO: Don't show the dragger, since replicant capabilities
	// don't work very well ATM.
	/*
	BRect rect(0, 0, 16, 16);
	rect.OffsetTo(Bounds().right - rect.Width(),
				Bounds().bottom - rect.Height());
	
	SetFlags(Flags() | B_DRAW_ON_CHILDREN | B_FOLLOW_ALL);
	AddChild(new BDragger(rect, this,
		B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM, B_WILL_DRAW));*/
}


TermView::TermView(BMessage *archive)
	:
	BView(archive),
	fShell(NULL),
	fWinchRunner(NULL),
	fCursorBlinkRunner(NULL),
	fAutoScrollRunner(NULL),
	fCharClassifier(NULL),
	fFontWidth(0),
	fFontHeight(0),
	fFontAscent(0),
	fFrameResized(false),
	fLastActivityTime(0),
	fCursorState(0),
	fCursorHeight(0),
	fCursor(0, 0),
	fTermRows(ROWS_DEFAULT),
	fTermColumns(COLUMNS_DEFAULT),
	fEncoding(M_UTF8),
	fTextBuffer(NULL),
	fVisibleTextBuffer(NULL),
	fScrollBar(NULL),
	fTextForeColor(kBlackColor),
	fTextBackColor(kWhiteColor),
	fCursorForeColor(kWhiteColor),
	fCursorBackColor(kBlackColor),
	fSelectForeColor(kWhiteColor),
	fSelectBackColor(kBlackColor),
	fScrBufSize(1000),
	fLastSyncTime(0),
	fScrolledSinceLastSync(0),
	fSyncRunner(NULL),
	fConsiderClockedSync(false),
	fSelStart(-1, -1),
	fSelEnd(-1, -1),
	fMouseTracking(false),
	fIMflag(false)	
{
	// We need this
	SetFlags(Flags() | B_WILL_DRAW | B_PULSE_NEEDED);
	
	if (archive->FindInt32("encoding", (int32 *)&fEncoding) < B_OK)
		fEncoding = M_UTF8;
	if (archive->FindInt32("columns", (int32 *)&fTermColumns) < B_OK)
		fTermColumns = COLUMNS_DEFAULT;
	if (archive->FindInt32("rows", (int32 *)&fTermRows) < B_OK)
		fTermRows = ROWS_DEFAULT;
	
	int32 argc = 0;
	if (archive->HasInt32("argc"))
		archive->FindInt32("argc", &argc);
		
	const char **argv = new const char*[argc];	
	for (int32 i = 0; i < argc; i++) {
		archive->FindString("argv", i, (const char **)&argv[i]);
	}
	
	// TODO: Retrieve colors, history size, etc. from archive
	_InitObject(argc, argv);
	
	delete[] argv;
}


status_t
TermView::_InitObject(int32 argc, const char **argv)
{
	fTextBuffer = new(std::nothrow) TerminalBuffer;
	if (fTextBuffer == NULL)
		return B_NO_MEMORY;

	fVisibleTextBuffer = new(std::nothrow) BasicTerminalBuffer;
	if (fVisibleTextBuffer == NULL)
		return B_NO_MEMORY;

	// TODO: Make the special word chars user-settable!
	fCharClassifier = new(std::nothrow) CharClassifier(
		kDefaultSpecialWordChars);
	if (fCharClassifier == NULL)
		return B_NO_MEMORY;

	status_t error = fTextBuffer->Init(fTermColumns, fTermRows, fScrBufSize);
	if (error != B_OK)
		return error;
	fTextBuffer->SetEncoding(fEncoding);

	error = fVisibleTextBuffer->Init(fTermColumns, fTermRows + 2, 0);
	if (error != B_OK)
		return error;

	fShell = new (std::nothrow) Shell();
	if (fShell == NULL)
		return B_NO_MEMORY;
	
	SetTermFont(be_fixed_font);
	SetTermSize(fTermRows, fTermColumns, false);
	//SetIMAware(false);
	
	status_t status = fShell->Open(fTermRows, fTermColumns,
								EncodingAsShortString(fEncoding),
								argc, argv);
	
	if (status < B_OK)
		return status;
		
	status = _AttachShell(fShell);
	if (status < B_OK)
		return status;
	
	
	SetLowColor(fTextBackColor);
	SetViewColor(B_TRANSPARENT_32_BIT);

	return B_OK;
}


TermView::~TermView()
{
	Shell *shell = fShell;
		// _DetachShell sets fShell to NULL
	
	_DetachShell();

	delete fSyncRunner;
	delete fAutoScrollRunner;
	delete fCharClassifier;
	delete fVisibleTextBuffer;
	delete fTextBuffer;
	delete shell;
}


/* static */
BArchivable *
TermView::Instantiate(BMessage* data)
{
	if (validate_instantiation(data, "TermView"))
		return new (std::nothrow) TermView(data);
	
	return NULL;
}


status_t
TermView::Archive(BMessage* data, bool deep) const
{
	status_t status = BView::Archive(data, deep);
	if (status == B_OK)
		status = data->AddString("add_on", TERM_SIGNATURE);	
	if (status == B_OK)
		status = data->AddInt32("encoding", (int32)fEncoding);
	if (status == B_OK)
		status = data->AddInt32("columns", (int32)fTermColumns);
	if (status == B_OK)
		status = data->AddInt32("rows", (int32)fTermRows);
	
	if (data->ReplaceString("class", "TermView") != B_OK)
		data->AddString("class", "TermView");
	
	return status;
}


void
TermView::GetPreferredSize(float *width, float *height)
{
	if (width)
		*width = fTermColumns * fFontWidth - 1;
	if (height)
		*height = fTermRows * fFontHeight - 1;
}


const char *
TermView::TerminalName() const
{
	if (fShell == NULL)
		return NULL;

	return fShell->TTYName();
}


//! Get width and height for terminal font
void
TermView::GetFontSize(int* _width, int* _height)
{
	*_width = fFontWidth;
	*_height = fFontHeight;
}


//! Set number of rows and columns in terminal
BRect
TermView::SetTermSize(int rows, int columns, bool resize)
{
//debug_printf("TermView::SetTermSize(%d, %d)\n", rows, columns);
	if (rows > 0)
		fTermRows = rows;
	if (columns > 0)
		fTermColumns = columns;

	// To keep things simple, get rid of the selection first.
	_Deselect();

	{
		BAutolock _(fTextBuffer);
		if (fTextBuffer->ResizeTo(columns, rows) != B_OK
			|| fVisibleTextBuffer->ResizeTo(columns, rows + 2, 0)
				!= B_OK) {
			return Bounds();
		}
	}

	fTermRows = rows;
	fTermColumns = columns;

//debug_printf("Invalidate()\n");
	Invalidate();

	if (fScrollBar != NULL) {
		_UpdateScrollBarRange();
		fScrollBar->SetSteps(fFontHeight, fFontHeight * fTermRows);
	}

	BRect rect(0, 0, fTermColumns * fFontWidth, fTermRows * fFontHeight);

	if (resize)
		ResizeTo(rect.Width(), rect.Height());


	// synchronize the visible text buffer
	{
		BAutolock _(fTextBuffer);

		_SynchronizeWithTextBuffer(0, -1);
		int32 offset = _LineAt(0);
		fVisibleTextBuffer->SynchronizeWith(fTextBuffer, offset, offset,
			offset + rows + 2);
	}
	
	return rect;
}


void
TermView::SetTextColor(rgb_color fore, rgb_color back)
{
	fTextForeColor = fore;
	fTextBackColor = back;

	SetLowColor(fTextBackColor);
}


void
TermView::SetSelectColor(rgb_color fore, rgb_color back)
{
	fSelectForeColor = fore;
	fSelectBackColor = back;
}


void
TermView::SetCursorColor(rgb_color fore, rgb_color back)
{
	fCursorForeColor = fore;
	fCursorBackColor = back;
}


int
TermView::Encoding() const
{
	return fEncoding;
}


void
TermView::SetEncoding(int encoding)
{
	// TODO: Shell::_Spawn() sets the "TTYPE" environment variable using
	// the string value of encoding. But when this function is called and 
	// the encoding changes, the new value is never passed to Shell.
	fEncoding = encoding;

	BAutolock _(fTextBuffer);
	fTextBuffer->SetEncoding(fEncoding);
}


void
TermView::GetTermFont(BFont *font) const
{
	if (font != NULL)
		*font = fHalfFont;
}


//! Sets font for terminal
void
TermView::SetTermFont(const BFont *font)
{
	int halfWidth = 0;

	fHalfFont = font;
	
	fHalfFont.SetSpacing(B_FIXED_SPACING);
	
	// calculate half font's max width
	// Not Bounding, check only A-Z(For case of fHalfFont is KanjiFont. )
	for (int c = 0x20 ; c <= 0x7e; c++){
		char buf[4];
		sprintf(buf, "%c", c);
		int tmpWidth = (int)fHalfFont.StringWidth(buf);
		if (tmpWidth > halfWidth)
			halfWidth = tmpWidth;
	}

	fFontWidth = halfWidth;

	font_height hh;
	fHalfFont.GetHeight(&hh);
	
	int font_ascent = (int)hh.ascent;
	int font_descent =(int)hh.descent;
	int font_leading =(int)hh.leading;
	
	if (font_leading == 0)
		font_leading = 1;

	fFontAscent = font_ascent;
	fFontHeight = font_ascent + font_descent + font_leading + 1;

	fCursorHeight = fFontHeight;
	
	_ScrollTo(0, false);
	if (fScrollBar != NULL)
		fScrollBar->SetSteps(fFontHeight, fFontHeight * fTermRows);
}


void
TermView::SetScrollBar(BScrollBar *scrollBar)
{
	fScrollBar = scrollBar;
	if (fScrollBar != NULL) {
		fScrollBar->SetSteps(fFontHeight, fFontHeight * fTermRows);
	}
}


void
TermView::SetTitle(const char *title)
{
	// TODO: Do something different in case we're a replicant,
	// or in case we are inside a BTabView ?
	if (Window())
		Window()->SetTitle(title);
}


void
TermView::Copy(BClipboard *clipboard)
{
	BAutolock _(fTextBuffer);

	if (!_HasSelection())
		return;

	BString copyStr;
	fTextBuffer->GetStringFromRegion(copyStr, fSelStart, fSelEnd);

	if (clipboard->Lock()) {
		BMessage *clipMsg = NULL;
		clipboard->Clear();

		if ((clipMsg = clipboard->Data()) != NULL) {
			clipMsg->AddData("text/plain", B_MIME_TYPE, copyStr.String(),
				copyStr.Length());
			clipboard->Commit();
		}
		clipboard->Unlock();
	}
}


void
TermView::Paste(BClipboard *clipboard)
{
	if (clipboard->Lock()) {
		BMessage *clipMsg = clipboard->Data();
		const char* text;
		ssize_t numBytes;
		if (clipMsg->FindData("text/plain", B_MIME_TYPE,
				(const void**)&text, &numBytes) == B_OK ) {
			_WritePTY(text, numBytes);
		}

		clipboard->Unlock();
	}
}


void
TermView::SelectAll()
{
	BAutolock _(fTextBuffer);

	_Select(TermPos(0, -fTextBuffer->HistorySize()),
		TermPos(0, fTextBuffer->Height()), false, true);
}


void
TermView::Clear()
{
	_Deselect();

	{
		BAutolock _(fTextBuffer);
		fTextBuffer->Clear(true);
	}
	fVisibleTextBuffer->Clear(true);

//debug_printf("Invalidate()\n");
	Invalidate();

	_ScrollTo(0, false);
	if (fScrollBar) {
		fScrollBar->SetRange(0, 0);
		fScrollBar->SetProportion(1);
	}
}


//! Draw region
void
TermView::_InvalidateTextRange(TermPos start, TermPos end)
{
	if (end < start)
		std::swap(start, end);

	if (start.y == end.y) {
		_InvalidateTextRect(start.x, start.y, end.x, end.y);
	} else {
		_InvalidateTextRect(start.x, start.y, fTermColumns, start.y);

		if (end.y - start.y > 0)
			_InvalidateTextRect(0, start.y + 1, fTermColumns, end.y - 1);

		_InvalidateTextRect(0, end.y, end.x, end.y);
	}
}


status_t
TermView::_AttachShell(Shell *shell)
{
	if (shell == NULL)
		return B_BAD_VALUE;
	
	fShell = shell;
	fShell->ViewAttached(this);	

	return B_OK;
}


void
TermView::_DetachShell()
{
	fShell->ViewDetached();
	fShell = NULL;
}


//! Draw part of a line in the given view.
void
TermView::_DrawLinePart(int32 x1, int32 y1, uint16 attr, char *buf,
	int32 width, bool mouse, bool cursor, BView *inView)
{
	rgb_color rgb_fore = fTextForeColor, rgb_back = fTextBackColor;

	inView->SetFont(&fHalfFont);

	// Set pen point
	int x2 = x1 + fFontWidth * width;
	int y2 = y1 + fFontHeight;

	// color attribute
	int forecolor = IS_FORECOLOR(attr);
	int backcolor = IS_BACKCOLOR(attr);

	if (IS_FORESET(attr))
		rgb_fore = kTermColorTable[forecolor];

	if (IS_BACKSET(attr))
		rgb_back = kTermColorTable[backcolor];

	// Selection check.
	if (cursor) {
		rgb_fore = fCursorForeColor;
		rgb_back = fCursorBackColor;
	} else if (mouse) {
		rgb_fore = fSelectForeColor;
		rgb_back = fSelectBackColor;
	} else {
		// Reverse attribute(If selected area, don't reverse color).
		if (IS_INVERSE(attr)) {
			rgb_color rgb_tmp = rgb_fore;
			rgb_fore = rgb_back;
			rgb_back = rgb_tmp;
		}
	}

	// Fill color at Background color and set low color.
	inView->SetHighColor(rgb_back);
	inView->FillRect(BRect(x1, y1, x2 - 1, y2 - 1));
	inView->SetLowColor(rgb_back);

	inView->SetHighColor(rgb_fore);

	// Draw character.
	inView->MovePenTo(x1, y1 + fFontAscent);
	inView->DrawString((char *) buf);

	// bold attribute.
	if (IS_BOLD(attr)) {
		inView->MovePenTo(x1 + 1, y1 + fFontAscent);
		
		inView->SetDrawingMode(B_OP_OVER);
		inView->DrawString((char *)buf);
		inView->SetDrawingMode(B_OP_COPY);
	}

	// underline attribute
	if (IS_UNDER(attr)) {
		inView->MovePenTo(x1, y1 + fFontAscent);
		inView->StrokeLine(BPoint(x1 , y1 + fFontAscent),
			BPoint(x2 , y1 + fFontAscent));
	}
}


/*!	Caller must have locked fTextBuffer.
*/
void
TermView::_DrawCursor()
{
	BRect rect(fFontWidth * fCursor.x, _LineOffset(fCursor.y), 0, 0);
	rect.right = rect.left + fFontWidth - 1;
	rect.bottom = rect.top + fCursorHeight - 1;
	int32 firstVisible = _LineAt(0);

	UTF8Char character;
	uint16 attr;

	bool cursorVisible = _IsCursorVisible();

	bool selected = _CheckSelectedRegion(TermPos(fCursor.x, fCursor.y));
	if (fVisibleTextBuffer->GetChar(fCursor.y - firstVisible, fCursor.x,
			character, attr) == A_CHAR) {
		int32 width;
		if (IS_WIDTH(attr))
			width = 2;
		else
			width = 1;

		char buffer[5];
		int32 bytes = UTF8Char::ByteCount(character.bytes[0]);
		memcpy(buffer, character.bytes, bytes);
		buffer[bytes] = '\0';

		_DrawLinePart(fCursor.x * fFontWidth, (int32)rect.top, attr, buffer,
			width, selected, cursorVisible, this);
	} else {
		if (selected)
			SetHighColor(fSelectBackColor);
		else
			SetHighColor(cursorVisible ? fCursorBackColor : fTextBackColor);

		FillRect(rect);
	}
}


bool
TermView::_IsCursorVisible() const
{
	return fCursorState < kCursorVisibleIntervals;
}


void
TermView::_BlinkCursor()
{
	bool wasVisible = _IsCursorVisible();

	bigtime_t now = system_time();
	if (Window()->IsActive() && now - fLastActivityTime >= kCursorBlinkInterval)
		fCursorState = (fCursorState + 1) % kCursorBlinkIntervals;
	else
		fCursorState = 0;

	if (wasVisible != _IsCursorVisible())
		_InvalidateTextRect(fCursor.x, fCursor.y, fCursor.x, fCursor.y);
}


void
TermView::_ActivateCursor(bool invalidate)
{
	fLastActivityTime = system_time();
	if (invalidate && fCursorState != 0)
		_BlinkCursor();
	else
		fCursorState = 0;
}


//! Update scroll bar range and knob size.
void
TermView::_UpdateScrollBarRange()
{
	if (fScrollBar == NULL)
		return;

	int32 historySize;
	{
		BAutolock _(fTextBuffer);
		historySize = fTextBuffer->HistorySize();
	}

	float viewHeight = fTermRows * fFontHeight;
	float historyHeight = (float)historySize * fFontHeight;

//debug_printf("TermView::_UpdateScrollBarRange(): history: %ld, range: %f - 0\n",
//historySize, -historyHeight);

	fScrollBar->SetRange(-historyHeight, 0);
	if (historySize > 0)
		fScrollBar->SetProportion(viewHeight / (viewHeight + historyHeight));
}


//!	Handler for SIGWINCH
void
TermView::_UpdateSIGWINCH()
{
	if (fFrameResized) {
		fShell->UpdateWindowSize(fTermRows, fTermColumns);
		fFrameResized = false;
	}
}


void
TermView::AttachedToWindow()
{
	MakeFocus(true);
	if (fScrollBar) {
		fScrollBar->SetSteps(fFontHeight, fFontHeight * fTermRows);
		_UpdateScrollBarRange();
	}

	BMessenger thisMessenger(this);

	BMessage message(kUpdateSigWinch);
	fWinchRunner = new (std::nothrow) BMessageRunner(thisMessenger,
		&message, 500000);

	{
		BAutolock _(fTextBuffer);
		fTextBuffer->SetListener(thisMessenger);
		_SynchronizeWithTextBuffer(0, -1);
	}
}


void
TermView::DetachedFromWindow()
{
	delete fWinchRunner;
	fWinchRunner = NULL;

	delete fCursorBlinkRunner;
	fCursorBlinkRunner = NULL;

	{
		BAutolock _(fTextBuffer);
		fTextBuffer->UnsetListener();
	}
}


void
TermView::Draw(BRect updateRect)
{
//	if (IsPrinting()) {
//		_DoPrint(updateRect);
//		return;
//	}

// debug_printf("TermView::Draw((%f, %f) - (%f, %f))\n", updateRect.left,
// updateRect.top, updateRect.right, updateRect.bottom);
// {
// BRect bounds(Bounds());
// debug_printf("Bounds(): (%f, %f) - (%f - %f)\n", bounds.left, bounds.top,
// 	bounds.right, bounds.bottom);
// debug_printf("clipping region:\n");
// BRegion region;
// GetClippingRegion(&region);
// for (int32 i = 0; i < region.CountRects(); i++) {
// 	BRect rect(region.RectAt(i));
// 	debug_printf("  (%f, %f) - (%f, %f)\n", rect.left, rect.top, rect.right,
// 		rect.bottom);
// }
// }

	int32 x1 = (int32)(updateRect.left) / fFontWidth;
	int32 x2 = (int32)(updateRect.right) / fFontWidth;

	int32 firstVisible = _LineAt(0);
	int32 y1 = _LineAt(updateRect.top);
	int32 y2 = _LineAt(updateRect.bottom);

//debug_printf("TermView::Draw(): (%ld, %ld) - (%ld, %ld), top: %f, fontHeight: %d, scrollOffset: %f\n",
//x1, y1, x2, y2, updateRect.top, fFontHeight, fScrollOffset);

	for (int32 j = y1; j <= y2; j++) {
		int32 k = x1;
		char buf[fTermColumns * 4 + 1];

		if (fVisibleTextBuffer->IsFullWidthChar(j - firstVisible, k))
			k--;

		if (k < 0)
			k = 0;

		for (int32 i = k; i <= x2;) {
			int32 lastColumn = x2;
			bool insideSelection = _CheckSelectedRegion(j, i, lastColumn);
			uint16 attr;
			int32 count = fVisibleTextBuffer->GetString(j - firstVisible, i,
				lastColumn, buf, attr);

//debug_printf("  fVisibleTextBuffer->GetString(%ld, %ld, %ld) -> (%ld, \"%.*s\"), selected: %d\n",
//j - firstVisible, i, lastColumn, count, (int)count, buf, insideSelection);

			if (count == 0) {
				BRect rect(fFontWidth * i, _LineOffset(j),
					fFontWidth * (lastColumn + 1) - 1, 0);
				rect.bottom = rect.top + fFontHeight - 1;

				SetHighColor(insideSelection ? fSelectBackColor
					: fTextBackColor);
				FillRect(rect);

				i = lastColumn + 1;
				continue;
			}

			_DrawLinePart(fFontWidth * i, (int32)_LineOffset(j),
				attr, buf, count, insideSelection, false, this);
			i += count;
		}
	}

	if (fCursor >= TermPos(x1, y1) && fCursor <= TermPos(x2, y2))
		_DrawCursor();
}


void
TermView::_DoPrint(BRect updateRect)
{
#if 0
	ushort attr;
	uchar buf[1024];

	const int numLines = (int)((updateRect.Height()) / fFontHeight);

	int y1 = (int)(updateRect.top) / fFontHeight;
	y1 = y1 -(fScrBufSize - numLines * 2);
	if (y1 < 0)
		y1 = 0;

	const int y2 = y1 + numLines -1;

	const int x1 = (int)(updateRect.left) / fFontWidth;
	const int x2 = (int)(updateRect.right) / fFontWidth;

	for (int j = y1; j <= y2; j++) {
		// If(x1, y1) Buffer is in string full width character,
		// alignment start position.

		int k = x1;
		if (fTextBuffer->IsFullWidthChar(j, k))
			k--;

		if (k < 0)
			k = 0;

		for (int i = k; i <= x2;) {
			int count = fTextBuffer->GetString(j, i, x2, buf, &attr);
			if (count < 0) {
				i += abs(count);
				continue;
			}

			_DrawLinePart(fFontWidth * i, fFontHeight * j,
				attr, buf, count, false, false, this);
			i += count;
		}
	}
#endif	// 0
} 


void
TermView::WindowActivated(bool active)
{
	BView::WindowActivated(active);
	if (active) {
		// start cursor blinking
		if (fCursorBlinkRunner == NULL) {
			BMessage blinkMessage(kBlinkCursor);
			fCursorBlinkRunner = new (std::nothrow) BMessageRunner(
				BMessenger(this), &blinkMessage, kCursorBlinkInterval);
		}
	} else {
		// DoIMConfirm();

		// make sure the cursor becomes visible
		fCursorState = 0;
		_InvalidateTextRect(fCursor.x, fCursor.y, fCursor.x, fCursor.y);
		delete fCursorBlinkRunner;
		fCursorBlinkRunner = NULL;
	}
}


void
TermView::KeyDown(const char *bytes, int32 numBytes)
{
	if (fIMflag)
		return;

	int32 key, mod, rawChar;
	BMessage *currentMessage = Looper()->CurrentMessage();
	if (currentMessage == NULL)
		return;
	
	currentMessage->FindInt32("modifiers", &mod);
	currentMessage->FindInt32("key", &key);
	currentMessage->FindInt32("raw_char", &rawChar);

	_ActivateCursor(true);

	// Terminal filters RET, ENTER, F1...F12, and ARROW key code.
	// TODO: Cleanup
	if (numBytes == 1) {
		const char *toWrite = NULL;
		switch (*bytes) {
			case B_RETURN:
				if (rawChar == B_RETURN)
					toWrite = "\r";
				break;

			case B_DELETE:
				toWrite = DELETE_KEY_CODE;
				break;

			case B_BACKSPACE:
				toWrite = BACKSPACE_KEY_CODE;
				break;

			case B_LEFT_ARROW:
				if (rawChar == B_LEFT_ARROW) {
					if (mod & B_SHIFT_KEY) {
						BMessage message(MSG_PREVIOUS_TAB);
						message.AddPointer("termView", this);
						Window()->PostMessage(&message);
					} else if (mod & B_CONTROL_KEY) {
						toWrite = CTRL_LEFT_ARROW_KEY_CODE;
					} else
						toWrite = LEFT_ARROW_KEY_CODE;
				}
				break;

			case B_RIGHT_ARROW:
				if (rawChar == B_RIGHT_ARROW) {
					if (mod & B_SHIFT_KEY) {
						BMessage message(MSG_NEXT_TAB);
						message.AddPointer("termView", this);
						Window()->PostMessage(&message);
					} else if (mod & B_CONTROL_KEY) {
						toWrite = CTRL_RIGHT_ARROW_KEY_CODE;
					} else
						toWrite = RIGHT_ARROW_KEY_CODE;
				}
				break;

			case B_UP_ARROW:
				if (mod & B_SHIFT_KEY) {
					_ScrollTo(fScrollOffset - fFontHeight, true);
					return;
				}
				if (rawChar == B_UP_ARROW) {
					if (mod & B_CONTROL_KEY)
						toWrite = CTRL_UP_ARROW_KEY_CODE;
					else
						toWrite = UP_ARROW_KEY_CODE;
				}
				break;
	 
			case B_DOWN_ARROW:
				if (mod & B_SHIFT_KEY) {
					_ScrollTo(fScrollOffset + fFontHeight, true);
					return;
				}

				if (rawChar == B_DOWN_ARROW) {
					if (mod & B_CONTROL_KEY)
						toWrite = CTRL_DOWN_ARROW_KEY_CODE;
					else
						toWrite = DOWN_ARROW_KEY_CODE;
				}
				break;

			case B_INSERT:
				if (rawChar == B_INSERT)
					toWrite = INSERT_KEY_CODE;
				break;

			case B_HOME:
				if (rawChar == B_HOME)
					toWrite = HOME_KEY_CODE;
				break;

			case B_END:
				if (rawChar == B_END)
					toWrite = END_KEY_CODE;
				break;

			case B_PAGE_UP:
				if (mod & B_SHIFT_KEY) {
					_ScrollTo(fScrollOffset - fFontHeight  * fTermRows, true);
					return;
				}
				if (rawChar == B_PAGE_UP)
					toWrite = PAGE_UP_KEY_CODE;
				break;

			case B_PAGE_DOWN:
				if (mod & B_SHIFT_KEY) {
					_ScrollTo(fScrollOffset + fFontHeight * fTermRows, true);
					return;
				}
	 
				if (rawChar == B_PAGE_DOWN)
					toWrite = PAGE_DOWN_KEY_CODE;
				break;

			case B_FUNCTION_KEY:
				for (int32 i = 0; i < 12; i++) {
					if (key == function_keycode_table[i]) {
						fShell->Write(function_key_char_table[i], 5);
						return;
					}
				}
				break;
			default:
				break;
		}
		if (toWrite) {
			_ScrollTo(0, true);
			fShell->Write(toWrite, strlen(toWrite));
			return;
		}
	} else {
		// input multibyte character
		if (fEncoding != M_UTF8) {
			char destBuffer[16];
			int cnum = CodeConv::ConvertFromInternal(bytes, numBytes,
				(char *)destBuffer, fEncoding);
			_ScrollTo(0, true);
			fShell->Write(destBuffer, cnum);
			return;
		}
	}

	_ScrollTo(0, true);
	fShell->Write(bytes, numBytes);
}


void
TermView::FrameResized(float width, float height)
{
//debug_printf("TermView::FrameResized(%f, %f)\n", width, height);
	int32 columns = ((int32)width + 1) / fFontWidth;
	int32 rows = ((int32)height + 1) / fFontHeight;

	if (columns == fTermColumns && rows == fTermRows)
		return;

	SetTermSize(rows, columns, false);

	fFrameResized = true;
}


void 
TermView::MessageReceived(BMessage *msg)
{
	entry_ref ref;
	char *ctrl_l = "\x0c";

	// first check for any dropped message
	if (msg->WasDropped()) {
		char *text;
		int32 numBytes;
		//rgb_color *color;
	
		int32 i = 0;

		if (msg->FindRef("refs", i++, &ref) == B_OK) {
			_DoFileDrop(ref);

			while (msg->FindRef("refs", i++, &ref) == B_OK) {
				_WritePTY(" ", 1);
				_DoFileDrop(ref);
			}
			return;
#if 0
		} else if (msg->FindData("RGBColor", B_RGB_COLOR_TYPE, 
				(const void **)&color, &numBytes) == B_OK
				 && numBytes == sizeof(color)) {
			// TODO: handle color drop
			// maybe only on replicants ?
			return;
#endif
		} else if (msg->FindData("text/plain", B_MIME_TYPE, 
			 	(const void **)&text, &numBytes) == B_OK) {
			_WritePTY(text, numBytes);
			return;
		}
	}

	switch (msg->what){
		case B_ABOUT_REQUESTED:
			// (replicant) about box requested 
			_AboutRequested();
			break;

		case B_SIMPLE_DATA:
		case B_REFS_RECEIVED:
		{
			// handle refs if they weren't dropped
			int32 i = 0;
			if (msg->FindRef("refs", i++, &ref) == B_OK) {
				_DoFileDrop(ref);

				while (msg->FindRef("refs", i++, &ref) == B_OK) {
					_WritePTY(" ", 1);
					_DoFileDrop(ref);
				}
			} else
				BView::MessageReceived(msg);
			break;
		}

		case B_COPY:
			Copy(be_clipboard);
			break;

		case B_PASTE:
		{
			int32 code;
			if (msg->FindInt32("index", &code) == B_OK)
				Paste(be_clipboard);
			break;
		}

		case B_SELECT_ALL:
			SelectAll();
			break;
		
		case B_SET_PROPERTY:
		{
			int32 i;
			int32 encodingID;
			BMessage specifier;
			msg->GetCurrentSpecifier(&i, &specifier);
			if (!strcmp("encoding", specifier.FindString("property", i))){
				msg->FindInt32 ("data", &encodingID);
				SetEncoding(encodingID);
				msg->SendReply(B_REPLY);
			} else {
				BView::MessageReceived(msg);
			}
			break;
		}

		case B_GET_PROPERTY: 
		{
			int32 i;
			BMessage specifier;
			msg->GetCurrentSpecifier(&i, &specifier);
			if (!strcmp("encoding", specifier.FindString("property", i))){
				BMessage reply(B_REPLY);
				reply.AddInt32("result", Encoding());
				msg->SendReply(&reply);
			} else if (!strcmp("tty", specifier.FindString("property", i))) {
				BMessage reply(B_REPLY);
				reply.AddString("result", TerminalName());
				msg->SendReply(&reply);
			} else {
				BView::MessageReceived(msg);
			}
			break;
		}
	
		case MENU_CLEAR_ALL:
			Clear();
			fShell->Write(ctrl_l, 1);
			break;

		
//  case B_INPUT_METHOD_EVENT:
//    {
   //   int32 op;
  //    msg->FindInt32("be:opcode", &op);
   //   switch (op){ 
   //   case B_INPUT_METHOD_STARTED:
	//DoIMStart(msg);
//	break;

//      case B_INPUT_METHOD_STOPPED:
//	DoIMStop(msg);
//	break;

//      case B_INPUT_METHOD_CHANGED:
//	DoIMChange(msg);
//	break;

//      case B_INPUT_METHOD_LOCATION_REQUEST:
//	DoIMLocation(msg);
//	break;
    //  }
   // }
		case kBlinkCursor:
			_BlinkCursor();
			break;
		case kUpdateSigWinch:
			_UpdateSIGWINCH();
			break;
		case kAutoScroll:
			_AutoScrollUpdate();
			break;
		case MSG_TERMINAL_BUFFER_CHANGED:
		{
			BAutolock _(fTextBuffer);
			_SynchronizeWithTextBuffer(0, -1);
			break;
		}
		case MSG_SET_TERMNAL_TITLE:
		{
			const char* title;
			if (msg->FindString("title", &title) == B_OK)
				SetTitle(title);
			break;
		}
		case MSG_QUIT_TERMNAL:
		{
			int32 reason;
			if (msg->FindInt32("reason", &reason) != B_OK)
				reason = 0;
			NotifyQuit(reason);
			break;
		}
		default:
			BView::MessageReceived(msg);
			break;
	}
}  


status_t
TermView::GetSupportedSuites(BMessage *message) 
{ 
	BPropertyInfo propInfo(sPropList); 
	message->AddString("suites", "suite/vnd.naan-termview"); 	
	message->AddFlat("messages", &propInfo); 
	return BView::GetSupportedSuites(message); 
}


void
TermView::ScrollTo(BPoint where)
{
//debug_printf("TermView::ScrollTo(): %f -> %f\n", fScrollOffset, where.y);
	float diff = where.y - fScrollOffset;
	if (diff == 0)
		return;

	float bottom = Bounds().bottom;
	int32 oldFirstLine = _LineAt(0);
	int32 oldLastLine = _LineAt(bottom);
	int32 newFirstLine = _LineAt(diff);
	int32 newLastLine = _LineAt(bottom + diff);

	fScrollOffset = where.y;

	// invalidate the current cursor position before scrolling
	_InvalidateTextRect(fCursor.x, fCursor.y, fCursor.x, fCursor.y);

	// scroll contents
	BRect destRect(Frame().OffsetToCopy(Bounds().LeftTop()));
	BRect sourceRect(destRect.OffsetByCopy(0, diff));
//debug_printf("CopyBits(((%f, %f) - (%f, %f)) -> (%f, %f) - (%f, %f))\n",
//sourceRect.left, sourceRect.top, sourceRect.right, sourceRect.bottom,
//destRect.left, destRect.top, destRect.right, destRect.bottom);
	CopyBits(sourceRect, destRect);

	// sync visible text buffer with text buffer
	if (newFirstLine != oldFirstLine || newLastLine != oldLastLine) {
		if (newFirstLine != oldFirstLine)
{
//debug_printf("fVisibleTextBuffer->ScrollBy(%ld)\n", newFirstLine - oldFirstLine);
			fVisibleTextBuffer->ScrollBy(newFirstLine - oldFirstLine);
}
		BAutolock _(fTextBuffer);
		if (diff < 0)
			_SynchronizeWithTextBuffer(newFirstLine, oldFirstLine - 1);
		else
			_SynchronizeWithTextBuffer(oldLastLine + 1, newLastLine);
	}
}


BHandler*
TermView::ResolveSpecifier(BMessage *message, int32 index, BMessage *specifier,
				int32 what, const char *property)
{
	BHandler *target = this;
	BPropertyInfo propInfo(sPropList);
	if (propInfo.FindMatch(message, index, specifier, what, property) < B_OK)			
		target = BView::ResolveSpecifier(message, index, specifier, what, property);
	
	return target;
}


//! Gets dropped file full path and display it at cursor position.
void 
TermView::_DoFileDrop(entry_ref &ref)
{
	BEntry ent(&ref); 
	BPath path(&ent);
	BString string(path.Path());

	string.CharacterEscape(" ~`#$&*()\\|[]{};'\"<>?!",'\\');
	_WritePTY(string.String(), string.Length());
}


/*!	Text buffer must already be locked.
*/
void
TermView::_SynchronizeWithTextBuffer(int32 visibleDirtyTop,
	int32 visibleDirtyBottom)
{
	TerminalBufferDirtyInfo& info = fTextBuffer->DirtyInfo();
	int32 linesScrolled = info.linesScrolled;

//debug_printf("TermView::_SynchronizeWithTextBuffer(): dirty: %ld - %ld, "
//"scrolled: %ld, visible dirty: %ld - %ld\n", info.dirtyTop, info.dirtyBottom,
//info.linesScrolled, visibleDirtyTop, visibleDirtyBottom);

	bigtime_t now = system_time();
	bigtime_t timeElapsed = now - fLastSyncTime;
	if (timeElapsed > 2 * kSyncUpdateGranularity) {
		// last sync was ages ago
		fLastSyncTime = now;
		fScrolledSinceLastSync = linesScrolled;
	}

	if (fSyncRunner == NULL) {
		// We consider clocked syncing when more than a full screen height has
		// been scrolled in less than a sync update period. Once we're
		// actively considering it, the same condition will convince us to
		// actually do it.
		if (fScrolledSinceLastSync + linesScrolled <= fTermRows) {
			// Condition doesn't hold yet. Reset if time is up, or otherwise
			// keep counting.
			if (timeElapsed > kSyncUpdateGranularity) {
				fConsiderClockedSync = false;
				fLastSyncTime = now;
				fScrolledSinceLastSync = linesScrolled;
			} else
				fScrolledSinceLastSync += linesScrolled;
		} else if (fConsiderClockedSync) {
			// We are convinced -- create the sync runner.
			fLastSyncTime = now;
			fScrolledSinceLastSync = 0;

			BMessage message(MSG_TERMINAL_BUFFER_CHANGED);
			fSyncRunner = new(std::nothrow) BMessageRunner(BMessenger(this),
				&message, kSyncUpdateGranularity);
			if (fSyncRunner != NULL && fSyncRunner->InitCheck() == B_OK)
				return;

			delete fSyncRunner;
			fSyncRunner = NULL;
		} else {
			// Looks interesting so far. Reset the counts and consider clocked
			// syncing.
			fConsiderClockedSync = true;
			fLastSyncTime = now;
			fScrolledSinceLastSync = 0;
		}
	} else if (timeElapsed < kSyncUpdateGranularity) {
		// sync time not passed yet -- keep counting
		fScrolledSinceLastSync += linesScrolled;
		return;
	} else if (fScrolledSinceLastSync + linesScrolled <= fTermRows) {
		// time's up, but not enough happened
		delete fSyncRunner;
		fSyncRunner = NULL;
		fLastSyncTime = now;
		fScrolledSinceLastSync = linesScrolled;
	} else {
		// Things are still rolling, but the sync time's up.
		fLastSyncTime = now;
		fScrolledSinceLastSync = 0;
	}

	// Simple case first -- complete invalidation.
	if (info.invalidateAll) {
		Invalidate();
		_UpdateScrollBarRange();
		int32 offset = _LineAt(0);
		fVisibleTextBuffer->SynchronizeWith(fTextBuffer, offset, offset,
			offset + fTextBuffer->Height() + 2);
		info.Reset();
		return;
	}

	BRect bounds = Bounds();
	int32 firstVisible = _LineAt(0);
	int32 lastVisible = _LineAt(bounds.bottom);
	int32 historySize = fTextBuffer->HistorySize();

	bool doScroll = false;
	if (linesScrolled > 0) {
		_UpdateScrollBarRange();

		visibleDirtyTop -= linesScrolled;
		visibleDirtyBottom -= linesScrolled;

		if (firstVisible < 0) {
			firstVisible -= linesScrolled;
			lastVisible -= linesScrolled;

			float scrollOffset;
			if (firstVisible < -historySize) {
				firstVisible = -historySize;
				doScroll = true;
				scrollOffset = -historySize * fFontHeight;
				// We need to invalidate the lower linesScrolled lines of the
				// visible text buffer, since those will be scrolled up and
				// need to be replaced. We just use visibleDirty{Top,Bottom}
				// for that purpose. Unless invoked from ScrollTo() (i.e.
				// user-initiated scrolling) those are unused. In the unlikely
				// case that the user is scrolling at the same time we may
				// invalidate too many lines, since we have to extend the given
				// region.
				// Note that in the firstVisible == 0 case the new lines are
				// already in the dirty region, so they will be updated anyway.
				if (visibleDirtyTop <= visibleDirtyBottom) {
					if (lastVisible < visibleDirtyTop)
						visibleDirtyTop = lastVisible;
					if (visibleDirtyBottom < lastVisible + linesScrolled)
						visibleDirtyBottom = lastVisible + linesScrolled;
				} else {
					visibleDirtyTop = lastVisible + 1;
					visibleDirtyBottom = lastVisible + linesScrolled;
				}
			} else
				scrollOffset = fScrollOffset - linesScrolled * fFontHeight;

			_ScrollTo(scrollOffset, false);
		} else
			doScroll = true;

		if (doScroll && lastVisible >= firstVisible
			&& !(info.IsDirtyRegionValid() && firstVisible >= info.dirtyTop
				&& lastVisible <= info.dirtyBottom)) {
			// scroll manually
			float scrollBy = linesScrolled * fFontHeight;
			BRect destRect(Frame().OffsetToCopy(B_ORIGIN));
			BRect sourceRect(destRect.OffsetByCopy(0, scrollBy));

			// invalidate the current cursor position before scrolling
			_InvalidateTextRect(fCursor.x, fCursor.y, fCursor.x, fCursor.y);

//debug_printf("CopyBits(((%f, %f) - (%f, %f)) -> (%f, %f) - (%f, %f))\n",
//sourceRect.left, sourceRect.top, sourceRect.right, sourceRect.bottom,
//destRect.left, destRect.top, destRect.right, destRect.bottom);
			CopyBits(sourceRect, destRect);

			fVisibleTextBuffer->ScrollBy(linesScrolled);
		}

		// move selection
		if (fSelStart != fSelEnd) {
			fSelStart.y -= linesScrolled;
			fSelEnd.y -= linesScrolled;
			fInitialSelectionStart.y -= linesScrolled;
			fInitialSelectionEnd.y -= linesScrolled;

			if (fSelStart.y < -historySize)
				_Deselect();
		}
	}

	// invalidate dirty region
	if (info.IsDirtyRegionValid()) {
		_InvalidateTextRect(0, info.dirtyTop, fTextBuffer->Width() - 1,
			info.dirtyBottom);

		// clear the selection, if affected
		if (fSelStart != fSelEnd) {
			// TODO: We're clearing the selection more often than necessary --
			// to avoid that, we'd also need to track the x coordinates of the
			// dirty range.
			int32 selectionBottom = fSelEnd.x > 0 ? fSelEnd.y : fSelEnd.y - 1;
			if (fSelStart.y <= info.dirtyBottom
				&& info.dirtyTop <= selectionBottom) {
				_Deselect();
			}
		}
	}

	if (visibleDirtyTop <= visibleDirtyBottom)
		info.ExtendDirtyRegion(visibleDirtyTop, visibleDirtyBottom);

	if (linesScrolled != 0 || info.IsDirtyRegionValid()) {
		fVisibleTextBuffer->SynchronizeWith(fTextBuffer, firstVisible,
			info.dirtyTop, info.dirtyBottom);
	}

	// invalidate cursor, if it changed
	TermPos cursor = fTextBuffer->Cursor();
	if (fCursor != cursor || linesScrolled != 0) {
		// Before we scrolled we did already invalidate the old cursor.
		if (!doScroll)
			_InvalidateTextRect(fCursor.x, fCursor.y, fCursor.x, fCursor.y);
		fCursor = cursor;
		_InvalidateTextRect(fCursor.x, fCursor.y, fCursor.x, fCursor.y);
		_ActivateCursor(false);
	}

	info.Reset();
}


/*!	Write strings to PTY device. If encoding system isn't UTF8, change
	encoding to UTF8 before writing PTY.
*/
void
TermView::_WritePTY(const char* text, int32 numBytes)
{
	if (fEncoding != M_UTF8) {
		while (numBytes > 0) {
			char buffer[1024];
			int32 bufferSize = sizeof(buffer);
			int32 sourceSize = numBytes;
			int32 state = 0;
			if (convert_to_utf8(fEncoding, text, &sourceSize, buffer,
					&bufferSize, &state) != B_OK || bufferSize == 0) {
				break;
			}

			fShell->Write(buffer, bufferSize);
			text += sourceSize;
			numBytes -= sourceSize;
		}
	} else {
		fShell->Write(text, numBytes);
	}
}


void
TermView::MouseDown(BPoint where)
{
	if (!IsFocus())
		MakeFocus();

	int32 buttons;
	Window()->CurrentMessage()->FindInt32("buttons", &buttons); 

	// paste button
	if ((buttons & (B_SECONDARY_MOUSE_BUTTON | B_TERTIARY_MOUSE_BUTTON)) != 0) {
		if (_HasSelection()) {
			// copy text from region
			BString copy;
			fTextBuffer->Lock();
			fTextBuffer->GetStringFromRegion(copy, fSelStart, fSelEnd);
			fTextBuffer->Unlock();
			_WritePTY(copy.String(), copy.Length());
		} else {
			// copy text from clipboard.
			Paste(be_clipboard);
		}
		return;
	}

	// Select Region
	if (buttons == B_PRIMARY_MOUSE_BUTTON) {
		int32 mod, clicks;
		Window()->CurrentMessage()->FindInt32("modifiers", &mod);
		Window()->CurrentMessage()->FindInt32("clicks", &clicks);

		if (_HasSelection()) {
			TermPos inPos = _ConvertToTerminal(where);
			if (_CheckSelectedRegion(inPos)) {
				if (mod & B_CONTROL_KEY) {
					BPoint p;
					uint32 bt;
					do {
						GetMouse(&p, &bt);
					
						if (bt == 0) {
							_Deselect();
							return;
						}
					
						snooze(40000);
					
					} while (abs((int)(where.x - p.x)) < 4
						&& abs((int)(where.y - p.y)) < 4);
				
					InitiateDrag();
					return;
				}
			}
		}

		// If mouse has a lot of movement, disable double/triple click.
		/*BPoint inPoint = fClickPoint - where;
		if (abs((int)inPoint.x) > 16 || abs((int)inPoint.y) > 16)
			clicks = 1;
		*/
		
		SetMouseEventMask(B_POINTER_EVENTS | B_KEYBOARD_EVENTS,
				B_NO_POINTER_HISTORY | B_LOCK_WINDOW_FOCUS);

		TermPos clickPos = _ConvertToTerminal(where);
	    
		if (mod & B_SHIFT_KEY) {
			fInitialSelectionStart = clickPos;
			fInitialSelectionEnd = clickPos;
			_ExtendSelection(fInitialSelectionStart, true, false);
		} else {
			_Deselect();
			fInitialSelectionStart = clickPos;
			fInitialSelectionEnd = clickPos;
		}
	    
		// If clicks larger than 3, reset mouse click counter.
		clicks = (clicks - 1) % 3 + 1;
	    
		switch (clicks) {
			case 1:
				fMouseTracking = true;
				fSelectGranularity = SELECT_CHARS;
	      		break;
	  
			case 2:
				_SelectWord(where, (mod & B_SHIFT_KEY) != 0, false);
				fMouseTracking = true;
				fSelectGranularity = SELECT_WORDS;
				break;
	
			case 3:
	 			_SelectLine(where, (mod & B_SHIFT_KEY) != 0, false);
				fMouseTracking = true;
				fSelectGranularity = SELECT_LINES;
				break;
		}
		return;
  	}

	BView::MouseDown(where);
}


void
TermView::MouseMoved(BPoint where, uint32 transit, const BMessage *message)
{
	BView::MouseMoved(where, transit, message);
	if (!fMouseTracking)
		return;

	bool doAutoScroll = false;

	if (where.y < 0) {
		doAutoScroll = true;
		fAutoScrollSpeed = where.y;
		where.x = 0;
		where.y = 0;
	}

	BRect bounds(Bounds());
	if (where.y > bounds.bottom) {
		doAutoScroll = true;
		fAutoScrollSpeed = where.y - bounds.bottom;
		where.x = bounds.right;
		where.y = bounds.bottom;
	}

	if (doAutoScroll) {
		if (fAutoScrollRunner == NULL) {
			BMessage message(kAutoScroll);
			fAutoScrollRunner = new (std::nothrow) BMessageRunner(
				BMessenger(this), &message, 10000);
		}
	} else {
		delete fAutoScrollRunner;
		fAutoScrollRunner = NULL;
	}

	switch (fSelectGranularity) {
		case SELECT_CHARS:
			_ExtendSelection(_ConvertToTerminal(where), true, true);
      		break;
		case SELECT_WORDS:
			_SelectWord(where, true, true);
      		break;
		case SELECT_LINES:
			_SelectLine(where, true, true);
      		break;
	  }
}


void
TermView::MouseUp(BPoint where)
{
	BView::MouseUp(where);
	fMouseTracking = false;

	if (fAutoScrollRunner != NULL) {
		delete fAutoScrollRunner;
		fAutoScrollRunner = NULL;
	}
}


// Select a range of text
void
TermView::_Select(TermPos start, TermPos end, bool inclusive,
	bool setInitialSelection)
{
	BAutolock _(fTextBuffer);

	_SynchronizeWithTextBuffer(0, -1);

	if (end < start)
		std::swap(start, end);

	if (inclusive)
		end.x++;

//debug_printf("TermView::_Select(): (%ld, %ld) - (%ld, %ld)\n", start.x,
//start.y, end.x, end.y);

	if (start.x < 0)
		start.x = 0;
	if (end.x >= fTermColumns)
		end.x = fTermColumns - 1;

	TermPos minPos(0, -fTextBuffer->HistorySize());
	TermPos maxPos(0, fTextBuffer->Height());
	start = restrict_value(start, minPos, maxPos);
	end = restrict_value(end, minPos, maxPos);

	// if the end is past the end of the line, select the line break, too
	if (fTextBuffer->LineLength(end.y) < end.x
			&& end.y < fTextBuffer->Height()) {
		end.y++;
		end.x = 0;
	}
	
	if (fTextBuffer->IsFullWidthChar(start.y, start.x)) {
		start.x--;
		if (start.x < 0)
			start.x = 0;
	}
	
	if (fTextBuffer->IsFullWidthChar(end.y, end.x)) {
		end.x++;
		if (end.x >= fTermColumns)
			end.x = fTermColumns;
	}

	if (fSelStart != fSelEnd)
		_InvalidateTextRange(fSelStart, fSelEnd);
	
	fSelStart = start;
	fSelEnd = end;

	if (setInitialSelection) {
		fInitialSelectionStart = fSelStart;
		fInitialSelectionEnd = fSelEnd;
	}

	_InvalidateTextRange(fSelStart, fSelEnd);
}


// extend selection (shift + mouse click)
void
TermView::_ExtendSelection(TermPos pos, bool inclusive,
	bool useInitialSelection)
{
	if (!useInitialSelection && !_HasSelection())
		return;

	TermPos start = fSelStart;
	TermPos end = fSelEnd;

	if (useInitialSelection) {
		start = fInitialSelectionStart;
		end = fInitialSelectionEnd;
	}

	if (inclusive) {
		if (pos >= start && pos >= end)
			pos.x++;
	}

	if (pos < start)
		_Select(pos, end, false, !useInitialSelection);
	else if (pos > end)
		_Select(start, pos, false, !useInitialSelection);
}


// clear the selection.
void
TermView::_Deselect()
{
//debug_printf("TermView::_Deselect(): has selection: %d\n", _HasSelection());
	if (!_HasSelection())
		return;
	
	_InvalidateTextRange(fSelStart, fSelEnd);
	
	fSelStart.SetTo(0, 0);
	fSelEnd.SetTo(0, 0);
	fInitialSelectionStart.SetTo(0, 0);
	fInitialSelectionEnd.SetTo(0, 0);
}


bool
TermView::_HasSelection() const
{
	return fSelStart != fSelEnd;
}


void
TermView::_SelectWord(BPoint where, bool extend, bool useInitialSelection)
{
	BAutolock _(fTextBuffer);

	TermPos pos = _ConvertToTerminal(where);
	TermPos start, end;
	if (!fTextBuffer->FindWord(pos, fCharClassifier, true, start, end))
		return;

	if (extend) {
		if (start < (useInitialSelection ? fInitialSelectionStart : fSelStart))
			_ExtendSelection(start, false, useInitialSelection);
		else if (end > (useInitialSelection ? fInitialSelectionEnd : fSelEnd))
			_ExtendSelection(end, false, useInitialSelection);
	} else
		_Select(start, end, false, !useInitialSelection);
}


void
TermView::_SelectLine(BPoint where, bool extend, bool useInitialSelection)
{
	TermPos start = TermPos(0, _ConvertToTerminal(where).y);
	TermPos end = TermPos(0, start.y + 1);

	if (extend) {
		if (start < (useInitialSelection ? fInitialSelectionStart : fSelStart))
			_ExtendSelection(start, false, useInitialSelection);
		else if (end > (useInitialSelection ? fInitialSelectionEnd : fSelEnd))
			_ExtendSelection(end, false, useInitialSelection);
		
	} else
		_Select(start, end, false, !useInitialSelection);
}


void
TermView::_AutoScrollUpdate()
{
	if (fMouseTracking && fAutoScrollRunner != NULL && fScrollBar != NULL) {
		float value = fScrollBar->Value();
		_ScrollTo(value + fAutoScrollSpeed, true);
		if (fAutoScrollSpeed < 0) {
			_ExtendSelection(_ConvertToTerminal(BPoint(0, 0)), true, true);
		} else {
			_ExtendSelection(_ConvertToTerminal(Bounds().RightBottom()), true,
				true);
		}
	}
}


bool
TermView::_CheckSelectedRegion(const TermPos &pos) const
{
	return pos >= fSelStart && pos < fSelEnd;
}


bool
TermView::_CheckSelectedRegion(int32 row, int32 firstColumn,
	int32& lastColumn) const
{
	if (fSelStart == fSelEnd)
		return false;

	if (row == fSelStart.y && firstColumn < fSelStart.x
			&& lastColumn >= fSelStart.x) {
		// region starts before the selection, but intersects with it
		lastColumn = fSelStart.x - 1;
		return false;
	}

	if (row == fSelEnd.y && firstColumn < fSelEnd.x
			&& lastColumn >= fSelEnd.x) {
		// region starts in the selection, but exceeds the end
		lastColumn = fSelEnd.x - 1;
		return true;
	}

	TermPos pos(firstColumn, row);
	return pos >= fSelStart && pos < fSelEnd;
}


void
TermView::GetFrameSize(float *width, float *height)
{
	int32 historySize;
	{
		BAutolock _(fTextBuffer);
		historySize = fTextBuffer->HistorySize();
	}

	if (width != NULL)
		*width = fTermColumns * fFontWidth;
	
	if (height != NULL)
		*height = (fTermRows + historySize) * fFontHeight;
}


// Find a string, and select it if found
bool
TermView::Find(const BString &str, bool forwardSearch, bool matchCase,
	bool matchWord)
{
	BAutolock _(fTextBuffer);
	_SynchronizeWithTextBuffer(0, -1);

	TermPos start;
	if (_HasSelection()) {
		if (forwardSearch)
			start = fSelEnd;
		else
			start = fSelStart;
	} else {
		// search from the very beginning/end
		if (forwardSearch)
			start = TermPos(0, -fTextBuffer->HistorySize());
		else
			start = TermPos(0, fTextBuffer->Height());
	}

	TermPos matchStart, matchEnd;
	if (!fTextBuffer->Find(str.String(), start, forwardSearch, matchCase,
			matchWord, matchStart, matchEnd)) {
		return false;
	}

	_Select(matchStart, matchEnd, false, true);
	_ScrollToRange(fSelStart, fSelEnd);

	return true;
}


//! Get the selected text and copy to str
void
TermView::GetSelection(BString &str)
{
	str.SetTo("");
	BAutolock _(fTextBuffer);
	fTextBuffer->GetStringFromRegion(str, fSelStart, fSelEnd);
}


void
TermView::NotifyQuit(int32 reason)
{
	// implemented in subclasses
}


void
TermView::CheckShellGone()
{
	if (!fShell)
		return;

	// check, if the shell does still live
	pid_t pid = fShell->ProcessID();
	team_info info;
	if (get_team_info(pid, &info) == B_BAD_TEAM_ID) {
		// the shell is gone
		NotifyQuit(0);
	}
}


void
TermView::InitiateDrag()
{
	BAutolock _(fTextBuffer);

	BString copyStr("");
	fTextBuffer->GetStringFromRegion(copyStr, fSelStart, fSelEnd);

	BMessage message(B_MIME_DATA);
	message.AddData("text/plain", B_MIME_TYPE, copyStr.String(),
		copyStr.Length());

	BPoint start = _ConvertFromTerminal(fSelStart);
	BPoint end = _ConvertFromTerminal(fSelEnd);

	BRect rect;	
	if (fSelStart.y == fSelEnd.y)
		rect.Set(start.x, start.y, end.x + fFontWidth, end.y + fFontHeight);
	else
		rect.Set(0, start.y, fTermColumns * fFontWidth, end.y + fFontHeight);

	rect = rect & Bounds();
	
	DragMessage(&message, rect);
}


void
TermView::_AboutRequested()
{
	BAlert *alert = new (std::nothrow) BAlert("about",
		"Terminal\n"
		"\twritten by Kazuho Okui and Takashi Murai\n"
		"\tupdated by Kian Duffy and others\n\n"
		"\tCopyright " B_UTF8_COPYRIGHT "2003-2008, Haiku.\n", "Ok");
	if (alert != NULL)
		alert->Go();
}


void
TermView::_ScrollTo(float y, bool scrollGfx)
{
	if (!scrollGfx)
		fScrollOffset = y;

	if (fScrollBar != NULL)
		fScrollBar->SetValue(y);
	else
		ScrollTo(BPoint(0, y));
}


void
TermView::_ScrollToRange(TermPos start, TermPos end)
{
	if (start > end)
		std::swap(start, end);

	float startY = _LineOffset(start.y);
	float endY = _LineOffset(end.y) + fFontHeight - 1;
	float height = Bounds().Height();

	if (endY - startY > height) {
		// The range is greater than the height. Scroll to the closest border.

		// already as good as it gets?
		if (startY <= 0 && endY >= height)
			return;

		if (startY > 0) {
			// scroll down to align the start with the top of the view
			_ScrollTo(fScrollOffset + startY, true);
		} else {
			// scroll up to align the end with the bottom of the view
			_ScrollTo(fScrollOffset + endY - height, true);
		}
	} else {
		// The range is smaller than the height.

		// already visible?
		if (startY >= 0 && endY <= height)
			return;

		if (startY < 0) {
			// scroll up to make the start visible
			_ScrollTo(fScrollOffset + startY, true);
		} else {
			// scroll down to make the end visible
			_ScrollTo(fScrollOffset + endY - height, true);
		}
	}
}
