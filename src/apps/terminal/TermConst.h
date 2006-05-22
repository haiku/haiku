/*
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

#ifndef NODEBUG
#define MALLOC_DEBUG
#endif

//////////////////////////////////////////////////////////////////////////////
// Application signature (Must same in Muterminal.rsrc)//
//////////////////////////////////////////////////////////////////////////////
#define TERM_SIGNATURE "application/x-vnd.Haiku-Terminal"
#define PREFFILE_MIMETYPE "text/x-terminal-pref"

//////////////////////////////////////////////////////////////////////////////
// Signature of R5's Terminal. Needed for proper drop-in window count support
//////////////////////////////////////////////////////////////////////////////
#define R5_TERM_SIGNATURE "application/x-vnd.Be-SHEL"

//////////////////////////////////////////////////////////////////////////////
// Message constants for menu items.
//////////////////////////////////////////////////////////////////////////////

// Menu Message.
//
const uint32 MENU_SWITCH_TERM   = 'MSWT';
const uint32 MENU_NEW_TREM      = 'MNTE';
const uint32 MENU_PREF_OPEN     = 'MPre';
const uint32 MENU_FILE_QUIT     = 'MFqu';
const uint32 MENU_CLEAR_ALL     = 'MCAl';
const uint32 MENU_HELP          = 'Mhlp';
const uint32 MENU_COMMAND_HELP  = 'Mchp';
const uint32 MENU_SHOW_GPL      = 'Mgpl';
const uint32 MENU_ENCODING      = 'Menc';
const uint32 MENU_PAGE_SETUP    = 'Mpst';
const uint32 MENU_PRINT         = 'Mpnt';
const uint32 MENU_FIND_STRING   = 'Mfnd';
const uint32 MENU_FIND_AGAIN    = 'Mfag';
const uint32 MENU_SHOW_COLOR	= 'Mcol';

const uint32 M_GET_DEVICE_NUM	= 'Mgdn';

// Message Runner.

const uint32 MSGRUN_CURSOR	= 'Rcur';
const uint32 MSGRUN_WINDOW	= 'Rwin';

// Preference Message.
//
const ulong PSET__COLS		= 'pcol';
const ulong PSET__ROWS		= 'prow';
const ulong PSET__HFONT		= 'phfn';
const ulong PSET__FFONT		= 'pffn';
const ulong PSET__HFONT_SIZE	= 'phfs';
const ulong PSET__FFONT_SIZE	= 'pffs';
const ulong PSET_FORE_COLOR	= 'pfcl';
const ulong PSET_BACK_COLOR	= 'pbcl';
const ulong PSET__CODING	= 'pcod';

// Terminal Size Messages
const uint32 EIGHTYTWENTYFOUR 		= 'etfo';
const uint32 EIGHTYTWENTYFIVE 		= 'etfv';
const uint32 EIGHTYFORTY 			= 'efor';
const uint32 ONETHREETWOTWENTYFOUR 	= 'hunf';
const uint32 ONETHREETWOTWENTYFIVE 	= 'hunv';
const uint32 FULLSCREEN			= 'fscr';

const uint32 MSG_FONT_CHANGED		= 'fntc';

const uint32 SAVE_AS_DEFAULT 		= 'sadf';

////////////////////////////////////////////////////////////////////////////
// Preference Read/Write Keys
////////////////////////////////////////////////////////////////////////////
const char * const PREF_HALF_FONT_FAMILY = "Half Font Famly";
const char * const PREF_HALF_FONT_STYLE = "Half Font Style";
const char * const PREF_HALF_FONT_SIZE = "Half Font Size";

const char * const PREF_FULL_FONT_FAMILY = "Full Font Famly";
const char * const PREF_FULL_FONT_STYLE = "Full Font Style";
const char * const PREF_FULL_FONT_SIZE = "Full Font Size";

const char * const PREF_TEXT_FORE_COLOR = "Text Foreground Color";
const char * const PREF_TEXT_BACK_COLOR = "Text Background Color";
const char * const PREF_SELECT_FORE_COLOR = "Selection Foreground Color";
const char * const PREF_SELECT_BACK_COLOR = "Selection Background Color";
const char * const PREF_CURSOR_FORE_COLOR = "Cursor Foreground Color";
const char * const PREF_CURSOR_BACK_COLOR = "Cursor Background Color";

const char * const PREF_IM_FORE_COLOR = "IM Foreground Color";
const char * const PREF_IM_BACK_COLOR = "IM Background Color";
const char * const PREF_IM_SELECT_COLOR = "IM Selection Color";
const char * const PREF_DRAGN_COPY = "Drag'n Copy";

const char * const PREF_HISTORY_SIZE = "Histry Size";
const char * const PREF_CURSOR_BLINKING = "Cursor Blinking rate";

const char * const PREF_IM_AWARE = "Input Method Aware";
const char * const PREF_MOUSE_IMAGE = "Cursor Image";

const char * const PREF_SELECT_MBUTTON = "Select Region Buttion";
const char * const PREF_SUBMENU_MBUTTON = "Submenu Mouse Button";
const char * const PREF_PASTE_MBUTTON = "Paste Mouse Button";

const char * const PREF_COLS = "Cols";
const char * const PREF_ROWS = "Rows";
const char * const PREF_SHELL = "Shell";

const char * const PREF_TEXT_ENCODING = "Text encoding";
const char * const PREF_GUI_LANGUAGE = "Language";

////////////////////////////////////////////////////////////////////////////
// Color type
////////////////////////////////////////////////////////////////////////////
enum {
  TEXT_FOREGROUND_COLOR,
  TEXT_BACKGROUND_COLOR,
  SELECTION_FOREGROUND_COLOR,
  SELECTION_BACKGROUND_COLOR
};

////////////////////////////////////////////////////////////////////////////
// Preference Folder and setting path
////////////////////////////////////////////////////////////////////////////

const int32 DEFAULT = -1;

////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////
/* Font Width */
const int  HALF_WIDTH = 1;
const int  FULL_WIDTH = 2;

