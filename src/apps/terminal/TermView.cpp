/*
 * Copyright 2001-2014, Haiku, Inc.
 * Copyright 2003-2004 Kian Duffy, myob@users.sourceforge.net
 * Parts Copyright 1998-1999 Kazuho Okui and Takashi Murai.
 * All rights reserved. Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Stefano Ceccherini, stefano.ceccherini@gmail.com
 *		Kian Duffy, myob@users.sourceforge.net
 *		Y.Hayakawa, hida@sawada.riec.tohoku.ac.jp
 *		Jonathan Schleifer, js@webkeks.org
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 *		Clemens Zeidler, haiku@Clemens-Zeidler.de
 *		Siarzhuk Zharski, zharik@gmx.li
 */


#include "TermView.h"

#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>

#include <algorithm>
#include <new>
#include <vector>

#include <Alert.h>
#include <Application.h>
#include <Beep.h>
#include <Catalog.h>
#include <Clipboard.h>
#include <Debug.h>
#include <Directory.h>
#include <Dragger.h>
#include <Input.h>
#include <Locale.h>
#include <MenuItem.h>
#include <Message.h>
#include <MessageRunner.h>
#include <Node.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <PropertyInfo.h>
#include <Region.h>
#include <Roster.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <String.h>
#include <StringView.h>
#include <UTF8.h>
#include <Window.h>

#include "ActiveProcessInfo.h"
#include "Colors.h"
#include "InlineInput.h"
#include "PrefHandler.h"
#include "Shell.h"
#include "ShellParameters.h"
#include "TermApp.h"
#include "TermConst.h"
#include "TerminalBuffer.h"
#include "TerminalCharClassifier.h"
#include "TermViewStates.h"
#include "VTkeymap.h"


#define ROWS_DEFAULT 25
#define COLUMNS_DEFAULT 80


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Terminal TermView"

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

static const bigtime_t kSyncUpdateGranularity = 100000;	// 0.1 s

static const int32 kCursorBlinkIntervals = 3;
static const int32 kCursorVisibleIntervals = 2;
static const bigtime_t kCursorBlinkInterval = 500000;

static const rgb_color kBlackColor = { 0, 0, 0, 255 };
static const rgb_color kWhiteColor = { 255, 255, 255, 255 };

// secondary mouse button drop
const int32 kSecondaryMouseDropAction = 'SMDA';

enum {
	kInsert,
	kChangeDirectory,
	kLinkFiles,
	kMoveFiles,
	kCopyFiles
};


template<typename Type>
static inline Type
restrict_value(const Type& value, const Type& min, const Type& max)
{
	return value < min ? min : (value > max ? max : value);
}


template<typename Type>
static inline Type
saturated_add(Type a, Type b)
{
	const Type max = (Type)(-1);
	return (max - a >= b ? a + b : max);
}


//	#pragma mark - TextBufferSyncLocker


class TermView::TextBufferSyncLocker {
public:
	TextBufferSyncLocker(TermView* view)
		:
		fView(view)
	{
		fView->fTextBuffer->Lock();
	}

	~TextBufferSyncLocker()
	{
		fView->fTextBuffer->Unlock();

		if (fView->fVisibleTextBufferChanged)
			fView->_VisibleTextBufferChanged();
	}

private:
	TermView*	fView;
};


//	#pragma mark - TermView


TermView::TermView(BRect frame, const ShellParameters& shellParameters,
	int32 historySize)
	:
	BView(frame, "termview", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE),
	fListener(NULL),
	fColumns(COLUMNS_DEFAULT),
	fRows(ROWS_DEFAULT),
	fEncoding(M_UTF8),
	fActive(false),
	fScrBufSize(historySize),
	fReportX10MouseEvent(false),
	fReportNormalMouseEvent(false),
	fReportButtonMouseEvent(false),
	fReportAnyMouseEvent(false)
{
	status_t status = _InitObject(shellParameters);
	if (status != B_OK)
		throw status;
	SetTermSize(frame);
}


