/*
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Parts Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. 
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files or portions
 * thereof (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice
 *    in the  binary, as well as this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided with
 *    the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <support/Debug.h>
#include <PopUpMenu.h>
#include <ScrollBar.h>
#include <storage/Path.h>
#include <support/Beep.h>
#include <Input.h>
#include <String.h>
#include <Clipboard.h>
#include <Roster.h>
#include <Autolock.h>

#include <string.h>
#include <stdlib.h>

#include "TermView.h"
#include "TermWindow.h"
#include "TermApp.h"
#include "TermParse.h"
#include "TermBuffer.h"
#include "CodeConv.h"
#include "VTkeymap.h"
#include "TermConst.h"
#include "PrefHandler.h"
#include "MenuUtil.h"
#include "PrefView.h"

#include <termios.h>
#include <signal.h>
#include "spawn.h"

// defined VTKeyTbl.c
extern int function_keycode_table[];
extern char *function_key_char_table[];

extern int gNowCoding;	/* defined TermParse.cpp */

rgb_color gTermColorTable[16] = {
  {  0,   0,   0, 0},
  {255,   0,   0, 0},
  {  0, 255,   0, 0},
  {255, 255,   0, 0},
  {  0,   0, 255, 0},
  {255,   0, 255, 0},
  {  0, 255, 255, 0},
  {255, 255, 255, 0},
};

extern PrefHandler *gTermPref;	/* Gloval Preference Handler */

/***************************************************************************
 *
 * Constructor and Destructor.
 *
 **************************************************************************/

////////////////////////////////////////////////////////////////////////////
// TermView ()
//	Constructor.
////////////////////////////////////////////////////////////////////////////
TermView::TermView (BRect frame, CodeConv *inCodeConv)
  :BView (frame, "termview",
	  B_FOLLOW_NONE, B_WILL_DRAW  | B_FRAME_EVENTS)
{
  int rows, cols;

  // Cursor reset.
  fCurPos.Set(0, 0);
  fCurStack.Set(0, 0);
  fBufferStartPos = -1;

  rows = gTermPref->getInt32 (PREF_ROWS);
  cols = gTermPref->getInt32 (PREF_COLS);
  
  fTermRows = rows;
  fTermColumns = cols;

  fCodeConv = inCodeConv;
  
  // scroll pointer.
  fTop = 0;

  // create TermBuffer and CodeConv Class.
  fTextBuffer = new TermBuffer(rows, cols);
  
  // cursor Blinking and draw flag.
  fCursorStatus = CURON;
  fCursorDrawFlag = CURON;
  fUpdateFlag = false;
  fCursorBlinkingFlag = CURON;

  fSelStart.Set (-1, -1);
  fSelEnd.Set (-1, -1);
  fSelected = false;
  fMouseTracking = false;

  // scroll bar varriables.
  fScrollUpCount = 0;
  fScrollBarRange = 0;
  fScrRegionSet = 0;
  fScrBufSize = gTermPref->getInt32 (PREF_HISTORY_SIZE);

  // resize flag.
  fFrameResized = 0;
  
  // terminal mode flag.
  fInsertModeFlag = MODE_OVER;
  fInverseFlag = fBoldFlag = fUnderlineFlag = 0;

  // terminal scroll flag.
  fScrTop = 0;
  fScrBot = rows - 1;
  
  fPopMenu = NULL;
  this->SetMouseButton ();
  this->SetMouseCursor ();
  fPreviousMousePoint.Set (0, 0);

 // SetIMAware (gTermPref->getInt32 (PREF_IM_AWARE));
  fIMflag = false;
  
  fViewThread = -1;
  fMouseThread = -1;
  fQuitting = 1;
  this->InitViewThread();

  fDrawRect_p = 0;

}

////////////////////////////////////////////////////////////////////////////
// ~TermView ()
//	 Destructor.
////////////////////////////////////////////////////////////////////////////
TermView::~TermView ()
{
//  delete InputMessenger;
  delete fTextBuffer;
  fQuitting = 0;
  kill_thread (fViewThread);
  kill_thread (fMouseThread);
  delete_sem (fDrawRectSem);
}

/***************************************************************************
 *
 * Public Member Functions:
 *
 **************************************************************************/

////////////////////////////////////////////////////////////////////////////
// GetFontSize (int &, int &)
//	Get terminal font width and height size.
////////////////////////////////////////////////////////////////////////////
void
TermView::GetFontSize (int *width, int *height)
{
  *width = fFontWidth;
  *height = fFontHeight;
}

////////////////////////////////////////////////////////////////////////////
// SetTermSize()
//	Sets terninal rows and cols.
////////////////////////////////////////////////////////////////////////////
BRect
TermView::SetTermSize (int rows, int cols, bool flag)
{
  if (rows) fTermRows = rows;
  if (cols) fTermColumns = cols;

  fTextBuffer->ResizeTo (fTermRows, fTermColumns, 0);

  fScrTop = 0;
  fScrBot = fTermRows - 1;

  BRect r(0, 0,
	  fTermColumns * fFontWidth,
	  fTermRows * fFontHeight);
  if (flag) {
    ResizeTo (fTermColumns * fFontWidth -1,
	      fTermRows * fFontHeight -1);
  }

  this->Invalidate (this->Frame());
  
  return r;

}

////////////////////////////////////////////////////////////////////////////
// SetMouseButton()
//	Sets mouse button assignment.
////////////////////////////////////////////////////////////////////////////
void
TermView::SetMouseButton (void)
{
  mSelectButton =
    SetupMouseButton (gTermPref->getString(PREF_SELECT_MBUTTON));
  mSubMenuButton =
    SetupMouseButton (gTermPref->getString(PREF_SUBMENU_MBUTTON));
  mPasteMenuButton =
    SetupMouseButton (gTermPref->getString(PREF_PASTE_MBUTTON));

  return;

}
////////////////////////////////////////////////////////////////////////////
// SetMouseCursor()
//	Sets mouse cursor image.
////////////////////////////////////////////////////////////////////////////
void
TermView::SetMouseCursor (void)
{
  if (!strcmp (gTermPref->getString (PREF_MOUSE_IMAGE), "Hand cursor"))
    fMouseImage = false;
  else
    fMouseImage = true;

  return;

}

////////////////////////////////////////////////////////////////////////////
// SetTermColor()
//	Sets terninal rows and cols.
////////////////////////////////////////////////////////////////////////////
void
TermView::SetTermColor (void)
{

  fTextForeColor = gTermPref->getRGB (PREF_TEXT_FORE_COLOR);
  fTextBackColor = gTermPref->getRGB (PREF_TEXT_BACK_COLOR);
  fSelectForeColor = gTermPref->getRGB(PREF_SELECT_FORE_COLOR);
  fSelectBackColor = gTermPref->getRGB(PREF_SELECT_BACK_COLOR);
  fCursorForeColor = gTermPref->getRGB(PREF_CURSOR_FORE_COLOR);
  fCursorBackColor = gTermPref->getRGB(PREF_CURSOR_BACK_COLOR);
  
  SetLowColor (fTextBackColor);
  SetViewColor (fTextBackColor);

  return;

}

////////////////////////////////////////////////////////////////////////////
// SetTermFont()
//	Sets Terninal fonts. (half and full)
////////////////////////////////////////////////////////////////////////////
void
TermView::SetTermFont (BFont *halfFont, BFont *fullFont)
{
  char  buf[4];
  int  halfWidth = 0;

  fHalfFont = halfFont;
  fFullFont = fullFont;

  // calc half font's max width
  // Not Bounding, check only A-Z (For case of fHalfFont is KanjiFont. )
  for(int c = 0x20 ; c <= 0x7e; c++){
    sprintf(buf, "%c", c);
    int  tmpWidth = (int)fHalfFont.StringWidth(buf);
    if(tmpWidth > halfWidth)  halfWidth = tmpWidth;
  }
  // How to calculate FullWidth ?
  fFontWidth = halfWidth;

  // Second, Calc Font Height
  font_height  fh, hh;
  fHalfFont.GetHeight(&hh);
  fFullFont.GetHeight(&fh);

  int font_ascent, font_descent,font_leading;

  font_ascent  = (int)((fh.ascent > hh.ascent) ? fh.ascent : hh.ascent);
  font_descent = (int)((fh.descent > hh.descent) ? fh.descent : hh.descent);
  font_leading = (int)((fh.leading > hh.leading) ? fh.leading : hh.leading);

  if (font_leading == 0) font_leading = 1;

  if (fTop)
    fTop = fTop / fFontHeight;

  fFontHeight = font_ascent + font_descent + font_leading + 1;

  fTop = fTop * fFontHeight;

  fFontAscent = font_ascent;
  fCursorHeight = font_ascent + font_descent + font_leading + 1;

  return;

}

