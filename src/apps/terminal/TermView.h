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

#include <Autolock.h>
#include <Messenger.h>
#include <String.h>
#include <View.h>

#include "TerminalBuffer.h"
#include "TermPos.h"


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

	inline TerminalBuffer* TextBuffer() const	{ return fTextBuffer; }
	
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
	
	// Cursor setting
	void	BlinkCursor();
	void	SetCurDraw(bool flag);
	void	SetCurBlinking(bool flag);

	// Other
	void	GetFrameSize(float *width, float *height);
	bool	Find(const BString &str, bool forwardSearch, bool matchCase, bool matchWord);
	void	GetSelection(BString &str);

	void	CheckShellGone();

	void	InitiateDrag();

protected:
	virtual void	AttachedToWindow();
	virtual void	DetachedFromWindow();
	virtual void	Draw(BRect updateRect);
	virtual void	WindowActivated(bool active);
	virtual void	KeyDown(const char*, int32);
	
	virtual void	MouseDown(BPoint where);
	virtual void	MouseMoved(BPoint, uint32, const BMessage *);
	virtual void	MouseUp(BPoint where);

	virtual void	FrameResized(float width, float height);
	virtual void	MessageReceived(BMessage* message);

	virtual void	ScrollTo(BPoint where);

	virtual status_t GetSupportedSuites(BMessage *msg);
	virtual BHandler* ResolveSpecifier(BMessage *msg, int32 index,
						BMessage *specifier, int32 form,
						const char *property);

private:
	// point and text offset conversion
	inline int32 _LineAt(float y);
	inline float _LineOffset(int32 index);
	inline TermPos _ConvertToTerminal(const BPoint &p);
	inline BPoint _ConvertFromTerminal(const TermPos &pos);

	inline void _InvalidateTextRect(int32 x1, int32 y1, int32 x2, int32 y2);

	status_t _InitObject(int32 argc, const char **argv);

	status_t _AttachShell(Shell *shell);
	void _DetachShell();

	void _AboutRequested();

	void _DrawLinePart(int32 x1, int32 y1, uint16 attr, char *buf,
		int32 width, bool mouse, bool cursor, BView *inView);
	void _DrawCursor();
	void _InvalidateTextRange(TermPos start, TermPos end);	
	
	void _DoPrint(BRect updateRect);
	void _UpdateScrollBarRange();
	void _DoFileDrop(entry_ref &ref);

	void _SynchronizeWithTextBuffer(BRect* invalidateWhenScrolling);

	void _WritePTY(const char* text, int32 numBytes);

	// Comunicate Input Method 
	//  void _DoIMStart (BMessage* message);
	//  void _DoIMStop (BMessage* message);
	//  void _DoIMChange (BMessage* message);
	//  void _DoIMLocation (BMessage* message);
	//  void _DoIMConfirm (void);
	//	void _ConfirmString(const char *, int32);
	
	// selection
	void _Select(TermPos start, TermPos end, bool inclusive,
		bool setInitialSelection);
	void _ExtendSelection(TermPos, bool inclusive, bool useInitialSelection);
	void _Deselect();
	bool _HasSelection() const;
	void _SelectWord(BPoint where, bool extend, bool useInitialSelection); 
	void _SelectLine(BPoint where, bool extend, bool useInitialSelection);

	void _AutoScrollUpdate();

	bool _CheckSelectedRegion(const TermPos &pos) const;
	bool _CheckSelectedRegion(int32 row, int32 firstColumn,
		int32& lastColumn) const;

	void _UpdateSIGWINCH();

	void _ScrollTo(float y, bool scrollGfx);
	void _ScrollToRange(TermPos start, TermPos end);

private:
	class CharClassifier;

	Shell *fShell;

	BMessageRunner *fWinchRunner;
	BMessageRunner *fCursorBlinkRunner;
	BMessageRunner *fAutoScrollRunner;

	CharClassifier *fCharClassifier;

	// Font and Width
	BFont fHalfFont;
	int fFontWidth;
	int fFontHeight;
	int fFontAscent;
	struct escapement_delta fEscapement;

	// frame resized flag.
	bool fFrameResized;

	// Cursor Blinking, draw flag.
	bigtime_t fLastCursorTime;		
	bool fCursorDrawFlag;
	bool fCursorStatus;
	bool fCursorBlinkingFlag;
	bool fCursorRedrawFlag;
	int fCursorHeight;

	// Cursor position.
	TermPos fCursor;

	// Terminal rows and columns.
	int fTermRows;
	int fTermColumns;

	int fEncoding;

	// Object pointer.
	TerminalBuffer	*fTextBuffer;
	BScrollBar		*fScrollBar;

	// Color and Attribute.
	rgb_color fTextForeColor, fTextBackColor;
	rgb_color fCursorForeColor, fCursorBackColor;
	rgb_color fSelectForeColor, fSelectBackColor;
	
	// Scroll Region
	float fScrollOffset;
	int32 fScrBufSize;
		// TODO: That's the history capacity -- only needed until the text
		// buffer is created.
	float fAutoScrollSpeed;

	// selection
	TermPos fSelStart;
	TermPos fSelEnd;
	TermPos fInitialSelectionStart;
	TermPos fInitialSelectionEnd;
	bool fMouseTracking;
	int fSelectGranularity;

	// Input Method parameter.
	int fIMViewPtr;
	TermPos fIMStartPos;
	TermPos fIMEndPos;
	BString fIMString;
	bool fIMflag;
	BMessenger fIMMessenger;
	int32 fImCodeState;
};


#endif //TERMVIEW_H
