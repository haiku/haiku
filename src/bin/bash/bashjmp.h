/* bashjmp.h -- wrapper for setjmp.h with necessary bash definitions. */

/* Copyright (C) 1987,1991 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne Again SHell.

   Bash is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   Bash is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with Bash; see the file COPYING.  If not, write to the Free
   Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111 USA. */

#ifndef _BASHJMP_H_
#define _BASHJMP_H_

#include "posixjmp.h"

extern procenv_t	top_level;
extern procenv_t	subshell_top_level;
extern procenv_t	return_catch;	/* used by `return' builtin */

#define SHFUNC_RETURN()	longjmp (return_catch, 1)

#define COPY_PROCENV(old, save) \
	xbcopy ((char *)old, (char *)save, sizeof (procenv_t));

/* Values for the second argument to longjmp/siglongjmp. */
#define NOT_JUMPED 0		/* Not returning from a longjmp. */
#define FORCE_EOF 1		/* We want to stop parsing. */
#define DISCARD 2		/* Discard current command. */
#define EXITPROG 3		/* Unconditionally exit the program now. */

#endif /* _BASHJMP_H_ */
