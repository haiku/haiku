/* Subprocesses with pipes.
   Copyright (C) 2002, 2004 Free Software Foundation, Inc.

   This file is part of Bison, the GNU Compiler Compiler.

   Bison is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   Bison is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with Bison; see the file COPYING.  If not, write to the Free
   Software Foundation, 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.  */

/* Written by Paul Eggert <eggert@twinsun.com>
   and Florian Krohm <florian@edamail.fishkill.ibm.com>.  */

#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

void init_subpipe (void);
pid_t create_subpipe (char const * const *, int[2]);
void reap_subpipe (pid_t, char const *);