////////////////////////////////////////////////////////////////////////////
// void SetScrollBar (BScrollBar *scrbar)
// 	Set scrollbar object.
////////////////////////////////////////////////////////////////////////////
void
TermView::SetScrollBar (BScrollBar *scrbar)
{

  fScrollBar = scrbar;
}

////////////////////////////////////////////////////////////////////////////
// void PutChar (uchar *string, ushort attr, int width)
// 	Put one charactor.
////////////////////////////////////////////////////////////////////////////
void
TermView::PutChar (uchar *string, ushort attr, int width)
{

  if (width == FULL_WIDTH) attr |= A_WIDTH;
  
  /* check column over flow. */
  if ((fCurPos.x + width) > fTermColumns) {
    this->UpdateLine ();
    fCurPos.x = 0;

    if (fCurPos.y == fTermRows -1) {
      this->ScrollScreen ();
    } else {
      fCurPos.y++;
    }
  }

  if (fInsertModeFlag == MODE_INSERT) {
    fTextBuffer->InsertSpace (fCurPos, width);
  }
  fTextBuffer->WriteChar (fCurPos, string, attr);
  
  if (fUpdateFlag == false) {
    fBufferStartPos = fCurPos.x;
  }
  
  fCurPos.x += width;
  fUpdateFlag = true;
  
  return;
}
////////////////////////////////////////////////////////////////////////////
// PutCR ()
//	 Put CR and cursor move.
////////////////////////////////////////////////////////////////////////////
void
TermView::PutCR (void)
{
  this->UpdateLine ();
  fTextBuffer->WriteCR (fCurPos);
  fCurPos.x = 0;
  
  return;
}
////////////////////////////////////////////////////////////////////////////
// PutLF ()
//	Put LF and cursor screen move.
////////////////////////////////////////////////////////////////////////////
void
TermView::PutLF (void)
{

  this->UpdateLine ();
  
  if (fScrRegionSet)
    if (fCurPos.y == fScrBot) {
      this->ScrollRegion (-1, -1, SCRUP, 1);
      return;
    }

  if (fCurPos.x != fTermColumns){
    if (fCurPos.y == fTermRows -1) {
      this->ScrollScreen ();
    } else {
      fCurPos.y++;
    }
}

  return;
}
////////////////////////////////////////////////////////////////////////////
// PutNL ()
//	Put New line after cursor rows.
////////////////////////////////////////////////////////////////////////////
void
TermView::PutNL (int num)
{

  this->ScrollRegion (fCurPos.y, -1, SCRDOWN, num);
  return;
  
}
////////////////////////////////////////////////////////////////////////////
// InsertSpace (int num)
//	Put space.
////////////////////////////////////////////////////////////////////////////
void
TermView::InsertSpace (int num)
{
  this->UpdateLine ();
  
  fTextBuffer->InsertSpace (fCurPos, num);
  this->TermDraw (fCurPos, CurPos(fTermColumns - 1, fCurPos.y));

  return;
  
}
////////////////////////////////////////////////////////////////////////////
// SetInsertMode (int flag)
//	Set and Reset Insert Mode
////////////////////////////////////////////////////////////////////////////
void
TermView::SetInsertMode (int flag)
{
  this->UpdateLine();
  fInsertModeFlag = flag;
      
  return;
}

////////////////////////////////////////////////////////////////////////////
// TermDraw (const CurPos start, const CurPos end)
//	Draw Region
////////////////////////////////////////////////////////////////////////////
inline int
TermView::TermDraw (const CurPos &start, const CurPos &end)
{
  int x1 = start.x;
  int y1 = start.y;
  int x2 = end.x;
  int y2 = end.y;
  /*
   * Send Draw Rectangle data to Draw Engine Thread.
   */
  SendDataToDrawEngine (x1, y1 + fTop / fFontHeight,
			x2, y2 + fTop / fFontHeight);
  
  return 0;
}

////////////////////////////////////////////////////////////////////////////
// TermDrawSelectedRegion (CurPos start, CurPos end)
//	Draw Region
////////////////////////////////////////////////////////////////////////////
int
TermView::TermDrawSelectedRegion (CurPos start, CurPos end)
{
  CurPos inPos;

  if (end < start) {
    inPos = start;
    start = end;
    end = inPos;
  }

  if (start.y == end.y) {
    SendDataToDrawEngine (start.x, start.y,
			  end.x, end.y);

  } else {
    SendDataToDrawEngine (start.x, start.y,
			  fTermColumns, start.y);
    
    if (end.y - start.y > 0) {
      SendDataToDrawEngine (0, start.y + 1,
			    fTermColumns, end.y - 1);
    }
    SendDataToDrawEngine (0, end.y,
			  end.x, end.y);
    
  }
    
  return 0;
}

////////////////////////////////////////////////////////////////////////////
// TermDrawRegion (CurPos start, CurPos end)
//	Draw Region
////////////////////////////////////////////////////////////////////////////
int
TermView::TermDrawRegion (CurPos start, CurPos end)
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
    SendDataToDrawEngine (start.x, start.y,
			  end.x, end.y);

  } else {
    SendDataToDrawEngine (start.x, start.y,
			  fTermColumns - 1, start.y);
    
    if (end.y - start.y > 0) {
      SendDataToDrawEngine (0, start.y + 1,
			    fTermColumns - 1, end.y - 1);
    }
    SendDataToDrawEngine (0, end.y,
			  end.x, end.y);
    
  }
    
  return 0;
}


////////////////////////////////////////////////////////////////////////////
// EraseBelow (void)
//	Erase cursor below.
////////////////////////////////////////////////////////////////////////////
void
TermView::EraseBelow (void)
{
  this->UpdateLine ();

  fTextBuffer->EraseBelow (fCurPos);
  this->TermDraw (fCurPos, CurPos(fTermColumns - 1, fCurPos.y));
  if (fCurPos.y != fTermRows - 1) {
    this->TermDraw (CurPos(0, fCurPos.y + 1), CurPos(fTermColumns - 1, fTermRows - 1));
  }
  return;
}

////////////////////////////////////////////////////////////////////////////
// DeleteChar (int num)
//	Delete num Chars from Current Position.
////////////////////////////////////////////////////////////////////////////
void
TermView::DeleteChar (int num)
{
  this->UpdateLine ();

  fTextBuffer->DeleteChar(fCurPos, num);
  this->TermDraw (fCurPos, CurPos(fTermColumns - 1, fCurPos.y));

  return;
}
////////////////////////////////////////////////////////////////////////////
// DeleteColumn (void)
//	Delete cursor right charactors.
////////////////////////////////////////////////////////////////////////////
void
TermView::DeleteColumns (void)
{
  this->UpdateLine ();

  fTextBuffer->DeleteChar (fCurPos, fTermColumns - fCurPos.x);
  this->TermDraw (fCurPos, CurPos(fTermColumns - 1, fCurPos.y));

  return;
}
////////////////////////////////////////////////////////////////////////////
// DeleteLine (int num)
//	Delete num Lines from Current Position with scrolling. 
////////////////////////////////////////////////////////////////////////////
void
TermView::DeleteLine (int num)
{
  this->ScrollRegion (fCurPos.y, -1, SCRUP, num);
}

/*
 *  Cursor position get and set functions.
 */
////////////////////////////////////////////////////////////////////////////
// SetCurPos (int x, int y)
//	Sets Cursor to (X, Y)
////////////////////////////////////////////////////////////////////////////
void
TermView::SetCurPos (int x, int y)
{
  this->UpdateLine ();

  if (x >= 0 && x < fTermColumns)
    fCurPos.x = x;
  if (y >= 0 && y < fTermRows)
    fCurPos.y = y;

  return;
}

////////////////////////////////////////////////////////////////////////////
// SetCurX (int x)
//	Sets Cursor X Coordinate.
////////////////////////////////////////////////////////////////////////////
void
TermView::SetCurX (int x)
{
  if (x >= 0 && x < fTermRows) {
    this->UpdateLine ();
    fCurPos.x = x;
  }
  return;
}
////////////////////////////////////////////////////////////////////////////
// SetCurY (int y)
//	Sets Cursor Y Coordinate.
////////////////////////////////////////////////////////////////////////////
void
TermView::SetCurY (int y)
{
  if (y >= 0 && y < fTermColumns) {
    this->UpdateLine ();
    fCurPos.y = y;
  }
  return;
}

