/* Generic simulator abort.
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

#include <stdio.h>

#include "sim-main.h"
#include "sim-assert.h"

/* This is an implementation of sim_engine_abort that does not use
   longjmp, instead it just calls sim_io_error.  sim_io_error will
   jump right out of the simulator.

   It is intended as a holder for simulators that have started to use
   sim-core et al. but are not yet in a position to use sim-engine
   (the setjmp/longjmp code). */


void
sim_engine_abort (SIM_DESC sd,
		  sim_cpu *cpu,
		  sim_cia cia,
		  const char *fmt,
		  ...)
{
  ASSERT (sd == NULL || STATE_MAGIC (sd) == SIM_MAGIC_NUMBER);
  if (sd != NULL)
    {
      va_list ap;
      va_start(ap, fmt);
      sim_io_evprintf (sd, fmt, ap);
      va_end(ap);
      sim_io_error (sd, "\n");
    }
  else
    {
      va_list ap;
      va_start(ap, fmt);
      vfprintf (stderr, fmt, ap);
      va_end(ap);
      fprintf (stderr, "\n");
      abort ();
    }
}