TermView::TermView(int rows, int columns,
	const ShellParameters& shellParameters, int32 historySize)
	:
	BView(BRect(0, 0, 0, 0), "termview", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE),
	fListener(NULL),
	fColumns(columns),
	fRows(rows),
	fEncoding(M_UTF8),
	fActive(false),
	fScrBufSize(historySize),
	fReportX10MouseEvent(false),
	fReportNormalMouseEvent(false),
	fReportButtonMouseEvent(false),
	fReportAnyMouseEvent(false)
{
	status_t status = _InitObject(shellParameters);
	if (status != B_OK)
		throw status;

	ResizeToPreferred();

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


TermView::TermView(BMessage* archive)
	:
	BView(archive),
	fListener(NULL),
	fColumns(COLUMNS_DEFAULT),
	fRows(ROWS_DEFAULT),
	fEncoding(M_UTF8),
	fActive(false),
	fScrBufSize(1000),
	fReportX10MouseEvent(false),
	fReportNormalMouseEvent(false),
	fReportButtonMouseEvent(false),
	fReportAnyMouseEvent(false)
{
	BRect frame = Bounds();

	if (archive->FindInt32("encoding", (int32*)&fEncoding) < B_OK)
		fEncoding = M_UTF8;
	if (archive->FindInt32("columns", (int32*)&fColumns) < B_OK)
		fColumns = COLUMNS_DEFAULT;
	if (archive->FindInt32("rows", (int32*)&fRows) < B_OK)
		fRows = ROWS_DEFAULT;

	int32 argc = 0;
	if (archive->HasInt32("argc"))
		archive->FindInt32("argc", &argc);

	const char **argv = new const char*[argc];
	for (int32 i = 0; i < argc; i++) {
		archive->FindString("argv", i, (const char**)&argv[i]);
	}

	// TODO: Retrieve colors, history size, etc. from archive
	status_t status = _InitObject(ShellParameters(argc, argv));
	if (status != B_OK)
		throw status;

	bool useRect = false;
	if ((archive->FindBool("use_rect", &useRect) == B_OK) && useRect)
		SetTermSize(frame);

	delete[] argv;
}


/*!	Initializes the object for further use.
	The members fRows, fColumns, fEncoding, and fScrBufSize must
	already be initialized; they are not touched by this method.
*/
status_t
TermView::_InitObject(const ShellParameters& shellParameters)
{
	SetFlags(Flags() | B_WILL_DRAW | B_FRAME_EVENTS
		| B_FULL_UPDATE_ON_RESIZE/* | B_INPUT_METHOD_AWARE*/);

	fShell = NULL;
	fWinchRunner = NULL;
	fCursorBlinkRunner = NULL;
	fAutoScrollRunner = NULL;
	fResizeRunner = NULL;
	fResizeView = NULL;
	fCharClassifier = NULL;
	fFontWidth = 0;
	fFontHeight = 0;
	fFontAscent = 0;
	fEmulateBold = false;
	fAllowBold = false;
	fFrameResized = false;
	fResizeViewDisableCount = 0;
	fLastActivityTime = 0;
	fCursorState = 0;
	fCursorStyle = BLOCK_CURSOR;
	fCursorBlinking = true;
	fCursorHidden = false;
	fCursor = TermPos(0, 0);
	fTextBuffer = NULL;
	fVisibleTextBuffer = NULL;
	fVisibleTextBufferChanged = false;
	fScrollBar = NULL;
	fInline = NULL;
	fSelectForeColor = kWhiteColor;
	fSelectBackColor = kBlackColor;
	fScrollOffset = 0;
	fLastSyncTime = 0;
	fScrolledSinceLastSync = 0;
	fSyncRunner = NULL;
	fConsiderClockedSync = false;
	fSelection.SetHighlighter(this);
	fSelection.SetRange(TermPos(0, 0), TermPos(0, 0));
	fPrevPos = TermPos(-1, - 1);
	fReportX10MouseEvent = false;
	fReportNormalMouseEvent = false;
	fReportButtonMouseEvent = false;
	fReportAnyMouseEvent = false;
	fMouseClipboard = be_clipboard;
	fDefaultState = new(std::nothrow) DefaultState(this);
	fSelectState = new(std::nothrow) SelectState(this);
	fHyperLinkState = new(std::nothrow) HyperLinkState(this);
	fHyperLinkMenuState = new(std::nothrow) HyperLinkMenuState(this);
	fActiveState = NULL;

	fTextBuffer = new(std::nothrow) TerminalBuffer;
	if (fTextBuffer == NULL)
		return B_NO_MEMORY;

	fVisibleTextBuffer = new(std::nothrow) BasicTerminalBuffer;
	if (fVisibleTextBuffer == NULL)
		return B_NO_MEMORY;

	// TODO: Make the special word chars user-settable!
	fCharClassifier = new(std::nothrow) DefaultCharClassifier(
		kDefaultAdditionalWordCharacters);
	if (fCharClassifier == NULL)
		return B_NO_MEMORY;

	status_t error = fTextBuffer->Init(fColumns, fRows, fScrBufSize);
	if (error != B_OK)
		return error;
	fTextBuffer->SetEncoding(fEncoding);

	error = fVisibleTextBuffer->Init(fColumns, fRows + 2, 0);
	if (error != B_OK)
		return error;

	fShell = new (std::nothrow) Shell();
	if (fShell == NULL)
		return B_NO_MEMORY;

	SetTermFont(be_fixed_font);

	// set the shell parameters' encoding
	ShellParameters modifiedShellParameters(shellParameters);
	modifiedShellParameters.SetEncoding(fEncoding);

	error = fShell->Open(fRows, fColumns, modifiedShellParameters);

	if (error < B_OK)
		return error;

	error = _AttachShell(fShell);
	if (error < B_OK)
		return error;

	fHighlights.AddItem(&fSelection);

	if (fDefaultState == NULL || fSelectState == NULL || fHyperLinkState == NULL
		|| fHyperLinkMenuState == NULL) {
		return B_NO_MEMORY;
	}

	SetLowColor(fTextBackColor);
	SetViewColor(B_TRANSPARENT_32_BIT);

	_NextState(fDefaultState);

	return B_OK;
}


TermView::~TermView()
{
	Shell* shell = fShell;
		// _DetachShell sets fShell to NULL

	_DetachShell();

	delete fDefaultState;
	delete fSelectState;
	delete fHyperLinkState;
	delete fHyperLinkMenuState;
	delete fSyncRunner;
	delete fAutoScrollRunner;
	delete fCharClassifier;
	delete fVisibleTextBuffer;
	delete fTextBuffer;
	delete shell;
}


bool
TermView::IsShellBusy() const
{
	return fShell != NULL && fShell->HasActiveProcesses();
}


bool
TermView::GetActiveProcessInfo(ActiveProcessInfo& _info) const
{
	if (fShell == NULL) {
		_info.Unset();
		return false;
	}

	return fShell->GetActiveProcessInfo(_info);
}


bool
TermView::GetShellInfo(ShellInfo& _info) const
{
	if (fShell == NULL) {
		_info = ShellInfo();
		return false;
	}

	_info = fShell->Info();
	return true;
}


/* static */
BArchivable *
TermView::Instantiate(BMessage* data)
{
	if (validate_instantiation(data, "TermView")) {
		TermView *view = new (std::nothrow) TermView(data);
		return view;
	}

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
		status = data->AddInt32("columns", (int32)fColumns);
	if (status == B_OK)
		status = data->AddInt32("rows", (int32)fRows);

	if (data->ReplaceString("class", "TermView") != B_OK)
		data->AddString("class", "TermView");

	return status;
}


rgb_color
TermView::ForegroundColor()
{
	return fSelectForeColor;
}


rgb_color
TermView::BackgroundColor()
{
	return fSelectBackColor;
}


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
TermPos
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
	// assume the worst case with full-width characters - invalidate 2 cells
	BRect rect(x1 * fFontWidth, _LineOffset(y1),
		(x2 + 1) * fFontWidth * 2 - 1, _LineOffset(y2 + 1) - 1);
//debug_printf("Invalidate((%f, %f) - (%f, %f))\n", rect.left, rect.top,
//rect.right, rect.bottom);
	Invalidate(rect);
}


