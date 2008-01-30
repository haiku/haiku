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


class BClipboard;
class BMessageRunner;
class BScrollBar;
class BString;
class Shell;
class TermBuffer;
class TermView : public BView {
public:
	TermView(BRect frame, int32 argc, const char **argv, int32 historySize = 1000);
	TermView(int rows, int columns, int32 argc, const char **argv, int32 historySize = 1000);
	TermView(BMessage *archive);	
	~TermView();

	static	BArchivable* Instantiate(BMessage* data);
	virtual status_t Archive(BMessage* data, bool deep = true) const;
	
	virtual void GetPreferredSize(float *width, float *height);

	const char *TerminalName() const;
	
	void	GetTermFont(BFont *font) const;
	void	SetTermFont(const BFont *font);
	
	void	GetFontSize(int *width, int *height);
	BRect	SetTermSize(int rows, int cols, bool resize);

	void	SetTextColor(rgb_color fore, rgb_color back);
	void	SetSelectColor(rgb_color fore, rgb_color back);
	void	SetCursorColor(rgb_color fore, rgb_color back);

	int	Encoding() const;
	void	SetEncoding(int encoding);

	// void  SetIMAware (bool);
	void	SetScrollBar(BScrollBar *scrbar);
	BScrollBar  *ScrollBar() const { return fScrollBar; };

	virtual void	SetTitle(const char *title);
	virtual void	NotifyQuit(int32 reason);

	// edit functions
	void	Copy(BClipboard *clipboard);	
	void	Paste(BClipboard *clipboard);
	void	SelectAll();
	void	Clear();	
	
	// Output Charactor
	void	Insert(uchar *string, ushort attr);
	void	InsertCR();
	void	InsertLF();
	void	InsertNewLine(int num);
	void	SetInsertMode(int flag);
	void	InsertSpace(int num);
	
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
	bool	Find(const BString &str, bool forwardSearch, bool matchCase, bool matchWord);
	void	GetSelection(BString &str);

	void	CheckShellGone();

	void	InitiateDrag();

protected:
	virtual void	AttachedToWindow();
	virtual void	DetachedFromWindow();
	virtual void	Pulse();
	virtual void	Draw(BRect updateRect);
	virtual void	WindowActivated(bool active);
	virtual void	KeyDown(const char*, int32);
	
	virtual void	MouseDown(BPoint where);
	virtual void	MouseMoved(BPoint, uint32, const BMessage *);
	virtual void	MouseUp(BPoint where);

	virtual void	FrameResized(float width, float height);
	virtual void	MessageReceived(BMessage* message);

	virtual status_t GetSupportedSuites(BMessage *msg);
	virtual BHandler* ResolveSpecifier(BMessage *msg, int32 index,
						BMessage *specifier, int32 form,
						const char *property);

private:
	status_t _InitObject(int32 argc, const char **argv);

	status_t _AttachShell(Shell *shell);
	void _DetachShell();

	void _AboutRequested();

	void _DrawLines(int , int, ushort, uchar *, int, int, int, BView *);
	int _TermDraw(const CurPos &start, const CurPos &end);
	int _TermDrawRegion(CurPos start, CurPos end);
	int _TermDrawSelectedRegion(CurPos start, CurPos end);	
	inline void _Redraw(int, int, int, int);
	
	void _DoPrint(BRect updateRect);
	void _ResizeScrBarRange (void);
	void _DoFileDrop(entry_ref &ref);

	void _WritePTY(const uchar *text, int num_byteses);

	// Comunicate Input Method 
	//  void _DoIMStart (BMessage* message);
	//  void _DoIMStop (BMessage* message);
	//  void _DoIMChange (BMessage* message);
	//  void _DoIMLocation (BMessage* message);
	//  void _DoIMConfirm (void);
	//	void _ConfirmString(const char *, int32);
	
	// Mouse select
	void _Select(CurPos start, CurPos end);
	void _AddSelectRegion(CurPos);
	void _ResizeSelectRegion(CurPos);

	void _DeSelect();
	bool _HasSelection() const;

	// select word function
	void _SelectWord(BPoint where, int mod); 
	void _SelectLine(BPoint where, int mod);

	// point and text offset conversion.
	CurPos _ConvertToTerminal(const BPoint &p);
	BPoint _ConvertFromTerminal(const CurPos &pos);

	bool _CheckSelectedRegion(const CurPos &pos);
	
	void _UpdateSIGWINCH();

	static void _FixFontAttributes(BFont &font);
	
private:
	Shell *fShell;

	BMessageRunner *fWinchRunner;

	// Font and Width
	BFont fHalfFont;
	int fFontWidth;
	int fFontHeight;
	int fFontAscent;
	struct escapement_delta fEscapement;

	// Flags

	// Update flag (Set on Insert).
	bool fUpdateFlag;

	// Terminal insertmode flag (use Insert).
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

	BPoint fClickPoint;

	// view selection
	CurPos fSelStart;
	CurPos fSelEnd;
	bool fMouseTracking;

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