////////////////////////////////////////////////////////////////////////////
// GetCurPos (CurPos *)
//	Get cursor position.
////////////////////////////////////////////////////////////////////////////
void
TermView::GetCurPos (CurPos *inCurPos)
{
  inCurPos->x = fCurPos.x;
  inCurPos->y = fCurPos.y;
}
////////////////////////////////////////////////////////////////////////////
// GetCurX (void)
//	Get cursor X position.
////////////////////////////////////////////////////////////////////////////
int
TermView::GetCurX (void)
{
  return fCurPos.x;
}
////////////////////////////////////////////////////////////////////////////
// GetCurY (void)
//	Get cursor Y position.
////////////////////////////////////////////////////////////////////////////
int
TermView::GetCurY (void)
{
  return fCurPos.y;
}
////////////////////////////////////////////////////////////////////////////
// SaveCursor (void)
//	Save cursor position.
////////////////////////////////////////////////////////////////////////////
void
TermView::SaveCursor (void)
{
  fCurStack = fCurPos;
  return;
}
////////////////////////////////////////////////////////////////////////////
// RestoreCursor (void)
//	Restore cursor position.
////////////////////////////////////////////////////////////////////////////
void
TermView::RestoreCursor (void)
{
  this->UpdateLine ();
  fCurPos = fCurStack;

  return;
}


/*
 *  Cursor Movement.
 */
////////////////////////////////////////////////////////////////////////////
// MoveCurRight (int num)
//	Move cursor to right by num steps.
////////////////////////////////////////////////////////////////////////////
void
TermView::MoveCurRight (int num)
{

  this->UpdateLine ();

  if (fCurPos.x + num >= fTermColumns) {
    /* Wrap around */
    fCurPos.x = 0;
    PutCR ();
    PutLF ();
  } else {
    fCurPos.x += num;
  }

  return;
}
////////////////////////////////////////////////////////////////////////////
// MoveCurLeft (int num)
//	Move cursor to left by num steps.
////////////////////////////////////////////////////////////////////////////
void
TermView::MoveCurLeft (int num)
{

  this->UpdateLine ();
  
  fCurPos.x -= num;
  if (fCurPos.x < 0)
    fCurPos.x = 0;

  return;
}
////////////////////////////////////////////////////////////////////////////
// MoveCurUp (int num)
//	Move cursor up by num steps.
////////////////////////////////////////////////////////////////////////////
void
TermView::MoveCurUp (int num)
{

  this->UpdateLine ();
  
  fCurPos.y -= num;
  
  if (fCurPos.y < 0)
    fCurPos.y = 0;

  return;
}
////////////////////////////////////////////////////////////////////////////
// MoveCurDown (int num)
//	Move cursor down by num steps.
////////////////////////////////////////////////////////////////////////////
void
TermView::MoveCurDown (int num)
{

  this->UpdateLine ();

  fCurPos.y += num;
  
  if (fCurPos.y >= fTermRows) {
    fCurPos.y = fTermRows - 1;
  }

  return;
}

////////////////////////////////////////////////////////////////////////////
// DrawCursor (void)
//	Draw Cursor.
////////////////////////////////////////////////////////////////////////////
void
TermView::DrawCursor (void)
{
  CURSOR_RECT;
  uchar buf[4];
  ushort attr;
  int top = fTop / fFontHeight;
  int m_flag = false;

  m_flag = CheckSelectedRegion (CurPos(fCurPos.x,
				       fCurPos.y + fTop / fFontHeight));
  if (fTextBuffer->
      GetChar (fCurPos.y + top, fCurPos.x, buf, &attr) == A_CHAR) {

    int width;
    if (IS_WIDTH(attr))
      width = 2;
    else
      width = 1;
    
    DrawLines (fCurPos.x * fFontWidth,
	       fCurPos.y * fFontHeight + fTop,
	       attr, buf, width, m_flag, true, this);
  } else {
    if (m_flag)
      SetHighColor (fSelectBackColor);
    else
      SetHighColor (fCursorBackColor);
    
    FillRect (r);
  }

  Sync();

  return;
}
////////////////////////////////////////////////////////////////////////////
// BlinkCursor (void)
//	Cursor Blinking
////////////////////////////////////////////////////////////////////////////
void
TermView::BlinkCursor (void)
{
  if (fCursorDrawFlag == CURON &&
      fCursorBlinkingFlag == CURON &&
      Window()->IsActive()){
    if (fCursorStatus == CURON) {
	  this->TermDraw (fCurPos, fCurPos);
	}
	else {
	  DrawCursor();
	}
    fCursorStatus = (fCursorStatus == CURON) ? CUROFF : CURON;
  }
}

////////////////////////////////////////////////////////////////////////////
// SetCurDraw (bool flag)
//	Draw / Clear cursor.
////////////////////////////////////////////////////////////////////////////
void
TermView::SetCurDraw (bool flag)
{

  if (flag == CUROFF) {
    if (fCursorStatus == CURON) {
      this->TermDraw (fCurPos, fCurPos);
    }
    fCursorStatus = CUROFF;
    fCursorDrawFlag = CUROFF;
  }
  else {
    if (fCursorDrawFlag == CUROFF) {
      fCursorDrawFlag = CURON;
      fCursorStatus = CURON;

      if (this -> LockLooper ()) {
        DrawCursor ();
        this -> UnlockLooper ();
      }
    }
  }
  
  return;
  
}
////////////////////////////////////////////////////////////////////////////
// SetCurBlinking (bool flag)
//	Sets cursor Blinking flag.
////////////////////////////////////////////////////////////////////////////
void
TermView::SetCurBlinking (bool flag)
{
  fCursorBlinkingFlag = flag;
}

/*
 *  Scroll screen.
 */
