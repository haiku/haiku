/*
 * Copyright 2001-2015, Haiku.
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Copyright (C) 1998,99 Kazuho Okui and Takashi Murai.
 *
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Kian Duffy, myob@users.sourceforge.net
 *		Simon South, simon@simonsouth.net
 *		Siarzhuk Zharski, zharik@gmx.li
 */
#ifndef TERMCONST_H_INCLUDED
#define TERMCONST_H_INCLUDED

// Application signature (Must same in Terminal.rdef)
#define TERM_SIGNATURE "application/x-vnd.Haiku-Terminal"
#define PREFFILE_MIMETYPE "text/x-terminal-pref"

// Signature of R5's Terminal. Needed for proper drop-in window count support
#define R5_TERM_SIGNATURE "application/x-vnd.Be-SHEL"

// Name of the clipboard used for mouse copy'n'paste.
#define MOUSE_CLIPBOARD_NAME TERM_SIGNATURE "/mouse"

// Message constants for menu items

#include <SupportDefs.h>


// define to get Ctrl-Cmd-S / Ctrl-Cmd-C shortcuts
// to get the debug buffers snapshots and control
// sequences capture logging
#define USE_DEBUG_SNAPSHOTS
#ifdef USE_DEBUG_SNAPSHOTS
const uint32 SHORTCUT_DEBUG_SNAPSHOTS	= 'sdbs';
const uint32 SHORTCUT_DEBUG_CAPTURE		= 'srdc';
#endif

// Menu Message
static const uint32 MENU_SWITCH_TERM	= 'MSWT';
static const uint32 MENU_NEW_TERM		= 'MNTE';
static const uint32 MENU_PREF_OPEN		= 'MPre';
static const uint32 MENU_CLEAR_ALL		= 'MCAl';
static const uint32 MENU_HELP			= 'Mhlp';
static const uint32 MENU_COMMAND_HELP	= 'Mchp';
static const uint32 MENU_SHOW_GPL		= 'Mgpl';
static const uint32 MENU_ENCODING		= 'Menc';
static const uint32 MENU_PAGE_SETUP		= 'Mpst';
static const uint32 MENU_PRINT			= 'Mpnt';
static const uint32 MENU_FIND_STRING	= 'Mfpr';
static const uint32 MENU_FIND_NEXT		= 'Mfnx';
static const uint32 MENU_FIND_PREVIOUS	= 'Mfbw';
static const uint32 MENU_SHOW_COLOR		= 'Mcol';

static const uint32 M_GET_DEVICE_NUM	= 'Mgdn';


// Preference Message
static const ulong PSET__COLS							= 'pcol';
static const ulong PSET__ROWS							= 'prow';
static const ulong PSET__HFONT							= 'phfn';
static const ulong PSET__HFONT_SIZE						= 'phfs';
static const ulong PSET_FORE_COLOR						= 'pfcl';
static const ulong PSET_BACK_COLOR						= 'pbcl';
static const ulong PSET__CODING							= 'pcod';

// Terminal Size Messages
static const uint32 EIGHTYTWENTYFIVE					= 'etfv';
static const uint32 EIGHTYFORTY							= 'efor';
static const uint32 ONETHREETWOTWENTYFIVE				= 'hunv';
static const uint32 FULLSCREEN							= 'fscr';

static const uint32 MSG_FONT_CHANGED					= 'fntc';
static const uint32 MSG_SAVE_AS_DEFAULT					= 'sadf';
static const uint32 MSG_CHECK_CHILDREN					= 'ckch';
static const uint32 MSG_REMOVE_RESIZE_VIEW_IF_NEEDED	= 'rmrv';
static const uint32 MSG_TERMINAL_BUFFER_CHANGED			= 'bufc';
static const uint32 MSG_SET_TERMINAL_TITLE				= 'sett';
static const uint32 MSG_SET_TERMINAL_COLORS				= 'setc';
static const uint32 MSG_RESET_TERMINAL_COLORS			= 'rstc';
static const uint32 MSG_QUIT_TERMNAL					= 'qutt';
static const uint32 MSG_ENABLE_META_KEY					= 'emtk';
static const uint32 MSG_REPORT_MOUSE_EVENT				= 'mous';
static const uint32 MSG_SAVE_WINDOW_POSITION			= 'swps';
static const uint32 MSG_MOVE_TAB_LEFT					= 'mvtl';
static const uint32 MSG_MOVE_TAB_RIGHT					= 'mvtr';
static const uint32 MSG_ACTIVATE_TERM					= 'msat';
static const uint32 MSG_SET_CURSOR_STYLE				= 'mscs';


// Preference Read/Write Keys
static const char* const PREF_HALF_FONT_FAMILY = "Half Font Family";
static const char* const PREF_HALF_FONT_STYLE = "Half Font Style";
static const char* const PREF_HALF_FONT_SIZE = "Half Font Size";

static const char* const PREF_TEXT_FORE_COLOR = "Text";
static const char* const PREF_TEXT_BACK_COLOR = "Background";
static const char* const PREF_CURSOR_FORE_COLOR = "Text under cursor";
static const char* const PREF_CURSOR_BACK_COLOR = "Cursor";
static const char* const PREF_SELECT_FORE_COLOR = "Selected text";
static const char* const PREF_SELECT_BACK_COLOR = "Selected background";

