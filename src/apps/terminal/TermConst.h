/*
 * Copyright 2001-2009, Haiku.
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Copyright (C) 1998,99 Kazuho Okui and Takashi Murai.
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


// Menu Message
const uint32 MENU_SWITCH_TERM   = 'MSWT';
const uint32 MENU_NEW_TERM      = 'MNTE';
const uint32 MENU_PREF_OPEN     = 'MPre';
const uint32 MENU_CLEAR_ALL     = 'MCAl';
const uint32 MENU_HELP          = 'Mhlp';
const uint32 MENU_COMMAND_HELP  = 'Mchp';
const uint32 MENU_SHOW_GPL      = 'Mgpl';
const uint32 MENU_ENCODING      = 'Menc';
const uint32 MENU_PAGE_SETUP    = 'Mpst';
const uint32 MENU_PRINT         = 'Mpnt';
const uint32 MENU_FIND_STRING   = 'Mfpr';
const uint32 MENU_FIND_NEXT  	= 'Mfnx';
const uint32 MENU_FIND_PREVIOUS = 'Mfbw';
const uint32 MENU_SHOW_COLOR	= 'Mcol';

const uint32 M_GET_DEVICE_NUM	= 'Mgdn';


// Preference Message
const ulong PSET__COLS							= 'pcol';
const ulong PSET__ROWS							= 'prow';
const ulong PSET__HFONT							= 'phfn';
const ulong PSET__HFONT_SIZE					= 'phfs';
const ulong PSET_FORE_COLOR						= 'pfcl';
const ulong PSET_BACK_COLOR						= 'pbcl';
const ulong PSET__CODING						= 'pcod';

// Terminal Size Messages
const uint32 EIGHTYTWENTYFOUR					= 'etfo';
const uint32 EIGHTYTWENTYFIVE					= 'etfv';
const uint32 EIGHTYFORTY						= 'efor';
const uint32 ONETHREETWOTWENTYFOUR				= 'hunf';
const uint32 ONETHREETWOTWENTYFIVE				= 'hunv';
const uint32 FULLSCREEN							= 'fscr';

const uint32 MSG_FONT_CHANGED					= 'fntc';
const uint32 SAVE_AS_DEFAULT 					= 'sadf';
const uint32 MSG_CHECK_CHILDREN					= 'ckch';
const uint32 MSG_PREVIOUS_TAB					= 'ptab';
const uint32 MSG_NEXT_TAB						= 'ntab';
const uint32 MSG_REMOVE_RESIZE_VIEW_IF_NEEDED	= 'rmrv';
const uint32 MSG_TERMINAL_BUFFER_CHANGED		= 'bufc';
const uint32 MSG_SET_TERMNAL_TITLE				= 'sett';
const uint32 MSG_QUIT_TERMNAL					= 'qutt';
const uint32 MSG_REPORT_MOUSE_EVENT				= 'mous';
const uint32 MSG_SAVE_WINDOW_POSITION			= 'swps';

// Preference Read/Write Keys
const char* const PREF_HALF_FONT_FAMILY = "Half Font Family";
const char* const PREF_HALF_FONT_STYLE = "Half Font Style";
const char* const PREF_HALF_FONT_SIZE = "Half Font Size";

const char* const PREF_TEXT_FORE_COLOR = "Text";
const char* const PREF_TEXT_BACK_COLOR = "Background";
const char* const PREF_SELECT_FORE_COLOR = "Selected text";
const char* const PREF_SELECT_BACK_COLOR = "Selected background";

const char* const PREF_IM_FORE_COLOR = "IM foreground color";
const char* const PREF_IM_BACK_COLOR = "IM background color";
const char* const PREF_IM_SELECT_COLOR = "IM selection color";

const char* const PREF_ANSI_BLACK_COLOR = "ANSI black color";
const char* const PREF_ANSI_RED_COLOR = "ANSI red color";
const char* const PREF_ANSI_GREEN_COLOR = "ANSI green color";
const char* const PREF_ANSI_YELLOW_COLOR = "ANSI yellow color";
const char* const PREF_ANSI_BLUE_COLOR = "ANSI blue color";
const char* const PREF_ANSI_MAGENTA_COLOR = "ANSI magenta color";
const char* const PREF_ANSI_CYAN_COLOR = "ANSI cyan color";
const char* const PREF_ANSI_WHITE_COLOR = "ANSI white color";

const char* const PREF_HISTORY_SIZE = "History size";
const char* const PREF_CURSOR_BLINKING = "Cursor blinking rate";

const char* const PREF_IM_AWARE = "Input method aware";

const char* const PREF_COLS = "Cols";
const char* const PREF_ROWS = "Rows";
const char* const PREF_SHELL = "Shell";

const char* const PREF_TEXT_ENCODING = "Text encoding";
const char* const PREF_GUI_LANGUAGE = "Language";

const char* const PREF_WARN_ON_EXIT = "Warn on exit";

// Color type
enum {
  TEXT_FOREGROUND_COLOR,
  TEXT_BACKGROUND_COLOR,
  SELECTION_FOREGROUND_COLOR,
  SELECTION_BACKGROUND_COLOR
};

// Preference Folder and setting path

const int32 DEFAULT = -1;

// Font Width
const int  HALF_WIDTH = 1;
const int  FULL_WIDTH = 2;

// Scroll direction flag
enum{
  SCRUP,
  SCRDOWN
};

#define M_UTF8 -1

#define TAB_WIDTH 8

#define MIN_COLS 10
#define MAX_COLS 256
#define MIN_ROWS 10
#define MAX_ROWS 256

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
