/* Generic simulator stop.
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

/* Generic implementation of sim_stop.  */

static void
control_c_simulation (SIM_DESC sd,
		      void *data)
{
  ASSERT (STATE_MAGIC (sd) == SIM_MAGIC_NUMBER);
  sim_engine_halt (sd, NULL, NULL, NULL_CIA, sim_stopped, SIM_SIGINT);
}

int
sim_stop (SIM_DESC sd)
{
  ASSERT (STATE_MAGIC (sd) == SIM_MAGIC_NUMBER);
  sim_events_schedule_after_signal(sd,
				   0 /*NOW*/,
				   control_c_simulation,
				   sd /*data*/);
  return 1;
}
