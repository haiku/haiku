/* Copyright (C) 1991 Free Software Foundation, Inc.
This file is part of the GNU C Library.

The GNU C Library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The GNU C Library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with the GNU C Library; see the file COPYING.LIB.  If
not, write to the Free Software Foundation, Inc.,
59 Temple Place, Suite 330, Boston, MA 02111 USA.  */

#ifndef	_STRMATCH_H
#define	_STRMATCH_H	1

#ifdef HAVE_LIBC_FNM_EXTMATCH

#include <fnmatch.h>

#else /* !HAVE_LIBC_FNM_EXTMATCH */

#include "stdc.h"

/* We #undef these before defining them because some losing systems
   (HP-UX A.08.07 for example) define these in <unistd.h>.  */
#undef  FNM_PATHNAME
#undef  FNM_NOESCAPE
#undef  FNM_PERIOD

/* Bits set in the FLAGS argument to `strmatch'.  */

/* standard flags are like fnmatch(3). */
#define	FNM_PATHNAME	(1 << 0) /* No wildcard can ever match `/'.  */
#define	FNM_NOESCAPE	(1 << 1) /* Backslashes don't quote special chars.  */
#define	FNM_PERIOD	(1 << 2) /* Leading `.' is matched only explicitly.  */

/* extended flags not available in most libc fnmatch versions, but we undef
   them to avoid any possible warnings. */
#undef FNM_LEADING_DIR
#undef FNM_CASEFOLD
#undef FNM_EXTMATCH

#define FNM_LEADING_DIR	(1 << 3) /* Ignore `/...' after a match. */
#define FNM_CASEFOLD	(1 << 4) /* Compare without regard to case. */
#define FNM_EXTMATCH	(1 << 5) /* Use ksh-like extended matching. */

/* Value returned by `strmatch' if STRING does not match PATTERN.  */
#undef FNM_NOMATCH

#define	FNM_NOMATCH	1

/* Match STRING against the filename pattern PATTERN,
   returning zero if it matches, FNM_NOMATCH if not.  */
extern int strmatch __P((char *, char *, int));

#endif /* !HAVE_LIBC_FNM_EXTMATCH */

#endif /* _STRMATCH_H */
