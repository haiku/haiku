/* Determine the number of screen columns needed for a character.
   Copyright (C) 2006, 2007 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifndef _gl_WCWIDTH_H
#define _gl_WCWIDTH_H

#if HAVE_WCHAR_T

/* Get wcwidth if available, along with wchar_t.  */
# include <wchar.h>

/* Get iswprint.  */
# include <wctype.h>

# ifndef HAVE_DECL_WCWIDTH
"this configure-time declaration test was not run"
# endif
# ifndef wcwidth
#  if !HAVE_WCWIDTH

/* wcwidth doesn't exist, so assume all printable characters have
   width 1.  */
static inline int
wcwidth (wchar_t wc)
{
  return wc == 0 ? 0 : iswprint (wc) ? 1 : -1;
}

#  elif !HAVE_DECL_WCWIDTH

/* wcwidth exists but is not declared.  */
extern
#   ifdef __cplusplus
"C"
#   endif
int wcwidth (int /* actually wchar_t */);

#  endif
# endif

#endif /* HAVE_WCHAR_T */

#endif /* _gl_WCWIDTH_H */
