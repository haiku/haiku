/* i960 simulator support code
   Copyright (C) 1998 Free Software Foundation, Inc.
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

#define WANT_CPU
#define WANT_CPU_I960BASE

#include "sim-main.h"
#include "cgen-mem.h"
#include "cgen-ops.h"

/* The contents of BUF are in target byte order.  */

int
i960base_fetch_register (SIM_CPU *current_cpu, int rn, unsigned char *buf,
			 int len)
{
  if (rn < 32)
    SETTWI (buf, a_i960_h_gr_get (current_cpu, rn));
  else
    switch (rn)
      {
      case PC_REGNUM :
	SETTWI (buf, a_i960_h_pc_get (current_cpu));
	break;
      default :
	return 0;
      }

  return -1; /*FIXME*/

}

/* The contents of BUF are in target byte order.  */

int
i960base_store_register (SIM_CPU *current_cpu, int rn, unsigned char *buf,
			 int len)
{
  if (rn < 32)
    a_i960_h_gr_set (current_cpu, rn, GETTWI (buf));
  else
    switch (rn)
      {
      case PC_REGNUM :
	a_i960_h_pc_set (current_cpu, GETTWI (buf));
	break;
      default :
	return 0;
      }

  return -1; /*FIXME*/
}

/* Cover fns for mach independent register accesses.  */

SI
a_i960_h_gr_get (SIM_CPU *current_cpu, UINT regno)
{
  switch (MACH_NUM (CPU_MACH (current_cpu)))
    {
#ifdef HAVE_CPU_I960BASE
    case MACH_I960_KA_SA :
    case MACH_I960_CA :
      return i960base_h_gr_get (current_cpu, regno);
#endif
    default :
      abort ();
    }
}

void
a_i960_h_gr_set (SIM_CPU *current_cpu, UINT regno, SI newval)
{
  switch (MACH_NUM (CPU_MACH (current_cpu)))
    {
#ifdef HAVE_CPU_I960BASE
    case MACH_I960_KA_SA :
    case MACH_I960_CA :
      i960base_h_gr_set (current_cpu, regno, newval);
      break;
#endif
    default :
      abort ();
    }
}

IADDR
a_i960_h_pc_get (SIM_CPU *current_cpu)
{
  switch (MACH_NUM (CPU_MACH (current_cpu)))
    {
#ifdef HAVE_CPU_I960BASE
    case MACH_I960_KA_SA :
    case MACH_I960_CA :
      return i960base_h_pc_get (current_cpu);
#endif
    default :
      abort ();
    }
}

void
a_i960_h_pc_set (SIM_CPU *current_cpu, IADDR newval)
{
  switch (MACH_NUM (CPU_MACH (current_cpu)))
    {
#ifdef HAVE_CPU_I960BASE
    case MACH_I960_KA_SA :
    case MACH_I960_CA :
      i960base_h_pc_set (current_cpu, newval);
      break;
#endif
    default :
      abort ();
    }
}

#if WITH_PROFILE_MODEL_P

/* FIXME: Some of these should be inline or macros.  Later.  */

/* Initialize cycle counting for an insn.
   FIRST_P is non-zero if this is the first insn in a set of parallel
   insns.  */

void
i960base_model_insn_before (SIM_CPU *cpu, int first_p)
{
}

/* Record the cycles computed for an insn.
   LAST_P is non-zero if this is the last insn in a set of parallel insns,
   and we update the total cycle count.
   CYCLES is the cycle count of the insn.  */

void
i960base_model_insn_after (SIM_CPU *cpu, int last_p, int cycles)
{
}

/* Initialize cycle counting for an insn.
   FIRST_P is non-zero if this is the first insn in a set of parallel
   insns.  */

void
i960_model_init_insn_cycles (SIM_CPU *cpu, int first_p)
{
}

/* Record the cycles computed for an insn.
   LAST_P is non-zero if this is the last insn in a set of parallel insns,
   and we update the total cycle count.  */

void
i960_model_update_insn_cycles (SIM_CPU *cpu, int last_p)
{
}

void
i960_model_record_cycles (SIM_CPU *cpu, unsigned long cycles)
{
}

void
i960base_model_mark_get_h_gr (SIM_CPU *cpu, ARGBUF *abuf)
{
}

void
i960base_model_mark_set_h_gr (SIM_CPU *cpu, ARGBUF *abuf)
{
}

#endif /* WITH_PROFILE_MODEL_P */

int
i960base_model_i960KA_u_exec (SIM_CPU *cpu, const IDESC *idesc,
			   int unit_num, int referenced)
{
  return idesc->timing->units[unit_num].done;
}

int
i960base_model_i960CA_u_exec (SIM_CPU *cpu, const IDESC *idesc,
			   int unit_num, int referenced)
{
  return idesc->timing->units[unit_num].done;
}