////////////////////////////////////////////////////////////////////////////
// ScrollRegion ( top, bot, dir, num)
//	Scroll terminal dir directory by num of steps.
////////////////////////////////////////////////////////////////////////////
void
TermView::ScrollRegion (int top, int bot, int dir, int num)
{

  this->UpdateLine ();

  if (top == -1)
    top = fScrTop;

  if (bot == -1)
    bot = fScrBot;

  if (top < fScrTop)
    top = fScrTop;

  if (bot > fScrBot)
    bot = fScrBot; 

 
  fTextBuffer->ScrollRegion(top, bot , dir ,num);
  this->TermDraw (CurPos(0, top), CurPos(fTermColumns - 1, bot));
  return;
  
}
////////////////////////////////////////////////////////////////////////////
// SetScrollRegion (top, bot)
//	Sets terminal scroll region.
////////////////////////////////////////////////////////////////////////////
void
TermView::SetScrollRegion (int top, int bot)
{
  if (top >= 0 && top < fTermRows) {
    if (bot >= 0 && bot < fTermRows) {
      if (top > bot) {
        fScrTop = bot;
        fScrBot = top;
      }
      else if ( top < bot ) {
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
////////////////////////////////////////////////////////////////////////////
// ScrollAtCursor (void)
//	Scroll to cursor position.
////////////////////////////////////////////////////////////////////////////
void
TermView::ScrollAtCursor (void)
{
  if (this->LockLooper()) {
    this->ResizeScrBarRange ();
    fScrollUpCount = 0;
    ScrollTo (0, fTop);
    this->UnlockLooper();
  }
  
  return;
}

/************************************************************************
 *
 * private member functions.
 *
 ***********************************************************************/

////////////////////////////////////////////////////////////////////////////
// thread_id InitViewThread (void)
// 	Init View Thread.
////////////////////////////////////////////////////////////////////////////
thread_id
TermView::InitViewThread (void)
{

  /*
   * spwan Draw Engine thread.
   */
  if (fViewThread < 0) {
    fViewThread = spawn_thread (ViewThread,
				"DrawEngine",
				B_DISPLAY_PRIORITY,
				this);
  }
  else {
    return B_BAD_THREAD_ID;
  }
  fDrawRectSem = create_sem (0, "draw_engine_sem");
  resume_thread (fViewThread);


  /*
   * spwan Mouse Tracking thread.
   */
  if (fMouseThread < 0) {
    fMouseThread = spawn_thread (MouseTracking,
				 "MouseTracking",
				 B_NORMAL_PRIORITY,
				 this);
  }
  else {
    return B_BAD_THREAD_ID;
  }
  resume_thread (fMouseThread);
  
  return (fViewThread);
}


////////////////////////////////////////////////////////////////////////////
// static int32 ViewThread (void *)
// 	Thread of Draw Character to View.
////////////////////////////////////////////////////////////////////////////

int32
TermView::ViewThread (void *data)
{
  int width, height;
  sDrawRect pos;

//#define INVALIDATE
#ifndef INVALIDATE
  int i, j, count;
  int m_flag;
  ushort attr;
  uchar buf[256];
  BRect eraseRect;
#endif

  int inDrawRect_p = 0;
  
  TermView *theObj = (TermView *)data;

  while (theObj->fQuitting) {

    // Wait semaphore
    acquire_sem (theObj->fDrawRectSem);

    pos = theObj->fDrawRectBuffer[inDrawRect_p];
    inDrawRect_p++;
    inDrawRect_p %= RECT_BUF_SIZE;

    width = theObj->fFontWidth;
    height = theObj->fFontHeight;

#ifdef INVALIDATE
    BRect r(pos.x1 * width, pos.y1 * height,
	    (pos.x2 + 1) * width -1, (pos.y2 + 1) * height -1);
    if (theObj->LockLooper()) {
      theObj->Invalidate (r);
      theObj->UnlockLooper();
    }

#else
  
    if (theObj->LockLooper()) {

      for (j = pos.y1; j <= pos.y2; j++) {
	for (i = pos.x1; i <= pos.x2;) {
	  count = theObj->fTextBuffer->GetString (j, i, pos.x2, buf, &attr);
	  m_flag = theObj->CheckSelectedRegion (CurPos(i, j));
	  
	  if (count < 0) {
	    eraseRect.Set (width  * i,
			   height * j,
			   width  * (i - count) - 1,
			   height * (j + 1) - 1);
	    if (m_flag) {
	      theObj->SetHighColor (theObj->fSelectBackColor);
	    } else {
	      theObj->SetHighColor (theObj->fTextBackColor);
	    }
	    
	    theObj->FillRect (eraseRect);
	    count = -count;
	  }
	  else {
	    theObj->DrawLines ( width * i, height * j,
				attr, buf, count, m_flag, false, theObj);
	  }
	  i += count;
	}
      }
      theObj->UnlockLooper();
    }
#endif
  }

  exit_thread (B_OK);
  return 0;
}

////////////////////////////////////////////////////////////////////////////
// static int32 MouseTracking (void *)
// 	Thread of Tracking Mouse.
////////////////////////////////////////////////////////////////////////////

int32
TermView::MouseTracking (void *data)
{
  int32 code, selected = false;
  uint32 button;
  thread_id sender;
  CurPos stpos, edpos;
  BPoint stpoint, edpoint;
  float scr_start, scr_end, scr_pos;

  TermView *theObj = (TermView *)data;

    
  while (theObj->fQuitting) {
  if (1) {
#ifdef CHANGE_CURSOR_IMAGE    
    if (!has_data (find_thread (NULL))) {
      BRect r;
      
      if (theObj->fSelected &&
	  ( gTermPref->getInt32 (PREF_DRAGN_COPY) ||
	    modifiers() & B_CONTROL_KEY)) {
	if (theObj->LockLooper ()) {
	  theObj->GetMouse (&stpoint, &button);
	  r = theObj->Bounds();
	  theObj->UnlockLooper();
	}
	if (r.Contains (stpoint)) {
	  CurPos tmppos = theObj->BPointToCurPos (stpoint);
	  if (theObj->fSelStart > theObj->fSelEnd) {
	    stpos = theObj->fSelEnd;
	    edpos = theObj->fSelStart;
	  } else {
	    stpos = theObj->fSelStart;
	    edpos = theObj->fSelEnd;
	  }
	  if (tmppos > stpos && tmppos < edpos) {
	    be_app->SetCursor (M_ADD_CURSOR);
	  }
	  else {
	    be_app->SetCursor (B_HAND_CURSOR);
	  }
	}
      }
      snooze (50 * 1000);
      continue;
    }
    else {
#endif
      code = receive_data (&sender, (void *)&stpoint, sizeof(BPoint));
    }
    
    if (code != MOUSE_THR_CODE) continue;

    selected = theObj->fSelected;
    edpoint.Set (-1, -1);

    stpos = theObj->BPointToCurPos (stpoint);
    
    do {
      snooze (40 * 1000);
      if (theObj->LockLooper()) {
	theObj->GetMouse (&edpoint, &button);
	theObj->UnlockLooper();
      }

      edpos = theObj->BPointToCurPos (edpoint);

      if (stpoint == edpoint) {
	continue;
      } else {
	if (!selected) {
	  theObj->Select (stpos, edpos);
	  selected = true;
	} else {
	  /*
	   * Align cursor point to text.
	   */
	  if (stpos == edpos) continue;

	  if (edpos > stpos) {
	    edpoint.x -= theObj->fFontWidth / 2;
	    edpos = theObj->BPointToCurPos (edpoint);
	    //edpos.x--;
	    if (edpos.x < 0) edpos.x = 0;
	  }
	  else if (edpos < stpos) {
	    edpoint.x += theObj->fFontWidth / 2;
	    edpos = theObj->BPointToCurPos (edpoint);
	    //edpos.x++;
	    if (edpos.x > theObj->fTermColumns) edpos.x = theObj->fTermColumns;
	  }

	  /*
	   * Scroll check
	   */
	  if (theObj->LockLooper()) {

	    /* Get now scroll point */
	    theObj->fScrollBar->GetRange (&scr_start, &scr_end);
	    scr_pos = theObj->fScrollBar->Value();
	    
	    if (edpoint.y < theObj->Bounds().LeftTop().y )
	      /* mouse point left of window */
	      if (scr_pos != scr_start)
		theObj->ScrollTo (0, edpoint.y);

	    if (edpoint.y > theObj->Bounds().LeftBottom().y) {
	      /* mouse point left of window */
	      if (scr_pos != scr_end)
		theObj->ScrollTo (0, edpoint.y);
	    }
	    theObj->UnlockLooper();
	  }
	  theObj->ResizeSelectRegion (edpos);
	}
      }
    } while (button);
    theObj->fMouseTracking = false;
  }
  
  exit_thread (B_OK);
  return 0;
}

////////////////////////////////////////////////////////////////////////////
// void DrawLines (int, int, ushort, uchar *, int, int, int, BView *)
// 	Draw Character on offscreen bitmap.
////////////////////////////////////////////////////////////////////////////
void
TermView::DrawLines (int x1, int y1,
		     ushort attr, uchar *buf,
		     int width, int mouse, int cursor,
		     BView *inView)
{
  //  int width;
  int x2, y2;
  int forecolor, backcolor;
  rgb_color rgb_fore = fTextForeColor, rgb_back = fTextBackColor, rgb_tmp;

  /*
   * Set Font.
   */
  if (IS_WIDTH(attr)) {
    inView->SetFont (&fFullFont);
  } else { 
    inView->SetFont (&fHalfFont);
  }

  /*
   * Set pen point.
   */
  x2 = x1 + fFontWidth * width;
  y2 = y1 + fFontHeight;
	
  /*
   * color attribute
   */
  forecolor = IS_FORECOLOR(attr);
  backcolor = IS_BACKCOLOR(attr);
  
  if (IS_FORESET (attr)) {
    rgb_fore = gTermColorTable[forecolor];
  }
  if (IS_BACKSET (attr)) {
    rgb_back = gTermColorTable[backcolor];
  }

  /*
   * Selection check.
   */
  if (cursor) {
    rgb_fore = fCursorForeColor;
    rgb_back = fCursorBackColor;
  }
  else if (mouse){
    rgb_fore = fSelectForeColor;
    rgb_back = fSelectBackColor;

  } else {
    /*
     * Reverse attribute (If selected area, don't reverse color).
     */
    if (IS_INVERSE (attr)) {
      rgb_tmp = rgb_fore;
      rgb_fore = rgb_back;
      rgb_back = rgb_tmp;
    }
  }
  
  /*
   * Fill color at Background color and set low color.
   */
  inView->SetHighColor (rgb_back);
  inView->FillRect (BRect (x1, y1, x2 - 1, y2 - 1));
  inView->SetLowColor (rgb_back);

  inView->SetHighColor (rgb_fore);

  /*
   * Draw character.
   */
  inView->MovePenTo (x1, y1 + fFontAscent);
  inView->DrawString ((char *) buf);
  
  /*
   * bold attribute.
   */
  if (IS_BOLD (attr)) {
    inView->MovePenTo (x1 + 1, y1 + fFontAscent);
	  
    inView->SetDrawingMode (B_OP_OVER);
    inView->DrawString ((char *)buf);
    inView->SetDrawingMode (B_OP_COPY);
  }
	
  /*
   * underline attribute
   */
  if (IS_UNDER (attr)) {
    inView->MovePenTo (x1, y1 + fFontAscent);
    inView->StrokeLine (BPoint (x1 , y1 + fFontAscent),
			BPoint (x2 , y1 + fFontAscent));
  }

  return;
  
}

////////////////////////////////////////////////////////////////////////////
// ResizeScrBarRange (void)
//	Resize scroll bar range and knob size.
////////////////////////////////////////////////////////////////////////////
void
TermView::ResizeScrBarRange (void)
{
  float viewheight, start_pos; 

  viewheight = fTermRows * fFontHeight;
  start_pos = fTop - (fScrBufSize - fTermRows *2) * fFontHeight;

  if (start_pos > 0) {
    fScrollBar->SetRange (start_pos, viewheight + fTop - fFontHeight);
  }
  else {
    fScrollBar->SetRange(0, viewheight + fTop - fFontHeight);
    fScrollBar->SetProportion ( viewheight / (viewheight + fTop));
  }
}

////////////////////////////////////////////////////////////////////////////
// ScrollScreen (void)
//	Scrolls screen.
////////////////////////////////////////////////////////////////////////////
void
TermView::ScrollScreen (void)
{
  fTop += fFontHeight;
  fScrollUpCount++;
  fTextBuffer->ScrollLine ();

  if (fScrollUpCount > fTermRows ) {
    if (this->LockLooper()) {
      this->ResizeScrBarRange ();
      fScrollBarRange += fScrollUpCount;
      fScrollUpCount = 0;
      ScrollTo (0, fTop);
      this->UnlockLooper();
    }
  }

  return;
}

////////////////////////////////////////////////////////////////////////////
// ScrollScreenDraw (void)
//	Scrolls screen.
////////////////////////////////////////////////////////////////////////////
void
TermView::ScrollScreenDraw (void)
{
  if (fScrollUpCount){
    if (this->LockLooper()) {
      this->ResizeScrBarRange ();
    
      fScrollBarRange += fScrollUpCount;
      fScrollUpCount = 0;
      ScrollTo (0, fTop);
      this->UnlockLooper();
    }
  }
}

////////////////////////////////////////////////////////////////////////////
// UpdateSIGWINCH (void)
// 	SIGWINCH Handler.
////////////////////////////////////////////////////////////////////////////
void
TermView::UpdateSIGWINCH()
{
  struct winsize ws;
  
  if (fFrameResized) {
    if (fSelected)
      this->TermDrawSelectedRegion (fSelStart, fSelEnd);
    ScrollTo (0, fTop);
    this->ResizeScrBarRange ();

    ws.ws_row = fTermRows;
    ws.ws_col = fTermColumns;
    ioctl (pfd, TIOCSWINSZ, &ws);
    kill (-sh_pid, SIGWINCH);

    fFrameResized = 0;
    if (fScrRegionSet == 0) {
      fScrBot = fTermRows - 1;
    }
  }
  
}
  
////////////////////////////////////////////////////////////////////////////
// DeviceStatusReport (int)
// 	Device Status.
////////////////////////////////////////////////////////////////////////////
// Q & D hack by Y.Hayakawa (hida@sawada.riec.tohoku.ac.jp)
// 21-JUL-99
void
TermView::DeviceStatusReport(int n)
{
  char sbuf[16] ;
  int len;
  
  switch (n) {
  case 5:
    len = sprintf(sbuf,"\033[0n") ;
    write (pfd, sbuf, len);
    break ;
  case 6:
    len = sprintf(sbuf,"\033[%d;%dR",fTermRows,fTermColumns) ;
    write (pfd, sbuf, len);
    break ;
  default:
    return;
  }
  return ;
}

////////////////////////////////////////////////////////////////////////////
// UpdateLine (void)
// 	Update line buffer.
////////////////////////////////////////////////////////////////////////////
void
TermView::UpdateLine (void)
{

  if (fUpdateFlag == true) {
    if (fInsertModeFlag == MODE_INSERT) {
      this->TermDraw (CurPos(fBufferStartPos, fCurPos.y),
		      CurPos(fTermColumns - 1, fCurPos.y));
    } else {
      this->TermDraw (CurPos(fBufferStartPos, fCurPos.y),
		      CurPos(fCurPos.x - 1, fCurPos.y));
    }
    fUpdateFlag = false;
  }
  
  return;
}

/*
 * Private Member Functions (hook functions).
 */
////////////////////////////////////////////////////////////////////////////
// AttachedToWindow
//
////////////////////////////////////////////////////////////////////////////
void
TermView::AttachedToWindow (void)
{
  this->SetFont (&fHalfFont);
  this->MakeFocus(true);
  fScrollBar->SetSteps (fFontHeight, fFontHeight * fTermRows);

}
/////////////////////////////////////////////////////////////////////////////
// Draw
//	updates view.
/////////////////////////////////////////////////////////////////////////////
void
TermView::Draw (BRect updateRect)
{

  if (IsPrinting()){
    DoPrint(updateRect);
    return;
  }
  
  int x1, x2, y1, y2;
  int i, j, k, count;
  ushort attr;
  uchar buf[256];
  int m_flag;
  BRect eraseRect;
  
  x1 = (int)updateRect.left / fFontWidth;
  x2 = (int)updateRect.right / fFontWidth;

  y1 = (int)updateRect.top / fFontHeight;
  y2 = (int)updateRect.bottom / fFontHeight;

  Window()->BeginViewTransaction();
  
  for (j = y1; j <= y2; j++) {
    /*
     * If (x1, y1) Buffer is in string full width character,
     * allinment start position.
     */
    k = x1;
    if (fTextBuffer->GetChar (j, k, buf, &attr) == IN_STRING) {
      k--;
    }
    if (k < 0) k = 0;

    for (i = k; i <= x2;) {

      count = fTextBuffer->GetString (j, i, x2, buf, &attr);

      m_flag = CheckSelectedRegion (CurPos(i, j));
      
      if (count < 0) {
	if (m_flag) {
	  
	  eraseRect.Set (fFontWidth * i,
			 fFontHeight * j,
			 fFontWidth * (i - count) -1,
			 fFontHeight * (j + 1) -1);

	  SetHighColor (fSelectBackColor);
	  FillRect (eraseRect);
	}
        i += abs (count);
        continue;
      }
      
      DrawLines (fFontWidth * i, fFontHeight * j,
                 attr, buf, count, m_flag, false, this);
      i += count;
      if (i >= fTermColumns) break;
      
    }
  }

  if (fCursorStatus == CURON) {
    DrawCursor();
  }
  Window()->EndViewTransaction();


  return;
}
/////////////////////////////////////////////////////////////////////////////
// Draw view on printer.
//	
/////////////////////////////////////////////////////////////////////////////
void 
TermView::DoPrint(BRect updateRect)
{
#if 1
  ushort attr;
  uchar buf[256];
 
  const int numLines = (int)((updateRect.Height()) / fFontHeight);

  int y1 = (int)updateRect.top / fFontHeight;
  y1 = y1 - (fScrBufSize - numLines * 2);
  if(y1 < 0) y1 = 0;

  const int y2 = y1 + numLines -1;

  const int x1 = (int)updateRect.left / fFontWidth;
  const int x2 = (int)updateRect.right / fFontWidth;

  for (int j = y1; j <= y2; j++) {
    /*
     * If (x1, y1) Buffer is in string full width character,
     * allinment start position.
     */
    int k = x1;
    if (fTextBuffer->GetChar (j, k, buf, &attr) == IN_STRING) {
      k--;
    }
    if (k < 0) k = 0;

    for (int i = k; i <= x2;) {

      int count = fTextBuffer->GetString (j, i, x2, buf, &attr);
      if (count < 0) {
        i += abs (count);
        continue;
      }
            
      DrawLines (fFontWidth * i, fFontHeight * j,
                 attr, buf, count, false, false, this);
      i += count;
    }
  }

#endif
} 
//////////////////////////////////////////////////////////////////////////////
// WindowActivated
//	
//////////////////////////////////////////////////////////////////////////////
void
TermView::WindowActivated (bool active)
{
  if (active == false) {
 //   DoIMConfirm ();
  }
  if (active && fMouseImage) {
    be_app->SetCursor (B_I_BEAM_CURSOR);
  }

}


//////////////////////////////////////////////////////////////////////////////
// KeyDown
//	Key down event.
//////////////////////////////////////////////////////////////////////////////
void
TermView::KeyDown (const char *bytes, int32 numBytes)
{
  char c;
  struct termios tio;
  int32 key, mod;

  uchar dstbuf[1024];
  this->Looper()->CurrentMessage()->FindInt32("modifiers", &mod);
  this->Looper()->CurrentMessage()->FindInt32("key", &key);

  if (fIMflag)
    return;
  
  /*
   * If bytes[0] equal intr charactor,
   * send signal to shell process group.
   */
  tcgetattr (pfd, &tio);
  if (*bytes == tio.c_cc[VINTR]) {
    if (tio.c_lflag & ISIG) {
      kill (-sh_pid, SIGINT);
    }
  }

  /*
   * MuTerminal changes RET, ENTER, F1...F12, and ARROW key code.
   */

  if (numBytes == 1) {
    if (mod & B_OPTION_KEY) {
      c = *bytes;
      c  |= 0x80;
      write (pfd, &c, 1);
      return;
    }
    
    switch (*bytes) {
    case B_RETURN:
      c = 0x0d;
      if (key == RETURN_KEY || key == ENTER_KEY) {
        write (pfd, &c, 1);
        return;
      }
      else {
      	write (pfd, bytes, numBytes);
	return;
      }

    case B_LEFT_ARROW:
      if (key == LEFT_ARROW_KEY) {
        write (pfd, LEFT_ARROW_KEY_CODE, sizeof(LEFT_ARROW_KEY_CODE)-1);
        return;
      }
      break;
	    
    case B_RIGHT_ARROW:
      if (key == RIGHT_ARROW_KEY) {
        write (pfd, RIGHT_ARROW_KEY_CODE, sizeof(RIGHT_ARROW_KEY_CODE)-1);
        return;
      }
      break;

    case B_UP_ARROW:
      if (mod & B_SHIFT_KEY) {
        if (Bounds().top <= 0)
          return;
        ScrollBy (0, -fFontHeight);
        Window()->UpdateIfNeeded();
        return;
      }
      
      if (key == UP_ARROW_KEY) {
        write (pfd, UP_ARROW_KEY_CODE, sizeof(UP_ARROW_KEY_CODE)-1);
        return;
      }
      break;

    case B_DOWN_ARROW:
      if (mod & B_SHIFT_KEY) {
        ScrollBy (0, fFontHeight);
        Window()->UpdateIfNeeded();
        return;
      }
      
      if (key == DOWN_ARROW_KEY) {
        write (pfd, DOWN_ARROW_KEY_CODE, sizeof(DOWN_ARROW_KEY_CODE)-1);
        return;
      }
      break;

    case B_INSERT:
      if (key == INSERT_KEY) {
        write (pfd, INSERT_KEY_CODE, sizeof(INSERT_KEY_CODE)-1);
        return;
      }

    case B_HOME:
      if (key == HOME_KEY) {
        write (pfd, HOME_KEY_CODE, sizeof(HOME_KEY_CODE)-1);
        return;
      }

    case B_PAGE_UP:
      if (mod & B_SHIFT_KEY) {
        if (Bounds().top <= 0)
          return;
        ScrollBy (0, -fFontHeight * fTermRows );
        Window()->UpdateIfNeeded();
        return;
      }
      
      if (key == PAGE_UP_KEY) {
        write (pfd, PAGE_UP_KEY_CODE, sizeof(PAGE_UP_KEY_CODE)-1);
        return;
      }

    case B_PAGE_DOWN:
      if (mod & B_SHIFT_KEY) {
        ScrollBy (0, fFontHeight * fTermRows);
        Window()->UpdateIfNeeded();
        return;
      }
      
      if (key == PAGE_DOWN_KEY) {
        write (pfd, PAGE_DOWN_KEY_CODE, sizeof(PAGE_DOWN_KEY_CODE)-1);
        return;
      }
      
    case B_END:
      if (key == END_KEY) {
        write (pfd, END_KEY_CODE, sizeof(END_KEY_CODE)-1);
        return;
      }
      
      
    case B_FUNCTION_KEY:
      for (c = 0; c < 12; c++) {
        if (key == function_keycode_table[c]) {
          write (pfd, function_key_char_table[c], 5);
          return;
        }
      }
      break;

    default:
      break;
    }
  }
  else {
    /* input multibyte character */
    
    if (gNowCoding != M_UTF8) {
      int cnum = fCodeConv->ConvertFromInternal
	((uchar *)bytes, dstbuf, gNowCoding);
      write (pfd, dstbuf, cnum);
      return;
    }
  }
    

  write (pfd, bytes, numBytes);
  return;
  
}

/////////////////////////////////////////////////////////////////////////////
// FrameResized
//
/////////////////////////////////////////////////////////////////////////////

void
TermView::FrameResized (float width, float height)
{

  const int cols = ((int)width + 1) / fFontWidth;
  const int rows = ((int)height + 1) / fFontHeight;

  int offset = 0;
  
  if (rows < fCurPos.y + 1) {
    fTop += (fCurPos.y  + 1 - rows) * fFontHeight;
    offset = fCurPos.y + 1 - rows;
    fCurPos.y = rows - 1;
  }
  fTextBuffer->ResizeTo (rows, cols, offset);
  fTermRows = rows;
  fTermColumns = cols;

  fFrameResized = 1;

  return;
  
}

////////////////////////////////////////////////////////////////////////////
// MessageReceived
//
////////////////////////////////////////////////////////////////////////////
void 
TermView::MessageReceived(BMessage *msg)
{
  entry_ref ref;
  char *ctrl_l = "";

  switch(msg->what){

  case B_SIMPLE_DATA:
    if (msg->FindRef("refs", &ref) == B_OK ){
      this->DoFileDrop(ref);
    } else {
      BView::MessageReceived(msg);
    }
    break;

  case B_MIME_DATA:
    char *text;
    int32 num_bytes;
    status_t sts;
    
    if (msg->WasDropped()){
      sts = msg->FindData ((const char *)"text/plain",
		     (type_code)B_MIME_TYPE,
		     (const void **)&text,
		     &num_bytes);

      if (sts != B_OK) break;
      
      /* Clipboard text doesn't attached EOF? */
      text[num_bytes] = '\0';
      WritePTY ((uchar *)text, num_bytes);
      
    }
    break;
    
    
  case B_COPY:
    this->DoCopy();
    break;
    
  case B_PASTE:
    int32 code;
    if (msg->FindInt32 ("index", &code) == B_OK)
      this->DoPaste();
    break;
    
  case B_SELECT_ALL:
    this->DoSelectAll();
    break;
    
  case MENU_CLEAR_ALL:
    this->DoClearAll();
    write (pfd, ctrl_l, 1);
    break;

  case MSGRUN_CURSOR:
    this->BlinkCursor ();
    break;

//  case B_INPUT_METHOD_EVENT:
//    {
   //   int32 op;
  //    msg->FindInt32("be:opcode", &op);
   //   switch(op){ 
   //   case B_INPUT_METHOD_STARTED:
	//this->DoIMStart(msg);
//	break;

//      case B_INPUT_METHOD_STOPPED:
//	this->DoIMStop(msg);
//	break;

//      case B_INPUT_METHOD_CHANGED:
//	this->DoIMChange(msg);
//	break;

//      case B_INPUT_METHOD_LOCATION_REQUEST:
//	this->DoIMLocation(msg);
//	break;
    //  }
   // }

  default:
    BView::MessageReceived(msg);
    break;
  }
}  
////////////////////////////////////////////////////////////////////////////
// DoFileDrop
//	Gets dropped file full path and display it at cursor position.
////////////////////////////////////////////////////////////////////////////
void 
TermView::DoFileDrop(entry_ref &ref)
{
  char p[B_PATH_NAME_LENGTH * 2 + 1];
  
  BEntry ent(&ref); 
  BPath path(&ent);

  int num_bytes = SubstMetaChar (path.Path(), p);

  WritePTY ((uchar *)p, num_bytes);

  return;
  
}
////////////////////////////////////////////////////////////////////////////
// DoCopy()
//	Copy selection text to Clipboard.
////////////////////////////////////////////////////////////////////////////
void 
TermView::DoCopy()
{
  if(!fSelected) return;
  
  BString copyStr(""); 
  
  fTextBuffer->GetStringFromRegion (copyStr);
   
  if (be_clipboard->Lock()) { 
    BMessage *clipMsg = NULL; 
    be_clipboard->Clear(); 
    if ((clipMsg = be_clipboard->Data()) != NULL) { 
      clipMsg->AddData("text/plain", B_MIME_TYPE, copyStr.String(), copyStr.Length()); 
      be_clipboard->Commit(); 
    } 
    be_clipboard->Unlock(); 
  }

  if (!fMouseTracking) {
    this->DeSelect ();
  }
  
}
////////////////////////////////////////////////////////////////////////////
// DoPaste()
//	Inserts text of Clipboard at cursor position.
////////////////////////////////////////////////////////////////////////////
void 
TermView::DoPaste(void)
{
  if ( be_clipboard->Lock() ) { 
    BMessage *clipMsg = be_clipboard->Data(); 
    char *text;
    ssize_t num_bytes;

    if ( clipMsg->FindData((const char *)"text/plain",
			   (type_code) B_MIME_TYPE,
			   (const void **)&text,
			   &num_bytes) == B_OK ) {
      
      /* Clipboard text doesn't attached EOF? */
      text[num_bytes] = '\0';
      
      WritePTY ((uchar *)text, num_bytes);
    }    
    
    be_clipboard->Unlock(); 
  }

}
////////////////////////////////////////////////////////////////////////////
// DoSelectAll()
//	Select all displayed text and text /in buffer.
////////////////////////////////////////////////////////////////////////////
void 
TermView::DoSelectAll(void)
{
  CurPos start, end;
  int screen_top;
  int viewheight, start_pos;

  screen_top = fTop / fFontHeight;
  viewheight = fTermRows;

  start_pos = screen_top - (fScrBufSize - viewheight * 2);
      
  start.x = 0;
  end.x = fTermColumns -1;

  if (start_pos > 0) {
    start.y = start_pos;
  }
  else {
    start.y = 0;
  }
  end.y = fCurPos.y  + screen_top;

  this->Select (start, end);
}
////////////////////////////////////////////////////////////////////////////
// DoClearAll()
//	Clear display and text buffer, then moves Cursorr at home position.
////////////////////////////////////////////////////////////////////////////
void 
TermView::DoClearAll(void)
{
  this->DeSelect ();
  fTextBuffer->ClearAll();

  fTop = 0;
  ScrollTo (0, 0);

  if (this -> LockLooper ()) {
    SetHighColor (fTextBackColor);
    
    FillRect (Bounds());
    SetHighColor (fTextForeColor);
    this -> UnlockLooper ();
  }
    
  // reset cursor pos
  this->SetCurPos (0, 0);
  
  // reset selection.
  fSelected = false;

  fScrollBar->SetRange(0, 0);
  fScrollBar->SetProportion (1);

}
////////////////////////////////////////////////////////////////////////////
// SubstMetaChar (char *, char *)
//	Substitution meta character.
////////////////////////////////////////////////////////////////////////////
int
TermView::SubstMetaChar (const char *p, char *q)
{
  char *metachar = " ~`#$&*()\\|[]{};'\"<>?!";
  int num_char = 0;
  
  while (*p) {
    char *mp = metachar;
    for (int i = 0; i < 22; i++){
      if (*p == *mp++) {
	*q++ = '\\';
	num_char++;
	break;
      }
    }
    *q++ = *p++;
    num_char++;
  }
  /* Add null string. */
  *q = *p;
  return (num_char + 1);
  
}

////////////////////////////////////////////////////////////////////////////
// WritePTY (const uchar *)
//	Write strings to PTY device. If coding system isn't UTF8, change
//	coding to UTF8 before writting PTY.
////////////////////////////////////////////////////////////////////////////
void
TermView::WritePTY (const uchar *text, int num_bytes)
{

  if (gNowCoding != M_UTF8) {
    uchar *dstbuf = (uchar *)malloc (num_bytes * 3);
    num_bytes = fCodeConv->ConvertFromInternal(text, dstbuf, gNowCoding);
    write (pfd, dstbuf, num_bytes);
    free (dstbuf);
  } else{
    write (pfd, text, num_bytes);
  }

}

////////////////////////////////////////////////////////////////////////////
// SetupPop()
//	Make encoding pop up menu (make copy of window's one.)
////////////////////////////////////////////////////////////////////////////
void
TermView::SetupPop (void)
{
  fPopMenu = new BPopUpMenu("");
  MakeEncodingMenu(fPopMenu, gNowCoding, true);
  fPopMenu->SetTargetForItems(Window());
}
////////////////////////////////////////////////////////////////////////////
// SetupMouseButton()
//	convert button name to number.
////////////////////////////////////////////////////////////////////////////
int32
TermView::SetupMouseButton (const char *bname)
{
  if (!strcmp (bname, "Disable")){
  	return 0;
  }else if (!strcmp (bname, "Button 1")){
    return B_PRIMARY_MOUSE_BUTTON;
  }else if (!strcmp (bname, "Button 2")){
    return B_SECONDARY_MOUSE_BUTTON;
  }else if (!strcmp (bname, "Button 3")){
    return B_TERTIARY_MOUSE_BUTTON;
  }else{
    return 0; //Disable
  }
}
////////////////////////////////////////////////////////////////////////////
// MouseDown()
//	Handle mouse button operation.
////////////////////////////////////////////////////////////////////////////
void
TermView::MouseDown (BPoint where)
{
  int32 buttons = 0, clicks = 0;
  int32 mod;
  BPoint inPoint;
  ssize_t num_bytes;
 
  Window()->CurrentMessage()->FindInt32("buttons", &buttons); 

  /*
   * paste button
   */
  if(buttons == mPasteMenuButton) {
    if (fSelected) {
      /* If selected region, copy text from region. */
      BString copyStr ("");
      
      fTextBuffer->GetStringFromRegion (copyStr);

      num_bytes = copyStr.Length ();
      WritePTY ((uchar *)copyStr.String (), num_bytes);
      
    }
    else {
      /* If don't selected, copy text from clipboard. */
      DoPaste ();
    }
    
    return;
  }
  
  /*
   * Select Region
   */
  if (buttons == mSelectButton) {

    Window()->CurrentMessage()->FindInt32 ("modifiers", &mod);
    Window()->CurrentMessage()->FindInt32 ("clicks", &clicks);
    
    if (fSelected) {
      CurPos inPos, stPos, edPos;
      if (fSelStart < fSelEnd) {
	stPos = fSelStart;
	edPos = fSelEnd;
      }
      else {
	stPos = fSelEnd;
	edPos = fSelStart;
      }
      inPos = BPointToCurPos (where);

      /*
       * If mouse pointer is avove selected Region, start Drag'n Copy.
       */
      if (inPos > stPos && inPos < edPos) {
	if (mod & B_CONTROL_KEY || gTermPref->getInt32 (PREF_DRAGN_COPY)) {
	  
	  BPoint p;
	  uint32 bt;
	  
	  do {
	    GetMouse (&p, &bt);
	    if (bt == 0) {
	      this->DeSelect();
	      return;
	    }
	    
	    snooze (40 * 1000);
	  } while (abs((int)(where.x - p.x)) < 4
		   && abs((int)(where.y - p.y)) < 4);
	    

	  BString copyStr ("");
	  fTextBuffer->GetStringFromRegion (copyStr);
	
	  BMessage msg(B_MIME_TYPE);
	  msg.AddData ("text/plain",
		       B_MIME_TYPE,
		       copyStr.String(),
		       copyStr.Length());

	  BPoint st = CurPosToBPoint (stPos);
	  BPoint ed = CurPosToBPoint (edPos);
	  BRect r;

	  if (stPos.y == edPos.y) {
	    r.Set (st.x, st.y - fTop,
		   ed.x + fFontWidth, ed.y + fFontHeight - fTop);
	  }
	  else {
	    r.Set (0, st.y - fTop,
		   fTermColumns * fFontWidth, ed.y + fFontHeight - fTop);
	  }
	  r = r & Bounds();
	
	  DragMessage (&msg, r);
	  return;
	}
      }
    }

    /* If mouse has a lot of movement, disable double/triple
       click. */
    inPoint = fPreviousMousePoint - where;
    if (abs ((int)inPoint.x) > 16 || abs ((int)inPoint.y) > 16) clicks = 1;
    fPreviousMousePoint = where;
    
    if (mod & B_SHIFT_KEY) {
      AddSelectRegion (BPointToCurPos (where));
    } else {
      this->DeSelect ();
    }
    
    /* If clicks larger than 3, reset mouse click counter. */
    clicks = clicks % 3;
    if (clicks == 0) clicks = 3;
    
    switch (clicks){
    case 1:
      fMouseTracking = true;
      send_data (fMouseThread,
		 MOUSE_THR_CODE,
		 (void *)&where,
		 sizeof (BPoint));
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
  
  /*
   * Sub menu (coding popup menu)
   */
  if (buttons == mSubMenuButton){
    ConvertToScreen(&where); 
    this->SetupPop();
    fPopMenu->Go (where, true); 
    delete fPopMenu;
    return;
  }
  
  BView::MouseDown (where);
 
  return;
}

////////////////////////////////////////////////////////////////////////////
// MouseMoved()
//
////////////////////////////////////////////////////////////////////////////
void
TermView::MouseMoved (BPoint /*point*/, uint32 transit,
		      const BMessage */*message*/)
{
  if (fMouseImage && Window()->IsActive ()) {
    if (transit == B_ENTERED_VIEW)
      be_app->SetCursor (B_I_BEAM_CURSOR);
    if (transit == B_EXITED_VIEW)
      be_app->SetCursor (B_HAND_CURSOR);
  }
  return;
  
}


////////////////////////////////////////////////////////////////////////////
// Select
// Select a range of text
////////////////////////////////////////////////////////////////////////////
void
TermView::Select (CurPos start, CurPos end)
{

  uchar buf[4];
  ushort attr;

  if (start.x < 0)
    start.x = 0;
  if (end.x >= fTermColumns)
    end.x = fTermColumns - 1;
  
  if (fTextBuffer->GetChar (start.y, start.x, buf, &attr) == IN_STRING) {
    start.x--;
    if (start.x < 0) start.x = 0;
  }
  if (fTextBuffer->GetChar (end.y, end.x, buf, &attr) == IN_STRING) {
    end.x++;
    if (end.x >= fTermColumns) end.x = fTermColumns;
  }
  
  fSelStart = start;
  fSelEnd = end;

  fTextBuffer->Select(fSelStart, fSelEnd);
  this->TermDrawSelectedRegion (fSelStart, fSelEnd);
  fSelected = true;
}

////////////////////////////////////////////////////////////////////////////
// AddSelectRegion
//	Add select region (shift + mouse click)
////////////////////////////////////////////////////////////////////////////
void
TermView::AddSelectRegion (CurPos pos)
{
  uchar buf[4];
  ushort attr;
  CurPos start, end, inPos;

  if (!fSelected) return;

  /*
   * error check, and if mouse point to a plase full width character,
   * select point decliment.
   */ 
  if (pos.x >= fTermColumns)
    pos.x = fTermColumns - 1;
  else if (pos.x < 0)
    pos.x = 0;

  if (pos.y < 0)
    pos.y = 0;
  
  if (fTextBuffer->GetChar (pos.y, pos.x, buf, &attr) == IN_STRING) {
    pos.x++;
    if (pos.x >= fTermColumns) pos.x = fTermColumns - 1;
  }

  start = fSelStart;
  end = fSelEnd;

  /*
   * Mouse point is same as selected line.
   */
  if (pos.y == fSelStart.y && pos.y == fSelEnd.y) {
    if (abs (pos.x - start.x) > abs (pos.x - end.x)) {
      fSelStart = start;
      fSelEnd = pos;
      inPos = end;
    } else {
      fSelStart = end;
      fSelEnd = pos;
      inPos = start;
    }
  }
  /*
   * else, End point set to near the start or end point.
   */
  else if (abs (pos.y - start.y) > abs (pos.y - end.y)) {
    fSelStart = start;
    fSelEnd = pos;
    inPos = end;
  } else if (abs (pos.y - start.y) > abs (pos.y - end.y)) {
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
  
  fTextBuffer->Select (fSelStart, fSelEnd);
  this->TermDrawSelectedRegion (inPos, fSelEnd);

}

////////////////////////////////////////////////////////////////////////////
// ResizeSelectRegion
//	Resize select region (mouse drag)
////////////////////////////////////////////////////////////////////////////
void
TermView::ResizeSelectRegion (CurPos pos)
{
  CurPos inPos;
  uchar buf[4];
  ushort attr;
  
  inPos = fSelEnd;

  /*
   * error check, and if mouse point to a plase full width character,
   * select point decliment.
   */ 
  if (pos.x >= fTermColumns)
    pos.x = fTermColumns - 1;
  else if (pos.x < 0)
    pos.x = 0;

  if (pos.y < 0)
    pos.y = 0;
  
  if (fTextBuffer->GetChar (pos.y, pos.x, buf, &attr) == IN_STRING) {
    pos.x++;
    if (pos == inPos) return;
    if (pos.x >= fTermColumns) pos.x = fTermColumns - 1;
  }

  fSelEnd = pos;
    
  fTextBuffer->Select (fSelStart, pos);
  this->TermDrawSelectedRegion (inPos, pos);

}
  

////////////////////////////////////////////////////////////////////////////
// DeSelect
// DeSelect a range of text
////////////////////////////////////////////////////////////////////////////
void
TermView::DeSelect (void)
{
  CurPos start, end;
  
  if (!fSelected) return;
  
  fTextBuffer->DeSelect ();

  start = fSelStart;
  end = fSelEnd;
  
  fSelStart.Set (-1, -1);
  fSelEnd.Set (-1, -1);
  
  this->TermDrawSelectedRegion (start, end);
  
  fSelected = false;

}
////////////////////////////////////////////////////////////////////////////
// SelectWord
//	Hilite a word
////////////////////////////////////////////////////////////////////////////
void
TermView::SelectWord(BPoint where, int mod)
{
  CurPos start, end, pos;
  bool flag;

  pos = BPointToCurPos (where);
  flag = fTextBuffer->FindWord(pos, &start, &end);
  
  if (mod & B_SHIFT_KEY) {
    if (flag) {
      if (start < fSelStart) {
	AddSelectRegion (start);
      } else if (end > fSelEnd) {
	AddSelectRegion (end);
      }
    } else {
      AddSelectRegion (pos);
    }
  } else {
    this->DeSelect ();
    if (flag)
      this->Select (start, end);
  }
}

////////////////////////////////////////////////////////////////////////////
// SelectLine
// Hilite a Line
////////////////////////////////////////////////////////////////////////////
void
TermView::SelectLine(BPoint where, int mod)
{
  CurPos start, end, pos;

  pos = BPointToCurPos (where);
  
  if (mod & B_SHIFT_KEY) {
    start = CurPos(0, pos.y);
    end = CurPos (fTermColumns - 1, pos.y);
    if (start < fSelStart) {
      AddSelectRegion (start);
    } else if (end > fSelEnd) {
      AddSelectRegion (end);
    }
  }
  else {
    this->DeSelect ();
    this->Select (CurPos (0, pos.y), CurPos (fTermColumns - 1, pos.y));
  }
  
}
////////////////////////////////////////////////////////////////////////////
// BPointToCurPos
// Convert View visible area corrdination to cursor position.
////////////////////////////////////////////////////////////////////////////
CurPos
TermView::BPointToCurPos (const BPoint &p)
{

  return CurPos (p.x / fFontWidth,
                 p.y / fFontHeight);

}
////////////////////////////////////////////////////////////////////////////
// CurPosToBPoint
// Convert cursor position to view coordination.
////////////////////////////////////////////////////////////////////////////
BPoint
TermView::CurPosToBPoint (const CurPos &pos)
{
  return BPoint(fFontWidth * pos.x,
                pos.y * fFontHeight + fTop);
}
////////////////////////////////////////////////////////////////////////////
// CheckSelectedRegion
//
////////////////////////////////////////////////////////////////////////////
bool
TermView::CheckSelectedRegion (const CurPos &pos)
{

  CurPos start, end;

  if (fSelStart > fSelEnd) {
    start = fSelEnd;
    end = fSelStart;
  }
  else {
    start = fSelStart;
    end = fSelEnd;
  }
  
  if (pos >= start && pos <= end)
    return true;

  return false;
  
}
////////////////////////////////////////////////////////////////////////////
// GetFrameSize
//
////////////////////////////////////////////////////////////////////////////
void
TermView::GetFrameSize (float *width, float *height)
{
  if(width != NULL) *width = fTermColumns * fFontWidth;
  if(height == NULL) return;
  
  if (!fTop){
    *height = fTermRows * fFontHeight;
    return;
  }
  
  if (fTop - fTermRows * fFontHeight > fScrBufSize * fFontHeight){
    *height = fScrBufSize * fFontHeight;
    return;
  }
  
  *height = fTop + fTermRows * fFontHeight;
}

////////////////////////////////////////////////////////////////////////////
// GetFontInfo (int *, int*)
//	Sets terninal rows and cols.
////////////////////////////////////////////////////////////////////////////
void
TermView::GetFontInfo (int *width, int *height)
{
  *width = fFontWidth;
  *height = fFontHeight;
  return;
}

////////////////////////////////////////////////////////////////////////////
// SendDataToDrawEngine (int, int, int, int)
//	Send DrawRect data to Draw Engline thread.
////////////////////////////////////////////////////////////////////////////
inline void
TermView::SendDataToDrawEngine (int x1, int y1, int x2, int y2)
{

  sem_info info;
  
 retry:
  get_sem_info(fDrawRectSem, &info);
  if ((RECT_BUF_SIZE - info.count) < 2) {
    snooze (10 * 1000);
    goto retry;
  }
  
  fDrawRectBuffer[fDrawRect_p].x1 = x1;
  fDrawRectBuffer[fDrawRect_p].x2 = x2;
  fDrawRectBuffer[fDrawRect_p].y1 = y1;
  fDrawRectBuffer[fDrawRect_p].y2 = y2;

  fDrawRect_p++;
  fDrawRect_p %= RECT_BUF_SIZE;

  release_sem (fDrawRectSem);

  return;
}
