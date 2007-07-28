/*
 * Copyright 2001-2007, Haiku, Inc.
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
#include "PrefView.h"
#include "Shell.h"
#include "TermBuffer.h"
#include "TermConst.h"
#include "VTkeymap.h"

#include <Application.h>
#include <Autolock.h>
#include <Beep.h>
#include <Clipboard.h>
#include <Debug.h>
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

using std::nothrow;

// defined VTKeyTbl.c
extern int function_keycode_table[];
extern char *function_key_char_table[];

const static rgb_color kTermColorTable[16] = {
	{  0,   0,   0, 0},
	{255,   0,   0, 0},
	{  0, 255,   0, 0},
	{255, 255,   0, 0},
	{  0,   0, 255, 0},
	{255,   0, 255, 0},
	{  0, 255, 255, 0},
	{255, 255, 255, 0},
};


const unsigned char M_ADD_CURSOR[] = {
	16,	// Size
	1,	// Color depth
	0,	// Hot spot y
	1,	// Hot spot x

	// Cursor image
	0x70, 0x00,
	0x48, 0x00,
	0x48, 0x00,
	0x27, 0xc0,
	0x24, 0xb8,
	0x12, 0x54,
	0x10, 0x02,
	0x78, 0x02,
	0x98, 0x02,
	0x84, 0x02,
	0x60, 0x02,
	0x18, 0x12,
	0x04, 0x10,
	0x02, 0x7c,
	0x00, 0x10,
	0x00, 0x10,
	// Mask image
	0x70, 0x00,
	0x78, 0x00,
	0x78, 0x00,
	0x3f, 0xc0,
	0x3f, 0xf8,
	0x1f, 0xfc,
	0x1f, 0xfe,
	0x7f, 0xfe,
	0xff, 0xfe,
	0xff, 0xfe,
	0x7f, 0xfe,
	0x1f, 0xfe,
	0x07, 0xfc,
	0x03, 0xfc,
	0x00, 0x10,
	0x00, 0x10,
};



#define MOUSE_THR_CODE 'mtcd'
#define ROWS_DEFAULT 25
#define COLUMNS_DEFAULT 80

const static uint32 kUpdateSigWinch = 'Rwin';

const static rgb_color kBlackColor = { 0, 0, 0, 255 };
const static rgb_color kWhiteColor = { 255, 255, 255, 255 };


TermView::TermView(BRect frame, const char *command, int32 historySize)
	: BView(frame, "termview", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS | B_PULSE_NEEDED),
	fShell(NULL),
	fFontWidth(0),
	fFontHeight(0),
	fFontAscent(0),
	fUpdateFlag(false),
	fInsertModeFlag(MODE_OVER),
	fScrollUpCount(0),
	fScrollBarRange(0),
	fFrameResized(false),
	fLastCursorTime(0),
	fCursorDrawFlag(CURON),
	fCursorStatus(CURON),
	fCursorBlinkingFlag(CURON),
	fCursorRedrawFlag(CURON),
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
	fPreviousMousePoint(0, 0),
	fSelStart(-1, -1),
	fSelEnd(-1, -1),
	fMouseTracking(false),
	fMouseThread(-1),
	fQuitting(false),
	fIMflag(false)	
{	
	_InitObject(command);
}


TermView::TermView(BMessage *archive)
	:
	BView(archive),
	fShell(NULL),
	fFontWidth(0),
	fFontHeight(0),
	fFontAscent(0),
	fUpdateFlag(false),
	fInsertModeFlag(MODE_OVER),
	fScrollUpCount(0),
	fScrollBarRange(0),
	fFrameResized(false),
	fLastCursorTime(0),
	fCursorDrawFlag(CURON),
	fCursorStatus(CURON),
	fCursorBlinkingFlag(CURON),
	fCursorRedrawFlag(CURON),
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
	fPreviousMousePoint(0, 0),
	fSelStart(-1, -1),
	fSelEnd(-1, -1),
	fMouseTracking(false),
	fMouseThread(-1),
	fQuitting(false),
	fIMflag(false)	
{
	if (archive->FindInt32("encoding", (int32 *)&fEncoding) < B_OK)
		fEncoding = M_UTF8;
	if (archive->FindInt32("columns", (int32 *)&fTermColumns) < B_OK)
		fTermColumns = 80;
	if (archive->FindInt32("rows", (int32 *)&fTermRows) < B_OK)
		fTermRows = 25;
	
	// TODO: Retrieve command, colors, history size, etc. from archive
	_InitObject(NULL);
}


void
TermView::_InitObject(const char *command)
{
	fTextBuffer = new TermBuffer(fTermRows, fTermColumns, fScrBufSize);

	SetTermFont(be_fixed_font, be_fixed_font);
	
	//SetIMAware(false);

	fShell = new Shell();	
	status_t status = fShell->Open(fTermRows, fTermColumns,
					command, longname2shortname(id2longname(fEncoding)));	
	if (status < B_OK)
		throw status;

	status = AttachShell(fShell);
	if (status < B_OK)
		throw status;

	SetTermSize(fTermRows, fTermColumns, false);

	_InitMouseThread();
}


TermView::~TermView()
{
	DetachShell();
	
	delete fTextBuffer;
	delete fShell;

	fQuitting = true;
	kill_thread(fMouseThread);
}


/* static */
BArchivable *
TermView::Instantiate(BMessage* data)
{
	if (validate_instantiation(data, "TermView"))
		return new TermView(data);
	return NULL;
}