static const char* const PREF_IM_FORE_COLOR = "IM foreground color";
static const char* const PREF_IM_BACK_COLOR = "IM background color";
static const char* const PREF_IM_SELECT_COLOR = "IM selection color";

static const char* const PREF_ANSI_BLACK_COLOR = "ANSI black color";
static const char* const PREF_ANSI_RED_COLOR = "ANSI red color";
static const char* const PREF_ANSI_GREEN_COLOR = "ANSI green color";
static const char* const PREF_ANSI_YELLOW_COLOR = "ANSI yellow color";
static const char* const PREF_ANSI_BLUE_COLOR = "ANSI blue color";
static const char* const PREF_ANSI_MAGENTA_COLOR = "ANSI magenta color";
static const char* const PREF_ANSI_CYAN_COLOR = "ANSI cyan color";
static const char* const PREF_ANSI_WHITE_COLOR = "ANSI white color";

static const char* const PREF_ANSI_BLACK_HCOLOR = "ANSI bright black color";
static const char* const PREF_ANSI_RED_HCOLOR = "ANSI bright red color";
static const char* const PREF_ANSI_GREEN_HCOLOR = "ANSI bright green color";
static const char* const PREF_ANSI_YELLOW_HCOLOR = "ANSI bright yellow color";
static const char* const PREF_ANSI_BLUE_HCOLOR = "ANSI bright blue color";
static const char* const PREF_ANSI_MAGENTA_HCOLOR = "ANSI bright magenta color";
static const char* const PREF_ANSI_CYAN_HCOLOR = "ANSI bright cyan color";
static const char* const PREF_ANSI_WHITE_HCOLOR = "ANSI bright white color";

static const char* const PREF_HISTORY_SIZE = "History size";
static const char* const PREF_CURSOR_BLINKING = "Cursor blinking rate";

static const char* const PREF_IM_AWARE = "Input method aware";

static const char* const PREF_COLS = "Cols";
static const char* const PREF_ROWS = "Rows";

static const char* const PREF_TEXT_ENCODING = "Text encoding";

static const char* const PREF_BLINK_CURSOR = "Blinking cursor";
static const char* const PREF_ALLOW_BOLD = "Allow bold text";
static const char* const PREF_USE_OPTION_AS_META =
	"Use left Option as Meta key";
static const char* const PREF_WARN_ON_EXIT = "Warn on exit";
static const char* const PREF_CURSOR_STYLE = "Cursor style";
static const char* const PREF_EMULATE_BOLD = "Emulate bold";

static const char* const PREF_TAB_TITLE = "Tab title";
static const char* const PREF_WINDOW_TITLE = "Window title";

// shared strings
extern const char* const kTooTipSetTabTitlePlaceholders;
extern const char* const kTooTipSetWindowTitlePlaceholders;
extern const char* const kToolTipCommonTitlePlaceholders;

extern const char* const kShellEscapeCharacters;
extern const char* const kDefaultAdditionalWordCharacters;
extern const char* const kURLAdditionalWordCharacters;


// Cursor style
enum {
	BLOCK_CURSOR,
	UNDERLINE_CURSOR,
	IBEAM_CURSOR
};


// Preference Folder and setting path

static const int32 DEFAULT = -1;

// Font Width
enum {
	HALF_WIDTH = 1,
	FULL_WIDTH = 2
};

#define M_UTF8 -1

#define TAB_WIDTH 8

#define MIN_COLS 10
#define MAX_COLS 999
#define MIN_ROWS 10
#define MAX_ROWS 999

// Insert mode flag
#define MODE_OVER 0
#define MODE_INSERT 1

// Define TermBuffer internal code
#define NO_CHAR 0x00
#define A_CHAR 0x01
#define IN_STRING 0xFF

// TermBuffer extended attribute
#define A_WIDTH			0x8000
#define BOLD			0x4000
#define UNDERLINE		0x2000
#define INVERSE			0x1000
#define MOUSE			0x0800
#define FORESET			0x0400
#define BACKSET			0x0200
#define FONT			0x0100
#define RESERVE			0x0080
#define DUMPCR			0x0040
#define FORECOLOR		0xFF0000
#define BACKCOLOR		0xFF000000
#define CHAR_ATTRIBUTES	0xFFFF7700

#define IS_WIDTH(x)	(((x) & A_WIDTH)   )
#define IS_BOLD(x)	(((x) & BOLD)      )
#define IS_UNDER(x)	(((x) & UNDERLINE) )
#define IS_INVERSE(x)	(((x) & INVERSE)   )
#define IS_MOUSE(x)	(((x) & MOUSE)     )
#define IS_FORESET(x)	(((x) & FORESET)   )
#define IS_BACKSET(x)	(((x) & BACKSET)   )
#define IS_FONT(x)	(((x) & FONT)      )
#define IS_CR(x)	(((x) & DUMPCR)	   )
#define IS_FORECOLOR(x) (((x) & FORECOLOR) >> 16)
#define IS_BACKCOLOR(x) (((x) & BACKCOLOR) >> 24)

#define FORECOLORED(x) ((x) << 16)
#define BACKCOLORED(x) ((x) << 24)


#endif	// TERMCONST_H_INCLUDED
