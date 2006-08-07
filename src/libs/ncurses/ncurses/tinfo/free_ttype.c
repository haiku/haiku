/****************************************************************************
 * Copyright (c) 1999-2003,2005 Free Software Foundation, Inc.              *
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
 *  Author: Thomas E. Dickey <dickey@clark.net> 1999                        *
 ****************************************************************************/

/*
 * free_ttype.c -- allocation functions for TERMTYPE
 *
 *	_nc_free_termtype()
 *	use_extended_names()
 *
 */

#include <curses.priv.h>

#include <tic.h>
#include <term_entry.h>

MODULE_ID("$Id: free_ttype.c,v 1.11 2005/06/04 21:54:50 tom Exp $")

NCURSES_EXPORT(void)
_nc_free_termtype(TERMTYPE *ptr)
{
    T(("_nc_free_termtype(%s)", ptr->term_names));

    if (ptr->str_table == 0
	|| (ptr->term_names < ptr->str_table
	    || ptr->term_names >= ptr->str_table + MAX_ENTRY_SIZE)) {
	FreeIfNeeded(ptr->term_names);
    }
#if NO_LEAKS
    else {
	if (ptr->str_table != 0
	    && (ptr->term_names < ptr->str_table + MAX_ENTRY_SIZE)) {
	    int j;
	    char *last = ptr->str_table;
	    /*
	     * We should have saved the entry-size someplace.  Too late,
	     * but this is useful for the memory-leak checking, though more
	     * work/time than should be in the normal library.
	     */
	    for (j = 0; j < NUM_STRINGS(ptr); j++) {
		char *s = ptr->Strings[j];
		if (VALID_STRING(s)) {
		    char *t = s + strlen(s) + 1;
		    if (t > last)
			last = t;
		}
	    }
	    if (last < ptr->term_names) {
		FreeIfNeeded(ptr->term_names);
	    }
	}
    }
#endif
    FreeIfNeeded(ptr->str_table);
    FreeIfNeeded(ptr->Booleans);
    FreeIfNeeded(ptr->Numbers);
    FreeIfNeeded(ptr->Strings);
#if NCURSES_XNAMES
    FreeIfNeeded(ptr->ext_str_table);
    FreeIfNeeded(ptr->ext_Names);
#endif
    memset(ptr, 0, sizeof(TERMTYPE));
    _nc_free_entry(_nc_head, ptr);
}

#if NCURSES_XNAMES
NCURSES_EXPORT_VAR(bool) _nc_user_definable = TRUE;

NCURSES_EXPORT(int)
use_extended_names(bool flag)
{
    int oldflag = _nc_user_definable;

    T((T_CALLED("use_extended_names(%d)"), flag));
    _nc_user_definable = flag;
    returnBool(oldflag);
}
#endif
