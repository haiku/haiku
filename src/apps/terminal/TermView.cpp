/*
 * Copyright 2001-2010, Haiku, Inc.
 * Copyright 2003-2004 Kian Duffy, myob@users.sourceforge.net
 * Parts Copyright 1998-1999 Kazuho Okui and Takashi Murai.
 * All rights reserved. Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Stefano Ceccherini, stefano.ceccherini@gmail.com
 *		Kian Duffy, myob@users.sourceforge.net
 *		Y.Hayakawa, hida@sawada.riec.tohoku.ac.jp
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 *		Clemens Zeidler, haiku@Clemens-Zeidler.de
 */


#include "TermView.h"

#include <ctype.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>

#include <algorithm>
#include <new>

#include "ActiveProcessInfo.h"
#include <Alert.h>
#include <Application.h>
#include <Beep.h>
#include <Catalog.h>
#include <CharacterSet.h>
#include <CharacterSetRoster.h>
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

#include "InlineInput.h"
#include "Shell.h"
#include "ShellParameters.h"
#include "TermConst.h"
#include "TerminalBuffer.h"
#include "TerminalCharClassifier.h"
#include "VTkeymap.h"


using namespace BPrivate ; // BCharacterSet stuff

// defined in VTKeyTbl.c
extern int function_keycode_table[];
extern char *function_key_char_table[];