void
TermView::GetPreferredSize(float *width, float *height)
{
	if (width)
		*width = fColumns * fFontWidth - 1;
	if (height)
		*height = fRows * fFontHeight - 1;
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
TermView::GetFontSize(float* _width, float* _height)
{
	*_width = fFontWidth;
	*_height = fFontHeight;
}


int
TermView::Rows() const
{
	return fRows;
}


int
TermView::Columns() const
{
	return fColumns;
}


//! Set number of rows and columns in terminal
BRect
TermView::SetTermSize(int rows, int columns, bool notifyShell)
{
//debug_printf("TermView::SetTermSize(%d, %d)\n", rows, columns);
	if (rows > 0)
		fRows = rows;
	if (columns > 0)
		fColumns = columns;

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

//debug_printf("Invalidate()\n");
	Invalidate();

	if (fScrollBar != NULL) {
		_UpdateScrollBarRange();
		fScrollBar->SetSteps(fFontHeight, fFontHeight * fRows);
	}

	BRect rect(0, 0, fColumns * fFontWidth, fRows * fFontHeight);

	// synchronize the visible text buffer
	{
		TextBufferSyncLocker _(this);

		_SynchronizeWithTextBuffer(0, -1);
		int32 offset = _LineAt(0);
		fVisibleTextBuffer->SynchronizeWith(fTextBuffer, offset, offset,
			offset + rows + 2);
		fVisibleTextBufferChanged = true;
	}

	if (notifyShell)
		fFrameResized = true;

	return rect;
}


void
TermView::SetTermSize(BRect rect, bool notifyShell)
{
	int rows;
	int columns;

	GetTermSizeFromRect(rect, &rows, &columns);
	SetTermSize(rows, columns, notifyShell);
}


void
TermView::GetTermSizeFromRect(const BRect &rect, int *_rows,
	int *_columns)
{
	int columns = int((rect.IntegerWidth() + 1) / fFontWidth);
	int rows = int((rect.IntegerHeight() + 1) / fFontHeight);

	if (_rows)
		*_rows = rows;
	if (_columns)
		*_columns = columns;
}


void
TermView::SetTextColor(rgb_color fore, rgb_color back)
{
	fTextBackColor = back;
	fTextForeColor = fore;

	SetLowColor(fTextBackColor);
}


void
TermView::SetCursorColor(rgb_color fore, rgb_color back)
{
	fCursorForeColor = fore;
	fCursorBackColor = back;
}


void
TermView::SetSelectColor(rgb_color fore, rgb_color back)
{
	fSelectForeColor = fore;
	fSelectBackColor = back;
}


void
TermView::SetTermColor(uint index, rgb_color color, bool dynamic)
{
	if (!dynamic) {
		if (index < kTermColorCount)
			fTextBuffer->SetPaletteColor(index, color);
		return;
	}

	switch (index) {
		case 10:
			fTextForeColor = color;
			break;
		case 11:
			fTextBackColor = color;
			SetLowColor(fTextBackColor);
			break;
		case 110:
			fTextForeColor = PrefHandler::Default()->getRGB(
								PREF_TEXT_FORE_COLOR);
			break;
		case 111:
			fTextBackColor = PrefHandler::Default()->getRGB(
								PREF_TEXT_BACK_COLOR);
			SetLowColor(fTextBackColor);
			break;
		default:
			break;
	}
}


int
TermView::Encoding() const
{
	return fEncoding;
}


void
TermView::SetEncoding(int encoding)
{
	fEncoding = encoding;

	if (fShell != NULL)
		fShell->SetEncoding(fEncoding);

	BAutolock _(fTextBuffer);
	fTextBuffer->SetEncoding(fEncoding);
}


void
TermView::SetMouseClipboard(BClipboard *clipboard)
{
	fMouseClipboard = clipboard;
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
	float halfWidth = 0;

	fHalfFont = font;
	fBoldFont = font;
	uint16 face = fBoldFont.Face();
	fBoldFont.SetFace(B_BOLD_FACE | (face & ~B_REGULAR_FACE));

	fHalfFont.SetSpacing(B_FIXED_SPACING);

	// calculate half font's max width
	// Not Bounding, check only A-Z (For case of fHalfFont is KanjiFont.)
	for (int c = 0x20; c <= 0x7e; c++) {
		char buf[4];
		sprintf(buf, "%c", c);
		float tmpWidth = fHalfFont.StringWidth(buf);
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

	fCursorStyle = PrefHandler::Default() == NULL ? BLOCK_CURSOR
		: PrefHandler::Default()->getCursor(PREF_CURSOR_STYLE);
	bool blinking = PrefHandler::Default()->getBool(PREF_BLINK_CURSOR);
	SwitchCursorBlinking(blinking);

	fEmulateBold = PrefHandler::Default() == NULL ? false
		: PrefHandler::Default()->getBool(PREF_EMULATE_BOLD);

	fAllowBold = PrefHandler::Default() == NULL ? false
		: PrefHandler::Default()->getBool(PREF_ALLOW_BOLD);

	_ScrollTo(0, false);
	if (fScrollBar != NULL)
		fScrollBar->SetSteps(fFontHeight, fFontHeight * fRows);
}


void
TermView::SetScrollBar(BScrollBar *scrollBar)
{
	fScrollBar = scrollBar;
	if (fScrollBar != NULL)
		fScrollBar->SetSteps(fFontHeight, fFontHeight * fRows);
}


void
TermView::SwitchCursorBlinking(bool blinkingOn)
{
	fCursorBlinking = blinkingOn;
	if (blinkingOn) {
		if (fCursorBlinkRunner == NULL) {
			BMessage blinkMessage(kBlinkCursor);
			fCursorBlinkRunner = new (std::nothrow) BMessageRunner(
				BMessenger(this), &blinkMessage, kCursorBlinkInterval);
		}
	} else {
		// make sure the cursor becomes visible
		fCursorState = 0;
		_InvalidateTextRect(fCursor.x, fCursor.y, fCursor.x, fCursor.y);
		delete fCursorBlinkRunner;
		fCursorBlinkRunner = NULL;
	}
}


void
TermView::Copy(BClipboard *clipboard)
{
	BAutolock _(fTextBuffer);

	if (!_HasSelection())
		return;

	BString copyStr;
	fTextBuffer->GetStringFromRegion(copyStr, fSelection.Start(),
		fSelection.End());

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

		_ScrollTo(0, true);
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
		_InvalidateTextRect(start.x, start.y, fColumns, start.y);

		if (end.y - start.y > 0)
			_InvalidateTextRect(0, start.y + 1, fColumns, end.y - 1);

		_InvalidateTextRect(0, end.y, end.x, end.y);
	}
}


status_t
TermView::_AttachShell(Shell *shell)
{
	if (shell == NULL)
		return B_BAD_VALUE;

	fShell = shell;

	return fShell->AttachBuffer(TextBuffer());
}


void
TermView::_DetachShell()
{
	fShell->DetachBuffer();
	fShell = NULL;
}


void
TermView::_Activate()
{
	fActive = true;
	SwitchCursorBlinking(fCursorBlinking);
}


void
TermView::_Deactivate()
{
	// make sure the cursor becomes visible
	fCursorState = 0;
	_InvalidateTextRect(fCursor.x, fCursor.y, fCursor.x, fCursor.y);

	SwitchCursorBlinking(false);

	fActive = false;
}


//! Draw part of a line in the given view.
void
TermView::_DrawLinePart(float x1, float y1, uint32 attr, char *buf,
	int32 width, Highlight* highlight, bool cursor, BView *inView)
{
	if (highlight != NULL)
		attr = highlight->Highlighter()->AdjustTextAttributes(attr);

	inView->SetFont(IS_BOLD(attr) && !fEmulateBold && fAllowBold
		? &fBoldFont : &fHalfFont);

	// Set pen point
	float x2 = x1 + fFontWidth * width;
	float y2 = y1 + fFontHeight;

	rgb_color rgb_fore = fTextForeColor;
	rgb_color rgb_back = fTextBackColor;

	// color attribute
	int forecolor = IS_FORECOLOR(attr);
	int backcolor = IS_BACKCOLOR(attr);

	if (IS_FORESET(attr))
		rgb_fore = fTextBuffer->PaletteColor(forecolor);
	if (IS_BACKSET(attr))
		rgb_back = fTextBuffer->PaletteColor(backcolor);

	// Selection check.
	if (cursor) {
		rgb_fore = fCursorForeColor;
		rgb_back = fCursorBackColor;
	} else if (highlight != NULL) {
		rgb_fore = highlight->Highlighter()->ForegroundColor();
		rgb_back = highlight->Highlighter()->BackgroundColor();
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
	if (IS_BOLD(attr)) {
		if (fEmulateBold) {
			inView->MovePenTo(x1 - 1, y1 + fFontAscent - 1);
			inView->DrawString((char *)buf);
			inView->SetDrawingMode(B_OP_BLEND);
		} else {
			rgb_color bright = rgb_fore;

			bright.red = saturated_add<uint8>(bright.red, 64);
			bright.green = saturated_add<uint8>(bright.green, 64);
			bright.blue = saturated_add<uint8>(bright.blue, 64);

			inView->SetHighColor(bright);
		}
	}

	inView->MovePenTo(x1, y1 + fFontAscent);
	inView->DrawString((char *)buf);
	inView->SetDrawingMode(B_OP_COPY);

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
	rect.bottom = rect.top + fFontHeight - 1;
	int32 firstVisible = _LineAt(0);

	UTF8Char character;
	uint32 attr = 0;

	bool cursorVisible = _IsCursorVisible();

	if (cursorVisible) {
		switch (fCursorStyle) {
			case UNDERLINE_CURSOR:
				rect.top = rect.bottom - 2;
				break;
			case IBEAM_CURSOR:
				rect.right = rect.left + 1;
				break;
			case BLOCK_CURSOR:
			default:
				break;
		}
	}

	Highlight* highlight = _CheckHighlightRegion(TermPos(fCursor.x, fCursor.y));
	if (fVisibleTextBuffer->GetChar(fCursor.y - firstVisible, fCursor.x,
			character, attr) == A_CHAR
			&& (fCursorStyle == BLOCK_CURSOR || !cursorVisible)) {

		int32 width = IS_WIDTH(attr) ? FULL_WIDTH : HALF_WIDTH;
		char buffer[5];
		int32 bytes = UTF8Char::ByteCount(character.bytes[0]);
		memcpy(buffer, character.bytes, bytes);
		buffer[bytes] = '\0';

		_DrawLinePart(fCursor.x * fFontWidth, (int32)rect.top, attr, buffer,
			width, highlight, cursorVisible, this);
	} else {
		if (highlight != NULL)
			SetHighColor(highlight->Highlighter()->BackgroundColor());
		else if (cursorVisible)
			SetHighColor(fCursorBackColor );
		else {
			uint32 count = 0;
			rgb_color rgb_back = fTextBackColor;
			if (fTextBuffer->IsAlternateScreenActive())
				// alternate screen uses cell attributes beyond the line ends
				fTextBuffer->GetCellAttributes(
						fCursor.y, fCursor.x, attr, count);
			else
				attr = fVisibleTextBuffer->GetLineColor(
						fCursor.y - firstVisible);

			if (IS_BACKSET(attr))
				rgb_back = fTextBuffer->PaletteColor(IS_BACKCOLOR(attr));
			SetHighColor(rgb_back);
		}

		if (IS_WIDTH(attr) && fCursorStyle != IBEAM_CURSOR)
			rect.right += fFontWidth;

		FillRect(rect);
	}
}


bool
TermView::_IsCursorVisible() const
{
	return !fCursorHidden && fCursorState < kCursorVisibleIntervals;
}


void
TermView::_BlinkCursor()
{
	bool wasVisible = _IsCursorVisible();

	if (!wasVisible && fInline && fInline->IsActive())
		return;

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

	float viewHeight = fRows * fFontHeight;
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
		fShell->UpdateWindowSize(fRows, fColumns);
		fFrameResized = false;
	}
}


void
TermView::AttachedToWindow()
{
	fMouseButtons = 0;

	_UpdateModifiers();

	// update the terminal size because it may have changed while the TermView
	// was detached from the window. On such conditions FrameResized was not
	// called when the resize occured
	SetTermSize(Bounds(), true);
	MakeFocus(true);
	if (fScrollBar) {
		fScrollBar->SetSteps(fFontHeight, fFontHeight * fRows);
		_UpdateScrollBarRange();
	}

	BMessenger thisMessenger(this);

	BMessage message(kUpdateSigWinch);
	fWinchRunner = new (std::nothrow) BMessageRunner(thisMessenger,
		&message, 500000);

	{
		TextBufferSyncLocker _(this);
		fTextBuffer->SetListener(thisMessenger);
		_SynchronizeWithTextBuffer(0, -1);
	}

	be_clipboard->StartWatching(thisMessenger);
}


void
TermView::DetachedFromWindow()
{
	be_clipboard->StopWatching(BMessenger(this));

	 _NextState(fDefaultState);

	delete fWinchRunner;
	fWinchRunner = NULL;

	delete fCursorBlinkRunner;
	fCursorBlinkRunner = NULL;

	delete fResizeRunner;
	fResizeRunner = NULL;

	{
		BAutolock _(fTextBuffer);
		fTextBuffer->UnsetListener();
	}
}


void
TermView::Draw(BRect updateRect)
{
	int32 x1 = (int32)(updateRect.left / fFontWidth);
	int32 x2 = std::min((int)(updateRect.right / fFontWidth), fColumns - 1);

	int32 firstVisible = _LineAt(0);
	int32 y1 = _LineAt(updateRect.top);
	int32 y2 = std::min(_LineAt(updateRect.bottom), (int32)fRows - 1);

	// clear the area to the right of the line ends
	if (y1 <= y2) {
		float clearLeft = fColumns * fFontWidth;
		if (clearLeft <= updateRect.right) {
			BRect rect(clearLeft, updateRect.top, updateRect.right,
				updateRect.bottom);
			SetHighColor(fTextBackColor);
			FillRect(rect);
		}
	}

	// clear the area below the last line
	if (y2 == fRows - 1) {
		float clearTop = _LineOffset(fRows);
		if (clearTop <= updateRect.bottom) {
			BRect rect(updateRect.left, clearTop, updateRect.right,
				updateRect.bottom);
			SetHighColor(fTextBackColor);
			FillRect(rect);
		}
	}

	// draw the affected line parts
	if (x1 <= x2) {
		uint32 attr = 0;

		for (int32 j = y1; j <= y2; j++) {
			int32 k = x1;
			char buf[fColumns * 4 + 1];

			if (fVisibleTextBuffer->IsFullWidthChar(j - firstVisible, k))
				k--;

			if (k < 0)
				k = 0;

			for (int32 i = k; i <= x2;) {
				int32 lastColumn = x2;
				Highlight* highlight = _CheckHighlightRegion(j, i, lastColumn);
					// This will clip lastColumn to the selection start or end
					// to ensure the selection is not drawn at the same time as
					// something else
				int32 count = fVisibleTextBuffer->GetString(j - firstVisible, i,
					lastColumn, buf, attr);

// debug_printf("  fVisibleTextBuffer->GetString(%ld, %ld, %ld) -> (%ld, \"%.*s\"), highlight: %p\n",
// j - firstVisible, i, lastColumn, count, (int)count, buf, highlight);

				if (count == 0) {
					// No chars to draw : we just fill the rectangle with the
					// back color of the last char at the left
					int nextColumn = lastColumn + 1;
					BRect rect(fFontWidth * i, _LineOffset(j),
						fFontWidth * nextColumn - 1, 0);
					rect.bottom = rect.top + fFontHeight - 1;

					rgb_color rgb_back = highlight != NULL
						? highlight->Highlighter()->BackgroundColor()
						: fTextBackColor;

					if (fTextBuffer->IsAlternateScreenActive()) {
						// alternate screen uses cell attributes
						// beyond the line ends
						uint32 count = 0;
						fTextBuffer->GetCellAttributes(j, i, attr, count);
						rect.right = rect.left + fFontWidth * count - 1;
						nextColumn = i + count;
					} else
						attr = fVisibleTextBuffer->GetLineColor(j - firstVisible);

					if (IS_BACKSET(attr)) {
						int backcolor = IS_BACKCOLOR(attr);
						rgb_back = fTextBuffer->PaletteColor(backcolor);
					}

					SetHighColor(rgb_back);
					rgb_back = HighColor();
					FillRect(rect);

					// Go on to the next block
					i = nextColumn;
					continue;
				}

				// Note: full-width characters GetString()-ed always
				// with count 1, so this hardcoding is safe. From the other
				// side - drawing the whole string with one call render the
				// characters not aligned to cells grid - that looks much more
				// inaccurate for full-width strings than for half-width ones.
				if (IS_WIDTH(attr))
					count = FULL_WIDTH;

				_DrawLinePart(fFontWidth * i, (int32)_LineOffset(j),
					attr, buf, count, highlight, false, this);
				i += count;
			}
		}
	}

	if (fInline && fInline->IsActive())
		_DrawInlineMethodString();

	if (fCursor >= TermPos(x1, y1) && fCursor <= TermPos(x2, y2))
		_DrawCursor();
}


void
TermView::_DoPrint(BRect updateRect)
{
#if 0
	uint32 attr;
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
	if (active && IsFocus()) {
		if (!fActive)
			_Activate();
	} else {
		if (fActive)
			_Deactivate();
	}

	_UpdateModifiers();

	fActiveState->WindowActivated(active);
}


void
TermView::MakeFocus(bool focusState)
{
	BView::MakeFocus(focusState);

	if (focusState && Window() && Window()->IsActive()) {
		if (!fActive)
			_Activate();
	} else {
		if (fActive)
			_Deactivate();
	}
}


void
TermView::KeyDown(const char *bytes, int32 numBytes)
{
	_UpdateModifiers();

	fActiveState->KeyDown(bytes, numBytes);
}


void
TermView::FrameResized(float width, float height)
{
//debug_printf("TermView::FrameResized(%f, %f)\n", width, height);
	int32 columns = (int32)((width + 1) / fFontWidth);
	int32 rows = (int32)((height + 1) / fFontHeight);

	if (columns == fColumns && rows == fRows)
		return;

	bool hasResizeView = fResizeRunner != NULL;
	if (!hasResizeView) {
		// show the current size in a view
		fResizeView = new BStringView(BRect(100, 100, 300, 140), "size", "");
		fResizeView->SetAlignment(B_ALIGN_CENTER);
		fResizeView->SetFont(be_bold_font);
		fResizeView->SetViewColor(fTextBackColor);
		fResizeView->SetLowColor(fTextBackColor);
		fResizeView->SetHighColor(fTextForeColor);

		BMessage message(MSG_REMOVE_RESIZE_VIEW_IF_NEEDED);
		fResizeRunner = new(std::nothrow) BMessageRunner(BMessenger(this),
			&message, 25000LL);
	}

	BString text;
	text << columns << " x " << rows;
	fResizeView->SetText(text.String());
	fResizeView->GetPreferredSize(&width, &height);
	fResizeView->ResizeTo(width * 1.5, height * 1.5);
	fResizeView->MoveTo((Bounds().Width() - fResizeView->Bounds().Width()) / 2,
		(Bounds().Height()- fResizeView->Bounds().Height()) / 2);
	if (!hasResizeView && fResizeViewDisableCount < 1)
		AddChild(fResizeView);

	if (fResizeViewDisableCount > 0)
		fResizeViewDisableCount--;

	SetTermSize(rows, columns, true);
}


void
TermView::MessageReceived(BMessage *msg)
{
	if (fActiveState->MessageReceived(msg))
		return;

	entry_ref ref;
	const char *ctrl_l = "\x0c";

	// first check for any dropped message
	if (msg->WasDropped() && (msg->what == B_SIMPLE_DATA
			|| msg->what == B_MIME_DATA)) {
		char *text;
		ssize_t numBytes;
		//rgb_color *color;

		int32 i = 0;

		if (msg->FindRef("refs", i++, &ref) == B_OK) {
			// first check if secondary mouse button is pressed
			int32 buttons = 0;
			msg->FindInt32("buttons", &buttons);

			if (buttons == B_SECONDARY_MOUSE_BUTTON) {
				// start popup menu
				_SecondaryMouseButtonDropped(msg);
				return;
			}

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

	switch (msg->what) {
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

		case B_CLIPBOARD_CHANGED:
			// This message originates from the system clipboard. Overwrite
			// the contents of the mouse clipboard with the ones from the
			// system clipboard, in case it contains text data.
			if (be_clipboard->Lock()) {
				if (fMouseClipboard->Lock()) {
					BMessage* clipMsgA = be_clipboard->Data();
					const char* text;
					ssize_t numBytes;
					if (clipMsgA->FindData("text/plain", B_MIME_TYPE,
							(const void**)&text, &numBytes) == B_OK ) {
						fMouseClipboard->Clear();
						BMessage* clipMsgB = fMouseClipboard->Data();
						clipMsgB->AddData("text/plain", B_MIME_TYPE,
							text, numBytes);
						fMouseClipboard->Commit();
					}
					fMouseClipboard->Unlock();
				}
				be_clipboard->Unlock();
			}
			break;

		case B_SELECT_ALL:
			SelectAll();
			break;

		case B_SET_PROPERTY:
		{
			int32 i;
			int32 encodingID;
			BMessage specifier;
			if (msg->GetCurrentSpecifier(&i, &specifier) == B_OK
				&& strcmp("encoding",
					specifier.FindString("property", i)) == 0) {
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
			if (msg->GetCurrentSpecifier(&i, &specifier) == B_OK
				&& strcmp("encoding",
					specifier.FindString("property", i)) == 0) {
				BMessage reply(B_REPLY);
				reply.AddInt32("result", Encoding());
				msg->SendReply(&reply);
			} else if (strcmp("tty",
					specifier.FindString("property", i)) == 0) {
				BMessage reply(B_REPLY);
				reply.AddString("result", TerminalName());
				msg->SendReply(&reply);
			} else {
				BView::MessageReceived(msg);
			}
			break;
		}

		case B_MODIFIERS_CHANGED:
		{
			_UpdateModifiers();
			break;
		}

		case B_INPUT_METHOD_EVENT:
		{
			int32 opcode;
			if (msg->FindInt32("be:opcode", &opcode) == B_OK) {
				switch (opcode) {
					case B_INPUT_METHOD_STARTED:
					{
						BMessenger messenger;
						if (msg->FindMessenger("be:reply_to",
								&messenger) == B_OK) {
							fInline = new (std::nothrow)
								InlineInput(messenger);
						}
						break;
					}

					case B_INPUT_METHOD_STOPPED:
						delete fInline;
						fInline = NULL;
						break;

					case B_INPUT_METHOD_CHANGED:
						if (fInline != NULL)
							_HandleInputMethodChanged(msg);
						break;

					case B_INPUT_METHOD_LOCATION_REQUEST:
						if (fInline != NULL)
							_HandleInputMethodLocationRequest();
						break;

					default:
						break;
				}
			}
			break;
		}

		case B_MOUSE_WHEEL_CHANGED:
		{
			// overridden to allow scrolling emulation in alternative screen
			// mode
			BAutolock locker(fTextBuffer);
			float deltaY = 0;
			if (fTextBuffer->IsAlternateScreenActive()
				&& msg->FindFloat("be:wheel_delta_y", &deltaY) == B_OK
				&& deltaY != 0) {
				// We are in alternative screen mode and have a vertical delta
				// we can work with -- emulate scrolling via terminal escape
				// sequences.
				locker.Unlock();

				// scroll pagewise, if one of Option, Command, or Control is
				// pressed
				int32 steps;
				const char* stepString;
				if ((modifiers() & B_SHIFT_KEY) != 0) {
					// pagewise
					stepString = deltaY > 0
						? PAGE_DOWN_KEY_CODE : PAGE_UP_KEY_CODE;
					steps = abs((int)deltaY);
				} else {
					// three lines per step
					stepString = deltaY > 0
						? DOWN_ARROW_KEY_CODE : UP_ARROW_KEY_CODE;
					steps = 3 * abs((int)deltaY);
				}

				// We want to do only a single write(), so compose a string
				// repeating the sequence as often as required by the delta.
				BString toWrite;
				for (int32 i = 0; i <steps; i++)
					toWrite << stepString;

				_WritePTY(toWrite.String(), toWrite.Length());
			} else {
				// let the BView's implementation handle the standard scrolling
				locker.Unlock();
				BView::MessageReceived(msg);
			}

			break;
		}

		case MENU_CLEAR_ALL:
			Clear();
			fShell->Write(ctrl_l, 1);
			break;
		case kBlinkCursor:
			_BlinkCursor();
			break;
		case kUpdateSigWinch:
			_UpdateSIGWINCH();
			break;
		case kSecondaryMouseDropAction:
			_DoSecondaryMouseDropAction(msg);
			break;
		case MSG_TERMINAL_BUFFER_CHANGED:
		{
			TextBufferSyncLocker _(this);
			_SynchronizeWithTextBuffer(0, -1);
			break;
		}
		case MSG_SET_TERMINAL_TITLE:
		{
			const char* title;
			if (msg->FindString("title", &title) == B_OK) {
				if (fListener != NULL)
					fListener->SetTermViewTitle(this, title);
			}
			break;
		}
		case MSG_SET_TERMINAL_COLORS:
		{
			int32 count  = 0;
			if (msg->FindInt32("count", &count) != B_OK)
				break;
			bool dynamic  = false;
			if (msg->FindBool("dynamic", &dynamic) != B_OK)
				break;
			for (int i = 0; i < count; i++) {
				uint8 index = 0;
				if (msg->FindUInt8("index", i, &index) != B_OK)
					break;

				ssize_t bytes = 0;
				rgb_color* color = 0;
				if (msg->FindData("color", B_RGB_COLOR_TYPE,
							i, (const void**)&color, &bytes) != B_OK)
					break;
				SetTermColor(index, *color, dynamic);
			}
			break;
		}
		case MSG_RESET_TERMINAL_COLORS:
		{
			int32 count  = 0;
			if (msg->FindInt32("count", &count) != B_OK)
				break;
			bool dynamic  = false;
			if (msg->FindBool("dynamic", &dynamic) != B_OK)
				break;
			for (int i = 0; i < count; i++) {
				uint8 index = 0;
				if (msg->FindUInt8("index", i, &index) != B_OK)
					break;

				SetTermColor(index,
					TermApp::DefaultPalette()[index], dynamic);
			}
			break;
		}
		case MSG_SET_CURSOR_STYLE:
		{
			int32 style = BLOCK_CURSOR;
			if (msg->FindInt32("style", &style) == B_OK)
				fCursorStyle = style;

			bool blinking = fCursorBlinking;
			if (msg->FindBool("blinking", &blinking) == B_OK)
				SwitchCursorBlinking(blinking);

			bool hidden = fCursorHidden;
			if (msg->FindBool("hidden", &hidden) == B_OK)
				fCursorHidden = hidden;
			break;
		}
		case MSG_REPORT_MOUSE_EVENT:
		{
			bool report;
			if (msg->FindBool("reportX10MouseEvent", &report) == B_OK)
				fReportX10MouseEvent = report;

			if (msg->FindBool("reportNormalMouseEvent", &report) == B_OK)
				fReportNormalMouseEvent = report;

			if (msg->FindBool("reportButtonMouseEvent", &report) == B_OK)
				fReportButtonMouseEvent = report;

			if (msg->FindBool("reportAnyMouseEvent", &report) == B_OK)
				fReportAnyMouseEvent = report;
			break;
		}
		case MSG_REMOVE_RESIZE_VIEW_IF_NEEDED:
		{
			BPoint point;
			uint32 buttons;
			GetMouse(&point, &buttons, false);
			if (buttons != 0)
				break;

			if (fResizeView != NULL) {
				fResizeView->RemoveSelf();
				delete fResizeView;
				fResizeView = NULL;
			}
			delete fResizeRunner;
			fResizeRunner = NULL;
			break;
		}

		case MSG_QUIT_TERMNAL:
		{
			int32 reason;
			if (msg->FindInt32("reason", &reason) != B_OK)
				reason = 0;
			if (fListener != NULL)
				fListener->NotifyTermViewQuit(this, reason);
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
		TextBufferSyncLocker _(this);
		if (diff < 0)
			_SynchronizeWithTextBuffer(newFirstLine, oldFirstLine - 1);
		else
			_SynchronizeWithTextBuffer(oldLastLine + 1, newLastLine);
	}
}


void
TermView::TargetedByScrollView(BScrollView *scrollView)
{
	BView::TargetedByScrollView(scrollView);

	SetScrollBar(scrollView ? scrollView->ScrollBar(B_VERTICAL) : NULL);
}


BHandler*
TermView::ResolveSpecifier(BMessage* message, int32 index, BMessage* specifier,
	int32 what, const char* property)
{
	BHandler* target = this;
	BPropertyInfo propInfo(sPropList);
	if (propInfo.FindMatch(message, index, specifier, what, property) < B_OK) {
		target = BView::ResolveSpecifier(message, index, specifier, what,
			property);
	}

	return target;
}


void
TermView::_SecondaryMouseButtonDropped(BMessage* msg)
{
	// Launch menu to choose what is to do with the msg data
	BPoint point;
	if (msg->FindPoint("_drop_point_", &point) != B_OK)
		return;

	BMessage* insertMessage = new BMessage(*msg);
	insertMessage->what = kSecondaryMouseDropAction;
	insertMessage->AddInt8("action", kInsert);

	BMessage* cdMessage = new BMessage(*msg);
	cdMessage->what = kSecondaryMouseDropAction;
	cdMessage->AddInt8("action", kChangeDirectory);

	BMessage* lnMessage = new BMessage(*msg);
	lnMessage->what = kSecondaryMouseDropAction;
	lnMessage->AddInt8("action", kLinkFiles);

	BMessage* mvMessage = new BMessage(*msg);
	mvMessage->what = kSecondaryMouseDropAction;
	mvMessage->AddInt8("action", kMoveFiles);

	BMessage* cpMessage = new BMessage(*msg);
	cpMessage->what = kSecondaryMouseDropAction;
	cpMessage->AddInt8("action", kCopyFiles);

	BMenuItem* insertItem = new BMenuItem(
		B_TRANSLATE("Insert path"), insertMessage);
	BMenuItem* cdItem = new BMenuItem(
		B_TRANSLATE("Change directory"), cdMessage);
	BMenuItem* lnItem = new BMenuItem(
		B_TRANSLATE("Create link here"), lnMessage);
	BMenuItem* mvItem = new BMenuItem(B_TRANSLATE("Move here"), mvMessage);
	BMenuItem* cpItem = new BMenuItem(B_TRANSLATE("Copy here"), cpMessage);
	BMenuItem* chItem = new BMenuItem(B_TRANSLATE("Cancel"), NULL);

	// if the refs point to different directorys disable the cd menu item
	bool differentDirs = false;
	BDirectory firstDir;
	entry_ref ref;
	int i = 0;
	while (msg->FindRef("refs", i++, &ref) == B_OK) {
		BNode node(&ref);
		BEntry entry(&ref);
		BDirectory dir;
		if (node.IsDirectory())
			dir.SetTo(&ref);
		else
			entry.GetParent(&dir);

		if (i == 1) {
			node_ref nodeRef;
			dir.GetNodeRef(&nodeRef);
			firstDir.SetTo(&nodeRef);
		} else if (firstDir != dir) {
			differentDirs = true;
			break;
		}
	}
	if (differentDirs)
		cdItem->SetEnabled(false);

	BPopUpMenu *menu = new BPopUpMenu(
		"Secondary mouse button drop menu");
	menu->SetAsyncAutoDestruct(true);
	menu->AddItem(insertItem);
	menu->AddSeparatorItem();
	menu->AddItem(cdItem);
	menu->AddItem(lnItem);
	menu->AddItem(mvItem);
	menu->AddItem(cpItem);
	menu->AddSeparatorItem();
	menu->AddItem(chItem);
	menu->SetTargetForItems(this);
	menu->Go(point, true, true, true);
}


void
TermView::_DoSecondaryMouseDropAction(BMessage* msg)
{
	int8 action = -1;
	msg->FindInt8("action", &action);

	BString outString = "";
	BString itemString = "";

	switch (action) {
		case kInsert:
			break;
		case kChangeDirectory:
			outString = "cd ";
			break;
		case kLinkFiles:
			outString = "ln -s ";
			break;
		case kMoveFiles:
			outString = "mv ";
			break;
		case kCopyFiles:
			outString = "cp ";
			break;

		default:
			return;
	}

	bool listContainsDirectory = false;
	entry_ref ref;
	int32 i = 0;
	while (msg->FindRef("refs", i++, &ref) == B_OK) {
		BEntry ent(&ref);
		BNode node(&ref);
		BPath path(&ent);
		BString string(path.Path());

		if (node.IsDirectory())
			listContainsDirectory = true;

		if (i > 1)
			itemString += " ";

		if (action == kChangeDirectory) {
			if (!node.IsDirectory()) {
				int32 slash = string.FindLast("/");
				string.Truncate(slash);
			}
			string.CharacterEscape(kShellEscapeCharacters, '\\');
			itemString += string;
			break;
		}
		string.CharacterEscape(kShellEscapeCharacters, '\\');
		itemString += string;
	}

	if (listContainsDirectory && action == kCopyFiles)
		outString += "-R ";

	outString += itemString;

	if (action == kLinkFiles || action == kMoveFiles || action == kCopyFiles)
		outString += " .";

	if (action != kInsert)
		outString += "\n";

	_WritePTY(outString.String(), outString.Length());
}


//! Gets dropped file full path and display it at cursor position.
void
TermView::_DoFileDrop(entry_ref& ref)
{
	BEntry ent(&ref);
	BPath path(&ent);
	BString string(path.Path());

	string.CharacterEscape(kShellEscapeCharacters, '\\');
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
		if (fScrolledSinceLastSync + linesScrolled <= fRows) {
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
	}

	if (fScrolledSinceLastSync + linesScrolled <= fRows) {
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

	fVisibleTextBufferChanged = true;

	// Simple case first -- complete invalidation.
	if (info.invalidateAll) {
		Invalidate();
		_UpdateScrollBarRange();
		_Deselect();

		fCursor = fTextBuffer->Cursor();
		_ActivateCursor(false);

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

		// move highlights
		for (int32 i = 0; Highlight* highlight = fHighlights.ItemAt(i); i++) {
			if (highlight->IsEmpty())
				continue;

			highlight->ScrollRange(linesScrolled);
			if (highlight == &fSelection) {
				fInitialSelectionStart.y -= linesScrolled;
				fInitialSelectionEnd.y -= linesScrolled;
			}

			if (highlight->Start().y < -historySize) {
				if (highlight == &fSelection)
					_Deselect();
				else
					_ClearHighlight(highlight);
			}
		}
	}

	// invalidate dirty region
	if (info.IsDirtyRegionValid()) {
		_InvalidateTextRect(0, info.dirtyTop, fTextBuffer->Width() - 1,
			info.dirtyBottom);

		// clear the selection, if affected
		if (!fSelection.IsEmpty()) {
			// TODO: We're clearing the selection more often than necessary --
			// to avoid that, we'd also need to track the x coordinates of the
			// dirty range.
			int32 selectionBottom = fSelection.End().x > 0
				? fSelection.End().y : fSelection.End().y - 1;
			if (fSelection.Start().y <= info.dirtyBottom
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


void
TermView::_VisibleTextBufferChanged()
{
	if (!fVisibleTextBufferChanged)
		return;

	fVisibleTextBufferChanged = false;
	fActiveState->VisibleTextBufferChanged();
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


//! Returns the square of the actual pixel distance between both points
float
TermView::_MouseDistanceSinceLastClick(BPoint where)
{
	return (fLastClickPoint.x - where.x) * (fLastClickPoint.x - where.x)
		+ (fLastClickPoint.y - where.y) * (fLastClickPoint.y - where.y);
}


void
TermView::_SendMouseEvent(int32 buttons, int32 mode, int32 x, int32 y,
	bool motion)
{
	char xtermButtons;
	if (buttons == B_PRIMARY_MOUSE_BUTTON)
		xtermButtons = 32 + 0;
	else if (buttons == B_SECONDARY_MOUSE_BUTTON)
		xtermButtons = 32 + 1;
	else if (buttons == B_TERTIARY_MOUSE_BUTTON)
		xtermButtons = 32 + 2;
	else
		xtermButtons = 32 + 3;

	if (motion)
		xtermButtons += 32;

	char xtermX = x + 1 + 32;
	char xtermY = y + 1 + 32;

	char destBuffer[6];
	destBuffer[0] = '\033';
	destBuffer[1] = '[';
	destBuffer[2] = 'M';
	destBuffer[3] = xtermButtons;
	destBuffer[4] = xtermX;
	destBuffer[5] = xtermY;
	fShell->Write(destBuffer, 6);
}


void
TermView::MouseDown(BPoint where)
{
	if (!IsFocus())
		MakeFocus();

	_UpdateModifiers();

	BMessage* currentMessage = Window()->CurrentMessage();
	int32 buttons = currentMessage->GetInt32("buttons", 0);

	fActiveState->MouseDown(where, buttons, fModifiers);

	fMouseButtons = buttons;
	fLastClickPoint = where;
}


void
TermView::MouseMoved(BPoint where, uint32 transit, const BMessage *message)
{
	_UpdateModifiers();

	fActiveState->MouseMoved(where, transit, message, fModifiers);
}


void
TermView::MouseUp(BPoint where)
{
	_UpdateModifiers();

	int32 buttons = Window()->CurrentMessage()->GetInt32("buttons", 0);

	fActiveState->MouseUp(where, buttons);

	fMouseButtons = buttons;
}


//! Select a range of text.
void
TermView::_Select(TermPos start, TermPos end, bool inclusive,
	bool setInitialSelection)
{
	TextBufferSyncLocker _(this);

	_SynchronizeWithTextBuffer(0, -1);

	if (end < start)
		std::swap(start, end);

	if (inclusive)
		end.x++;

//debug_printf("TermView::_Select(): (%ld, %ld) - (%ld, %ld)\n", start.x,
//start.y, end.x, end.y);

	if (start.x < 0)
		start.x = 0;
	if (end.x >= fColumns)
		end.x = fColumns;

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
		if (end.x >= fColumns)
			end.x = fColumns;
	}

	if (!fSelection.IsEmpty())
		_InvalidateTextRange(fSelection.Start(), fSelection.End());

	fSelection.SetRange(start, end);

	if (setInitialSelection) {
		fInitialSelectionStart = fSelection.Start();
		fInitialSelectionEnd = fSelection.End();
	}

	_InvalidateTextRange(fSelection.Start(), fSelection.End());
}


//! Extend selection (shift + mouse click).
void
TermView::_ExtendSelection(TermPos pos, bool inclusive,
	bool useInitialSelection)
{
	if (!useInitialSelection && !_HasSelection())
		return;

	TermPos start = fSelection.Start();
	TermPos end = fSelection.End();

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
	else if (useInitialSelection)
		_Select(start, end, false, false);
}


// clear the selection.
void
TermView::_Deselect()
{
//debug_printf("TermView::_Deselect(): has selection: %d\n", _HasSelection());
	if (_ClearHighlight(&fSelection)) {
		fInitialSelectionStart.SetTo(0, 0);
		fInitialSelectionEnd.SetTo(0, 0);
	}
}


bool
TermView::_HasSelection() const
{
	return !fSelection.IsEmpty();
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
		if (start
				< (useInitialSelection
					? fInitialSelectionStart : fSelection.Start())) {
			_ExtendSelection(start, false, useInitialSelection);
		} else if (end
				> (useInitialSelection
					? fInitialSelectionEnd : fSelection.End())) {
			_ExtendSelection(end, false, useInitialSelection);
		} else if (useInitialSelection)
			_Select(start, end, false, false);
	} else
		_Select(start, end, false, !useInitialSelection);
}


void
TermView::_SelectLine(BPoint where, bool extend, bool useInitialSelection)
{
	TermPos start = TermPos(0, _ConvertToTerminal(where).y);
	TermPos end = TermPos(0, start.y + 1);

	if (extend) {
		if (start
				< (useInitialSelection
					? fInitialSelectionStart : fSelection.Start())) {
			_ExtendSelection(start, false, useInitialSelection);
		} else if (end
				> (useInitialSelection
					? fInitialSelectionEnd : fSelection.End())) {
			_ExtendSelection(end, false, useInitialSelection);
		} else if (useInitialSelection)
			_Select(start, end, false, false);
	} else
		_Select(start, end, false, !useInitialSelection);
}


void
TermView::_AddHighlight(Highlight* highlight)
{
	fHighlights.AddItem(highlight);

	if (!highlight->IsEmpty())
		_InvalidateTextRange(highlight->Start(), highlight->End());
}


void
TermView::_RemoveHighlight(Highlight* highlight)
{
	if (!highlight->IsEmpty())
		_InvalidateTextRange(highlight->Start(), highlight->End());

	fHighlights.RemoveItem(highlight);
}


bool
TermView::_ClearHighlight(Highlight* highlight)
{
	if (highlight->IsEmpty())
		return false;

	_InvalidateTextRange(highlight->Start(), highlight->End());

	highlight->SetRange(TermPos(0, 0), TermPos(0, 0));
	return true;
}


TermView::Highlight*
TermView::_CheckHighlightRegion(const TermPos &pos) const
{
	for (int32 i = 0; Highlight* highlight = fHighlights.ItemAt(i); i++) {
		if (highlight->RangeContains(pos))
			return highlight;
	}

	return NULL;
}


TermView::Highlight*
TermView::_CheckHighlightRegion(int32 row, int32 firstColumn,
	int32& lastColumn) const
{
	Highlight* nextHighlight = NULL;

	for (int32 i = 0; Highlight* highlight = fHighlights.ItemAt(i); i++) {
		if (highlight->IsEmpty())
			continue;

		if (row == highlight->Start().y && firstColumn < highlight->Start().x
				&& lastColumn >= highlight->Start().x) {
			// region starts before the highlight, but intersects with it
			if (nextHighlight == NULL
				|| highlight->Start().x < nextHighlight->Start().x) {
				nextHighlight = highlight;
			}
			continue;
		}

		if (row == highlight->End().y && firstColumn < highlight->End().x
				&& lastColumn >= highlight->End().x) {
			// region starts in the highlight, but exceeds the end
			lastColumn = highlight->End().x - 1;
			return highlight;
		}

		TermPos pos(firstColumn, row);
		if (highlight->RangeContains(pos))
			return highlight;
	}

	if (nextHighlight != NULL)
		lastColumn = nextHighlight->Start().x - 1;
	return NULL;
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
		*width = fColumns * fFontWidth;

	if (height != NULL)
		*height = (fRows + historySize) * fFontHeight;
}


// Find a string, and select it if found
bool
TermView::Find(const BString &str, bool forwardSearch, bool matchCase,
	bool matchWord)
{
	TextBufferSyncLocker _(this);
	_SynchronizeWithTextBuffer(0, -1);

	TermPos start;
	if (_HasSelection()) {
		if (forwardSearch)
			start = fSelection.End();
		else
			start = fSelection.Start();
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
	_ScrollToRange(fSelection.Start(), fSelection.End());

	return true;
}


//! Get the selected text and copy to str
void
TermView::GetSelection(BString &str)
{
	str.SetTo("");
	BAutolock _(fTextBuffer);
	fTextBuffer->GetStringFromRegion(str, fSelection.Start(), fSelection.End());
}


bool
TermView::CheckShellGone() const
{
	if (!fShell)
		return false;

	// check, if the shell does still live
	pid_t pid = fShell->ProcessID();
	team_info info;
	return get_team_info(pid, &info) == B_BAD_TEAM_ID;
}


void
TermView::InitiateDrag()
{
	BAutolock _(fTextBuffer);

	BString copyStr("");
	fTextBuffer->GetStringFromRegion(copyStr, fSelection.Start(),
		fSelection.End());

	BMessage message(B_MIME_DATA);
	message.AddData("text/plain", B_MIME_TYPE, copyStr.String(),
		copyStr.Length());

	BPoint start = _ConvertFromTerminal(fSelection.Start());
	BPoint end = _ConvertFromTerminal(fSelection.End());

	BRect rect;
	if (fSelection.Start().y == fSelection.End().y)
		rect.Set(start.x, start.y, end.x + fFontWidth, end.y + fFontHeight);
	else
		rect.Set(0, start.y, fColumns * fFontWidth, end.y + fFontHeight);

	rect = rect & Bounds();

	DragMessage(&message, rect);
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


void
TermView::DisableResizeView(int32 disableCount)
{
	fResizeViewDisableCount += disableCount;
}


void
TermView::_DrawInlineMethodString()
{
	if (!fInline || !fInline->String())
		return;

	const int32 numChars = BString(fInline->String()).CountChars();

	BPoint startPoint = _ConvertFromTerminal(fCursor);
	BPoint endPoint = startPoint;
	endPoint.x += fFontWidth * numChars;
	endPoint.y += fFontHeight + 1;

	BRect eraseRect(startPoint, endPoint);

	PushState();
	SetHighColor(fTextForeColor);
	FillRect(eraseRect);
	PopState();

	BPoint loc = _ConvertFromTerminal(fCursor);
	loc.y += fFontHeight;
	SetFont(&fHalfFont);
	SetHighColor(fTextBackColor);
	SetLowColor(fTextForeColor);
	DrawString(fInline->String(), loc);
}


void
TermView::_HandleInputMethodChanged(BMessage *message)
{
	const char *string = NULL;
	if (message->FindString("be:string", &string) < B_OK || string == NULL)
		return;

	_ActivateCursor(false);

	if (IsFocus())
		be_app->ObscureCursor();

	// If we find the "be:confirmed" boolean (and the boolean is true),
	// it means it's over for now, so the current InlineInput object
	// should become inactive. We will probably receive a
	// B_INPUT_METHOD_STOPPED message after this one.
	bool confirmed;
	if (message->FindBool("be:confirmed", &confirmed) != B_OK)
		confirmed = false;

	fInline->SetString("");

	Invalidate();
	// TODO: Debug only
	snooze(100000);

	fInline->SetString(string);
	fInline->ResetClauses();

	if (!confirmed && !fInline->IsActive())
		fInline->SetActive(true);

	// Get the clauses, and pass them to the InlineInput object
	// TODO: Find out if what we did it's ok, currently we don't consider
	// clauses at all, while the bebook says we should; though the visual
	// effect we obtained seems correct. Weird.
	int32 clauseCount = 0;
	int32 clauseStart;
	int32 clauseEnd;
	while (message->FindInt32("be:clause_start", clauseCount, &clauseStart)
			== B_OK
		&& message->FindInt32("be:clause_end", clauseCount, &clauseEnd)
			== B_OK) {
		if (!fInline->AddClause(clauseStart, clauseEnd))
			break;
		clauseCount++;
	}

	if (confirmed) {
		fInline->SetString("");
		_ActivateCursor(true);

		// now we need to feed ourselves the individual characters as if the
		// user would have pressed them now - this lets KeyDown() pick out all
		// the special characters like B_BACKSPACE, cursor keys and the like:
		const char* currPos = string;
		const char* prevPos = currPos;
		while (*currPos != '\0') {
			if ((*currPos & 0xC0) == 0xC0) {
				// found the start of an UTF-8 char, we collect while it lasts
				++currPos;
				while ((*currPos & 0xC0) == 0x80)
					++currPos;
			} else if ((*currPos & 0xC0) == 0x80) {
				// illegal: character starts with utf-8 intermediate byte, skip it
				prevPos = ++currPos;
			} else {
				// single byte character/code, just feed that
				++currPos;
			}
			KeyDown(prevPos, currPos - prevPos);
			prevPos = currPos;
		}

		Invalidate();
	} else {
		// temporarily show transient state of inline input
		int32 selectionStart = 0;
		int32 selectionEnd = 0;
		message->FindInt32("be:selection", 0, &selectionStart);
		message->FindInt32("be:selection", 1, &selectionEnd);

		fInline->SetSelectionOffset(selectionStart);
		fInline->SetSelectionLength(selectionEnd - selectionStart);
		Invalidate();
	}
}


void
TermView::_HandleInputMethodLocationRequest()
{
	BMessage message(B_INPUT_METHOD_EVENT);
	message.AddInt32("be:opcode", B_INPUT_METHOD_LOCATION_REQUEST);

	BString string(fInline->String());

	const int32 &limit = string.CountChars();
	BPoint where = _ConvertFromTerminal(fCursor);
	where.y += fFontHeight;

	for (int32 i = 0; i < limit; i++) {
		// Add the location of the UTF8 characters

		where.x += fFontWidth;
		ConvertToScreen(&where);

		message.AddPoint("be:location_reply", where);
		message.AddFloat("be:height_reply", fFontHeight);
	}

	fInline->Method()->SendMessage(&message);
}


void
TermView::_CancelInputMethod()
{
	if (!fInline)
		return;

	InlineInput *inlineInput = fInline;
	fInline = NULL;

	if (inlineInput->IsActive() && Window()) {
		Invalidate();

		BMessage message(B_INPUT_METHOD_EVENT);
		message.AddInt32("be:opcode", B_INPUT_METHOD_STOPPED);
		inlineInput->Method()->SendMessage(&message);
	}

	delete inlineInput;
}


void
TermView::_UpdateModifiers()
{
	// TODO: This method is a general work-around for missing or out-of-order
	// B_MODIFIERS_CHANGED messages. This should really be fixed where it is
	// broken (app server?).
	int32 oldModifiers = fModifiers;
	fModifiers = modifiers();
	if (fModifiers != oldModifiers && fActiveState != NULL)
		fActiveState->ModifiersChanged(oldModifiers, fModifiers);
}


void
TermView::_NextState(State* state)
{
	if (state != fActiveState) {
		if (fActiveState != NULL)
			fActiveState->Exited();
		fActiveState = state;
		fActiveState->Entered();
	}
}


// #pragma mark - Listener


TermView::Listener::~Listener()
{
}


void
TermView::Listener::NotifyTermViewQuit(TermView* view, int32 reason)
{
}


void
TermView::Listener::SetTermViewTitle(TermView* view, const char* title)
{
}


void
TermView::Listener::PreviousTermView(TermView* view)
{
}


void
TermView::Listener::NextTermView(TermView* view)
{
}


// #pragma mark -


#ifdef USE_DEBUG_SNAPSHOTS

void
TermView::MakeDebugSnapshots()
{
	BAutolock _(fTextBuffer);
	time_t timeStamp = time(NULL);
	fTextBuffer->MakeLinesSnapshots(timeStamp, ".TextBuffer.dump");
	fVisibleTextBuffer->MakeLinesSnapshots(timeStamp, ".VisualTextBuffer.dump");
}


void
TermView::StartStopDebugCapture()
{
	BAutolock _(fTextBuffer);
	fTextBuffer->StartStopDebugCapture();
}

#endif
