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

#ifndef TERMVIEW_H
#define TERMVIEW_H

#include <View.h>
#include <String.h>
#include <MessageRunner.h>
#include "TermConst.h"
#include "TermWindow.h"
#include "CurPos.h"

/* Cursor Blinking flag */
#define CUROFF 0
#define CURON  1

#define VIEW_THR_CODE 'vtcd'
#define MOUSE_THR_CODE 'mtcd'
#define RECT_BUF_SIZE 32

const unsigned char M_ADD_CURSOR [] = {
  16,				// Size
  1,				// Color depth
  0,				// Hot spot y
  1,				// Hot spot x
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

class TermBuffer;
class CodeConv;
class BPopUpMenu;
class BScrollBar;
class BString;

class TermView : public BView
{
public:
  //
  // Constructor, Destructor, and Initializer.
  //
  TermView (BRect frame, CodeConv *inCodeConv);
  ~TermView ();

  //
  // Initializer, Set function
  //
  void	SetTermFont(BFont *halfFont, BFont *fullFont);
  void	GetFontSize (int *width, int *height);
  BRect	SetTermSize (int rows, int cols, bool flag);
  void	SetTermColor (void);
  void	SetMouseButton (void);
  void	SetMouseCursor (void);
 // void  SetIMAware (bool);
  void	SetScrollBar (BScrollBar *scrbar);

  //
  // Output Charactor.
  //
  void	PutChar (uchar *string, ushort attr, int width);
  void	PutCR (void);
  void	PutLF (void);
  void	PutNL (int num);
  void	SetInsertMode (int flag);
  void	InsertSpace (int num);
  
  int TermDraw (const CurPos &start, const CurPos &end);
  int TermDrawRegion (CurPos start, CurPos end);
  int TermDrawSelectedRegion (CurPos start, CurPos end);

  //  
  // Delete Charactor.
  //
  void	EraseBelow (void);
  void	DeleteChar (int num);
  void	DeleteColumns (void);
  void	DeleteLine (int num);
  
  //
  // Get and Set Cursor position.
  //
  void	SetCurPos (int x, int y);
  void	SetCurX (int x);
  void	SetCurY (int y);
  
  void	GetCurPos (CurPos *inCurPos);
  int	GetCurX (void);
  int	GetCurY (void);

  void	SaveCursor (void);
  void	RestoreCursor (void);
  
  //
  // Move Cursor
  //
  void	MoveCurRight (int num);
  void	MoveCurLeft (int num);
  void	MoveCurUp (int num);
  void	MoveCurDown (int num);

  //
  // Cursor setting.
  //
  void	DrawCursor (void);
  void	BlinkCursor (void);
  void	SetCurDraw (bool flag);
  void	SetCurBlinking (bool flag);

  //
  // Scroll region.
  //
  void	ScrollRegion (int top, int bot, int dir, int num);
  void	SetScrollRegion (int top, int bot);
  void	ScrollAtCursor (void);

  //
  // Other.
  //
  void	UpdateSIGWINCH();
  void	DeviceStatusReport (int);
  inline void UpdateLine (void);
  void	ScrollScreen (void);
  void	ScrollScreenDraw (void);
  void  GetFrameSize (float *width, float *height);
  void  GetFontInfo (int *, int*);

  /*
   *	PRIVATE MEMBER.
   */

private:		
  /*
   * static Functions (Thread functions).
   */

  static int32 ViewThread (void *);
  static int32 MouseTracking (void *);

  /*
   * Hook Functions.
   */
  void	AttachedToWindow (void);
  void	Draw (BRect updateRect);
  void	WindowActivated (bool active);
  void	KeyDown (const char*, int32);
  void	MouseDown (BPoint where);
  void  MouseMoved (BPoint, uint32, const BMessage *);
  
  void	FrameResized(float width, float height);
  void	MessageReceived(BMessage* message);

  /*
   * Private Member Functions.
   */

  thread_id InitViewThread (void);

  inline void DrawLines (int , int, ushort, uchar *, int, int, int, BView *);

  void DoPrint(BRect updateRect);

  void ResizeScrBarRange (void);
  
  void DoFileDrop(entry_ref &ref);
  
  // edit menu function.
  void DoCopy (void);
  void DoPaste (void);
  void DoSelectAll (void);
  void DoClearAll (void);

  int SubstMetaChar (const char *p, char *q);
  void WritePTY (const uchar *text, int num_byteses);
  
  // Comunicate Input Method 
//  void DoIMStart (BMessage* message);
//  void DoIMStop (BMessage* message);
//  void DoIMChange (BMessage* message);
//  void DoIMLocation (BMessage* message);
//  void DoIMConfirm (void);
  void ConfirmString (const char *, int32);
  int32 GetCharFromUTF8String (const char *, char *);
  int32 GetWidthFromUTF8String (const char *);
  
  // mouse operation
  void SetupPop (void);
  int32 SetupMouseButton (const char *bname);

  // Mouse select
  void  Select (CurPos start, CurPos end);
  void  AddSelectRegion (CurPos);
  void  ResizeSelectRegion (CurPos);

  void  DeSelect(void);

  // select word function
  void  SelectWord (BPoint where, int mod); 
  void  SelectLine (BPoint where, int mod);

  // point and text offset conversion.
  CurPos  BPointToCurPos (const BPoint &p);
  BPoint  CurPosToBPoint (const CurPos &pos);

  bool	CheckSelectedRegion (const CurPos &pos);

  inline void SendDataToDrawEngine (int, int, int, int);
  
  /*
   * DATA Member.
   */

  //
  // Font and Width.
  //
  BFont fHalfFont;
  BFont fFullFont;

  int fFontWidth;
  int fFontHeight;

  int fFontAscent;
  struct escapement_delta fEscapement;
  
  //
  // Flags.
  //

  // Update flag (Set on PutChar).
  bool fUpdateFlag;

  // Terminal insertmode flag (use PutChar).
  bool fInsertModeFlag;

  // Scroll count, range.
  int fScrollUpCount;
  int fScrollBarRange;

  // Frame Resized flag.
  bool fFrameResized;
  
  // Cursor Blinking, draw flag.
  bool fCursorDrawFlag;
  bool fCursorStatus;
  bool fCursorBlinkingFlag;
  bool fCursorRedrawFlag;
  
  int fCursorHeight;

  // terminal text attribute flag.
  bool fInverseFlag;
  bool fBoldFlag;
  bool fUnderlineFlag;
  
  //
  // Cursor position.
  //
  CurPos fCurPos;
  CurPos fCurStack;

  int fBufferStartPos;

  //
  // Terminal rows and columns.
  //
  int fTermRows;
  int fTermColumns;

  //
  // Terminal view pointer.
  //
  int fTop;

  //
  // Object pointer.
  //
  TermBuffer	*fTextBuffer;
  CodeConv	*fCodeConv;
  BScrollBar	*fScrollBar;
  
  //
  // Offscreen Bitmap and View.
  //

  BRect fSrcRect;
  BRect fDstRect;
  
  //
  // Color and Attribute.
  //

  // text fore and back color
  rgb_color fTextForeColor, fTextBackColor;

  // cursor fore and back color
  rgb_color fCursorForeColor, fCursorBackColor;

  // selected region color
  rgb_color fSelectForeColor, fSelectBackColor;
  
  uchar fTermAttr;

  //
  // Scroll Region
  //
  int fScrTop;
  int fScrBot;
  int fScrBufSize;
  bool fScrRegionSet;

  BPopUpMenu *fPopMenu; 
  BMenu *fPopEncoding;
  BMenu *fPopSize;
  
  // mouse
  int32 mSelectButton;
  int32 mSubMenuButton;
  int32 mPasteMenuButton;

  bool	fMouseImage;

  BPoint fPreviousMousePoint;

  // view selection
  CurPos fSelStart;
  CurPos fSelEnd;
  int fSelected;
  int fMouseTracking;
  
  // thread ID / flags.
  thread_id fViewThread;
  thread_id fMouseThread;
  int fQuitting;

  // DrawEngine parameter.
  sem_id fDrawRectSem;
  sDrawRect fDrawRectBuffer[RECT_BUF_SIZE];
  int fDrawRect_p;

  // Input Method parameter.
  int fIMViewPtr;
  CurPos fIMStartPos;
  CurPos fIMEndPos;
  BString fIMString;
  bool fIMflag;
  BMessenger fIMMessenger;

  int32 fImCodeState;

};


#endif //TERMVIEW_H