static rgb_color kTermColorTable[256] = {
	{ 40,  40,  40, 0},	// black
	{204,   0,   0, 0},	// red
	{ 78, 154,   6, 0},	// green
	{218, 168,   0, 0},	// yellow
	{ 51, 102, 152, 0},	// blue
	{115,  68, 123, 0},	// magenta
	{  6, 152, 154, 0},	// cyan
	{245, 245, 245, 0},	// white

	{128, 128, 128, 0},	// black
	{255,   0,   0, 0},	// red
	{  0, 255,   0, 0},	// green
	{255, 255,   0, 0},	// yellow
	{  0,   0, 255, 0},	// blue
	{255,   0, 255, 0},	// magenta
	{  0, 255, 255, 0},	// cyan
	{255, 255, 255, 0},	// white

	{  0,   0,   0, 0},
	{  0,   0,  51, 0},
	{  0,   0, 102, 0},
	{  0,   0, 153, 0},
	{  0,   0, 204, 0},
	{  0,   0, 255, 0},
	{  0,  51,   0, 0},
	{  0,  51,  51, 0},
	{  0,  51, 102, 0},
	{  0,  51, 153, 0},
	{  0,  51, 204, 0},
	{  0,  51, 255, 0},
	{  0, 102,   0, 0},
	{  0, 102,  51, 0},
	{  0, 102, 102, 0},
	{  0, 102, 153, 0},
	{  0, 102, 204, 0},
	{  0, 102, 255, 0},
	{  0, 153,   0, 0},
	{  0, 153,  51, 0},
	{  0, 153, 102, 0},
	{  0, 153, 153, 0},
	{  0, 153, 204, 0},
	{  0, 153, 255, 0},
	{  0, 204,   0, 0},
	{  0, 204,  51, 0},
	{  0, 204, 102, 0},
	{  0, 204, 153, 0},
	{  0, 204, 204, 0},
	{  0, 204, 255, 0},
	{  0, 255,   0, 0},
	{  0, 255,  51, 0},
	{  0, 255, 102, 0},
	{  0, 255, 153, 0},
	{  0, 255, 204, 0},
	{  0, 255, 255, 0},
	{ 51,   0,   0, 0},
	{ 51,   0,  51, 0},
	{ 51,   0, 102, 0},
	{ 51,   0, 153, 0},
	{ 51,   0, 204, 0},
	{ 51,   0, 255, 0},
	{ 51,  51,   0, 0},
	{ 51,  51,  51, 0},
	{ 51,  51, 102, 0},
	{ 51,  51, 153, 0},
	{ 51,  51, 204, 0},
	{ 51,  51, 255, 0},
	{ 51, 102,   0, 0},
	{ 51, 102,  51, 0},
	{ 51, 102, 102, 0},
	{ 51, 102, 153, 0},
	{ 51, 102, 204, 0},
	{ 51, 102, 255, 0},
	{ 51, 153,   0, 0},
	{ 51, 153,  51, 0},
	{ 51, 153, 102, 0},
	{ 51, 153, 153, 0},
	{ 51, 153, 204, 0},
	{ 51, 153, 255, 0},
	{ 51, 204,   0, 0},
	{ 51, 204,  51, 0},
	{ 51, 204, 102, 0},
	{ 51, 204, 153, 0},
	{ 51, 204, 204, 0},
	{ 51, 204, 255, 0},
	{ 51, 255,   0, 0},
	{ 51, 255,  51, 0},
	{ 51, 255, 102, 0},
	{ 51, 255, 153, 0},
	{ 51, 255, 204, 0},
	{ 51, 255, 255, 0},
	{102,   0,   0, 0},
	{102,   0,  51, 0},
	{102,   0, 102, 0},
	{102,   0, 153, 0},
	{102,   0, 204, 0},
	{102,   0, 255, 0},
	{102,  51,   0, 0},
	{102,  51,  51, 0},
	{102,  51, 102, 0},
	{102,  51, 153, 0},
	{102,  51, 204, 0},
	{102,  51, 255, 0},
	{102, 102,   0, 0},
	{102, 102,  51, 0},
	{102, 102, 102, 0},
	{102, 102, 153, 0},
	{102, 102, 204, 0},
	{102, 102, 255, 0},
	{102, 153,   0, 0},
	{102, 153,  51, 0},
	{102, 153, 102, 0},
	{102, 153, 153, 0},
	{102, 153, 204, 0},
	{102, 153, 255, 0},
	{102, 204,   0, 0},
	{102, 204,  51, 0},
	{102, 204, 102, 0},
	{102, 204, 153, 0},
	{102, 204, 204, 0},
	{102, 204, 255, 0},
	{102, 255,   0, 0},
	{102, 255,  51, 0},
	{102, 255, 102, 0},
	{102, 255, 153, 0},
	{102, 255, 204, 0},
	{102, 255, 255, 0},
	{153,   0,   0, 0},
	{153,   0,  51, 0},
	{153,   0, 102, 0},
	{153,   0, 153, 0},
	{153,   0, 204, 0},
	{153,   0, 255, 0},
	{153,  51,   0, 0},
	{153,  51,  51, 0},
	{153,  51, 102, 0},
	{153,  51, 153, 0},
	{153,  51, 204, 0},
	{153,  51, 255, 0},
	{153, 102,   0, 0},
	{153, 102,  51, 0},
	{153, 102, 102, 0},
	{153, 102, 153, 0},
	{153, 102, 204, 0},
	{153, 102, 255, 0},
	{153, 153,   0, 0},
	{153, 153,  51, 0},
	{153, 153, 102, 0},
	{153, 153, 153, 0},
	{153, 153, 204, 0},
	{153, 153, 255, 0},
	{153, 204,   0, 0},
	{153, 204,  51, 0},
	{153, 204, 102, 0},
	{153, 204, 153, 0},
	{153, 204, 204, 0},
	{153, 204, 255, 0},
	{153, 255,   0, 0},
	{153, 255,  51, 0},
	{153, 255, 102, 0},
	{153, 255, 153, 0},
	{153, 255, 204, 0},
	{153, 255, 255, 0},
	{204,   0,   0, 0},
	{204,   0,  51, 0},
	{204,   0, 102, 0},
	{204,   0, 153, 0},
	{204,   0, 204, 0},
	{204,   0, 255, 0},
	{204,  51,   0, 0},
	{204,  51,  51, 0},
	{204,  51, 102, 0},
	{204,  51, 153, 0},
	{204,  51, 204, 0},
	{204,  51, 255, 0},
	{204, 102,   0, 0},
	{204, 102,  51, 0},
	{204, 102, 102, 0},
	{204, 102, 153, 0},
	{204, 102, 204, 0},
	{204, 102, 255, 0},
	{204, 153,   0, 0},
	{204, 153,  51, 0},
	{204, 153, 102, 0},
	{204, 153, 153, 0},
	{204, 153, 204, 0},
	{204, 153, 255, 0},
	{204, 204,   0, 0},
	{204, 204,  51, 0},
	{204, 204, 102, 0},
	{204, 204, 153, 0},
	{204, 204, 204, 0},
	{204, 204, 255, 0},
	{204, 255,   0, 0},
	{204, 255,  51, 0},
	{204, 255, 102, 0},
	{204, 255, 153, 0},
	{204, 255, 204, 0},
	{204, 255, 255, 0},
	{255,   0,   0, 0},
	{255,   0,  51, 0},
	{255,   0, 102, 0},
	{255,   0, 153, 0},
	{255,   0, 204, 0},
	{255,   0, 255, 0},
	{255,  51,   0, 0},
	{255,  51,  51, 0},
	{255,  51, 102, 0},
	{255,  51, 153, 0},
	{255,  51, 204, 0},
	{255,  51, 255, 0},
	{255, 102,   0, 0},
	{255, 102,  51, 0},
	{255, 102, 102, 0},
	{255, 102, 153, 0},
	{255, 102, 204, 0},
	{255, 102, 255, 0},
	{255, 153,   0, 0},
	{255, 153,  51, 0},
	{255, 153, 102, 0},
	{255, 153, 153, 0},
	{255, 153, 204, 0},
	{255, 153, 255, 0},
	{255, 204,   0, 0},
	{255, 204,  51, 0},
	{255, 204, 102, 0},
	{255, 204, 153, 0},
	{255, 204, 204, 0},
	{255, 204, 255, 0},
	{255, 255,   0, 0},
	{255, 255,  51, 0},
	{255, 255, 102, 0},
	{255, 255, 153, 0},
	{255, 255, 204, 0},
	{255, 255, 255, 0},

	{ 10,  10,  10, 0},
	{ 20,  20,  20, 0},
	{ 30,  30,  30, 0},
	{ 40,  40,  40, 0},
	{ 50,  50,  50, 0},
	{ 60,  60,  60, 0},
	{ 70,  70,  70, 0},
	{ 80,  80,  80, 0},
	{ 90,  90,  90, 0},
	{100, 100, 100, 0},
	{110, 110, 110, 0},
	{120, 120, 120, 0},
	{130, 130, 130, 0},
	{140, 140, 140, 0},
	{150, 150, 150, 0},
	{160, 160, 160, 0},
	{170, 170, 170, 0},
	{180, 180, 180, 0},
	{190, 190, 190, 0},
	{200, 200, 200, 0},
	{210, 210, 210, 0},
	{220, 220, 220, 0},
	{230, 230, 230, 0},
	{240, 240, 240, 0},
};

