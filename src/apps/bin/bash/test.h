/* test.h -- external interface to the conditional command code. */

/* Copyright (C) 1997 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne Again SHell.

   Bash is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 2, or (at your option) any later
   version.

   Bash is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.

   You should have received a copy of the GNU General Public License along
   with Bash; see the file COPYING.  If not, write to the Free Software
   Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA. */

#ifndef _TEST_H_
#define _TEST_H_

#include "stdc.h"

/* Values for the flags argument to binary_test */
#define TEST_PATMATCH	0x01
#define TEST_ARITHEXP	0x02

extern int test_eaccess __P((char *, int));

extern int test_unop __P((char *));
extern int test_binop __P((char *));

extern int unary_test __P((char *, char *));
extern int binary_test __P((char *, char *, char *, int));

extern int test_command __P((int, char **));

#endif /* _TEST_H_ */
