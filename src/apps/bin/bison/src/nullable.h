/* Part of the bison parser generator,
   Copyright (C) 2000, 2002 Free Software Foundation, Inc.

   This file is part of Bison, the GNU Compiler Compiler.

   Bison is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   Bison is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Bison; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#ifndef NULLABLE_H_
# define NULLABLE_H_

/* A vector saying which nonterminals can expand into the null string.
   NULLABLE[I - NTOKENS] is nonzero if symbol I can do so.  */
extern bool *nullable;

/* Set up NULLABLE. */
extern void nullable_compute (void);

/* Free NULLABLE. */
extern void nullable_free (void);
#endif /* !NULLABLE_H_ */