#define ROWS_DEFAULT 25
#define COLUMNS_DEFAULT 80

// selection granularity
enum {
	SELECT_CHARS,
	SELECT_WORDS,
	SELECT_LINES
};

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
static const uint32 kAutoScroll = 'AScr';

static const bigtime_t kSyncUpdateGranularity = 100000;	// 0.1 s

static const int32 kCursorBlinkIntervals = 3;
static const int32 kCursorVisibleIntervals = 2;
static const bigtime_t kCursorBlinkInterval = 500000;

static const rgb_color kBlackColor = { 0, 0, 0, 255 };
static const rgb_color kWhiteColor = { 255, 255, 255, 255 };

static const char* kDefaultSpecialWordChars = ":@-./_~";
static const char* kEscapeCharacters = " ~`#$&*()\\|[]{};'\"<>?!";

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


// #pragma mark - CharClassifier


class TermView::CharClassifier : public TerminalCharClassifier {
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
	fFrameResized = false;
	fResizeViewDisableCount = 0;
	fLastActivityTime = 0;
	fCursorState = 0;
	fCursorHeight = 0;
	fCursor = TermPos(0, 0);
	fTextBuffer = NULL;
	fVisibleTextBuffer = NULL;
	fScrollBar = NULL;
	fInline = NULL;
	fSelectForeColor = kWhiteColor;
	fSelectBackColor = kBlackColor;
	fScrollOffset = 0;
	fLastSyncTime = 0;
	fScrolledSinceLastSync = 0;
	fSyncRunner = NULL;
	fConsiderClockedSync = false;
	fSelStart = TermPos(-1, -1);
	fSelEnd = TermPos(-1, -1);
	fMouseTracking = false;
	fCheckMouseTracking = false;
	fPrevPos = TermPos(-1, - 1);
	fReportX10MouseEvent = false;
	fReportNormalMouseEvent = false;
	fReportButtonMouseEvent = false;
	fReportAnyMouseEvent = false;
	fMouseClipboard = be_clipboard;

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
	
	const BCharacterSet* charset
		= BCharacterSetRoster::GetCharacterSetByConversionID(fEncoding);
	modifiedShellParameters.SetEncoding(charset ? charset->GetName() : "UTF-8");

	error = fShell->Open(fRows, fColumns, modifiedShellParameters);

