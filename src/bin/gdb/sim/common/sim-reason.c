/* Generic simulator stop_reason.
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

#include "sim-main.h"
#include "sim-assert.h"

/* Generic implementation of sim_stop_reason */

void
sim_stop_reason (SIM_DESC sd, enum sim_stop *reason, int *sigrc)
{
  sim_engine *engine = NULL;
  SIM_ASSERT (STATE_MAGIC (sd) == SIM_MAGIC_NUMBER);
  engine = STATE_ENGINE (sd);
  *reason = engine->reason;
  switch (*reason)
    {
    case sim_exited :
      *sigrc = engine->sigrc;
      break;
    case sim_signalled :
      /* ??? See the comment below case `sim_signalled' in
	 gdb/remote-sim.c:gdbsim_wait.
	 ??? Consider the case of the target requesting that it
	 kill(2) itself with SIGNAL.  That SIGNAL, being target
	 specific, will not correspond to either of the SIM_SIGNAL
	 enum nor the HOST_SIGNAL.  A mapping from TARGET_SIGNAL to
	 HOST_SIGNAL is needed.  */
      *sigrc = sim_signal_to_host (sd, engine->sigrc);
      break;
    case sim_stopped :
      /* The gdb/simulator interface calls for us to return the host
	 version of the signal which gdb then converts into the
	 target's version.  This is obviously a bit clumsy.  */
      *sigrc = sim_signal_to_host (sd, engine->sigrc);
      break;
    default :
      abort ();
    }
}
