/* Misc. support for CPU family i960base.

THIS FILE IS MACHINE GENERATED WITH CGEN.

Copyright (C) 1996, 1997, 1998, 1999 Free Software Foundation, Inc.

This file is part of the GNU Simulators.

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
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#define WANT_CPU i960base
#define WANT_CPU_I960BASE

#include "sim-main.h"
#include "cgen-ops.h"

/* Get the value of h-pc.  */

USI
i960base_h_pc_get (SIM_CPU *current_cpu)
{
  return CPU (h_pc);
}

/* Set a value for h-pc.  */

void
i960base_h_pc_set (SIM_CPU *current_cpu, USI newval)
{
  CPU (h_pc) = newval;
}

/* Get the value of h-gr.  */

SI
i960base_h_gr_get (SIM_CPU *current_cpu, UINT regno)
{
  return CPU (h_gr[regno]);
}

/* Set a value for h-gr.  */

void
i960base_h_gr_set (SIM_CPU *current_cpu, UINT regno, SI newval)
{
  CPU (h_gr[regno]) = newval;
}

/* Get the value of h-cc.  */

SI
i960base_h_cc_get (SIM_CPU *current_cpu)
{
  return CPU (h_cc);
}

/* Set a value for h-cc.  */

void
i960base_h_cc_set (SIM_CPU *current_cpu, SI newval)
{
  CPU (h_cc) = newval;
}

/* Record trace results for INSN.  */

void
i960base_record_trace_results (SIM_CPU *current_cpu, CGEN_INSN *insn,
			    int *indices, TRACE_RECORD *tr)
{
}