	if (error < B_OK)
		return error;

	error = _AttachShell(fShell);
	if (error < B_OK)
		return error;

	SetLowColor(kTermColorTable[8]);
	SetViewColor(B_TRANSPARENT_32_BIT);

	return B_OK;
}


TermView::~TermView()
{
	Shell* shell = fShell;
		// _DetachShell sets fShell to NULL

	_DetachShell();

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
TermView::GetFontSize(int* _width, int* _height)
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
TermView::SetTermSize(int rows, int columns)
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
		BAutolock _(fTextBuffer);

		_SynchronizeWithTextBuffer(0, -1);
		int32 offset = _LineAt(0);
		fVisibleTextBuffer->SynchronizeWith(fTextBuffer, offset, offset,
			offset + rows + 2);
	}

	return rect;
}


void
TermView::SetTermSize(BRect rect)
{
	int rows;
	int columns;

	GetTermSizeFromRect(rect, &rows, &columns);
	SetTermSize(rows, columns);
}


void
TermView::GetTermSizeFromRect(const BRect &rect, int *_rows,
	int *_columns)
{
	int columns = (rect.IntegerWidth() + 1) / fFontWidth;
	int rows = (rect.IntegerHeight() + 1) / fFontHeight;

	if (_rows)
		*_rows = rows;
	if (_columns)
		*_columns = columns;
}


void
TermView::SetTextColor(rgb_color fore, rgb_color back)
{
	kTermColorTable[0] = back;
	kTermColorTable[7] = fore;

	SetLowColor(back);
}


void
TermView::SetSelectColor(rgb_color fore, rgb_color back)
{
	fSelectForeColor = fore;
	fSelectBackColor = back;
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

	if (fCursorBlinkRunner == NULL) {
		BMessage blinkMessage(kBlinkCursor);
		fCursorBlinkRunner = new (std::nothrow) BMessageRunner(
			BMessenger(this), &blinkMessage, kCursorBlinkInterval);
	}
}


void
TermView::_Deactivate()
{
	// make sure the cursor becomes visible
	fCursorState = 0;
	_InvalidateTextRect(fCursor.x, fCursor.y, fCursor.x, fCursor.y);
	delete fCursorBlinkRunner;
	fCursorBlinkRunner = NULL;

	fActive = false;
}


//! Draw part of a line in the given view.
void
TermView::_DrawLinePart(int32 x1, int32 y1, uint32 attr, char *buf,
	int32 width, bool mouse, bool cursor, BView *inView)
{
	rgb_color rgb_fore, rgb_back;

	inView->SetFont(&fHalfFont);

	// Set pen point
	int x2 = x1 + fFontWidth * width;
	int y2 = y1 + fFontHeight;

	// color attribute
	int forecolor = IS_FORECOLOR(attr);
	int backcolor = IS_BACKCOLOR(attr);
	rgb_fore = kTermColorTable[forecolor];
	rgb_back = kTermColorTable[backcolor];

	// Selection check.
	if (cursor) {
		rgb_fore.red = 255 - rgb_fore.red;
		rgb_fore.green = 255 - rgb_fore.green;
		rgb_fore.blue = 255 - rgb_fore.blue;

		rgb_back.red = 255 - rgb_back.red;
		rgb_back.green = 255 - rgb_back.green;
		rgb_back.blue = 255 - rgb_back.blue;
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
	uint32 attr = 0;

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
		else {
			rgb_color color = kTermColorTable[IS_BACKCOLOR(attr)];
			if (cursorVisible) {
				color.red = 255 - color.red;
				color.green = 255 - color.green;
				color.blue = 255 - color.blue;
			}
			SetHighColor(color);
		}

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

	// update the terminal size because it may have changed while the TermView
	// was detached from the window. On such conditions FrameResized was not
	// called when the resize occured
	int rows;
	int columns;
	GetTermSizeFromRect(Bounds(), &rows, &columns);
	SetTermSize(rows, columns);
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
		BAutolock _(fTextBuffer);
		fTextBuffer->SetListener(thisMessenger);
		_SynchronizeWithTextBuffer(0, -1);
	}

	be_clipboard->StartWatching(thisMessenger);
}