/* Scroll direction flag */
enum{
  SCRUP,
  SCRDOWN
};

/* Insert mode flag */
#define MODE_OVER 0
#define MODE_INSERT 1

const float VIEW_OFFSET = 3;

#define CURSOR_RECT \
	BRect r (fFontWidth * fCurPos.x, \
		 fFontHeight * fCurPos.y + fTop, \
		 fFontWidth * (fCurPos.x + 1) - 1, \
		 fFontHeight * fCurPos.y + fTop + fCursorHeight - 1)

////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////
/* Define TermBuffer internal code */
#define NO_CHAR 0x00
#define A_CHAR 0x01
#define IN_STRING 0xFF

/* TermBuffer extended attribute */
#define A_WIDTH		0x8000
#define BOLD		0x4000
#define UNDERLINE	0x2000
#define INVERSE		0x1000
#define MOUSE		0x0800
#define FORESET		0x0400
#define BACKSET		0x0200
#define FONT		0x0100
#define RESERVE		0x0080
#define DUMPCR		0x0040
#define FORECOLOR	0x0038
#define BACKCOLOR	0x0007

#define IS_WIDTH(x)	(((x) & A_WIDTH)   )
#define IS_BOLD(x)	(((x) & BOLD)      )
#define IS_UNDER(x)	(((x) & UNDERLINE) )
#define IS_INVERSE(x)	(((x) & INVERSE)   )
#define IS_MOUSE(x)	(((x) & MOUSE)     )
#define IS_FORESET(x)	(((x) & FORESET)   )
#define IS_BACKSET(x)	(((x) & BACKSET)   )
#define IS_FONT(x)	(((x) & FONT)      )
#define IS_CR(x)	(((x) & DUMPCR)	   )
#define IS_FORECOLOR(x) (((x) & FORECOLOR) >> 3)
#define IS_BACKCOLOR(x) (((x) & BACKCOLOR) )

#define FORECOLORED(x) ((x) << 3)
#define BACKCOLORED(x) ((x) << 0)

////////////////////////////////////////////////////////////////////////////
// TypeDefs
////////////////////////////////////////////////////////////////////////////
typedef unsigned short ushort;

typedef struct
{
  int x1;
  int y1;
  int x2;
  int y2;
}
sDrawRect;
  


#endif //TERMCONST_H_INCLUDED
