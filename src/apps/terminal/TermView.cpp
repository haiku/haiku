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
 */


#include "TermView.h"

#include "CodeConv.h"
#include "Shell.h"
#include "TermBuffer.h"
#include "TermConst.h"
#include "VTkeymap.h"

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
#include <Roster.h>
#include <ScrollBar.h>
#include <String.h>
#include <Window.h>

#include <ctype.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>

#include <new>
#include <algorithm>

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

// Space at the borders of the view
const int32 kOffset = 3;

#define ROWS_DEFAULT 25
#define COLUMNS_DEFAULT 80


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


const static uint32 kUpdateSigWinch = 'Rwin';

const static rgb_color kBlackColor = { 0, 0, 0, 255 };
const static rgb_color kWhiteColor = { 255, 255, 255, 255 };



TermView::TermView(BRect frame, int32 argc, const char **argv, int32 historySize)
	: BView(frame, "termview", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE| B_PULSE_NEEDED),
	fShell(NULL),
	fWinchRunner(NULL),
	fFontWidth(0),
	fFontHeight(0),
	fFontAscent(0),
	fUpdateFlag(false),
	fInsertModeFlag(MODE_OVER),
	fScrollUpCount(0),
	fScrollBarRange(0),
	fFrameResized(false),
	fLastCursorTime(0),
	fCursorDrawFlag(true),
	fCursorStatus(true),
	fCursorBlinkingFlag(true),
	fCursorRedrawFlag(true),
	fCursorHeight(0),
	fCurPos(0, 0),
	fCurStack(0, 0),
	fBufferStartPos(-1),
	fTermRows(ROWS_DEFAULT),
	fTermColumns(COLUMNS_DEFAULT),
	fEncoding(M_UTF8),
	fTop(0),	
	fTextBuffer(NULL),
	fScrollBar(NULL),
	fTextForeColor(kBlackColor),
	fTextBackColor(kWhiteColor),
	fCursorForeColor(kWhiteColor),
	fCursorBackColor(kBlackColor),
	fSelectForeColor(kWhiteColor),
	fSelectBackColor(kBlackColor),
	fScrTop(0),
	fScrBot(fTermRows - 1),
	fScrBufSize(historySize),
	fScrRegionSet(0),
	fClickPoint(0, 0),
	fSelStart(-1, -1),
	fSelEnd(-1, -1),
	fMouseTracking(false),
	fIMflag(false)	
{	
	_InitObject(argc, argv);
}


TermView::TermView(int rows, int columns, int32 argc, const char **argv, int32 historySize)
	: BView(BRect(0, 0, 0, 0), "termview", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE| B_PULSE_NEEDED),
	fShell(NULL),
	fWinchRunner(NULL),
	fFontWidth(0),
	fFontHeight(0),
	fFontAscent(0),
	fUpdateFlag(false),
	fInsertModeFlag(MODE_OVER),
	fScrollUpCount(0),
	fScrollBarRange(0),
	fFrameResized(false),
	fLastCursorTime(0),
	fCursorDrawFlag(true),
	fCursorStatus(true),
	fCursorBlinkingFlag(true),
	fCursorRedrawFlag(true),
	fCursorHeight(0),
	fCurPos(0, 0),
	fCurStack(0, 0),
	fBufferStartPos(-1),
	fTermRows(rows),
	fTermColumns(columns),
	fEncoding(M_UTF8),
	fTop(0),	
	fTextBuffer(NULL),
	fScrollBar(NULL),
	fTextForeColor(kBlackColor),
	fTextBackColor(kWhiteColor),
	fCursorForeColor(kWhiteColor),
	fCursorBackColor(kBlackColor),
	fSelectForeColor(kWhiteColor),
	fSelectBackColor(kBlackColor),
	fScrTop(0),
	fScrBot(fTermRows - 1),
	fScrBufSize(historySize),
	fScrRegionSet(0),
	fClickPoint(0, 0),
	fSelStart(-1, -1),
	fSelEnd(-1, -1),
	fMouseTracking(false),
	fIMflag(false)	
{	
	_InitObject(argc, argv);
	SetTermSize(fTermRows, fTermColumns, true);
	
	BRect rect(0, 0, 16, 16);
	rect.OffsetTo(Bounds().right - rect.Width(),
				Bounds().bottom - rect.Height());
	
	SetFlags(Flags() | B_DRAW_ON_CHILDREN | B_FOLLOW_ALL);
	AddChild(new BDragger(rect, this,
		B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM, B_WILL_DRAW));
}


TermView::TermView(BMessage *archive)
	:
	BView(archive),
	fShell(NULL),
	fWinchRunner(NULL),
	fFontWidth(0),
	fFontHeight(0),
	fFontAscent(0),
	fUpdateFlag(false),
	fInsertModeFlag(MODE_OVER),
	fScrollUpCount(0),
	fScrollBarRange(0),
	fFrameResized(false),
	fLastCursorTime(0),
	fCursorDrawFlag(true),
	fCursorStatus(true),
	fCursorBlinkingFlag(true),
	fCursorRedrawFlag(true),
	fCursorHeight(0),
	fCurPos(0, 0),
	fCurStack(0, 0),
	fBufferStartPos(-1),
	fTermRows(ROWS_DEFAULT),
	fTermColumns(COLUMNS_DEFAULT),
	fEncoding(M_UTF8),
	fTop(0),	
	fTextBuffer(NULL),
	fScrollBar(NULL),
	fTextForeColor(kBlackColor),
	fTextBackColor(kWhiteColor),
	fCursorForeColor(kWhiteColor),
	fCursorBackColor(kBlackColor),
	fSelectForeColor(kWhiteColor),
	fSelectBackColor(kBlackColor),
	fScrTop(0),
	fScrBot(fTermRows - 1),
	fScrBufSize(1000),
	fScrRegionSet(0),
	fClickPoint(0, 0),
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
	fTextBuffer = new (std::nothrow) TermBuffer(fTermRows, fTermColumns, fScrBufSize);
	if (fTextBuffer == NULL)
		return B_NO_MEMORY;

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
	SetViewColor(fTextBackColor);
	
	return B_OK;
}