void
TermView::DetachedFromWindow()
{
	be_clipboard->StopWatching(BMessenger(this));

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
	int32 x1 = (int32)updateRect.left / fFontWidth;
	int32 x2 = std::min((int)updateRect.right / fFontWidth, fColumns - 1);

	int32 firstVisible = _LineAt(0);
	int32 y1 = _LineAt(updateRect.top);
	int32 y2 = std::min(_LineAt(updateRect.bottom), (int32)fRows - 1);

	// clear the area to the right of the line ends
	if (y1 <= y2) {
		float clearLeft = fColumns * fFontWidth;
		if (clearLeft <= updateRect.right) {
			BRect rect(clearLeft, updateRect.top, updateRect.right,
				updateRect.bottom);
			SetHighColor(kTermColorTable[0]);
			FillRect(rect);
		}
	}

	// clear the area below the last line
	if (y2 == fRows - 1) {
		float clearTop = _LineOffset(fRows);
		if (clearTop <= updateRect.bottom) {
			BRect rect(updateRect.left, clearTop, updateRect.right,
				updateRect.bottom);
			SetHighColor(kTermColorTable[0]);
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
				bool insideSelection = _CheckSelectedRegion(j, i, lastColumn);
					// This will clip lastColumn to the selection start or end
					// to ensure the selection is not drawn at the same time as
					// something else
				int32 count = fVisibleTextBuffer->GetString(j - firstVisible, i,
					lastColumn, buf, attr);

// debug_printf("  fVisibleTextBuffer->GetString(%ld, %ld, %ld) -> (%ld, \"%.*s\"), selected: %d\n",
// j - firstVisible, i, lastColumn, count, (int)count, buf, insideSelection);

				if (count == 0) {
					// No chars to draw : we just fill the rectangle with the
					// back color of the last char at the left
					BRect rect(fFontWidth * i, _LineOffset(j),
						fFontWidth * (lastColumn + 1) - 1, 0);
					rect.bottom = rect.top + fFontHeight - 1;

					if (insideSelection) {
						// This area is selected, fill it with the select color
						SetHighColor(fSelectBackColor);
						FillRect(rect);
					} else {
						int lineIndexInHistory = j + fTextBuffer->HistorySize();
						uint32 backcolor = IS_BACKCOLOR(fVisibleTextBuffer->GetLineColor(
							lineIndexInHistory));
						rgb_color rgb_back = kTermColorTable[backcolor];
						SetHighColor(rgb_back);
						FillRect(rect);
					}

					// Go on to the next block
					i = lastColumn + 1;
					continue;
				}

				if (IS_WIDTH(attr))
					count = 2;

				_DrawLinePart(fFontWidth * i, (int32)_LineOffset(j),
					attr, buf, count, insideSelection, false, this);
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
	int32 key, mod, rawChar;
	BMessage *currentMessage = Looper()->CurrentMessage();
	if (currentMessage == NULL)
		return;

	currentMessage->FindInt32("modifiers", &mod);
	currentMessage->FindInt32("key", &key);
	currentMessage->FindInt32("raw_char", &rawChar);

	_ActivateCursor(true);

	// handle multi-byte chars
	if (numBytes > 1) {
		if (fEncoding != M_UTF8) {
			char destBuffer[16];
			int32 destLen = sizeof(destBuffer);
			int32 state = 0;
			convert_from_utf8(fEncoding, bytes, &numBytes, destBuffer,
				&destLen, &state, '?');
			_ScrollTo(0, true);
			fShell->Write(destBuffer, destLen);
			return;
		}

		_ScrollTo(0, true);
		fShell->Write(bytes, numBytes);
		return;
	}

	// Terminal filters RET, ENTER, F1...F12, and ARROW key code.
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
			// Translate only the actual backspace key to the backspace
			// code. CTRL-H shall just be echoed.
			if (!((mod & B_CONTROL_KEY) && rawChar == 'h'))
				toWrite = BACKSPACE_KEY_CODE;
			break;

		case B_LEFT_ARROW:
			if (rawChar == B_LEFT_ARROW) {
				if ((mod & B_SHIFT_KEY) != 0) {
					if (fListener != NULL)
						fListener->PreviousTermView(this);
					return;
				}
				if ((mod & B_CONTROL_KEY) || (mod & B_COMMAND_KEY))
					toWrite = CTRL_LEFT_ARROW_KEY_CODE;
				else
					toWrite = LEFT_ARROW_KEY_CODE;
			}
			break;

		case B_RIGHT_ARROW:
			if (rawChar == B_RIGHT_ARROW) {
				if ((mod & B_SHIFT_KEY) != 0) {
					if (fListener != NULL)
						fListener->NextTermView(this);
					return;
				}
				if ((mod & B_CONTROL_KEY) || (mod & B_COMMAND_KEY))
					toWrite = CTRL_RIGHT_ARROW_KEY_CODE;
				else
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
				_ScrollTo(fScrollOffset - fFontHeight  * fRows, true);
				return;
			}
			if (rawChar == B_PAGE_UP)
				toWrite = PAGE_UP_KEY_CODE;
			break;

		case B_PAGE_DOWN:
			if (mod & B_SHIFT_KEY) {
				_ScrollTo(fScrollOffset + fFontHeight * fRows, true);
				return;
			}
			if (rawChar == B_PAGE_DOWN)
				toWrite = PAGE_DOWN_KEY_CODE;
			break;

		case B_FUNCTION_KEY:
			for (int32 i = 0; i < 12; i++) {
				if (key == function_keycode_table[i]) {
					toWrite = function_key_char_table[i];
					break;
				}
			}
			break;
	}

	// If the above code proposed an alternative string to write, we get it's
	// length. Otherwise we write exactly the bytes passed to this method.
	size_t toWriteLen;
	if (toWrite != NULL) {
		toWriteLen = strlen(toWrite);
	} else {
		toWrite = bytes;
		toWriteLen = numBytes;
	}

	_ScrollTo(0, true);
	fShell->Write(toWrite, toWriteLen);
}


void
TermView::FrameResized(float width, float height)
{
//debug_printf("TermView::FrameResized(%f, %f)\n", width, height);
	int32 columns = ((int32)width + 1) / fFontWidth;
	int32 rows = ((int32)height + 1) / fFontHeight;

	if (columns == fColumns && rows == fRows)
		return;

	bool hasResizeView = fResizeRunner != NULL;
	if (!hasResizeView) {
		// show the current size in a view
		fResizeView = new BStringView(BRect(100, 100, 300, 140),
			B_TRANSLATE("size"), "");
		fResizeView->SetAlignment(B_ALIGN_CENTER);
		fResizeView->SetFont(be_bold_font);

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

	SetTermSize(rows, columns);

	fFrameResized = true;
}


void
TermView::MessageReceived(BMessage *msg)
{
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

	switch (msg->what){
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
				&& !strcmp("encoding", specifier.FindString("property", i))) {
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
				&& !strcmp("encoding", specifier.FindString("property", i))) {
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
				if ((modifiers()
						& (B_OPTION_KEY | B_COMMAND_KEY | B_CONTROL_KEY))
						!= 0) {
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
		case kAutoScroll:
			_AutoScrollUpdate();
			break;
		case kSecondaryMouseDropAction:
			_DoSecondaryMouseDropAction(msg);
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
			if (msg->FindString("title", &title) == B_OK) {
				if (fListener != NULL)
					fListener->SetTermViewTitle(this, title);
			}
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
		BAutolock _(fTextBuffer);
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
			string.CharacterEscape(kEscapeCharacters, '\\');
			itemString += string;
			break;
		}
		string.CharacterEscape(kEscapeCharacters, '\\');
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

	string.CharacterEscape(kEscapeCharacters, '\\');
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
	} else if (fScrolledSinceLastSync + linesScrolled <= fRows) {
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

	int32 buttons;
	int32 modifier;
	Window()->CurrentMessage()->FindInt32("buttons", &buttons);
	Window()->CurrentMessage()->FindInt32("modifiers", &modifier);

	fMouseButtons = buttons;

	if (fReportAnyMouseEvent || fReportButtonMouseEvent
		|| fReportNormalMouseEvent || fReportX10MouseEvent) {
  		TermPos clickPos = _ConvertToTerminal(where);
  		_SendMouseEvent(buttons, modifier, clickPos.x, clickPos.y, false);
		return;
	}

	// paste button
	if ((buttons & (B_SECONDARY_MOUSE_BUTTON | B_TERTIARY_MOUSE_BUTTON)) != 0) {
		Paste(fMouseClipboard);
		fLastClickPoint = where;
		return;
	}

	// Select Region
	if (buttons == B_PRIMARY_MOUSE_BUTTON) {
		int32 clicks;
		Window()->CurrentMessage()->FindInt32("clicks", &clicks);

		if (_HasSelection()) {
			TermPos inPos = _ConvertToTerminal(where);
			if (_CheckSelectedRegion(inPos)) {
				if (modifier & B_CONTROL_KEY) {
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

		// If mouse has moved too much, disable double/triple click.
		if (_MouseDistanceSinceLastClick(where) > 8)
			clicks = 1;

		SetMouseEventMask(B_POINTER_EVENTS | B_KEYBOARD_EVENTS,
			B_NO_POINTER_HISTORY | B_LOCK_WINDOW_FOCUS);

		TermPos clickPos = _ConvertToTerminal(where);

		if (modifier & B_SHIFT_KEY) {
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
				fCheckMouseTracking = true;
				fSelectGranularity = SELECT_CHARS;
				break;

			case 2:
				_SelectWord(where, (modifier & B_SHIFT_KEY) != 0, false);
				fMouseTracking = true;
				fSelectGranularity = SELECT_WORDS;
				break;

			case 3:
				_SelectLine(where, (modifier & B_SHIFT_KEY) != 0, false);
				fMouseTracking = true;
				fSelectGranularity = SELECT_LINES;
				break;
		}
	}
	fLastClickPoint = where;
}


void
TermView::MouseMoved(BPoint where, uint32 transit, const BMessage *message)
{
	if (fReportAnyMouseEvent || fReportButtonMouseEvent) {
		int32 modifier;
		Window()->CurrentMessage()->FindInt32("modifiers", &modifier);

  		TermPos clickPos = _ConvertToTerminal(where);

  		if (fReportButtonMouseEvent) {
  			if (fPrevPos.x != clickPos.x || fPrevPos.y != clickPos.y) {
		  		_SendMouseEvent(fMouseButtons, modifier, clickPos.x, clickPos.y,
					true);
  			}
  			fPrevPos = clickPos;
  			return;
  		}
  		_SendMouseEvent(fMouseButtons, modifier, clickPos.x, clickPos.y, true);
		return;
	}

	if (fCheckMouseTracking) {
		if (_MouseDistanceSinceLastClick(where) > 9)
			fMouseTracking = true;
	}
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
		{
			// If we just start selecting, we first select the initially
			// hit char, so that we get a proper initial selection -- the char
			// in question, which will thus always be selected, regardless of
			// whether selecting forward or backward.
			if (fInitialSelectionStart == fInitialSelectionEnd) {
				_Select(fInitialSelectionStart, fInitialSelectionEnd, true,
					true);
			}

			_ExtendSelection(_ConvertToTerminal(where), true, true);
			break;
		}
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
	fCheckMouseTracking = false;
	fMouseTracking = false;

	if (fAutoScrollRunner != NULL) {
		delete fAutoScrollRunner;
		fAutoScrollRunner = NULL;
	}

	// When releasing the first mouse button, we copy the selected text to the
	// clipboard.
	int32 buttons;
	Window()->CurrentMessage()->FindInt32("buttons", &buttons);

	if (fReportAnyMouseEvent || fReportButtonMouseEvent
		|| fReportNormalMouseEvent) {
	  	TermPos clickPos = _ConvertToTerminal(where);
	  	_SendMouseEvent(0, 0, clickPos.x, clickPos.y, false);
	} else {
		if ((buttons & B_PRIMARY_MOUSE_BUTTON) == 0
			&& (fMouseButtons & B_PRIMARY_MOUSE_BUTTON) != 0) {
			Copy(fMouseClipboard);
		}

	}
	fMouseButtons = buttons;
}


//! Select a range of text.
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


//! Extend selection (shift + mouse click).
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
	else if (useInitialSelection)
		_Select(start, end, false, false);
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
		else if (useInitialSelection)
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
		if (start < (useInitialSelection ? fInitialSelectionStart : fSelStart))
			_ExtendSelection(start, false, useInitialSelection);
		else if (end > (useInitialSelection ? fInitialSelectionEnd : fSelEnd))
			_ExtendSelection(end, false, useInitialSelection);
		else if (useInitialSelection)
			_Select(start, end, false, false);
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
		*width = fColumns * fFontWidth;

	if (height != NULL)
		*height = (fRows + historySize) * fFontHeight;
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
	SetHighColor(kTermColorTable[7]);
	FillRect(eraseRect);
	PopState();

	BPoint loc = _ConvertFromTerminal(fCursor);
	loc.y += fFontHeight;
	SetFont(&fHalfFont);
	SetHighColor(kTermColorTable[0]);
	SetLowColor(kTermColorTable[7]);
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
