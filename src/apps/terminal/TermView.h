/*
 * Copyright 2001-2007, Haiku.
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Parts Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. 
 *
 * Distributed under the terms of the MIT license.
 * Authors:
 *		Stefano Ceccherini <stefano.ceccherini@gmail.com>
 *		Kian Duffy, myob@users.sourceforge.net
 */

#ifndef TERMVIEW_H
#define TERMVIEW_H


#include "CurPos.h"

#include <Messenger.h>
#include <String.h>
#include <View.h>

/* Cursor Blinking flag */
#define CUROFF 0
#define CURON  1

class BMessageRunner;
class BPopUpMenu;
class BScrollBar;
class BString;
class Shell;
class TermBuffer;
class TermView : public BView {
public:
	TermView(BRect frame, const char *command = NULL, int32 historySize = 1000);
	TermView(BMessage *archive);	
	~TermView();

	static	BArchivable* Instantiate(BMessage* data);
	virtual status_t Archive(BMessage* data, bool deep = true) const;
	virtual void GetPreferredSize(float *width, float *height);

	status_t AttachShell(Shell *shell);
	void	DetachShell();

	const char *TerminalName() const;

	void	SetTermFont(const BFont *halfFont, const BFont *fullFont);
	void	GetFontSize(int *width, int *height);

	BRect	SetTermSize(int rows, int cols, bool flag);
	void	SetTextColor(rgb_color fore, rgb_color back);
	void	SetSelectColor(rgb_color fore, rgb_color back);
	void	SetCursorColor(rgb_color fore, rgb_color back);

	int	Encoding() const;
	void	SetEncoding(int encoding);

	// void  SetIMAware (bool);
	void	SetScrollBar(BScrollBar *scrbar);
	BScrollBar  *ScrollBar() const { return fScrollBar; };

	// Output Charactor
	void	PutChar(uchar *string, ushort attr, int width);
	void	PutCR(void);
	void	PutLF(void);
	void	PutNL(int num);
	void	SetInsertMode(int flag);
	void	InsertSpace(int num);
	int	TermDraw(const CurPos &start, const CurPos &end);
	int	TermDrawRegion(CurPos start, CurPos end);
	int	TermDrawSelectedRegion(CurPos start, CurPos end);
		// Delete Charactor
	void	EraseBelow();
	void	DeleteChar(int num);
	void	DeleteColumns();
	void	DeleteLine(int num);

	// Get and Set Cursor position
	void	SetCurPos(int x, int y);
	void	SetCurX(int x);
	void	SetCurY(int y);
	void	GetCurPos(CurPos *inCurPos);
	int	GetCurX();
	int	GetCurY();
	void	SaveCursor();
	void	RestoreCursor();

	// Move Cursor
	void	MoveCurRight(int num);
	void	MoveCurLeft(int num);
	void	MoveCurUp(int num);
	void	MoveCurDown(int num);

	// Cursor setting
	void	DrawCursor();
	void	BlinkCursor();
	void	SetCurDraw(bool flag);
	void	SetCurBlinking(bool flag);

	// Scroll region
	void	ScrollRegion(int top, int bot, int dir, int num);
	void	SetScrollRegion(int top, int bot);
	void	ScrollAtCursor();

	// Other
	void	DeviceStatusReport(int);
	void	UpdateLine();
	void	ScrollScreen();
	void	ScrollScreenDraw();
	void	GetFrameSize(float *width, float *height);
	void	GetFontInfo(int *, int*);
	bool	Find(const BString &str, bool forwardSearch, bool matchCase, bool matchWord);
	void	GetSelection(BString &str);

protected:
	virtual void	AttachedToWindow();
	virtual void	DetachedFromWindow();
	virtual void	Pulse();
	virtual void	Draw(BRect updateRect);
	virtual void	WindowActivated(bool active);
	virtual void	KeyDown(const char*, int32);
	virtual void	MouseDown(BPoint where);
	virtual void	MouseMoved(BPoint, uint32, const BMessage *);

	virtual void	FrameResized(float width, float height);
	virtual void	MessageReceived(BMessage* message);

	virtual status_t GetSupportedSuites(BMessage *msg);
	virtual BHandler* ResolveSpecifier(BMessage *msg, int32 index,
						BMessage *specifier, int32 form,
						const char *property);

private:
	void _InitObject(const char *command);

	static int32	MouseTracking(void *);

	status_t	_InitMouseThread(void);
	void DrawLines(int , int, ushort, uchar *, int, int, int, BView *);
	void DoPrint(BRect updateRect);
	void ResizeScrBarRange (void);
	void DoFileDrop(entry_ref &ref);

	// edit menu function.
	void DoCopy();
	void DoPaste();
	void DoSelectAll();
	void DoClearAll();

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

	// Mouse select
	void	Select(CurPos start, CurPos end);
	void	AddSelectRegion(CurPos);
	void	ResizeSelectRegion(CurPos);

	void	DeSelect();
	bool	HasSelection() const;

	// select word function
	void  SelectWord(BPoint where, int mod); 
	void  SelectLine(BPoint where, int mod);

	// point and text offset conversion.
	CurPos  BPointToCurPos(const BPoint &p);
	BPoint  CurPosToBPoint(const CurPos &pos);

	bool	CheckSelectedRegion(const CurPos &pos);
	inline void Redraw(int, int, int, int);

	void _UpdateSIGWINCH();
	static void _FixFontAttributes(BFont &font);

	Shell *fShell;

	BMessageRunner *fWinchRunner;

	// Font and Width
	BFont fHalfFont;
	BFont fFullFont;
	int fFontWidth;
	int fFontHeight;
	int fFontAscent;
	struct escapement_delta fEscapement;

	// Flags

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
	bigtime_t fLastCursorTime;		
	bool fCursorDrawFlag;
	bool fCursorStatus;
	bool fCursorBlinkingFlag;
	bool fCursorRedrawFlag;
	int fCursorHeight;

	// Cursor position.
	CurPos fCurPos;
	CurPos fCurStack;

	int fBufferStartPos;

	// Terminal rows and columns.
	int fTermRows;
	int fTermColumns;

	int fEncoding;

	// Terminal view pointer.
	int fTop;

	// Object pointer.

	TermBuffer	*fTextBuffer;
	BScrollBar	*fScrollBar;

	// Color and Attribute.

	rgb_color fTextForeColor, fTextBackColor;
	rgb_color fCursorForeColor, fCursorBackColor;
	rgb_color fSelectForeColor, fSelectBackColor;
	
	// Scroll Region
	int fScrTop;
	int fScrBot;
	int32 fScrBufSize;
	bool fScrRegionSet;

	BPoint fPreviousMousePoint;

	// view selection
	CurPos fSelStart;
	CurPos fSelEnd;
	bool fMouseTracking;

	// thread ID / flags.
	thread_id fMouseThread;
	bool fQuitting;

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
