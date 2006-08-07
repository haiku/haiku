/****************************************************************************
 * Copyright (c) 1998-2002,2003 Free Software Foundation, Inc.              *
 *                                                                          *
 * Permission is hereby granted, free of charge, to any person obtaining a  *
 * copy of this software and associated documentation files (the            *
 * "Software"), to deal in the Software without restriction, including      *
 * without limitation the rights to use, copy, modify, merge, publish,      *
 * distribute, distribute with modifications, sublicense, and/or sell       *
 * copies of the Software, and to permit persons to whom the Software is    *
 * furnished to do so, subject to the following conditions:                 *
 *                                                                          *
 * The above copyright notice and this permission notice shall be included  *
 * in all copies or substantial portions of the Software.                   *
 *                                                                          *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS  *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF               *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.   *
 * IN NO EVENT SHALL THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,   *
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR    *
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR    *
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.                               *
 *                                                                          *
 * Except as contained in this notice, the name(s) of the above copyright   *
 * holders shall not be used in advertising or otherwise to promote the     *
 * sale, use or other dealings in this Software without prior written       *
 * authorization.                                                           *
 ****************************************************************************/

/****************************************************************************
 *  Author: Thomas E. Dickey <dickey@clark.net> 1996                        *
 ****************************************************************************/
/* $Id: test.priv.h,v 1.48 2004/04/10 20:11:37 tom Exp $ */

#ifndef __TEST_PRIV_H
#define __TEST_PRIV_H 1

#include <ncurses_cfg.h>

/*
 * Fix ifdef's that look for the form/menu/panel libraries, if we are building
 * with wide-character ncurses.
 */
#ifdef  HAVE_LIBFORMW
#define HAVE_LIBFORM 1
#endif

#ifdef  HAVE_LIBMENUW
#define HAVE_LIBMENU 1
#endif

#ifdef  HAVE_LIBPANELW
#define HAVE_LIBPANEL 1
#endif

/*
 * Fallback definitions to accommodate broken compilers
 */
#ifndef HAVE_CURSES_VERSION
#define HAVE_CURSES_VERSION 0
#endif

#ifndef HAVE_FORM_H
#define HAVE_FORM_H 0
#endif

#ifndef HAVE_LIBFORM
#define HAVE_LIBFORM 0
#endif

#ifndef HAVE_LIBMENU
#define HAVE_LIBMENU 0
#endif

#ifndef HAVE_LIBPANEL
#define HAVE_LIBPANEL 0
#endif

#ifndef HAVE_LOCALE_H
#define HAVE_LOCALE_H 0
#endif

#ifndef HAVE_MENU_H
#define HAVE_MENU_H 0
#endif

#ifndef HAVE_NAPMS
#define HAVE_NAPMS 1
#endif

#ifndef HAVE_NC_ALLOC_H
#define HAVE_NC_ALLOC_H 0
#endif

#ifndef HAVE_PANEL_H
#define HAVE_PANEL_H 0
#endif

#ifndef HAVE_SLK_COLOR
#define HAVE_SLK_COLOR 0
#endif

#ifndef HAVE_WRESIZE
#define HAVE_WRESIZE 0
#endif

#ifndef NCURSES_EXT_FUNCS
#define NCURSES_EXT_FUNCS 0
#endif

#ifndef NCURSES_NOMACROS
#define NCURSES_NOMACROS 0
#endif

#ifndef NEED_PTEM_H
#define NEED_PTEM_H 0
#endif

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <signal.h>	/* include before curses.h to work around glibc bug */

#if defined(HAVE_NCURSESW_NCURSES_H)
#include <ncursesw/curses.h>
#include <ncursesw/term.h>
#elif defined(HAVE_NCURSES_NCURSES_H)
#include <ncurses/curses.h>
#include <ncurses/term.h>
#else
#include <curses.h>
#include <term.h>
#endif

#ifdef NCURSES_VERSION
#define HAVE_COLOR_SET 1
#endif

#if NCURSES_NOMACROS
#include <nomacros.h>
#endif

#if HAVE_GETOPT_H
#include <getopt.h>
#else
/* 'getopt()' may be prototyped in <stdlib.h>, but declaring its variables
 * doesn't hurt.
 */
extern char *optarg;
extern int optind;
#endif /* HAVE_GETOPT_H */

#if HAVE_LOCALE_H
#include <locale.h>
#else
#define setlocale(name,string) /* nothing */
#endif