status_t
TermView::Archive(BMessage* data, bool deep) const
{
	status_t status = BView::Archive(data, deep);
	if (status < B_OK)
		return status;
	status = data->AddInt32("encoding", (int32)fEncoding);
	if (status < B_OK)
		return status;
	status = data->AddInt32("columns", (int32)fTermColumns);
	if (status < B_OK)
		return status;
	status = data->AddInt32("rows", (int32)fTermRows);
	
	return status;
}


void
TermView::GetPreferredSize(float *width, float *height)
{
	if (width)
		*width = fTermColumns * fFontWidth;
	if (height)
		*height = fTermRows * fFontHeight;
}


status_t
TermView::AttachShell(Shell *shell)
{
	if (shell == NULL)
		return B_BAD_VALUE;
	
	fShell = shell;
	fShell->ViewAttached(this);	

	return B_OK;
}


void
TermView::DetachShell()
{
	fShell->ViewDetached();
	fShell = NULL;
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

	BRect rect(0, 0, fTermColumns * fFontWidth, fTermRows * fFontHeight);

	if (resize)
		ResizeTo(fTermColumns * fFontWidth - 1, fTermRows * fFontHeight -1);

	Invalidate();

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


//! Sets half and full fonts for terminal
void
TermView::SetTermFont(const BFont *halfFont, const BFont *fullFont)
{
	char buf[4];
	int halfWidth = 0;

	fHalfFont = halfFont;
	fFullFont = fullFont;

	_FixFontAttributes(fHalfFont);
	_FixFontAttributes(fFullFont);

	// calculate half font's max width
	// Not Bounding, check only A-Z(For case of fHalfFont is KanjiFont. )
	for (int c = 0x20 ; c <= 0x7e; c++){
		sprintf(buf, "%c", c);
		int tmpWidth = (int)fHalfFont.StringWidth(buf);
		if (tmpWidth > halfWidth)
			halfWidth = tmpWidth;
	}

	// How to calculate FullWidth ?
	fFontWidth = halfWidth;

	// Second, Calc Font Height
	font_height fh, hh;
	fHalfFont.GetHeight(&hh);
	fFullFont.GetHeight(&fh);

	int font_ascent =(int)((fh.ascent > hh.ascent) ? fh.ascent : hh.ascent);
	int font_descent =(int)((fh.descent > hh.descent) ? fh.descent : hh.descent);
	int font_leading =(int)((fh.leading > hh.leading) ? fh.leading : hh.leading);

	if (font_leading == 0)
		font_leading = 1;

	if (fTop)
		fTop = fTop / fFontHeight;

	fFontHeight = font_ascent + font_descent + font_leading + 1;

	fTop = fTop * fFontHeight;

	fFontAscent = font_ascent;
	fCursorHeight = font_ascent + font_descent + font_leading + 1;
}


void
TermView::SetScrollBar(BScrollBar *scrollBar)
{
	fScrollBar = scrollBar;
}


//! Print one character
void
TermView::PutChar(uchar *string, ushort attr, int width)
{
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
TermView::PutCR()
{
	UpdateLine();
	fTextBuffer->WriteCR(fCurPos);
	fCurPos.x = 0;
}


//! Print a LF and move the cursor
void
TermView::PutLF()
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
TermView::PutNL(int num)
{
	ScrollRegion(fCurPos.y, -1, SCRDOWN, num);
}


//! Print a space
void
TermView::InsertSpace(int num)
{
	UpdateLine();

	fTextBuffer->InsertSpace(fCurPos, num);
	TermDraw(fCurPos, CurPos(fTermColumns - 1, fCurPos.y));
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
TermView::TermDraw(const CurPos &start, const CurPos &end)
{
	int x1 = start.x;
	int y1 = start.y;
	int x2 = end.x;
	int y2 = end.y;

	Redraw(x1, y1 + fTop / fFontHeight,
		x2, y2 + fTop / fFontHeight);
	
	return 0;
}


//! Draw region
int
TermView::TermDrawSelectedRegion(CurPos start, CurPos end)
{
	CurPos inPos;

	if (end < start) {
		inPos = start;
		start = end;
		end = inPos;
	}

	if (start.y == end.y) {
		Redraw(start.x, start.y, end.x, end.y);
	} else {
		Redraw(start.x, start.y, fTermColumns, start.y);

		if (end.y - start.y > 0)
			Redraw(0, start.y + 1, fTermColumns, end.y - 1);

		Redraw(0, end.y, end.x, end.y);
	}

	return 0;
}


//! Draw region
int
TermView::TermDrawRegion(CurPos start, CurPos end)
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
		Redraw(start.x, start.y, end.x, end.y);
	} else {
		Redraw(start.x, start.y, fTermColumns - 1, start.y);

		if (end.y - start.y > 0) {
			Redraw(0, start.y + 1, fTermColumns - 1, end.y - 1);
		}
		Redraw(0, end.y, end.x, end.y);
	}

	return 0;
}


//! Erase below cursor below.
void
TermView::EraseBelow()
{
	UpdateLine();

	fTextBuffer->EraseBelow(fCurPos);
	TermDraw(fCurPos, CurPos(fTermColumns - 1, fCurPos.y));
	if (fCurPos.y != fTermRows - 1)
		TermDraw(CurPos(0, fCurPos.y + 1), CurPos(fTermColumns - 1, fTermRows - 1));
}


//! Delete num characters from current position.
void
TermView::DeleteChar(int num)
{
	UpdateLine();

	fTextBuffer->DeleteChar(fCurPos, num);
	TermDraw(fCurPos, CurPos(fTermColumns - 1, fCurPos.y));
}


//! Delete cursor right characters.
void
TermView::DeleteColumns()
{
	UpdateLine();

	fTextBuffer->DeleteChar(fCurPos, fTermColumns - fCurPos.x);
	TermDraw(fCurPos, CurPos(fTermColumns - 1, fCurPos.y));
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
		PutCR();
		PutLF();
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
	BRect rect(fFontWidth * fCurPos.x, fFontHeight * fCurPos.y + fTop,
		fFontWidth * (fCurPos.x + 1) - 1, fFontHeight * fCurPos.y + fTop + fCursorHeight - 1);

	uchar buf[4];
	ushort attr;

	int top = fTop / fFontHeight;
	bool m_flag = CheckSelectedRegion(CurPos(fCurPos.x, fCurPos.y + fTop / fFontHeight));
	if (fTextBuffer->GetChar(fCurPos.y + top, fCurPos.x, buf, &attr) == A_CHAR) {
		int width;
		if (IS_WIDTH(attr))
			width = 2;
		else
			width = 1;

		DrawLines(fCurPos.x * fFontWidth,
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
	if (fCursorDrawFlag == CURON
		&& fCursorBlinkingFlag == CURON
		&& Window()->IsActive()) {
		if (fCursorStatus == CURON)
			TermDraw(fCurPos, fCurPos);
		else
			DrawCursor();

		fCursorStatus = fCursorStatus == CURON ? CUROFF : CURON;
		fLastCursorTime = system_time();
	}
}


//! Draw / Clear cursor.
void
TermView::SetCurDraw(bool flag)
{
	if (flag == CUROFF) {
		if (fCursorStatus == CURON)
			TermDraw(fCurPos, fCurPos);

		fCursorStatus = CUROFF;
		fCursorDrawFlag = CUROFF;
	} else {
		if (fCursorDrawFlag == CUROFF) {
			fCursorDrawFlag = CURON;
			fCursorStatus = CURON;

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
	TermDraw(CurPos(0, top), CurPos(fTermColumns - 1, bot));
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
		ResizeScrBarRange();
		fScrollUpCount = 0;
		ScrollTo(0, fTop);
		UnlockLooper();
	}
}


status_t
TermView::_InitMouseThread()
{
	// spawn Mouse Tracking thread.
	if (fMouseThread < 0) {
		fMouseThread = spawn_thread(MouseTracking, "MouseTracking",
			B_NORMAL_PRIORITY,this);
	} else
		return B_BAD_THREAD_ID;

	return resume_thread(fMouseThread);
}


//!	Thread for tracking mouse.
int32
TermView::MouseTracking(void *data)
{
	int32 code, selected = false;
	uint32 button;
	thread_id sender;
	CurPos stpos, edpos;
	BPoint stpoint, edpoint;
	float scr_start, scr_end, scr_pos;
	
	TermView *theObj =(TermView *)data;
	
	while(!theObj->fQuitting) {
  
	if (1) {
#ifdef CHANGE_CURSOR_IMAGE    
		if (!has_data(find_thread(NULL))) {
			BRect r;
			
			if (theObj->HasSelection()
				&& (modifiers() & B_CONTROL_KEY)) {
			
			if (theObj->LockLooper()) {
				theObj->GetMouse(&stpoint, &button);
				r = theObj->Bounds();
				theObj->UnlockLooper();
			}
			if (r.Contains(stpoint)) {
				CurPos tmppos = theObj->BPointToCurPos(stpoint);
				if (theObj->fSelStart > theObj->fSelEnd) {
					stpos = theObj->fSelEnd;
					edpos = theObj->fSelStart;
				} else {
					stpos = theObj->fSelStart;
					edpos = theObj->fSelEnd;
				}
				
				if (tmppos > stpos && tmppos < edpos)
					be_app->SetCursor(M_ADD_CURSOR);
				else 
					be_app->SetCursor(B_HAND_CURSOR);
			}
		}
		snooze(50 * 1000);
		continue;
	} else {
#endif
		code = receive_data(&sender,(void *)&stpoint, sizeof(BPoint));
	}

	if (code != MOUSE_THR_CODE)
		continue;

	selected = theObj->HasSelection();
	edpoint.Set(-1, -1);
	
	stpos = theObj->BPointToCurPos(stpoint);
	
	do {
		
		snooze(40 * 1000);
		
		if (theObj->LockLooper()) {
			theObj->GetMouse(&edpoint, &button);
			theObj->UnlockLooper();
		}
	
	edpos = theObj->BPointToCurPos(edpoint);
	if (edpos.y < 0)
		continue;

		if (stpoint == edpoint) {
			continue;
		} else {
			if (!selected) {
				theObj->Select(stpos, edpos);
				selected = true;
			} else {
			
				// Align cursor point to text.
				if (stpos == edpos)
					continue;
				
				if (edpos > stpos) {
					edpoint.x -= theObj->fFontWidth / 2;
					edpos = theObj->BPointToCurPos(edpoint);
					//edpos.x--;
					if (edpos.x < 0)
						edpos.x = 0;
				}
				else
				if (edpos < stpos) {
					edpoint.x += theObj->fFontWidth / 2;
					edpos = theObj->BPointToCurPos(edpoint);
					//edpos.x++;
					if (edpos.x > theObj->fTermColumns)
						edpos.x = theObj->fTermColumns;
				}
				
				// Scroll check
				if (theObj->fScrollBar != NULL && theObj->LockLooper()) {
					// Get now scroll point
					theObj->fScrollBar->GetRange(&scr_start, &scr_end);
					scr_pos = theObj->fScrollBar->Value();
					
					if (edpoint.y < theObj->Bounds().LeftTop().y )
					
						// mouse point left of window
						if (scr_pos != scr_start)
							theObj->ScrollTo(0, edpoint.y);
						
						if (edpoint.y > theObj->Bounds().LeftBottom().y) {
						
						// mouse point left of window
						if (scr_pos != scr_end)
							theObj->ScrollTo(0, edpoint.y);
					}
					theObj->UnlockLooper();
				}
				theObj->ResizeSelectRegion(edpos);
			}
		}
	} while(button);
	theObj->fMouseTracking = false;
	}
	
	exit_thread(B_OK);
	return 0;
}


//! Draw character on offscreen bitmap.
void
TermView::DrawLines(int x1, int y1, ushort attr, uchar *buf,
	int width, int mouse, int cursor, BView *inView)
{
	int x2, y2;
	int forecolor, backcolor;
	rgb_color rgb_fore = fTextForeColor, rgb_back = fTextBackColor, rgb_tmp;

	// Set Font.
	if (IS_WIDTH(attr))
		inView->SetFont(&fFullFont);
	else
		inView->SetFont(&fHalfFont);

	// Set pen point
	x2 = x1 + fFontWidth * width;
	y2 = y1 + fFontHeight;

	// color attribute
	forecolor = IS_FORECOLOR(attr);
	backcolor = IS_BACKCOLOR(attr);

	if (IS_FORESET(attr))
		rgb_fore = kTermColorTable[forecolor];

	if (IS_BACKSET(attr))
		rgb_back = kTermColorTable[backcolor];

	// Selection check.
	if (cursor) {
		rgb_fore = fCursorForeColor;
		rgb_back = fCursorBackColor;
	} else if (mouse){
		rgb_fore = fSelectForeColor;
		rgb_back = fSelectBackColor;
	} else {
		// Reverse attribute(If selected area, don't reverse color).
		if (IS_INVERSE(attr)) {
			rgb_tmp = rgb_fore;
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
TermView::ResizeScrBarRange()
{
	if (fScrollBar == NULL)
		return;

	float viewheight = fTermRows * fFontHeight;
	float start_pos = fTop -(fScrBufSize - fTermRows *2) * fFontHeight;

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
			ResizeScrBarRange();
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
			ResizeScrBarRange();

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
		if (HasSelection())
			TermDrawSelectedRegion(fSelStart, fSelEnd);
		ScrollTo(0, fTop);
		ResizeScrBarRange();

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
			TermDraw(CurPos(fBufferStartPos, fCurPos.y),
				CurPos(fTermColumns - 1, fCurPos.y));
		} else {
			TermDraw(CurPos(fBufferStartPos, fCurPos.y),
				CurPos(fCurPos.x - 1, fCurPos.y));
		}
		fUpdateFlag = false;
	}
}


void
TermView::AttachedToWindow()
{
	SetFont(&fHalfFont);
	MakeFocus(true);
	if (fScrollBar)
		fScrollBar->SetSteps(fFontHeight, fFontHeight * fTermRows);
	
	BMessage message(kUpdateSigWinch);
	fWinchRunner = new (std::nothrow) BMessageRunner(BMessenger(this), &message, 500000);

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
		DoPrint(updateRect);
		return;
	}

	int x1, x2, y1, y2;
	int i, j, k, count;
	ushort attr;
	uchar buf[256];
	int m_flag;
	BRect eraseRect;

	x1 =(int)updateRect.left / fFontWidth;
	x2 =(int)updateRect.right / fFontWidth;

	y1 =(int)updateRect.top / fFontHeight;
	y2 =(int)updateRect.bottom / fFontHeight;

	Window()->BeginViewTransaction();

	for (j = y1; j <= y2; j++) {
		// If(x1, y1) Buffer is in string full width character,
		// alignment start position.

		k = x1;
		if (fTextBuffer->GetChar(j, k, buf, &attr) == IN_STRING)
			k--;

		if (k < 0)
			k = 0;

		for (i = k; i <= x2;) {
			count = fTextBuffer->GetString(j, i, x2, buf, &attr);
			m_flag = CheckSelectedRegion(CurPos(i, j));

			if (count < 0) {
				if (m_flag) {
					eraseRect.Set(fFontWidth * i,
						fFontHeight * j,
						fFontWidth *(i - count) -1,
						fFontHeight *(j + 1) -1);

					SetHighColor(fSelectBackColor);
					FillRect(eraseRect);
				}
				i += abs(count);
				continue;
			}

			DrawLines(fFontWidth * i, fFontHeight * j,
				attr, buf, count, m_flag, false, this);
			i += count;
			if (i >= fTermColumns)
				break;
		}
	}

	if (fCursorStatus == CURON)
		DrawCursor();

	Window()->EndViewTransaction();
}


void
TermView::DoPrint(BRect updateRect)
{
	ushort attr;
	uchar buf[256];

	const int numLines =(int)((updateRect.Height()) / fFontHeight);

	int y1 =(int)updateRect.top / fFontHeight;
	y1 = y1 -(fScrBufSize - numLines * 2);
	if (y1 < 0)
		y1 = 0;

	const int y2 = y1 + numLines -1;

	const int x1 =(int)updateRect.left / fFontWidth;
	const int x2 =(int)updateRect.right / fFontWidth;

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

			DrawLines(fFontWidth * i, fFontHeight * j,
				attr, buf, count, false, false, this);
			i += count;
		}
	}
} 


void
TermView::WindowActivated(bool active)
{
	if (active == false) {
		// DoIMConfirm();
	}
}


void
TermView::KeyDown(const char *bytes, int32 numBytes)
{
	int32 key, mod;
	Looper()->CurrentMessage()->FindInt32("modifiers", &mod);
	Looper()->CurrentMessage()->FindInt32("key", &key);

	if (fIMflag)
		return;

	// If bytes[0] equal intr charactor,
	// send signal to shell process group.
	struct termios tio;
	fShell->GetAttr(tio);
	if (*bytes == tio.c_cc[VINTR]) {
		if (tio.c_lflag & ISIG)
			fShell->Signal(SIGINT);
	}

	// Terminal changes RET, ENTER, F1...F12, and ARROW key code.

	if (numBytes == 1) {
		switch (*bytes) {
			case B_RETURN:
			{
				char c = 0x0d;
				fShell->Write(&c, 1);
				break;
			}
			case B_LEFT_ARROW:
				fShell->Write(LEFT_ARROW_KEY_CODE, sizeof(LEFT_ARROW_KEY_CODE)-1);
				break;

			case B_RIGHT_ARROW:
				fShell->Write(RIGHT_ARROW_KEY_CODE, sizeof(RIGHT_ARROW_KEY_CODE)-1);
				break;

			case B_UP_ARROW:
				if (mod & B_SHIFT_KEY) {
					if (Bounds().top <= 0)
						return;
					ScrollBy(0, -fFontHeight);
					Window()->UpdateIfNeeded();
					return;
				}

				fShell->Write(UP_ARROW_KEY_CODE, sizeof(UP_ARROW_KEY_CODE)-1);
				break;

			case B_DOWN_ARROW:
				if (mod & B_SHIFT_KEY) {
					ScrollBy(0, fFontHeight);
					Window()->UpdateIfNeeded();
					return;
				}

				fShell->Write(DOWN_ARROW_KEY_CODE, sizeof(DOWN_ARROW_KEY_CODE)-1);
				break;

			case B_INSERT:
				fShell->Write(INSERT_KEY_CODE, sizeof(INSERT_KEY_CODE)-1);
				break;

			case B_HOME:
				fShell->Write(HOME_KEY_CODE, sizeof(HOME_KEY_CODE)-1);
				break;

			case B_PAGE_UP:
				if (mod & B_SHIFT_KEY) {
					if (Bounds().top <= 0)
						return;
					ScrollBy(0, -fFontHeight * fTermRows );
					Window()->UpdateIfNeeded();
					return;
				}

				fShell->Write(PAGE_UP_KEY_CODE, sizeof(PAGE_UP_KEY_CODE)-1);
				break;

			case B_PAGE_DOWN:
				if (mod & B_SHIFT_KEY) {
					ScrollBy(0, fFontHeight * fTermRows);
					Window()->UpdateIfNeeded();
					return;
				}

				fShell->Write(PAGE_DOWN_KEY_CODE, sizeof(PAGE_DOWN_KEY_CODE)-1);
				break;

			case B_END:
				fShell->Write(END_KEY_CODE, sizeof(END_KEY_CODE)-1);
				break;

			case B_FUNCTION_KEY:
				for (int32 c = 0; c < 12; c++) {
					if (key == function_keycode_table[c]) {
						fShell->Write(function_key_char_table[c], 5);
						return;
					}
				}
				break;

			default:
				fShell->Write(bytes, numBytes);
				break;
		}
	} else {
		// input multibyte character
		if (fEncoding != M_UTF8) {
			uchar dstbuf[1024];
			int cnum = CodeConv::ConvertFromInternal(bytes, numBytes,
				(char *)dstbuf, fEncoding);
			fShell->Write(dstbuf, cnum);
		}
	}
}


void
TermView::FrameResized(float width, float height)
{
	const int cols = ((int)width + 1) / fFontWidth;
	const int rows = ((int)height + 1) / fFontHeight;

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
	
	// TODO: Fix this	
	Invalidate();
}


void 
TermView::MessageReceived(BMessage *msg)
{
	entry_ref ref;
	char *ctrl_l = "";

	switch (msg->what){
		case B_SIMPLE_DATA:
		{
			int32 i = 0;
			if (msg->FindRef("refs", i++, &ref) == B_OK) {
				DoFileDrop(ref);

				while (msg->FindRef("refs", i++, &ref) == B_OK) {
					WritePTY((const uchar*)" ", 1);
					DoFileDrop(ref);
				}
			} else
				BView::MessageReceived(msg);
			break;
		}

		case B_MIME_DATA:
		{
			char *text;
			int32 numBytes;
			status_t sts;

			if (msg->WasDropped()) {
				sts = msg->FindData("text/plain",
					B_MIME_TYPE, (const void **)&text, &numBytes);
				if (sts != B_OK)
					break;

				WritePTY((uchar *)text, numBytes);
			}
			break;
		}

		case B_COPY:
			DoCopy();
			break;

		case B_PASTE:
		{
			int32 code;
			if (msg->FindInt32("index", &code) == B_OK)
				DoPaste();
			break;
		}

		case B_SELECT_ALL:
			DoSelectAll();
			break;
		
		case B_SET_PROPERTY: {
			int32 i;
			int32 encodingID;
			BMessage spe;
			msg->GetCurrentSpecifier(&i, &spe);
			if (!strcmp("encoding", spe.FindString("property", i))){
				msg->FindInt32 ("data",  &encodingID);
				SetEncoding(encodingID);
				msg->SendReply(B_REPLY);
			} else {
				BView::MessageReceived(msg);
			}
			break;
		}

		case B_GET_PROPERTY: {
			int32 i;
			BMessage spe;
			msg->GetCurrentSpecifier(&i, &spe);
			if (!strcmp("encoding", spe.FindString("property", i))){
				BMessage reply(B_REPLY);
				reply.AddInt32("result", Encoding());
				msg->SendReply(&reply);
			}
			else if (!strcmp("tty", spe.FindString("property", i))) {
				BMessage reply(B_REPLY);
				reply.AddString("result", TerminalName());
				msg->SendReply(&reply);
			} else {
				BView::MessageReceived(msg);
			}
			break;
		}
	
		case MENU_CLEAR_ALL:
			DoClearAll();
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
	static property_info propList[] = { 
		{ "encoding",
		{B_GET_PROPERTY, 0},
		{B_DIRECT_SPECIFIER, 0},
		"get muterminal encoding"}, 
		{ "encoding",
		{B_SET_PROPERTY, 0},
		{B_DIRECT_SPECIFIER, 0},
		"set muterminal encoding"}, 
		{ "tty",
		{B_GET_PROPERTY, 0},
		{B_DIRECT_SPECIFIER, 0},
		"get tty_name."}, 
		{ 0  }
		     
	};

	message->AddString("suites", "suite/vnd.naan-termview"); 
	BPropertyInfo propInfo(propList); 
	message->AddFlat("messages", &propInfo); 
	return BView::GetSupportedSuites(message); 
}


BHandler*
TermView::ResolveSpecifier(BMessage *msg, int32 index, BMessage *specifier,
				int32 form, const char *property)
{
	if (((strcmp(property, "encode") == 0) 
		&& ((msg->what == B_SET_PROPERTY) || (msg->what == B_GET_PROPERTY))) 
		|| ((strcmp(property, "tty") == 0) &&  (msg->what == B_GET_PROPERTY))) 
	return this;

	return BView::ResolveSpecifier(msg, index, specifier, form, property);
}


//! Gets dropped file full path and display it at cursor position.
void 
TermView::DoFileDrop(entry_ref &ref)
{
	BEntry ent(&ref); 
	BPath path(&ent);
	BString string(path.Path());

	string.CharacterEscape(" ~`#$&*()\\|[]{};'\"<>?!",'\\');
	WritePTY((const uchar *)string.String(), string.Length());
}


//! Copy selected text to Clipboard.
void 
TermView::DoCopy()
{
	if (!HasSelection())
		return;

	BString copyStr;
	fTextBuffer->GetStringFromRegion(copyStr, fSelStart, fSelEnd);

	if (be_clipboard->Lock()) {
		BMessage *clipMsg = NULL;
		be_clipboard->Clear();

		if ((clipMsg = be_clipboard->Data()) != NULL) {
			clipMsg->AddData("text/plain", B_MIME_TYPE, copyStr.String(),
				copyStr.Length());
			be_clipboard->Commit();
		}
		be_clipboard->Unlock();
	}

	// Deselecting the current selection is not the behavior that
	// R5's Terminal app displays. We want to mimic the behavior, so we will
	// no longer do the deselection
//	if (!fMouseTracking)
//		DeSelect();
}


//! Paste clipboard text at cursor position.
void 
TermView::DoPaste()
{
	if (be_clipboard->Lock()) {
		BMessage *clipMsg = be_clipboard->Data();
		char *text;
		ssize_t numBytes;
		if (clipMsg->FindData("text/plain", B_MIME_TYPE,
				(const void **)&text, &numBytes) == B_OK ) {
			// Clipboard text doesn't attached EOF?
			text[numBytes] = '\0';
			WritePTY((uchar *)text, numBytes);
		}

		be_clipboard->Unlock();
	}
}


//! Select all displayed text and text /in buffer.
void 
TermView::DoSelectAll(void)
{
	CurPos start, end;
	int screen_top;
	int viewheight, start_pos;
	
	screen_top = fTop / fFontHeight;
	viewheight = fTermRows;
	
	start_pos = screen_top -(fScrBufSize - viewheight * 2);
	
	start.x = 0;
	end.x = fTermColumns -1;
	
	if (start_pos > 0)
		start.y = start_pos;
	else
		start.y = 0;
	
	end.y = fCurPos.y  + screen_top;
	
	Select(start, end);
}

// Clear display and text buffer, then moves Cursorr at home position.
void 
TermView::DoClearAll(void)
{
	DeSelect();
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


/*!	Write strings to PTY device. If encoding system isn't UTF8, change
	encoding to UTF8 before writing PTY.
*/
void
TermView::WritePTY(const uchar *text, int numBytes)
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
	int32 buttons;
	Window()->CurrentMessage()->FindInt32("buttons", &buttons); 

	// paste button
	if ((buttons & (B_SECONDARY_MOUSE_BUTTON | B_TERTIARY_MOUSE_BUTTON)) != 0) {
		if (HasSelection()) {
			// copy text from region
			BString copy;
			fTextBuffer->GetStringFromRegion(copy, fSelStart, fSelEnd);
			WritePTY((uchar *)copy.String(), copy.Length());
		} else {
			// copy text from clipboard.
			DoPaste();
		}
		return;
	}

	// Select Region
	if (buttons == B_PRIMARY_MOUSE_BUTTON) {
		int32 mod, clicks;
		Window()->CurrentMessage()->FindInt32("modifiers", &mod);
		Window()->CurrentMessage()->FindInt32("clicks", &clicks);

		if (HasSelection()) {
			CurPos inPos, stPos, edPos;
			if (fSelStart < fSelEnd) {
				stPos = fSelStart;
				edPos = fSelEnd;
			} else {
				stPos = fSelEnd;
				edPos = fSelStart;
			}

			inPos = BPointToCurPos(where);

			// If mouse pointer is avove selected Region, start Drag'n Copy.
			if (inPos > stPos && inPos < edPos) {
				if (mod & B_CONTROL_KEY) {
					BPoint p;
					uint32 bt;
					do {
						GetMouse(&p, &bt);
					
						if (bt == 0) {
							DeSelect();
							return;
						}
					
						snooze(40 * 1000);
					
					} while (abs((int)(where.x - p.x)) < 4
						&& abs((int)(where.y - p.y)) < 4);
				
					BString copyStr("");
					fTextBuffer->GetStringFromRegion(copyStr, fSelStart, fSelEnd);
				
					BMessage msg(B_MIME_TYPE);
					msg.AddData("text/plain", B_MIME_TYPE, copyStr.String(), copyStr.Length());
				
					BPoint st = CurPosToBPoint(stPos);
					BPoint ed = CurPosToBPoint(edPos);
					BRect r;
					
					if (stPos.y == edPos.y) {
						r.Set(st.x, st.y - fTop,
						ed.x + fFontWidth, ed.y + fFontHeight - fTop);
					
					} else {
					
						r.Set(0, st.y - fTop,
						fTermColumns * fFontWidth, ed.y + fFontHeight - fTop);
					}
					
					r = r & Bounds();
					
					DragMessage(&msg, r);
					return;
				}
			}
		}

		// If mouse has a lot of movement, disable double/triple click.
		BPoint inPoint = fPreviousMousePoint - where;
		if (abs((int)inPoint.x) > 16 || abs((int)inPoint.y) > 16)
			clicks = 1;
	
		fPreviousMousePoint = where;
	    
		if (mod & B_SHIFT_KEY)
			AddSelectRegion(BPointToCurPos(where));
		else
			DeSelect();
	   
	    
		// If clicks larger than 3, reset mouse click counter.
		clicks = clicks % 3;
		if (clicks == 0)
			clicks = 3;
	    
		switch (clicks) {
			case 1:
				fMouseTracking = true;
				send_data(fMouseThread, MOUSE_THR_CODE, (void *)&where, sizeof(BPoint));
	      			break;
	  
			case 2:
				SelectWord(where, mod); 
				break;
	
			case 3:
	 			SelectLine(where, mod);
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
}


// Select a range of text
void
TermView::Select(CurPos start, CurPos end)
{
	uchar buf[4];
	ushort attr;
	
	if (start.x < 0)
		start.x = 0;
	if (end.x >= fTermColumns)
		end.x = fTermColumns - 1;
	
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
	TermDrawSelectedRegion(fSelStart, fSelEnd);
}

// Add select region(shift + mouse click)
void
TermView::AddSelectRegion(CurPos pos)
{
	uchar buf[4];
	ushort attr;
	CurPos start, end, inPos;
	
	if (!HasSelection())
		return;
	
	// error check, and if mouse point to a plase full width character,
	// select point decliment.
	if (pos.x >= fTermColumns)
		pos.x = fTermColumns - 1;
	else if (pos.x < 0)
		pos.x = 0;
	
	if (pos.y < 0)
		pos.y = 0;
	
	if (fTextBuffer->GetChar(pos.y, pos.x, buf, &attr) == IN_STRING) {
		pos.x++;
		if (pos.x >= fTermColumns)
			pos.x = fTermColumns - 1;
	}
	
	start = fSelStart;
	end = fSelEnd;
	
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
	TermDrawSelectedRegion(inPos, fSelEnd);
}

// Resize select region (mouse drag)
void
TermView::ResizeSelectRegion(CurPos pos)
{
	CurPos inPos;
	uchar buf[4];
	ushort attr;
	
	inPos = fSelEnd;
	
	// error check, and if mouse point to a plase full width character,
	// select point decliment.
	if (pos.x >= fTermColumns)
		pos.x = fTermColumns - 1;
	else if (pos.x < 0)
		pos.x = 0;
	
	if (pos.y < 0)
		pos.y = 0;
	
	if (fTextBuffer->GetChar(pos.y, pos.x, buf, &attr) == IN_STRING) {
		
		pos.x++;
		
		if (pos == inPos)
			return;
		
		if (pos.x >= fTermColumns)
			pos.x = fTermColumns - 1;
	}
	fSelEnd = pos;
	
	fTextBuffer->Select(fSelStart, pos);
	TermDrawSelectedRegion(inPos, pos);
}
  

// DeSelect a range of text
void
TermView::DeSelect(void)
{
	CurPos start, end;
	
	if (!HasSelection())
		return;
	
	fTextBuffer->DeSelect();
	
	start = fSelStart;
	end = fSelEnd;
	
	fSelStart.Set(-1, -1);
	fSelEnd.Set(-1, -1);
	
	TermDrawSelectedRegion(start, end);
}


bool
TermView::HasSelection() const
{
	return fSelStart != fSelEnd;
}


void
TermView::SelectWord(BPoint where, int mod)
{
	CurPos start, end, pos;
	bool flag;
	
	pos = BPointToCurPos(where);
	flag = fTextBuffer->FindWord(pos, &start, &end);
	fTextBuffer->Select(start, end);

	if (mod & B_SHIFT_KEY) {
	
		if (flag) {
		
			if (start < fSelStart)
				AddSelectRegion(start);
			else if (end > fSelEnd)
				AddSelectRegion(end);
	
		
		} else
			AddSelectRegion(pos);

		
	} else {
		DeSelect();
		if (flag)
			Select(start, end);
	}
}

void
TermView::SelectLine(BPoint where, int mod)
{
	CurPos start, end, pos;
	
	pos = BPointToCurPos(where);
	
	if (mod & B_SHIFT_KEY) {
		
		start = CurPos(0, pos.y);
		end = CurPos(fTermColumns - 1, pos.y);
		
		if (start < fSelStart)
			AddSelectRegion(start);
		else if (end > fSelEnd)
			AddSelectRegion(end);
		
	} else {
		DeSelect();
		Select(CurPos(0, pos.y), CurPos(fTermColumns - 1, pos.y));
	}
}

// Convert View visible area corrdination to cursor position.
CurPos
TermView::BPointToCurPos(const BPoint &p)
{
	return CurPos(p.x / fFontWidth, p.y / fFontHeight);
}

// Convert cursor position to view coordination.
BPoint
TermView::CurPosToBPoint(const CurPos &pos)
{
	return BPoint(fFontWidth * pos.x, pos.y * fFontHeight + fTop);
}

bool
TermView::CheckSelectedRegion(const CurPos &pos)
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
		*width = fTermColumns * fFontWidth;
	
	if (height == NULL)
		return;
	
	if (!fTop) {
		*height = fTermRows * fFontHeight;
		return;
	}
	
	if (fTop - fTermRows * fFontHeight > fScrBufSize * fFontHeight) {
		
		*height = fScrBufSize * fFontHeight;
		return;
	}
	
	*height = fTop + fTermRows * fFontHeight;
}

// Sets terninal rows and cols.
void
TermView::GetFontInfo(int *width, int *height)
{
	 *width = fFontWidth;
	 *height = fFontHeight;
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
	DeSelect();
	Select(selectionstart, selectionend);
 
	return true;
}

//! Get the selected text and copy to str
void
TermView::GetSelection(BString &str)
{
	str.SetTo("");
	fTextBuffer->GetStringFromRegion(str, fSelStart, fSelEnd);
}



inline void
TermView::Redraw(int x1, int y1, int x2, int y2)
{
	BRect rect(x1 * fFontWidth, y1 * fFontHeight,
		(x2 + 1) * fFontWidth -1, (y2 + 1) * fFontHeight -1);

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

 
