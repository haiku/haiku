/****************************************************************************
 * Copyright (c) 1998-2004,2005 Free Software Foundation, Inc.              *
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
 *  Author: Zeyd M. Ben-Halim <zmbenhal@netcom.com> 1992,1995               *
 *     and: Eric S. Raymond <esr@snark.thyrsus.com>                         *
 *     and: Thomas E. Dickey                        1996-on                 *
 *                                                                          *
 * some of the code in here was contributed by:                             *
 * Magnus Bengtsson, d6mbeng@dtek.chalmers.se (Nov'93)                      *
 ****************************************************************************/

#define __INTERNAL_CAPS_VISIBLE
#include <curses.priv.h>

#include <termcap.h>
#include <tic.h>
#include <ctype.h>

#include <term_entry.h>

MODULE_ID("$Id: lib_termcap.c,v 1.51 2005/07/16 23:12:51 tom Exp $")

NCURSES_EXPORT_VAR(char *) UP = 0;
NCURSES_EXPORT_VAR(char *) BC = 0;

static char *fix_me = 0;	/* this holds the filtered sgr0 string */

/***************************************************************************
 *
 * tgetent(bufp, term)
 *
 * In termcap, this function reads in the entry for terminal `term' into the
 * buffer pointed to by bufp. It must be called before any of the functions
 * below are called.
 * In this terminfo emulation, tgetent() simply calls setupterm() (which
 * does a bit more than tgetent() in termcap does), and returns its return
 * value (1 if successful, 0 if no terminal with the given name could be
 * found, or -1 if no terminal descriptions have been installed on the
 * system).  The bufp argument is ignored.
 *
 ***************************************************************************/

NCURSES_EXPORT(int)
tgetent(char *bufp GCC_UNUSED, const char *name)
{
    int errcode;

    START_TRACE();
    T((T_CALLED("tgetent()")));

    _nc_setupterm((NCURSES_CONST char *) name, STDOUT_FILENO, &errcode, TRUE);

    PC = 0;
    UP = 0;
    BC = 0;
    fix_me = 0;			/* don't free it - application may still use */

    if (errcode == 1) {

	if (cursor_left)
	    if ((backspaces_with_bs = !strcmp(cursor_left, "\b")) == 0)
		backspace_if_not_bs = cursor_left;

	/* we're required to export these */
	if (pad_char != NULL)
	    PC = pad_char[0];
	if (cursor_up != NULL)
	    UP = cursor_up;
	if (backspace_if_not_bs != NULL)
	    BC = backspace_if_not_bs;

	FreeIfNeeded(fix_me);
	if ((fix_me = _nc_trim_sgr0(&(cur_term->type))) != 0) {
	    if (!strcmp(fix_me, exit_attribute_mode)) {
		if (fix_me != exit_attribute_mode) {
		    free(fix_me);
		}
		fix_me = 0;
	    }
	}

	(void) baudrate();	/* sets ospeed as a side-effect */

/* LINT_PREPRO
#if 0*/
#include <capdefaults.c>
/* LINT_PREPRO
#endif*/

    }
    returnCode(errcode);
}

/***************************************************************************
 *
 * tgetflag(str)
 *
 * Look up boolean termcap capability str and return its value (TRUE=1 if
 * present, FALSE=0 if not).
 *
 ***************************************************************************/

NCURSES_EXPORT(int)
tgetflag(NCURSES_CONST char *id)
{
    unsigned i;

    T((T_CALLED("tgetflag(%s)"), id));
    if (cur_term != 0) {
	TERMTYPE *tp = &(cur_term->type);
	for_each_boolean(i, tp) {
	    const char *capname = ExtBoolname(tp, i, boolcodes);
	    if (!strncmp(id, capname, 2)) {
		/* setupterm forces invalid booleans to false */
		returnCode(tp->Booleans[i]);
	    }
	}
    }
    returnCode(0);		/* Solaris does this */
}

/***************************************************************************
 *
 * tgetnum(str)
 *
 * Look up numeric termcap capability str and return its value, or -1 if
 * not given.
 *
 ***************************************************************************/

NCURSES_EXPORT(int)
tgetnum(NCURSES_CONST char *id)
{
    unsigned i;

    T((T_CALLED("tgetnum(%s)"), id));
    if (cur_term != 0) {
	TERMTYPE *tp = &(cur_term->type);
	for_each_number(i, tp) {
	    const char *capname = ExtNumname(tp, i, numcodes);
	    if (!strncmp(id, capname, 2)) {
		if (!VALID_NUMERIC(tp->Numbers[i]))
		    returnCode(ABSENT_NUMERIC);
		returnCode(tp->Numbers[i]);
	    }
	}
    }
    returnCode(ABSENT_NUMERIC);
}

/***************************************************************************
 *
 * tgetstr(str, area)
 *
 * Look up string termcap capability str and return a pointer to its value,
 * or NULL if not given.
 *
 ***************************************************************************/

NCURSES_EXPORT(char *)
tgetstr(NCURSES_CONST char *id, char **area)
{
    unsigned i;
    char *result = NULL;

    T((T_CALLED("tgetstr(%s,%p)"), id, area));
    if (cur_term != 0) {
	TERMTYPE *tp = &(cur_term->type);
	for_each_string(i, tp) {
	    const char *capname = ExtStrname(tp, i, strcodes);
	    if (!strncmp(id, capname, 2)) {
		result = tp->Strings[i];
		TR(TRACE_DATABASE, ("found match : %s", _nc_visbuf(result)));
		/* setupterm forces canceled strings to null */
		if (VALID_STRING(result)) {
		    if (result == exit_attribute_mode
			&& fix_me != 0) {
			result = fix_me;
			TR(TRACE_DATABASE, ("altered to : %s", _nc_visbuf(result)));
		    }
		    if (area != 0
			&& *area != 0) {
			(void) strcpy(*area, result);
			*area += strlen(*area) + 1;
		    }
		}
		break;
	    }
	}
    }
    returnPtr(result);
}