#include <assert.h>
#include <ctype.h>

#ifndef GCC_NORETURN
#define GCC_NORETURN /* nothing */
#endif
#ifndef GCC_UNUSED
#define GCC_UNUSED /* nothing */
#endif

#ifndef HAVE_GETNSTR
#define getnstr(s,n) getstr(s)
#endif

#ifndef USE_WIDEC_SUPPORT
#if defined(_XOPEN_SOURCE_EXTENDED) && defined(WACS_ULCORNER)
#define USE_WIDEC_SUPPORT 1
#else
#define USE_WIDEC_SUPPORT 0
#endif
#endif

#if HAVE_PANEL_H && HAVE_LIBPANEL
#define USE_LIBPANEL 1
#else
#define USE_LIBPANEL 0
#endif

#if HAVE_MENU_H && HAVE_LIBMENU
#define USE_LIBMENU 1
#else
#define USE_LIBMENU 0
#endif

#if HAVE_FORM_H && HAVE_LIBFORM
#define USE_LIBFORM 1
#else
#define USE_LIBFORM 0
#endif

#ifndef HAVE_TYPE_ATTR_T
#if !USE_WIDEC_SUPPORT
#define attr_t long
#endif
#endif

#undef NCURSES_CH_T
#if !USE_WIDEC_SUPPORT
#define NCURSES_CH_T chtype
#else
#define NCURSES_CH_T cchar_t
#endif

#ifndef CCHARW_MAX
#define CCHARW_MAX 5
#endif

#ifndef CTRL
#define CTRL(x)		((x) & 0x1f)
#endif

#define QUIT		CTRL('Q')
#define ESCAPE		CTRL('[')

#ifndef KEY_MIN
#define KEY_MIN 256	/* not defined in Solaris 8 */
#endif

#ifndef getcurx
#define getcurx(win)            ((win)?(win)->_curx:ERR)
#define getcury(win)            ((win)?(win)->_cury:ERR)
#endif

#ifndef getbegx
#define getbegx(win)            ((win)?(win)->_begx:ERR)
#define getbegy(win)            ((win)?(win)->_begy:ERR)
#endif

#ifndef getmaxx
#define getmaxx(win)            ((win)?((win)->_maxx + 1):ERR)
#define getmaxy(win)            ((win)?((win)->_maxy + 1):ERR)
#endif

/* ncurses implements tparm() with varargs, X/Open with a fixed-parameter list
 * (which is incompatible with legacy usage, doesn't solve any problems).
 */
#define tparm3(a,b,c) tparm(a,b,c,0,0,0,0,0,0,0)
#define tparm2(a,b)   tparm(a,b,0,0,0,0,0,0,0,0)

#define UChar(c)    ((unsigned char)(c))

#define SIZEOF(table)	(sizeof(table)/sizeof(table[0]))

#if defined(NCURSES_VERSION) && HAVE_NC_ALLOC_H
#include <nc_alloc.h>
#else
#define typeMalloc(type,n) (type *) malloc((n) * sizeof(type))
#define typeRealloc(type,n,p) (type *) realloc(p, (n) * sizeof(type))
#endif

#ifndef ExitProgram
#define ExitProgram(code) exit(code)
#endif

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif
#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif

/* Use this to quiet gcc's -Wwrite-strings warnings, but accommodate SVr4
 * curses which doesn't have const parameters declared (so far) in the places
 * that XSI shows.
 */
#ifndef NCURSES_CONST
#define NCURSES_CONST /* nothing */
#endif

/* out-of-band values for representing absent capabilities */
#define ABSENT_BOOLEAN		((signed char)-1)	/* 255 */
#define ABSENT_NUMERIC		(-1)
#define ABSENT_STRING		(char *)0

/* out-of-band values for representing cancels */
#define CANCELLED_BOOLEAN	((signed char)-2)	/* 254 */
#define CANCELLED_NUMERIC	(-2)
#define CANCELLED_STRING	(char *)(-1)

#define VALID_BOOLEAN(s) ((unsigned char)(s) <= 1) /* reject "-1" */
#define VALID_NUMERIC(s) ((s) >= 0)
#define VALID_STRING(s)  ((s) != CANCELLED_STRING && (s) != ABSENT_STRING)

#define VT_ACSC "``aaffggiijjkkllmmnnooppqqrrssttuuvvwwxxyyzz{{||}}~~"

#endif /* __TEST_PRIV_H */