TermView::~TermView()
{
	Shell *shell = fShell;
		// _DetachShell sets fShell to NULL
	
	_DetachShell();
	
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
		*width = fTermColumns * fFontWidth + 2 * kOffset;
	if (height)
		*height = fTermRows * fFontHeight + 2 * kOffset;
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
TermView::SetTermSize(int rows, int cols, bool resize)
{
	if (rows > 0)
		fTermRows = rows;
	if (cols > 0)
		fTermColumns = cols;

	fTextBuffer->ResizeTo(fTermRows, fTermColumns, 0);

	fScrTop = 0;
	fScrBot = fTermRows - 1;

	BRect rect(0, 0, fTermColumns * fFontWidth + 2 * kOffset,
		fTermRows * fFontHeight + 2 * kOffset);

	if (resize)
		ResizeTo(rect.Width(), rect.Height());
	
	return rect;
}


void
TermView::SetTextColor(rgb_color fore, rgb_color back)
{
	fTextForeColor = fore;
	fTextBackColor = back;

	SetLowColor(fTextBackColor);
	SetViewColor(fTextBackColor);
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
	
	_FixFontAttributes(fHalfFont);
	
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

	if (fTop)
		fTop = fTop / fFontHeight;
		
	fFontAscent = font_ascent;
	fFontHeight = font_ascent + font_descent + font_leading + 1;

	fTop = fTop * fFontHeight;

	fCursorHeight = fFontHeight;
	
	if (fScrollBar != NULL) {
		fScrollBar->SetSteps(fFontHeight, fFontHeight * fTermRows);
	}
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

	// Deselecting the current selection is not the behavior that
	// R5's Terminal app displays. We want to mimic the behavior, so we will
	// no longer do the deselection
//	if (!fMouseTracking)
//		_DeSelect();
}


void
TermView::Paste(BClipboard *clipboard)
{
	if (clipboard->Lock()) {
		BMessage *clipMsg = clipboard->Data();
		char *text;
		ssize_t numBytes;
		if (clipMsg->FindData("text/plain", B_MIME_TYPE,
				(const void **)&text, &numBytes) == B_OK ) {
			// Clipboard text doesn't attached EOF?
			text[numBytes] = '\0';
			_WritePTY((uchar *)text, numBytes);
		}

		clipboard->Unlock();
	}
}


void
TermView::SelectAll()
{
	int screen_top = fTop / fFontHeight;
	int viewheight = fTermRows;
	
	int start_pos = screen_top -(fScrBufSize - viewheight * 2);
	
	CurPos start, end;
	start.x = 0;
	end.x = fTermColumns -1;
	
	if (start_pos > 0)
		start.y = start_pos;
	else
		start.y = 0;
	
	end.y = fCurPos.y  + screen_top;
	
	_Select(start, end);
}


void
TermView::Clear()
{
	_DeSelect();
	fTextBuffer->Clear();
	
	fTop = 0;
	ScrollTo(0, 0);
	
	if (LockLooper()) {
		SetHighColor(fTextBackColor);
		FillRect(Bounds());
		SetHighColor(fTextForeColor);
		UnlockLooper();
	}
	
	// reset cursor pos
	SetCurPos(0, 0);
	
	if (fScrollBar) {	
		fScrollBar->SetRange(0, 0);
		fScrollBar->SetProportion(1);
	}
}


//! Print one character
void
TermView::Insert(uchar *string, ushort attr)
{
	int width = CodeConv::UTF8GetFontWidth((char*)string);	
	if (width == FULL_WIDTH)
		attr |= A_WIDTH;

	// check column over flow.
	if (fCurPos.x + width > fTermColumns) {
		UpdateLine();
		fCurPos.x = 0;

		if (fCurPos.y == fTermRows -1)
			ScrollScreen();
		else
			fCurPos.y++;
	}

	if (fInsertModeFlag == MODE_INSERT)
		fTextBuffer->InsertSpace(fCurPos, width);

	fTextBuffer->WriteChar(fCurPos, string, attr);

	if (!fUpdateFlag)
		fBufferStartPos = fCurPos.x;

	fCurPos.x += width;
	fUpdateFlag = true;
}


//! Print a CR and move the cursor
void
TermView::InsertCR()
{
	UpdateLine();
	fTextBuffer->WriteCR(fCurPos);
	fCurPos.x = 0;
}


//! Print a LF and move the cursor
void
TermView::InsertLF()
{
	UpdateLine();

	if (fScrRegionSet) {
		if (fCurPos.y == fScrBot) {
			ScrollRegion(-1, -1, SCRUP, 1);
			return;
		}
	}

	if (fCurPos.x != fTermColumns){
		if (fCurPos.y == fTermRows -1)
			ScrollScreen();
		else
			fCurPos.y++;
	}
}


//! Print a NL and move the cursor
void
TermView::InsertNewLine(int num)
{
	ScrollRegion(fCurPos.y, -1, SCRDOWN, num);
}


//! Print a space
void
TermView::InsertSpace(int num)
{
	UpdateLine();

	fTextBuffer->InsertSpace(fCurPos, num);
	_TermDraw(fCurPos, CurPos(fTermColumns - 1, fCurPos.y));
}


//! Set or reset Insert mode
void
TermView::SetInsertMode(int flag)
{
	UpdateLine();
	fInsertModeFlag = flag;
}


//! Draw region
inline int
TermView::_TermDraw(const CurPos &start, const CurPos &end)
{
	int x1 = start.x;
	int y1 = start.y;
	int x2 = end.x;
	int y2 = end.y;

	_Redraw(x1, y1 + fTop / fFontHeight,
		x2, y2 + fTop / fFontHeight);
	
	return 0;
}


//! Draw region
int
TermView::_TermDrawSelectedRegion(CurPos start, CurPos end)
{
	CurPos inPos;

	if (end < start) {
		inPos = start;
		start = end;
		end = inPos;
	}

	if (start.y == end.y) {
		_Redraw(start.x, start.y, end.x, end.y);
	} else {
		_Redraw(start.x, start.y, fTermColumns, start.y);

		if (end.y - start.y > 0)
			_Redraw(0, start.y + 1, fTermColumns, end.y - 1);

		_Redraw(0, end.y, end.x, end.y);
	}

	return 0;
}


//! Draw region
int
TermView::_TermDrawRegion(CurPos start, CurPos end)
{
	CurPos inPos;
	int top = fTop / fFontHeight;

	if (end < start) {
		inPos = start;
		start = end;
		end = inPos;
	}

	start.y += top;
	end.y += top;

	if (start.y == end.y) {
		_Redraw(start.x, start.y, end.x, end.y);
	} else {
		_Redraw(start.x, start.y, fTermColumns - 1, start.y);

		if (end.y - start.y > 0) {
			_Redraw(0, start.y + 1, fTermColumns - 1, end.y - 1);
		}
		_Redraw(0, end.y, end.x, end.y);
	}

	return 0;
}


//! Erase below cursor below.
void
TermView::EraseBelow()
{
	UpdateLine();

	fTextBuffer->EraseBelow(fCurPos);
	_TermDraw(fCurPos, CurPos(fTermColumns - 1, fCurPos.y));
	if (fCurPos.y != fTermRows - 1)
		_TermDraw(CurPos(0, fCurPos.y + 1), CurPos(fTermColumns - 1, fTermRows - 1));
}


//! Delete num characters from current position.
void
TermView::DeleteChar(int num)
{
	UpdateLine();

	fTextBuffer->DeleteChar(fCurPos, num);
	_TermDraw(fCurPos, CurPos(fTermColumns - 1, fCurPos.y));
}


//! Delete cursor right characters.
void
TermView::DeleteColumns()
{
	UpdateLine();

	fTextBuffer->DeleteChar(fCurPos, fTermColumns - fCurPos.x);
	_TermDraw(fCurPos, CurPos(fTermColumns - 1, fCurPos.y));
}


//! Delete 'num' lines from current position with scrolling. 
void
TermView::DeleteLine(int num)
{
	ScrollRegion(fCurPos.y, -1, SCRUP, num);
}


//! Sets cursor position
void
TermView::SetCurPos(int x, int y)
{
	UpdateLine();

	if (x >= 0 && x < fTermColumns)
		fCurPos.x = x;
	if (y >= 0 && y < fTermRows)
		fCurPos.y = y;
}


//! Sets cursor x position
void
TermView::SetCurX(int x)
{
	if (x >= 0 && x < fTermRows) {
		UpdateLine();
		fCurPos.x = x;
	}
}


//! Sets cursor y position
void
TermView::SetCurY(int y)
{
	if (y >= 0 && y < fTermColumns) {
		UpdateLine();
		fCurPos.y = y;
	}
}


//! Gets cursor position
void
TermView::GetCurPos(CurPos *inCurPos)
{
	inCurPos->x = fCurPos.x;
	inCurPos->y = fCurPos.y;
}


//! Gets cursor x position
int
TermView::GetCurX()
{
	return fCurPos.x;
}


//! Gets cursor y position
int
TermView::GetCurY()
{
	return fCurPos.y;
}


//! Saves cursor position
void
TermView::SaveCursor()
{
	fCurStack = fCurPos;
}


//! Restores cursor position
void
TermView::RestoreCursor()
{
	UpdateLine();
	fCurPos = fCurStack;
}


//! Move cursor right by 'num' steps.
void
TermView::MoveCurRight(int num)
{
	UpdateLine();

	if (fCurPos.x + num >= fTermColumns) {
		// Wrap around
		fCurPos.x = 0;
		InsertCR();
		InsertLF();
	} else
		fCurPos.x += num;
}


//! Move cursor left by 'num' steps.
void
TermView::MoveCurLeft(int num)
{
	UpdateLine();

	fCurPos.x -= num;
	if (fCurPos.x < 0)
		fCurPos.x = 0;
}


//! Move cursor up by 'num' steps.
void
TermView::MoveCurUp(int num)
{
	UpdateLine();

	fCurPos.y -= num;

	if (fCurPos.y < 0)
		fCurPos.y = 0;
}


//! Move cursor down by 'num' steps.
void
TermView::MoveCurDown(int num)
{
	UpdateLine();

	fCurPos.y += num;

	if (fCurPos.y >= fTermRows)
		fCurPos.y = fTermRows - 1;
}


// TODO: Cleanup the next 3 functions!!!
void
TermView::DrawCursor()
{
	BRect rect(fFontWidth * fCurPos.x + kOffset,
		fFontHeight * fCurPos.y + fTop + kOffset,
		fFontWidth * (fCurPos.x + 1) - 1 + kOffset,
		fFontHeight * fCurPos.y + fTop + fCursorHeight - 1 + kOffset);

	uchar buf[4];
	ushort attr;

	int top = (fTop + kOffset) / fFontHeight;
	bool m_flag = _CheckSelectedRegion(CurPos(fCurPos.x, fCurPos.y + top));
	if (fTextBuffer->GetChar(fCurPos.y + top, fCurPos.x, buf, &attr) == A_CHAR) {
		int width;
		if (IS_WIDTH(attr))
			width = 2;
		else
			width = 1;

		_DrawLines(fCurPos.x * fFontWidth,
			fCurPos.y * fFontHeight + fTop,
			attr, buf, width, m_flag, true, this);
	} else {
		if (m_flag)
			SetHighColor(fSelectBackColor);
		else
			SetHighColor(fCursorBackColor);

		FillRect(rect);
	}

	Sync();
}


void
TermView::BlinkCursor()
{
	if (fCursorDrawFlag
		&& fCursorBlinkingFlag
		&& Window()->IsActive()) {
		if (fCursorStatus)
			_TermDraw(fCurPos, fCurPos);
		else
			DrawCursor();

		fCursorStatus = !fCursorStatus;
		fLastCursorTime = system_time();
	}
}


//! Draw / Clear cursor.
void
TermView::SetCurDraw(bool flag)
{
	if (!flag) {
		if (fCursorStatus)
			_TermDraw(fCurPos, fCurPos);

		fCursorStatus = false;
		fCursorDrawFlag = false;
	} else {
		if (!fCursorDrawFlag) {
			fCursorDrawFlag = true;
			fCursorStatus = true;

			if (LockLooper()) {
				DrawCursor();
				UnlockLooper();
			}
		}
	}
}


//! Sets cursor Blinking flag.
void
TermView::SetCurBlinking(bool flag)
{
	fCursorBlinkingFlag = flag;
}


//! Scroll terminal dir directory by 'num' steps.
void
TermView::ScrollRegion(int top, int bot, int dir, int num)
{
	UpdateLine();

	if (top == -1)
		top = fScrTop;

	if (bot == -1)
		bot = fScrBot;

	if (top < fScrTop)
		top = fScrTop;

	if (bot > fScrBot)
		bot = fScrBot; 

	fTextBuffer->ScrollRegion(top, bot , dir ,num);
	_TermDraw(CurPos(0, top), CurPos(fTermColumns - 1, bot));
}


//! Sets terminal scroll region.
void
TermView::SetScrollRegion(int top, int bot)
{
	if (top >= 0 && top < fTermRows) {
		if (bot >= 0 && bot < fTermRows) {
			if (top > bot) {
				fScrTop = bot;
				fScrBot = top;
			} else if (top < bot ) {
				fScrTop = top;
				fScrBot = bot;
			}
		}
	}

	if (fScrTop != 0 || fScrBot != fTermRows -1 )
		fScrRegionSet = 1;
	else
		fScrRegionSet = 0;
}


//! Scroll to cursor position.
void
TermView::ScrollAtCursor()
{
	if (LockLooper()) {
		_ResizeScrBarRange();
		fScrollUpCount = 0;
		ScrollTo(0, fTop);
		UnlockLooper();
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


//! Draw character on offscreen bitmap.
void
TermView::_DrawLines(int x1, int y1, ushort attr, uchar *buf,
	int width, int mouse, int cursor, BView *inView)
{
	rgb_color rgb_fore = fTextForeColor, rgb_back = fTextBackColor;

	inView->SetFont(&fHalfFont);

	// Move the whole thing to the right
	x1 += kOffset;
	y1 += kOffset;

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


//! Resize scroll bar range and knob size.
void
TermView::_ResizeScrBarRange()
{
	if (fScrollBar == NULL)
		return;

	float viewheight = fTermRows * fFontHeight;
	float start_pos = fTop -(fScrBufSize - fTermRows * 2) * fFontHeight;

	if (start_pos > 0) {
		fScrollBar->SetRange(start_pos, viewheight + fTop - fFontHeight);
	} else {
		fScrollBar->SetRange(0, viewheight + fTop - fFontHeight);
		fScrollBar->SetProportion( viewheight /(viewheight + fTop));
	}
}


//! Scrolls screen.
void
TermView::ScrollScreen()
{
	fTop += fFontHeight;
	fScrollUpCount++;
	fTextBuffer->ScrollLine();

	if (fScrollUpCount > fTermRows ) {
		if (LockLooper()) {
			_ResizeScrBarRange();
			fScrollBarRange += fScrollUpCount;
			fScrollUpCount = 0;
			ScrollTo(0, fTop);
			UnlockLooper();
		}
	}
}


//! Scrolls screen.
void
TermView::ScrollScreenDraw()
{
	if (fScrollUpCount){
		if (LockLooper()) {
			_ResizeScrBarRange();

			fScrollBarRange += fScrollUpCount;
			fScrollUpCount = 0;
			ScrollTo(0, fTop);
			UnlockLooper();
		}
	}
}


//!	Handler for SIGWINCH
void
TermView::_UpdateSIGWINCH()
{
	if (fFrameResized) {
		if (_HasSelection())
			_TermDrawSelectedRegion(fSelStart, fSelEnd);
		ScrollTo(0, fTop);
		_ResizeScrBarRange();

		fShell->UpdateWindowSize(fTermRows, fTermColumns);

		fFrameResized = false;
		if (fScrRegionSet == 0)
			fScrBot = fTermRows - 1;
	}
}


//!	Device Status.
void
TermView::DeviceStatusReport(int n)
{
	char sbuf[16] ;
	int len;

	switch (n) {
		case 5:
			len = sprintf(sbuf,"\033[0n") ;
			fShell->Write(sbuf, len);
			break ;
		case 6:
			len = sprintf(sbuf,"\033[%d;%dR", fTermRows, fTermColumns) ;
			fShell->Write(sbuf, len);
			break ;
		default:
			return;
	}
}


//!	Update line buffer.
void
TermView::UpdateLine()
{
	if (fUpdateFlag == true) {
		if (fInsertModeFlag == MODE_INSERT) {
			_TermDraw(CurPos(fBufferStartPos, fCurPos.y),
				CurPos(fTermColumns - 1, fCurPos.y));
		} else {
			_TermDraw(CurPos(fBufferStartPos, fCurPos.y),
				CurPos(fCurPos.x - 1, fCurPos.y));
		}
		fUpdateFlag = false;
	}
}


void
TermView::AttachedToWindow()
{
	MakeFocus(true);
	if (fScrollBar)
		fScrollBar->SetSteps(fFontHeight, fFontHeight * fTermRows);
	
	BMessage message(kUpdateSigWinch);
	fWinchRunner = new (std::nothrow) BMessageRunner(BMessenger(this), &message, 500000);

	// TODO: Since we can also be a replicant, messing
	// with the window, which is not ours, is not nice:
	// Switch to using a BMessageRunner for the
	// blinking caret too.
	Window()->SetPulseRate(1000000);
}


void
TermView::DetachedFromWindow()
{
	delete fWinchRunner;
	fWinchRunner = NULL;
}


void
TermView::Pulse()
{
	//if (system_time() > fLastCursorTime + 1000000)
		BlinkCursor();
}


void
TermView::Draw(BRect updateRect)
{
	if (IsPrinting()) {
		_DoPrint(updateRect);
		return;
	}

	int x1 =(int)(updateRect.left - kOffset) / fFontWidth;
	int x2 =(int)(updateRect.right - kOffset) / fFontWidth;

	int y1 =(int)(updateRect.top - kOffset) / fFontHeight;
	int y2 =(int)(updateRect.bottom - kOffset) / fFontHeight;

	Window()->BeginViewTransaction();

	for (int j = y1; j <= y2; j++) {
		// If(x1, y1) Buffer is in string full width character,
		// alignment start position.

		int k = x1;
		uchar buf[256];
	
		ushort attr;
		if (fTextBuffer->GetChar(j, k, buf, &attr) == IN_STRING)
			k--;

		if (k < 0)
			k = 0;

		for (int i = k; i <= x2;) {
			int count = fTextBuffer->GetString(j, i, x2, buf, &attr);
			bool insideSelection = _CheckSelectedRegion(CurPos(i, j));

			if (count < 0) {
				if (insideSelection) {
					BRect eraseRect;
					eraseRect.Set(fFontWidth * i + kOffset,
					    fFontHeight * j + kOffset,
					    fFontWidth * (i - count) -1 + kOffset,
					    fFontHeight * (j + 1) -1 + kOffset);

					SetHighColor(fSelectBackColor);
					FillRect(eraseRect);
				}
				i += abs(count);
				continue;
			}

			_DrawLines(fFontWidth * i, fFontHeight * j,
				attr, buf, count, insideSelection, false, this);
			i += count;
			if (i >= fTermColumns)
				break;
		}
	}

	if (fCursorStatus)
		DrawCursor();

	Window()->EndViewTransaction();
}


void
TermView::_DoPrint(BRect updateRect)
{
	ushort attr;
	uchar buf[256];

	const int numLines = (int)((updateRect.Height()) / fFontHeight);

	int y1 = (int)(updateRect.top - kOffset) / fFontHeight;
	y1 = y1 -(fScrBufSize - numLines * 2);
	if (y1 < 0)
		y1 = 0;

	const int y2 = y1 + numLines -1;

	const int x1 = (int)(updateRect.left - kOffset) / fFontWidth;
	const int x2 = (int)(updateRect.right - kOffset) / fFontWidth;

	for (int j = y1; j <= y2; j++) {
		// If(x1, y1) Buffer is in string full width character,
		// alignment start position.

		int k = x1;
		if (fTextBuffer->GetChar(j, k, buf, &attr) == IN_STRING)
			k--;

		if (k < 0)
			k = 0;

		for (int i = k; i <= x2;) {
			int count = fTextBuffer->GetString(j, i, x2, buf, &attr);
			if (count < 0) {
				i += abs(count);
				continue;
			}

			_DrawLines(fFontWidth * i, fFontHeight * j,
				attr, buf, count, false, false, this);
			i += count;
		}
	}
} 


void
TermView::WindowActivated(bool active)
{
	BView::WindowActivated(active);
	if (active == false) {
		// DoIMConfirm();
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

	// If bytes[0] equal intr character,
	// send signal to shell process group.
	struct termios tio;
	fShell->GetAttr(tio);
	if (*bytes == tio.c_cc[VINTR]) {
		if (tio.c_lflag & ISIG)
			fShell->Signal(SIGINT);
	}

	// Terminal filters RET, ENTER, F1...F12, and ARROW key code.
	// TODO: Cleanup
	if (numBytes == 1) {
		switch (*bytes) {
			case B_RETURN:
				if (rawChar == B_RETURN) {
					char c = 0x0d;
					fShell->Write(&c, 1);
					return;
				}
				break;
			
			case B_LEFT_ARROW:
				if (rawChar == B_LEFT_ARROW) {
					fShell->Write(LEFT_ARROW_KEY_CODE, sizeof(LEFT_ARROW_KEY_CODE) - 1);
					return;
				}
				break;

			case B_RIGHT_ARROW:
				if (rawChar == B_RIGHT_ARROW) {
					fShell->Write(RIGHT_ARROW_KEY_CODE, sizeof(RIGHT_ARROW_KEY_CODE) - 1);
					return;
				}
				break;

			case B_UP_ARROW:
				if (mod & B_SHIFT_KEY) {
					if (Bounds().top > 0) {
						ScrollBy(0, -fFontHeight);
						Window()->UpdateIfNeeded();
					}
					return;
				}
				if (rawChar == B_UP_ARROW) {
					fShell->Write(UP_ARROW_KEY_CODE, sizeof(UP_ARROW_KEY_CODE) - 1);
					return;
				}
				break;
	 
			case B_DOWN_ARROW:
				if (mod & B_SHIFT_KEY) {
					ScrollBy(0, fFontHeight);
					Window()->UpdateIfNeeded();
					return;
				}

				if (rawChar == B_DOWN_ARROW) {
					fShell->Write(DOWN_ARROW_KEY_CODE, sizeof(DOWN_ARROW_KEY_CODE) - 1);
					return;
				}
				break;

			case B_INSERT:
				if (rawChar == B_INSERT) {
					fShell->Write(INSERT_KEY_CODE, sizeof(INSERT_KEY_CODE) - 1);
					return;
				}
				break;

			case B_HOME:
				if (rawChar == B_HOME) {
					fShell->Write(HOME_KEY_CODE, sizeof(HOME_KEY_CODE) - 1);
					return;
				}
				break;

			case B_END:
				if (rawChar == B_END) {
					fShell->Write(END_KEY_CODE, sizeof(END_KEY_CODE) - 1);
					return;
				}
				break;

			case B_PAGE_UP:
				if (mod & B_SHIFT_KEY) {
					if (Bounds().top > 0) {
						ScrollBy(0, -fFontHeight * fTermRows );
						Window()->UpdateIfNeeded();
					}					
					return;
				}
				if (rawChar == B_PAGE_UP) {
					fShell->Write(PAGE_UP_KEY_CODE, sizeof(PAGE_UP_KEY_CODE) - 1);
					return;
				}
				break;

			case B_PAGE_DOWN:
				if (mod & B_SHIFT_KEY) {
					ScrollBy(0, fFontHeight * fTermRows);
					Window()->UpdateIfNeeded();
					return;
				}
	 
				if (rawChar == B_PAGE_DOWN) {
					fShell->Write(PAGE_DOWN_KEY_CODE, sizeof(PAGE_DOWN_KEY_CODE) - 1);
					return;
				}
				break;

			case B_FUNCTION_KEY:
				// TODO: Why not just fShell->Write(key) ?
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
	} else {
		// input multibyte character
		if (fEncoding != M_UTF8) {
			char destBuffer[16];
			int cnum = CodeConv::ConvertFromInternal(bytes, numBytes,
				(char *)destBuffer, fEncoding);
			fShell->Write(destBuffer, cnum);
			return;
		}
	}

	fShell->Write(bytes, numBytes);
}


void
TermView::FrameResized(float width, float height)
{
	const int cols = ((int)width + 1 - 2 * kOffset) / fFontWidth;
	const int rows = ((int)height + 1 - 2 * kOffset) / fFontHeight;
	
	int offset = 0;

	if (rows < fCurPos.y + 1) {
		fTop += (fCurPos.y  + 1 - rows) * fFontHeight;
		offset = fCurPos.y + 1 - rows;
		fCurPos.y = rows - 1;
	}
	fTextBuffer->ResizeTo(rows, cols, offset);
	fTermRows = rows;
	fTermColumns = cols;

	fFrameResized = true;
	
	if (fScrollBar != NULL)
		fScrollBar->SetSteps(fFontHeight, fFontHeight * fTermRows);
}


void 
TermView::MessageReceived(BMessage *msg)
{
	entry_ref ref;
	char *ctrl_l = "";

	// first check for any dropped message
	if (msg->WasDropped()) {
		char *text;
		int32 numBytes;
		//rgb_color *color;
	
		int32 i = 0;

		if (msg->FindRef("refs", i++, &ref) == B_OK) {
			_DoFileDrop(ref);

			while (msg->FindRef("refs", i++, &ref) == B_OK) {
				_WritePTY((const uchar*)" ", 1);
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
			_WritePTY((uchar *)text, numBytes);
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
					_WritePTY((const uchar*)" ", 1);
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
		case kUpdateSigWinch:
			_UpdateSIGWINCH();
			break;
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
	_WritePTY((const uchar *)string.String(), string.Length());
}


/*!	Write strings to PTY device. If encoding system isn't UTF8, change
	encoding to UTF8 before writing PTY.
*/
void
TermView::_WritePTY(const uchar *text, int numBytes)
{
	if (fEncoding != M_UTF8) {
		uchar *destBuffer = (uchar *)malloc(numBytes * 3);
		numBytes = CodeConv::ConvertFromInternal((char*)text, numBytes,
			(char*)destBuffer, fEncoding);
		fShell->Write(destBuffer, numBytes);
		free(destBuffer);
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
			fTextBuffer->GetStringFromRegion(copy, fSelStart, fSelEnd);
			_WritePTY((uchar *)copy.String(), copy.Length());
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
			CurPos inPos = _ConvertToTerminal(where);
			if (_CheckSelectedRegion(inPos)) {
				if (mod & B_CONTROL_KEY) {
					BPoint p;
					uint32 bt;
					do {
						GetMouse(&p, &bt);
					
						if (bt == 0) {
							_DeSelect();
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

		fClickPoint = where;
	    
		if (mod & B_SHIFT_KEY)
			_AddSelectRegion(_ConvertToTerminal(where));
		else
			_DeSelect();
	   
	    
		// If clicks larger than 3, reset mouse click counter.
		clicks = clicks % 3;
		if (clicks == 0)
			clicks = 3;
	    
		switch (clicks) {
			case 1:
				fMouseTracking = true;
	      			break;
	  
			case 2:
				_SelectWord(where, mod); 
				break;
	
			case 3:
	 			_SelectLine(where, mod);
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
	
	CurPos startPos = _ConvertToTerminal(fClickPoint);
	CurPos endPos = _ConvertToTerminal(where);
	if (endPos.y < 0)
		return;

	_DeSelect();
	_Select(startPos, endPos);
	
	// Scroll check
	if (fScrollBar != NULL) {
		// Get now scroll point
		float scrollStart, scrollEnd;
		fScrollBar->GetRange(&scrollStart, &scrollEnd);
		float scrollPos = fScrollBar->Value();
		
		if (where.y < Bounds().LeftTop().y ) {
			// mouse point left of window
			if (scrollPos != scrollStart)
				ScrollTo(0, where.y);
		}	
		
		if (where.y > Bounds().LeftBottom().y) {	
			// mouse point left of window
			if (scrollPos != scrollEnd)
				ScrollTo(0, where.y);
		}
	}
}


void
TermView::MouseUp(BPoint where)
{
	BView::MouseUp(where);
	fMouseTracking = false;
}


// Select a range of text
void
TermView::_Select(CurPos start, CurPos end)
{
	if (end < start)
		std::swap(start, end);	
	
	if (start.x < 0)
		start.x = 0;
	if (end.x >= fTermColumns)
		end.x = fTermColumns - 1;
	
	uchar buf[4];
	ushort attr;
	if (fTextBuffer->GetChar(start.y, start.x, buf, &attr) == IN_STRING) {
		start.x--;
		if (start.x < 0)
			start.x = 0;
	}
	
	if (fTextBuffer->GetChar(end.y, end.x, buf, &attr) == IN_STRING) {
		end.x++;
		if (end.x >= fTermColumns)
			end.x = fTermColumns;
	}
	
	fSelStart = start;
	fSelEnd = end;
	
	fTextBuffer->Select(fSelStart, fSelEnd);
	_TermDrawSelectedRegion(fSelStart, fSelEnd);
}


// Add select region(shift + mouse click)
void
TermView::_AddSelectRegion(CurPos pos)
{
	if (!_HasSelection())
		return;
	
	// error check, and if mouse point to a plase full width character,
	// select point decliment.
	if (pos.x >= fTermColumns)
		pos.x = fTermColumns - 1;
	else if (pos.x < 0)
		pos.x = 0;
	
	if (pos.y < 0)
		pos.y = 0;
	
	uchar buf[4];
	ushort attr;
	if (fTextBuffer->GetChar(pos.y, pos.x, buf, &attr) == IN_STRING) {
		pos.x++;
		if (pos.x >= fTermColumns)
			pos.x = fTermColumns - 1;
	}
	
	CurPos start = fSelStart;
	CurPos end = fSelEnd;
	CurPos inPos;
	
	// Mouse point is same as selected line.
	if (pos.y == fSelStart.y && pos.y == fSelEnd.y) {
		
		if (abs(pos.x - start.x) > abs(pos.x - end.x)) {
		
			fSelStart = start;
			fSelEnd = pos;
			inPos = end;
		
		} else {
		
			fSelStart = end;
			fSelEnd = pos;
			inPos = start;
		}
		// else, End point set to near the start or end point.
	} else if (abs(pos.y - start.y) > abs(pos.y - end.y)) {
	
		fSelStart = start;
		fSelEnd = pos;
		inPos = end;
	} else if (abs(pos.y - start.y) > abs(pos.y - end.y)) {
		fSelStart = end;
		fSelEnd = pos;
		inPos = start;
	
	} else {
		if (start > end) {
			inPos = start;
			start = end;
			end = inPos;
		}
		
		if (pos.y < start.y) {
			fSelStart = end;
			fSelEnd = pos;
			inPos = start;
		} else {
			fSelStart = start;
			fSelEnd = pos;
			inPos = end;
		}
	}
	
	fTextBuffer->Select(fSelStart, fSelEnd);
	_TermDrawSelectedRegion(inPos, fSelEnd);
}


// Resize select region (mouse drag)
void
TermView::_ResizeSelectRegion(CurPos pos)
{
	//TODO: Broken. Selecting from right to left doesn't work.

	CurPos inPos = fSelEnd;
	
	// error check, and if mouse point to a plase full width character,
	// select point decliment.
	if (pos.x >= fTermColumns)
		pos.x = fTermColumns - 1;
	else if (pos.x < 0)
		pos.x = 0;
	
	if (pos.y < 0)
		pos.y = 0;
	
	uchar buf[4];
	ushort attr;
	if (fTextBuffer->GetChar(pos.y, pos.x, buf, &attr) == IN_STRING) {	
		pos.x++;
		
		if (pos == inPos)
			return;
		
		if (pos.x >= fTermColumns)
			pos.x = fTermColumns - 1;
	}

	fSelEnd = pos;

	fTextBuffer->Select(fSelStart, fSelEnd);
	_TermDrawSelectedRegion(inPos, pos);
}
  

// DeSelect a range of text
void
TermView::_DeSelect(void)
{
	if (!_HasSelection())
		return;
	
	fTextBuffer->DeSelect();
	
	CurPos start = fSelStart;
	CurPos end = fSelEnd;
	
	fSelStart.Set(-1, -1);
	fSelEnd.Set(-1, -1);
	
	_TermDrawSelectedRegion(start, end);
}


bool
TermView::_HasSelection() const
{
	return fSelStart != fSelEnd;
}


void
TermView::_SelectWord(BPoint where, int mod)
{
	CurPos start, end, pos;
	bool flag;
	
	pos = _ConvertToTerminal(where);
	flag = fTextBuffer->FindWord(pos, &start, &end);
	fTextBuffer->Select(start, end);

	if (mod & B_SHIFT_KEY) {
		if (flag) {
			if (start < fSelStart)
				_AddSelectRegion(start);
			else if (end > fSelEnd)
				_AddSelectRegion(end);		
		} else
			_AddSelectRegion(pos);
	} else {
		_DeSelect();
		if (flag)
			_Select(start, end);
	}
}


void
TermView::_SelectLine(BPoint where, int mod)
{
	CurPos pos = _ConvertToTerminal(where);
	
	if (mod & B_SHIFT_KEY) {
		
		CurPos start = CurPos(0, pos.y);
		CurPos end = CurPos(fTermColumns - 1, pos.y);
		
		if (start < fSelStart)
			_AddSelectRegion(start);
		else if (end > fSelEnd)
			_AddSelectRegion(end);
		
	} else {
		_DeSelect();
		_Select(CurPos(0, pos.y), CurPos(fTermColumns - 1, pos.y));
	}
}


// Convert View visible area corrdination to cursor position.
CurPos
TermView::_ConvertToTerminal(const BPoint &p)
{
	return CurPos((p.x - kOffset) / fFontWidth, (p.y - kOffset) / fFontHeight);
}


// Convert cursor position to view coordination.
BPoint
TermView::_ConvertFromTerminal(const CurPos &pos)
{
	return BPoint(fFontWidth * pos.x + kOffset,
		pos.y * fFontHeight + fTop + kOffset);
}


bool
TermView::_CheckSelectedRegion(const CurPos &pos)
{
	CurPos start, end;
	
	if (fSelStart > fSelEnd) {
		start = fSelEnd;
		end = fSelStart;
	} else {
		start = fSelStart;
		end = fSelEnd;
	}
	
	if (pos >= start && pos <= end)
		return true;
	
	return false;
  
}

void
TermView::GetFrameSize(float *width, float *height)
{
	if (width != NULL)
		*width = 2 * kOffset + fTermColumns * fFontWidth;
	
	if (height == NULL)
		return;
	
	if (!fTop) {
		*height = 2 * kOffset + fTermRows * fFontHeight;
		return;
	}
	
	if (fTop - fTermRows * fFontHeight > fScrBufSize * fFontHeight) {
		
		*height = fScrBufSize * fFontHeight + 2 * kOffset;
		return;
	}
	
	*height = kOffset + fTop + fTermRows * fFontHeight;
}


// Find a string, and select it if found
bool
TermView::Find(const BString &str, bool forwardSearch, bool matchCase, bool matchWord)
{
	//Get the buffer contents
	BString buffer;	
	fTextBuffer->ToString(buffer);

	CurPos selectionstart = fSelStart;
	CurPos selectionend = fSelEnd;

	int offset = 0;
	if (selectionstart.x >= 0 || selectionstart.y >= 0) {
		if (forwardSearch)
			//Set the offset to the end of the selection
			offset = (selectionend.y) * fTermColumns + selectionend.x;
		else
			offset = (selectionstart.y) * fTermColumns + selectionstart.x;
	}

	int initialresult = -1;
	int result = B_ERROR;

	for (;;) {		
		//Actual search
		if (forwardSearch) {
			if (matchCase) 
				result = buffer.FindFirst(str, offset);
			else
				result = buffer.IFindFirst(str, offset);
		} else {
			if (matchCase)
				result = buffer.FindLast(str, offset);
			else
				result = buffer.IFindLast(str, offset);
		}
		
		if (result == B_ERROR) { //Wrap search like Be's Terminal
			if (forwardSearch) {
				if (matchCase)
					result = buffer.FindFirst(str, 0);
				else
					result = buffer.IFindFirst(str, 0);
			} else {
				if (matchCase)
					result = buffer.FindLast(str, buffer.Length());
				else
					result = buffer.IFindLast(str, buffer.Length());
			}
		}

		if (result < 0)
			return false;
	
		if (matchWord) {
			if (isalnum(buffer.ByteAt(result - 1)) || isalnum(buffer.ByteAt(result + str.Length()))) {
				if (initialresult == -1) //Set the initial offset to the first result to aid word matching
					initialresult = result;
				else if (initialresult == result) //We went round the buffer, nothing found
					return false;
				if (forwardSearch)
					offset = result + str.Length();
				else
					offset = result;
				continue;
			}
			else
				break;
		}
		else
			break;
	}
	
	//Select the found text
	selectionstart.y = result / fTermColumns;
	selectionstart.x = result % fTermColumns;
	//Length -1 since it seems to count the \0 as well
	selectionend.y = (result + str.Length() - 1) / fTermColumns;
	selectionend.x = (result + str.Length() - 1) % fTermColumns;
	//Update the contents of the view
	_DeSelect();
	_Select(selectionstart, selectionend);
 
	return true;
}


//! Get the selected text and copy to str
void
TermView::GetSelection(BString &str)
{
	str.SetTo("");
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
	BString copyStr("");
	fTextBuffer->GetStringFromRegion(copyStr, fSelStart, fSelEnd);

	BMessage message(B_MIME_DATA);
	message.AddData("text/plain", B_MIME_TYPE, copyStr.String(), copyStr.Length());

	BPoint start = _ConvertFromTerminal(fSelStart);
	BPoint end = _ConvertFromTerminal(fSelEnd);
	
	BRect rect;	
	if (fSelStart.y == fSelEnd.y) {
		rect.Set(start.x, start.y - fTop, end.x + fFontWidth,
			end.y + fFontHeight - fTop);
	
	} else {
	
		rect.Set(0, start.y - fTop, fTermColumns * fFontWidth,
			end.y + fFontHeight - fTop);
	}
	
	rect = rect & Bounds();
	
	DragMessage(&message, rect);
}


inline void
TermView::_Redraw(int x1, int y1, int x2, int y2)
{
	BRect rect(x1 * fFontWidth + kOffset,
	    y1 * fFontHeight + kOffset,
	    (x2 + 1) * fFontWidth + kOffset - 1,
	    (y2 + 1) * fFontHeight -1 + kOffset);

	if (LockLooper()) {
		Invalidate(rect);
		UnlockLooper();
	}	
}


/* static */
void
TermView::_FixFontAttributes(BFont &font)
{
	font.SetSpacing(B_FIXED_SPACING);
}


void
TermView::_AboutRequested()
{
	BAlert *alert = new (std::nothrow) BAlert("about",
					"Terminal\n"
					"\twritten by Kazuho Okui and Takashi Murai\n"
					"\tupdated by Kian Duffy and others\n\n"
					"\tCopyright " B_UTF8_COPYRIGHT "2003-2007, Haiku.\n", "Ok");
	if (alert != NULL)
		alert->Go();
}


