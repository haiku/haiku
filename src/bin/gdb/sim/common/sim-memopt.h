/* Header file for simulator memory argument handling.
   Copyright (C) 1997 Free Software Foundation, Inc.
   Contributed by Cygnus Support.

This file is part of GDB, the GNU debugger.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef SIM_MEMOPT_H
#define SIM_MEMOPT_H

/* Provides a command line interface for manipulating the memory core */

typedef struct _sim_memopt sim_memopt;
struct _sim_memopt {
  int level;
  int space;
  unsigned_word addr;
  unsigned_word nr_bytes;
  unsigned modulo;
  void *buffer;
  unsigned long munmap_length;
  sim_memopt *alias; /* linked list */
  sim_memopt *next;
};


/* Install the "memopt" module.  */

SIM_RC sim_memopt_install (SIM_DESC sd);


/* Was there a memory command? */

#endif
