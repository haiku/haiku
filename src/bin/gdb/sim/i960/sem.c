/* Simulator instruction semantics for i960base.

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
#include "cgen-mem.h"
#include "cgen-ops.h"

#undef GET_ATTR
#define GET_ATTR(cpu, num, attr) CGEN_ATTR_VALUE (NULL, abuf->idesc->attrs, CGEN_INSN_##attr)

/* This is used so that we can compile two copies of the semantic code,
   one with full feature support and one without that runs fast(er).
   FAST_P, when desired, is defined on the command line, -DFAST_P=1.  */
#if FAST_P
#define SEM_FN_NAME(cpu,fn) XCONCAT3 (cpu,_semf_,fn)
#undef TRACE_RESULT
#define TRACE_RESULT(cpu, abuf, name, type, val)
#else
#define SEM_FN_NAME(cpu,fn) XCONCAT3 (cpu,_sem_,fn)
#endif

/* x-invalid: --invalid-- */

static SEM_PC
SEM_FN_NAME (i960base,x_invalid) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.fmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
    /* Update the recorded pc in the cpu state struct.
       Only necessary for WITH_SCACHE case, but to avoid the
       conditional compilation ....  */
    SET_H_PC (pc);
    /* Virtual insns have zero size.  Overwrite vpc with address of next insn
       using the default-insn-bitsize spec.  When executing insns in parallel
       we may want to queue the fault and continue execution.  */
    vpc = SEM_NEXT_VPC (sem_arg, pc, 4);
    vpc = sim_engine_invalid_insn (current_cpu, pc, vpc);
  }

  return vpc;
#undef FLD
}

/* x-after: --after-- */

static SEM_PC
SEM_FN_NAME (i960base,x_after) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.fmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
#if WITH_SCACHE_PBB_I960BASE
    i960base_pbb_after (current_cpu, sem_arg);
#endif
  }

  return vpc;
#undef FLD
}

/* x-before: --before-- */

static SEM_PC
SEM_FN_NAME (i960base,x_before) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.fmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
#if WITH_SCACHE_PBB_I960BASE
    i960base_pbb_before (current_cpu, sem_arg);
#endif
  }

  return vpc;
#undef FLD
}

/* x-cti-chain: --cti-chain-- */

static SEM_PC
SEM_FN_NAME (i960base,x_cti_chain) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.fmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
#if WITH_SCACHE_PBB_I960BASE
#ifdef DEFINE_SWITCH
    vpc = i960base_pbb_cti_chain (current_cpu, sem_arg,
			       pbb_br_type, pbb_br_npc);
    BREAK (sem);
#else
    /* FIXME: Allow provision of explicit ifmt spec in insn spec.  */
    vpc = i960base_pbb_cti_chain (current_cpu, sem_arg,
			       CPU_PBB_BR_TYPE (current_cpu),
			       CPU_PBB_BR_NPC (current_cpu));
#endif
#endif
  }

  return vpc;
#undef FLD
}

/* x-chain: --chain-- */

static SEM_PC
SEM_FN_NAME (i960base,x_chain) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.fmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
#if WITH_SCACHE_PBB_I960BASE
    vpc = i960base_pbb_chain (current_cpu, sem_arg);
#ifdef DEFINE_SWITCH
    BREAK (sem);
#endif
#endif
  }

  return vpc;
#undef FLD
}

/* x-begin: --begin-- */

static SEM_PC
SEM_FN_NAME (i960base,x_begin) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.fmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
#if WITH_SCACHE_PBB_I960BASE
#ifdef DEFINE_SWITCH
    /* In the switch case FAST_P is a constant, allowing several optimizations
       in any called inline functions.  */
    vpc = i960base_pbb_begin (current_cpu, FAST_P);
#else
    vpc = i960base_pbb_begin (current_cpu, STATE_RUN_FAST_P (CPU_STATE (current_cpu)));
#endif
#endif
  }

  return vpc;
#undef FLD
}

/* mulo: mulo $src1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,mulo) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = MULSI (* FLD (i_src1), * FLD (i_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* mulo1: mulo $lit1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,mulo1) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = MULSI (FLD (f_src1), * FLD (i_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* mulo2: mulo $src1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,mulo2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = MULSI (* FLD (i_src1), FLD (f_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* mulo3: mulo $lit1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,mulo3) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = MULSI (FLD (f_src1), FLD (f_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* remo: remo $src1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,remo) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = UMODSI (* FLD (i_src2), * FLD (i_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* remo1: remo $lit1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,remo1) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = UMODSI (* FLD (i_src2), FLD (f_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* remo2: remo $src1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,remo2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = UMODSI (FLD (f_src2), * FLD (i_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* remo3: remo $lit1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,remo3) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = UMODSI (FLD (f_src2), FLD (f_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* divo: divo $src1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,divo) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = UDIVSI (* FLD (i_src2), * FLD (i_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* divo1: divo $lit1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,divo1) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = UDIVSI (* FLD (i_src2), FLD (f_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* divo2: divo $src1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,divo2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = UDIVSI (FLD (f_src2), * FLD (i_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* divo3: divo $lit1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,divo3) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = UDIVSI (FLD (f_src2), FLD (f_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* remi: remi $src1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,remi) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = MODSI (* FLD (i_src2), * FLD (i_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* remi1: remi $lit1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,remi1) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = MODSI (* FLD (i_src2), FLD (f_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* remi2: remi $src1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,remi2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = MODSI (FLD (f_src2), * FLD (i_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* remi3: remi $lit1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,remi3) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = MODSI (FLD (f_src2), FLD (f_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* divi: divi $src1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,divi) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = DIVSI (* FLD (i_src2), * FLD (i_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* divi1: divi $lit1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,divi1) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = DIVSI (* FLD (i_src2), FLD (f_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* divi2: divi $src1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,divi2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = DIVSI (FLD (f_src2), * FLD (i_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* divi3: divi $lit1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,divi3) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = DIVSI (FLD (f_src2), FLD (f_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* addo: addo $src1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,addo) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ADDSI (* FLD (i_src1), * FLD (i_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* addo1: addo $lit1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,addo1) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ADDSI (FLD (f_src1), * FLD (i_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* addo2: addo $src1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,addo2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ADDSI (* FLD (i_src1), FLD (f_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* addo3: addo $lit1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,addo3) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ADDSI (FLD (f_src1), FLD (f_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* subo: subo $src1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,subo) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = SUBSI (* FLD (i_src2), * FLD (i_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* subo1: subo $lit1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,subo1) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = SUBSI (* FLD (i_src2), FLD (f_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* subo2: subo $src1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,subo2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = SUBSI (FLD (f_src2), * FLD (i_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* subo3: subo $lit1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,subo3) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = SUBSI (FLD (f_src2), FLD (f_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* notbit: notbit $src1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,notbit) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = XORSI (SLLSI (1, * FLD (i_src1)), * FLD (i_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* notbit1: notbit $lit1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,notbit1) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = XORSI (SLLSI (1, FLD (f_src1)), * FLD (i_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* notbit2: notbit $src1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,notbit2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = XORSI (SLLSI (1, * FLD (i_src1)), FLD (f_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* notbit3: notbit $lit1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,notbit3) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = XORSI (SLLSI (1, FLD (f_src1)), FLD (f_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* and: and $src1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,and) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (* FLD (i_src1), * FLD (i_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* and1: and $lit1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,and1) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (FLD (f_src1), * FLD (i_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* and2: and $src1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,and2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (* FLD (i_src1), FLD (f_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* and3: and $lit1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,and3) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (FLD (f_src1), FLD (f_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* andnot: andnot $src1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,andnot) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (* FLD (i_src2), INVSI (* FLD (i_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* andnot1: andnot $lit1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,andnot1) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (* FLD (i_src2), INVSI (FLD (f_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* andnot2: andnot $src1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,andnot2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (FLD (f_src2), INVSI (* FLD (i_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* andnot3: andnot $lit1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,andnot3) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (FLD (f_src2), INVSI (FLD (f_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* setbit: setbit $src1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,setbit) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ORSI (SLLSI (1, * FLD (i_src1)), * FLD (i_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* setbit1: setbit $lit1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,setbit1) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ORSI (SLLSI (1, FLD (f_src1)), * FLD (i_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* setbit2: setbit $src1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,setbit2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ORSI (SLLSI (1, * FLD (i_src1)), FLD (f_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* setbit3: setbit $lit1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,setbit3) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ORSI (SLLSI (1, FLD (f_src1)), FLD (f_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* notand: notand $src1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,notand) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (INVSI (* FLD (i_src2)), * FLD (i_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* notand1: notand $lit1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,notand1) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (INVSI (* FLD (i_src2)), FLD (f_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* notand2: notand $src1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,notand2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (INVSI (FLD (f_src2)), * FLD (i_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* notand3: notand $lit1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,notand3) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (INVSI (FLD (f_src2)), FLD (f_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* xor: xor $src1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,xor) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = XORSI (* FLD (i_src1), * FLD (i_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* xor1: xor $lit1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,xor1) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = XORSI (FLD (f_src1), * FLD (i_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* xor2: xor $src1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,xor2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = XORSI (* FLD (i_src1), FLD (f_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* xor3: xor $lit1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,xor3) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = XORSI (FLD (f_src1), FLD (f_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* or: or $src1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,or) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ORSI (* FLD (i_src1), * FLD (i_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* or1: or $lit1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,or1) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ORSI (FLD (f_src1), * FLD (i_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* or2: or $src1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,or2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ORSI (* FLD (i_src1), FLD (f_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* or3: or $lit1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,or3) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ORSI (FLD (f_src1), FLD (f_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* nor: nor $src1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,nor) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (INVSI (* FLD (i_src2)), INVSI (* FLD (i_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* nor1: nor $lit1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,nor1) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (INVSI (* FLD (i_src2)), INVSI (FLD (f_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* nor2: nor $src1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,nor2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (INVSI (FLD (f_src2)), INVSI (* FLD (i_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* nor3: nor $lit1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,nor3) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (INVSI (FLD (f_src2)), INVSI (FLD (f_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* xnor: xnor $src1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,xnor) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = INVSI (XORSI (* FLD (i_src1), * FLD (i_src2)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* xnor1: xnor $lit1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,xnor1) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = INVSI (XORSI (FLD (f_src1), * FLD (i_src2)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* xnor2: xnor $src1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,xnor2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = INVSI (XORSI (* FLD (i_src1), FLD (f_src2)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* xnor3: xnor $lit1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,xnor3) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = INVSI (XORSI (FLD (f_src1), FLD (f_src2)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* not: not $src1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,not) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = INVSI (* FLD (i_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* not1: not $lit1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,not1) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = INVSI (FLD (f_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* not2: not $src1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,not2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = INVSI (* FLD (i_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* not3: not $lit1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,not3) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = INVSI (FLD (f_src1));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ornot: ornot $src1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,ornot) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ORSI (* FLD (i_src2), INVSI (* FLD (i_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ornot1: ornot $lit1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,ornot1) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ORSI (* FLD (i_src2), INVSI (FLD (f_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ornot2: ornot $src1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,ornot2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ORSI (FLD (f_src2), INVSI (* FLD (i_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ornot3: ornot $lit1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,ornot3) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ORSI (FLD (f_src2), INVSI (FLD (f_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* clrbit: clrbit $src1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,clrbit) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (INVSI (SLLSI (1, * FLD (i_src1))), * FLD (i_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* clrbit1: clrbit $lit1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,clrbit1) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (INVSI (SLLSI (1, FLD (f_src1))), * FLD (i_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* clrbit2: clrbit $src1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,clrbit2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (INVSI (SLLSI (1, * FLD (i_src1))), FLD (f_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* clrbit3: clrbit $lit1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,clrbit3) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (INVSI (SLLSI (1, FLD (f_src1))), FLD (f_src2));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* shlo: shlo $src1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,shlo) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (GEUSI (* FLD (i_src1), 32)) ? (0) : (SLLSI (* FLD (i_src2), * FLD (i_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* shlo1: shlo $lit1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,shlo1) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (GEUSI (FLD (f_src1), 32)) ? (0) : (SLLSI (* FLD (i_src2), FLD (f_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* shlo2: shlo $src1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,shlo2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (GEUSI (* FLD (i_src1), 32)) ? (0) : (SLLSI (FLD (f_src2), * FLD (i_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* shlo3: shlo $lit1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,shlo3) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (GEUSI (FLD (f_src1), 32)) ? (0) : (SLLSI (FLD (f_src2), FLD (f_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* shro: shro $src1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,shro) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (GEUSI (* FLD (i_src1), 32)) ? (0) : (SRLSI (* FLD (i_src2), * FLD (i_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* shro1: shro $lit1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,shro1) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (GEUSI (FLD (f_src1), 32)) ? (0) : (SRLSI (* FLD (i_src2), FLD (f_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* shro2: shro $src1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,shro2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (GEUSI (* FLD (i_src1), 32)) ? (0) : (SRLSI (FLD (f_src2), * FLD (i_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* shro3: shro $lit1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,shro3) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (GEUSI (FLD (f_src1), 32)) ? (0) : (SRLSI (FLD (f_src2), FLD (f_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* shli: shli $src1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,shli) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (GEUSI (* FLD (i_src1), 32)) ? (0) : (SLLSI (* FLD (i_src2), * FLD (i_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* shli1: shli $lit1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,shli1) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (GEUSI (FLD (f_src1), 32)) ? (0) : (SLLSI (* FLD (i_src2), FLD (f_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* shli2: shli $src1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,shli2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (GEUSI (* FLD (i_src1), 32)) ? (0) : (SLLSI (FLD (f_src2), * FLD (i_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* shli3: shli $lit1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,shli3) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (GEUSI (FLD (f_src1), 32)) ? (0) : (SLLSI (FLD (f_src2), FLD (f_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* shri: shri $src1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,shri) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (GEUSI (* FLD (i_src1), 32)) ? (SRASI (* FLD (i_src2), 31)) : (SRASI (* FLD (i_src2), * FLD (i_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* shri1: shri $lit1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,shri1) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (GEUSI (FLD (f_src1), 32)) ? (SRASI (* FLD (i_src2), 31)) : (SRASI (* FLD (i_src2), FLD (f_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* shri2: shri $src1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,shri2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (GEUSI (* FLD (i_src1), 32)) ? (SRASI (FLD (f_src2), 31)) : (SRASI (FLD (f_src2), * FLD (i_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* shri3: shri $lit1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,shri3) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (GEUSI (FLD (f_src1), 32)) ? (SRASI (FLD (f_src2), 31)) : (SRASI (FLD (f_src2), FLD (f_src1)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* emul: emul $src1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,emul) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  DI tmp_temp;
  SI tmp_dregno;
  tmp_temp = MULDI (ZEXTSIDI (* FLD (i_src1)), ZEXTSIDI (* FLD (i_src2)));
  tmp_dregno = FLD (f_srcdst);
  {
    SI opval = TRUNCDISI (tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = TRUNCDISI (SRLDI (tmp_temp, 32));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* emul1: emul $lit1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,emul1) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  DI tmp_temp;
  SI tmp_dregno;
  tmp_temp = MULDI (ZEXTSIDI (FLD (f_src1)), ZEXTSIDI (* FLD (i_src2)));
  tmp_dregno = FLD (f_srcdst);
  {
    SI opval = TRUNCDISI (tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = TRUNCDISI (SRLDI (tmp_temp, 32));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* emul2: emul $src1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,emul2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  DI tmp_temp;
  SI tmp_dregno;
  tmp_temp = MULDI (ZEXTSIDI (* FLD (i_src1)), ZEXTSIDI (FLD (f_src2)));
  tmp_dregno = FLD (f_srcdst);
  {
    SI opval = TRUNCDISI (tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = TRUNCDISI (SRLDI (tmp_temp, 32));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* emul3: emul $lit1, $lit2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,emul3) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  DI tmp_temp;
  SI tmp_dregno;
  tmp_temp = MULDI (ZEXTSIDI (FLD (f_src1)), ZEXTSIDI (FLD (f_src2)));
  tmp_dregno = FLD (f_srcdst);
  {
    SI opval = TRUNCDISI (tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = TRUNCDISI (SRLDI (tmp_temp, 32));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* mov: mov $src1, $dst */

static SEM_PC
SEM_FN_NAME (i960base,mov) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = * FLD (i_src1);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* mov1: mov $lit1, $dst */

static SEM_PC
SEM_FN_NAME (i960base,mov1) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = FLD (f_src1);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* movl: movl $src1, $dst */

static SEM_PC
SEM_FN_NAME (i960base,movl) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_movq.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_dregno;
  SI tmp_sregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_sregno = FLD (f_src1);
  {
    SI opval = * FLD (i_src1);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_src1)) + (1))]);
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* movl1: movl $lit1, $dst */

static SEM_PC
SEM_FN_NAME (i960base,movl1) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  {
    SI opval = FLD (f_src1);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = 0;
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* movt: movt $src1, $dst */

static SEM_PC
SEM_FN_NAME (i960base,movt) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_movq.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_dregno;
  SI tmp_sregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_sregno = FLD (f_src1);
  {
    SI opval = * FLD (i_src1);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_src1)) + (1))]);
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_src1)) + (2))]);
    CPU (h_gr[((FLD (f_srcdst)) + (2))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-2", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* movt1: movt $lit1, $dst */

static SEM_PC
SEM_FN_NAME (i960base,movt1) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_movq.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  {
    SI opval = FLD (f_src1);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = 0;
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
  {
    SI opval = 0;
    CPU (h_gr[((FLD (f_srcdst)) + (2))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-2", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* movq: movq $src1, $dst */

static SEM_PC
SEM_FN_NAME (i960base,movq) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_movq.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_dregno;
  SI tmp_sregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_sregno = FLD (f_src1);
  {
    SI opval = * FLD (i_src1);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_src1)) + (1))]);
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_src1)) + (2))]);
    CPU (h_gr[((FLD (f_srcdst)) + (2))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-2", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_src1)) + (3))]);
    CPU (h_gr[((FLD (f_srcdst)) + (3))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-3", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* movq1: movq $lit1, $dst */

static SEM_PC
SEM_FN_NAME (i960base,movq1) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_movq.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  {
    SI opval = FLD (f_src1);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = 0;
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
  {
    SI opval = 0;
    CPU (h_gr[((FLD (f_srcdst)) + (2))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-2", 'x', opval);
  }
  {
    SI opval = 0;
    CPU (h_gr[((FLD (f_srcdst)) + (3))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-3", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* modpc: modpc $src1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,modpc) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = * FLD (i_src2);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* modac: modac $src1, $src2, $dst */

static SEM_PC
SEM_FN_NAME (i960base,modac) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = * FLD (i_src2);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lda-offset: lda $offset, $dst */

static SEM_PC
SEM_FN_NAME (i960base,lda_offset) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = FLD (f_offset);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lda-indirect-offset: lda $offset($abase), $dst */

static SEM_PC
SEM_FN_NAME (i960base,lda_indirect_offset) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ADDSI (FLD (f_offset), * FLD (i_abase));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lda-indirect: lda ($abase), $dst */

static SEM_PC
SEM_FN_NAME (i960base,lda_indirect) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = * FLD (i_abase);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lda-indirect-index: lda ($abase)[$index*S$scale], $dst */

static SEM_PC
SEM_FN_NAME (i960base,lda_indirect_index) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lda-disp: lda $optdisp, $dst */

static SEM_PC
SEM_FN_NAME (i960base,lda_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = FLD (f_optdisp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lda-indirect-disp: lda $optdisp($abase), $dst */

static SEM_PC
SEM_FN_NAME (i960base,lda_indirect_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = ADDSI (FLD (f_optdisp), * FLD (i_abase));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lda-index-disp: lda $optdisp[$index*S$scale], $dst */

static SEM_PC
SEM_FN_NAME (i960base,lda_index_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lda-indirect-index-disp: lda $optdisp($abase)[$index*S$scale], $dst */

static SEM_PC
SEM_FN_NAME (i960base,lda_indirect_index_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ld-offset: ld $offset, $dst */

static SEM_PC
SEM_FN_NAME (i960base,ld_offset) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMSI (current_cpu, pc, FLD (f_offset));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ld-indirect-offset: ld $offset($abase), $dst */

static SEM_PC
SEM_FN_NAME (i960base,ld_indirect_offset) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (FLD (f_offset), * FLD (i_abase)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ld-indirect: ld ($abase), $dst */

static SEM_PC
SEM_FN_NAME (i960base,ld_indirect) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMSI (current_cpu, pc, * FLD (i_abase));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ld-indirect-index: ld ($abase)[$index*S$scale], $dst */

static SEM_PC
SEM_FN_NAME (i960base,ld_indirect_index) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ld-disp: ld $optdisp, $dst */

static SEM_PC
SEM_FN_NAME (i960base,ld_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMSI (current_cpu, pc, FLD (f_optdisp));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ld-indirect-disp: ld $optdisp($abase), $dst */

static SEM_PC
SEM_FN_NAME (i960base,ld_indirect_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), * FLD (i_abase)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ld-index-disp: ld $optdisp[$index*S$scale], $dst */

static SEM_PC
SEM_FN_NAME (i960base,ld_index_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ld-indirect-index-disp: ld $optdisp($abase)[$index*S$scale], $dst */

static SEM_PC
SEM_FN_NAME (i960base,ld_indirect_index_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))))));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldob-offset: ldob $offset, $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldob_offset) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMUQI (current_cpu, pc, FLD (f_offset));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldob-indirect-offset: ldob $offset($abase), $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldob_indirect_offset) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMUQI (current_cpu, pc, ADDSI (FLD (f_offset), * FLD (i_abase)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldob-indirect: ldob ($abase), $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldob_indirect) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMUQI (current_cpu, pc, * FLD (i_abase));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldob-indirect-index: ldob ($abase)[$index*S$scale], $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldob_indirect_index) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMUQI (current_cpu, pc, ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldob-disp: ldob $optdisp, $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldob_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMUQI (current_cpu, pc, FLD (f_optdisp));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldob-indirect-disp: ldob $optdisp($abase), $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldob_indirect_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMUQI (current_cpu, pc, ADDSI (FLD (f_optdisp), * FLD (i_abase)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldob-index-disp: ldob $optdisp[$index*S$scale], $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldob_index_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMUQI (current_cpu, pc, ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldob-indirect-index-disp: ldob $optdisp($abase)[$index*S$scale], $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldob_indirect_index_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMUQI (current_cpu, pc, ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))))));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldos-offset: ldos $offset, $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldos_offset) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMUHI (current_cpu, pc, FLD (f_offset));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldos-indirect-offset: ldos $offset($abase), $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldos_indirect_offset) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMUHI (current_cpu, pc, ADDSI (FLD (f_offset), * FLD (i_abase)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldos-indirect: ldos ($abase), $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldos_indirect) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMUHI (current_cpu, pc, * FLD (i_abase));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldos-indirect-index: ldos ($abase)[$index*S$scale], $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldos_indirect_index) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMUHI (current_cpu, pc, ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldos-disp: ldos $optdisp, $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldos_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMUHI (current_cpu, pc, FLD (f_optdisp));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldos-indirect-disp: ldos $optdisp($abase), $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldos_indirect_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMUHI (current_cpu, pc, ADDSI (FLD (f_optdisp), * FLD (i_abase)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldos-index-disp: ldos $optdisp[$index*S$scale], $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldos_index_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMUHI (current_cpu, pc, ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldos-indirect-index-disp: ldos $optdisp($abase)[$index*S$scale], $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldos_indirect_index_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMUHI (current_cpu, pc, ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))))));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldib-offset: ldib $offset, $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldib_offset) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMQI (current_cpu, pc, FLD (f_offset));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldib-indirect-offset: ldib $offset($abase), $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldib_indirect_offset) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMQI (current_cpu, pc, ADDSI (FLD (f_offset), * FLD (i_abase)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldib-indirect: ldib ($abase), $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldib_indirect) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMQI (current_cpu, pc, * FLD (i_abase));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldib-indirect-index: ldib ($abase)[$index*S$scale], $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldib_indirect_index) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMQI (current_cpu, pc, ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldib-disp: ldib $optdisp, $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldib_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMQI (current_cpu, pc, FLD (f_optdisp));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldib-indirect-disp: ldib $optdisp($abase), $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldib_indirect_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMQI (current_cpu, pc, ADDSI (FLD (f_optdisp), * FLD (i_abase)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldib-index-disp: ldib $optdisp[$index*S$scale], $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldib_index_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMQI (current_cpu, pc, ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldib-indirect-index-disp: ldib $optdisp($abase)[$index*S$scale], $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldib_indirect_index_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMQI (current_cpu, pc, ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))))));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldis-offset: ldis $offset, $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldis_offset) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMHI (current_cpu, pc, FLD (f_offset));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldis-indirect-offset: ldis $offset($abase), $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldis_indirect_offset) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMHI (current_cpu, pc, ADDSI (FLD (f_offset), * FLD (i_abase)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldis-indirect: ldis ($abase), $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldis_indirect) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMHI (current_cpu, pc, * FLD (i_abase));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldis-indirect-index: ldis ($abase)[$index*S$scale], $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldis_indirect_index) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMHI (current_cpu, pc, ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldis-disp: ldis $optdisp, $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldis_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMHI (current_cpu, pc, FLD (f_optdisp));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldis-indirect-disp: ldis $optdisp($abase), $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldis_indirect_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMHI (current_cpu, pc, ADDSI (FLD (f_optdisp), * FLD (i_abase)));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldis-index-disp: ldis $optdisp[$index*S$scale], $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldis_index_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMHI (current_cpu, pc, ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldis-indirect-index-disp: ldis $optdisp($abase)[$index*S$scale], $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldis_indirect_index_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMHI (current_cpu, pc, ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))))));
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldl-offset: ldl $offset, $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldl_offset) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = FLD (f_offset);
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* ldl-indirect-offset: ldl $offset($abase), $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldl_indirect_offset) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = ADDSI (FLD (f_offset), * FLD (i_abase));
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* ldl-indirect: ldl ($abase), $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldl_indirect) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = * FLD (i_abase);
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* ldl-indirect-index: ldl ($abase)[$index*S$scale], $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldl_indirect_index) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))));
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* ldl-disp: ldl $optdisp, $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldl_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = FLD (f_optdisp);
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* ldl-indirect-disp: ldl $optdisp($abase), $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldl_indirect_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = ADDSI (FLD (f_optdisp), * FLD (i_abase));
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* ldl-index-disp: ldl $optdisp[$index*S$scale], $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldl_index_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))));
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* ldl-indirect-index-disp: ldl $optdisp($abase)[$index*S$scale], $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldl_indirect_index_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))));
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* ldt-offset: ldt $offset, $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldt_offset) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = FLD (f_offset);
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 8));
    CPU (h_gr[((FLD (f_srcdst)) + (2))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-2", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* ldt-indirect-offset: ldt $offset($abase), $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldt_indirect_offset) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = ADDSI (FLD (f_offset), * FLD (i_abase));
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 8));
    CPU (h_gr[((FLD (f_srcdst)) + (2))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-2", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* ldt-indirect: ldt ($abase), $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldt_indirect) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = * FLD (i_abase);
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 8));
    CPU (h_gr[((FLD (f_srcdst)) + (2))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-2", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* ldt-indirect-index: ldt ($abase)[$index*S$scale], $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldt_indirect_index) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))));
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 8));
    CPU (h_gr[((FLD (f_srcdst)) + (2))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-2", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* ldt-disp: ldt $optdisp, $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldt_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = FLD (f_optdisp);
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 8));
    CPU (h_gr[((FLD (f_srcdst)) + (2))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-2", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* ldt-indirect-disp: ldt $optdisp($abase), $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldt_indirect_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = ADDSI (FLD (f_optdisp), * FLD (i_abase));
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 8));
    CPU (h_gr[((FLD (f_srcdst)) + (2))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-2", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* ldt-index-disp: ldt $optdisp[$index*S$scale], $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldt_index_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))));
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 8));
    CPU (h_gr[((FLD (f_srcdst)) + (2))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-2", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* ldt-indirect-index-disp: ldt $optdisp($abase)[$index*S$scale], $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldt_indirect_index_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))));
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 8));
    CPU (h_gr[((FLD (f_srcdst)) + (2))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-2", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* ldq-offset: ldq $offset, $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldq_offset) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = FLD (f_offset);
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 8));
    CPU (h_gr[((FLD (f_srcdst)) + (2))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-2", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 12));
    CPU (h_gr[((FLD (f_srcdst)) + (3))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-3", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* ldq-indirect-offset: ldq $offset($abase), $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldq_indirect_offset) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = ADDSI (FLD (f_offset), * FLD (i_abase));
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 8));
    CPU (h_gr[((FLD (f_srcdst)) + (2))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-2", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 12));
    CPU (h_gr[((FLD (f_srcdst)) + (3))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-3", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* ldq-indirect: ldq ($abase), $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldq_indirect) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = * FLD (i_abase);
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 8));
    CPU (h_gr[((FLD (f_srcdst)) + (2))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-2", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 12));
    CPU (h_gr[((FLD (f_srcdst)) + (3))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-3", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* ldq-indirect-index: ldq ($abase)[$index*S$scale], $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldq_indirect_index) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))));
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 8));
    CPU (h_gr[((FLD (f_srcdst)) + (2))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-2", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 12));
    CPU (h_gr[((FLD (f_srcdst)) + (3))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-3", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* ldq-disp: ldq $optdisp, $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldq_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = FLD (f_optdisp);
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 8));
    CPU (h_gr[((FLD (f_srcdst)) + (2))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-2", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 12));
    CPU (h_gr[((FLD (f_srcdst)) + (3))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-3", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* ldq-indirect-disp: ldq $optdisp($abase), $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldq_indirect_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = ADDSI (FLD (f_optdisp), * FLD (i_abase));
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 8));
    CPU (h_gr[((FLD (f_srcdst)) + (2))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-2", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 12));
    CPU (h_gr[((FLD (f_srcdst)) + (3))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-3", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* ldq-index-disp: ldq $optdisp[$index*S$scale], $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldq_index_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))));
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 8));
    CPU (h_gr[((FLD (f_srcdst)) + (2))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-2", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 12));
    CPU (h_gr[((FLD (f_srcdst)) + (3))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-3", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* ldq-indirect-index-disp: ldq $optdisp($abase)[$index*S$scale], $dst */

static SEM_PC
SEM_FN_NAME (i960base,ldq_indirect_index_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_temp;
  SI tmp_dregno;
  tmp_dregno = FLD (f_srcdst);
  tmp_temp = ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))));
  {
    SI opval = GETMEMSI (current_cpu, pc, tmp_temp);
    * FLD (i_dst) = opval;
    TRACE_RESULT (current_cpu, abuf, "dst", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 4));
    CPU (h_gr[((FLD (f_srcdst)) + (1))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-1", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 8));
    CPU (h_gr[((FLD (f_srcdst)) + (2))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-2", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (tmp_temp, 12));
    CPU (h_gr[((FLD (f_srcdst)) + (3))]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-add--DFLT-index-of--DFLT-dst-3", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* st-offset: st $st_src, $offset */

static SEM_PC
SEM_FN_NAME (i960base,st_offset) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, FLD (f_offset), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* st-indirect-offset: st $st_src, $offset($abase) */

static SEM_PC
SEM_FN_NAME (i960base,st_indirect_offset) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_offset), * FLD (i_abase)), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* st-indirect: st $st_src, ($abase) */

static SEM_PC
SEM_FN_NAME (i960base,st_indirect) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, * FLD (i_abase), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* st-indirect-index: st $st_src, ($abase)[$index*S$scale] */

static SEM_PC
SEM_FN_NAME (i960base,st_indirect_index) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* st-disp: st $st_src, $optdisp */

static SEM_PC
SEM_FN_NAME (i960base,st_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, FLD (f_optdisp), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* st-indirect-disp: st $st_src, $optdisp($abase) */

static SEM_PC
SEM_FN_NAME (i960base,st_indirect_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), * FLD (i_abase)), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* st-index-disp: st $st_src, $optdisp[$index*S$scale */

static SEM_PC
SEM_FN_NAME (i960base,st_index_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* st-indirect-index-disp: st $st_src, $optdisp($abase)[$index*S$scale] */

static SEM_PC
SEM_FN_NAME (i960base,st_indirect_index_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))))), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* stob-offset: stob $st_src, $offset */

static SEM_PC
SEM_FN_NAME (i960base,stob_offset) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    QI opval = * FLD (i_st_src);
    SETMEMQI (current_cpu, pc, FLD (f_offset), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* stob-indirect-offset: stob $st_src, $offset($abase) */

static SEM_PC
SEM_FN_NAME (i960base,stob_indirect_offset) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    QI opval = * FLD (i_st_src);
    SETMEMQI (current_cpu, pc, ADDSI (FLD (f_offset), * FLD (i_abase)), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* stob-indirect: stob $st_src, ($abase) */

static SEM_PC
SEM_FN_NAME (i960base,stob_indirect) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    QI opval = * FLD (i_st_src);
    SETMEMQI (current_cpu, pc, * FLD (i_abase), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* stob-indirect-index: stob $st_src, ($abase)[$index*S$scale] */

static SEM_PC
SEM_FN_NAME (i960base,stob_indirect_index) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    QI opval = * FLD (i_st_src);
    SETMEMQI (current_cpu, pc, ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* stob-disp: stob $st_src, $optdisp */

static SEM_PC
SEM_FN_NAME (i960base,stob_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    QI opval = * FLD (i_st_src);
    SETMEMQI (current_cpu, pc, FLD (f_optdisp), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* stob-indirect-disp: stob $st_src, $optdisp($abase) */

static SEM_PC
SEM_FN_NAME (i960base,stob_indirect_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    QI opval = * FLD (i_st_src);
    SETMEMQI (current_cpu, pc, ADDSI (FLD (f_optdisp), * FLD (i_abase)), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* stob-index-disp: stob $st_src, $optdisp[$index*S$scale */

static SEM_PC
SEM_FN_NAME (i960base,stob_index_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    QI opval = * FLD (i_st_src);
    SETMEMQI (current_cpu, pc, ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* stob-indirect-index-disp: stob $st_src, $optdisp($abase)[$index*S$scale] */

static SEM_PC
SEM_FN_NAME (i960base,stob_indirect_index_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    QI opval = * FLD (i_st_src);
    SETMEMQI (current_cpu, pc, ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))))), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* stos-offset: stos $st_src, $offset */

static SEM_PC
SEM_FN_NAME (i960base,stos_offset) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    HI opval = * FLD (i_st_src);
    SETMEMHI (current_cpu, pc, FLD (f_offset), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* stos-indirect-offset: stos $st_src, $offset($abase) */

static SEM_PC
SEM_FN_NAME (i960base,stos_indirect_offset) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    HI opval = * FLD (i_st_src);
    SETMEMHI (current_cpu, pc, ADDSI (FLD (f_offset), * FLD (i_abase)), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* stos-indirect: stos $st_src, ($abase) */

static SEM_PC
SEM_FN_NAME (i960base,stos_indirect) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    HI opval = * FLD (i_st_src);
    SETMEMHI (current_cpu, pc, * FLD (i_abase), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* stos-indirect-index: stos $st_src, ($abase)[$index*S$scale] */

static SEM_PC
SEM_FN_NAME (i960base,stos_indirect_index) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    HI opval = * FLD (i_st_src);
    SETMEMHI (current_cpu, pc, ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* stos-disp: stos $st_src, $optdisp */

static SEM_PC
SEM_FN_NAME (i960base,stos_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    HI opval = * FLD (i_st_src);
    SETMEMHI (current_cpu, pc, FLD (f_optdisp), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* stos-indirect-disp: stos $st_src, $optdisp($abase) */

static SEM_PC
SEM_FN_NAME (i960base,stos_indirect_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    HI opval = * FLD (i_st_src);
    SETMEMHI (current_cpu, pc, ADDSI (FLD (f_optdisp), * FLD (i_abase)), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* stos-index-disp: stos $st_src, $optdisp[$index*S$scale */

static SEM_PC
SEM_FN_NAME (i960base,stos_index_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    HI opval = * FLD (i_st_src);
    SETMEMHI (current_cpu, pc, ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* stos-indirect-index-disp: stos $st_src, $optdisp($abase)[$index*S$scale] */

static SEM_PC
SEM_FN_NAME (i960base,stos_indirect_index_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    HI opval = * FLD (i_st_src);
    SETMEMHI (current_cpu, pc, ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))))), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* stl-offset: stl $st_src, $offset */

static SEM_PC
SEM_FN_NAME (i960base,stl_offset) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, FLD (f_offset), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_offset), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* stl-indirect-offset: stl $st_src, $offset($abase) */

static SEM_PC
SEM_FN_NAME (i960base,stl_indirect_offset) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_offset), * FLD (i_abase)), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_offset), * FLD (i_abase)), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* stl-indirect: stl $st_src, ($abase) */

static SEM_PC
SEM_FN_NAME (i960base,stl_indirect) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, * FLD (i_abase), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (* FLD (i_abase), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* stl-indirect-index: stl $st_src, ($abase)[$index*S$scale] */

static SEM_PC
SEM_FN_NAME (i960base,stl_indirect_index) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* stl-disp: stl $st_src, $optdisp */

static SEM_PC
SEM_FN_NAME (i960base,stl_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, FLD (f_optdisp), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* stl-indirect-disp: stl $st_src, $optdisp($abase) */

static SEM_PC
SEM_FN_NAME (i960base,stl_indirect_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), * FLD (i_abase)), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_optdisp), * FLD (i_abase)), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* stl-index-disp: stl $st_src, $optdisp[$index*S$scale */

static SEM_PC
SEM_FN_NAME (i960base,stl_index_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* stl-indirect-index-disp: stl $st_src, $optdisp($abase)[$index*S$scale] */

static SEM_PC
SEM_FN_NAME (i960base,stl_indirect_index_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))))), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))))), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* stt-offset: stt $st_src, $offset */

static SEM_PC
SEM_FN_NAME (i960base,stt_offset) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, FLD (f_offset), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_offset), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (2))]);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_offset), 8), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* stt-indirect-offset: stt $st_src, $offset($abase) */

static SEM_PC
SEM_FN_NAME (i960base,stt_indirect_offset) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_offset), * FLD (i_abase)), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_offset), * FLD (i_abase)), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (2))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_offset), * FLD (i_abase)), 8), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* stt-indirect: stt $st_src, ($abase) */

static SEM_PC
SEM_FN_NAME (i960base,stt_indirect) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, * FLD (i_abase), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (* FLD (i_abase), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (2))]);
    SETMEMSI (current_cpu, pc, ADDSI (* FLD (i_abase), 8), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* stt-indirect-index: stt $st_src, ($abase)[$index*S$scale] */

static SEM_PC
SEM_FN_NAME (i960base,stt_indirect_index) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (2))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), 8), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* stt-disp: stt $st_src, $optdisp */

static SEM_PC
SEM_FN_NAME (i960base,stt_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, FLD (f_optdisp), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (2))]);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), 8), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* stt-indirect-disp: stt $st_src, $optdisp($abase) */

static SEM_PC
SEM_FN_NAME (i960base,stt_indirect_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), * FLD (i_abase)), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_optdisp), * FLD (i_abase)), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (2))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_optdisp), * FLD (i_abase)), 8), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* stt-index-disp: stt $st_src, $optdisp[$index*S$scale */

static SEM_PC
SEM_FN_NAME (i960base,stt_index_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (2))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), 8), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* stt-indirect-index-disp: stt $st_src, $optdisp($abase)[$index*S$scale] */

static SEM_PC
SEM_FN_NAME (i960base,stt_indirect_index_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))))), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))))), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (2))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))))), 8), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* stq-offset: stq $st_src, $offset */

static SEM_PC
SEM_FN_NAME (i960base,stq_offset) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, FLD (f_offset), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_offset), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (2))]);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_offset), 8), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (3))]);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_offset), 12), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* stq-indirect-offset: stq $st_src, $offset($abase) */

static SEM_PC
SEM_FN_NAME (i960base,stq_indirect_offset) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_offset), * FLD (i_abase)), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_offset), * FLD (i_abase)), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (2))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_offset), * FLD (i_abase)), 8), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (3))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_offset), * FLD (i_abase)), 12), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* stq-indirect: stq $st_src, ($abase) */

static SEM_PC
SEM_FN_NAME (i960base,stq_indirect) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, * FLD (i_abase), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (* FLD (i_abase), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (2))]);
    SETMEMSI (current_cpu, pc, ADDSI (* FLD (i_abase), 8), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (3))]);
    SETMEMSI (current_cpu, pc, ADDSI (* FLD (i_abase), 12), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* stq-indirect-index: stq $st_src, ($abase)[$index*S$scale] */

static SEM_PC
SEM_FN_NAME (i960base,stq_indirect_index) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (2))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), 8), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (3))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), 12), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* stq-disp: stq $st_src, $optdisp */

static SEM_PC
SEM_FN_NAME (i960base,stq_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, FLD (f_optdisp), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (2))]);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), 8), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (3))]);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), 12), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* stq-indirect-disp: stq $st_src, $optdisp($abase) */

static SEM_PC
SEM_FN_NAME (i960base,stq_indirect_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), * FLD (i_abase)), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_optdisp), * FLD (i_abase)), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (2))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_optdisp), * FLD (i_abase)), 8), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (3))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_optdisp), * FLD (i_abase)), 12), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* stq-index-disp: stq $st_src, $optdisp[$index*S$scale */

static SEM_PC
SEM_FN_NAME (i960base,stq_index_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (2))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), 8), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (3))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_optdisp), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale)))), 12), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* stq-indirect-index-disp: stq $st_src, $optdisp($abase)[$index*S$scale] */

static SEM_PC
SEM_FN_NAME (i960base,stq_indirect_index_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_sregno;
  tmp_sregno = FLD (f_srcdst);
  {
    SI opval = * FLD (i_st_src);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))))), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (1))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))))), 4), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (2))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))))), 8), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((FLD (f_srcdst)) + (3))]);
    SETMEMSI (current_cpu, pc, ADDSI (ADDSI (FLD (f_optdisp), ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))))), 12), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* cmpobe-reg: cmpobe $br_src1, $br_src2, $br_disp */

static SEM_PC
SEM_FN_NAME (i960base,cmpobe_reg) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (EQSI (* FLD (i_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* cmpobe-lit: cmpobe $br_lit1, $br_src2, $br_disp */

static SEM_PC
SEM_FN_NAME (i960base,cmpobe_lit) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (EQSI (FLD (f_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* cmpobne-reg: cmpobne $br_src1, $br_src2, $br_disp */

static SEM_PC
SEM_FN_NAME (i960base,cmpobne_reg) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (NESI (* FLD (i_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* cmpobne-lit: cmpobne $br_lit1, $br_src2, $br_disp */

static SEM_PC
SEM_FN_NAME (i960base,cmpobne_lit) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (NESI (FLD (f_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* cmpobl-reg: cmpobl $br_src1, $br_src2, $br_disp */

static SEM_PC
SEM_FN_NAME (i960base,cmpobl_reg) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (LTUSI (* FLD (i_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* cmpobl-lit: cmpobl $br_lit1, $br_src2, $br_disp */

static SEM_PC
SEM_FN_NAME (i960base,cmpobl_lit) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (LTUSI (FLD (f_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* cmpoble-reg: cmpoble $br_src1, $br_src2, $br_disp */

static SEM_PC
SEM_FN_NAME (i960base,cmpoble_reg) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (LEUSI (* FLD (i_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* cmpoble-lit: cmpoble $br_lit1, $br_src2, $br_disp */

static SEM_PC
SEM_FN_NAME (i960base,cmpoble_lit) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (LEUSI (FLD (f_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* cmpobg-reg: cmpobg $br_src1, $br_src2, $br_disp */

static SEM_PC
SEM_FN_NAME (i960base,cmpobg_reg) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (GTUSI (* FLD (i_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* cmpobg-lit: cmpobg $br_lit1, $br_src2, $br_disp */

static SEM_PC
SEM_FN_NAME (i960base,cmpobg_lit) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (GTUSI (FLD (f_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* cmpobge-reg: cmpobge $br_src1, $br_src2, $br_disp */

static SEM_PC
SEM_FN_NAME (i960base,cmpobge_reg) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (GEUSI (* FLD (i_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* cmpobge-lit: cmpobge $br_lit1, $br_src2, $br_disp */

static SEM_PC
SEM_FN_NAME (i960base,cmpobge_lit) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (GEUSI (FLD (f_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* cmpibe-reg: cmpibe $br_src1, $br_src2, $br_disp */

static SEM_PC
SEM_FN_NAME (i960base,cmpibe_reg) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (EQSI (* FLD (i_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* cmpibe-lit: cmpibe $br_lit1, $br_src2, $br_disp */

static SEM_PC
SEM_FN_NAME (i960base,cmpibe_lit) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (EQSI (FLD (f_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* cmpibne-reg: cmpibne $br_src1, $br_src2, $br_disp */

static SEM_PC
SEM_FN_NAME (i960base,cmpibne_reg) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (NESI (* FLD (i_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* cmpibne-lit: cmpibne $br_lit1, $br_src2, $br_disp */

static SEM_PC
SEM_FN_NAME (i960base,cmpibne_lit) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (NESI (FLD (f_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* cmpibl-reg: cmpibl $br_src1, $br_src2, $br_disp */

static SEM_PC
SEM_FN_NAME (i960base,cmpibl_reg) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (LTSI (* FLD (i_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* cmpibl-lit: cmpibl $br_lit1, $br_src2, $br_disp */

static SEM_PC
SEM_FN_NAME (i960base,cmpibl_lit) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (LTSI (FLD (f_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* cmpible-reg: cmpible $br_src1, $br_src2, $br_disp */

static SEM_PC
SEM_FN_NAME (i960base,cmpible_reg) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (LESI (* FLD (i_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* cmpible-lit: cmpible $br_lit1, $br_src2, $br_disp */

static SEM_PC
SEM_FN_NAME (i960base,cmpible_lit) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (LESI (FLD (f_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* cmpibg-reg: cmpibg $br_src1, $br_src2, $br_disp */

static SEM_PC
SEM_FN_NAME (i960base,cmpibg_reg) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (GTSI (* FLD (i_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* cmpibg-lit: cmpibg $br_lit1, $br_src2, $br_disp */

static SEM_PC
SEM_FN_NAME (i960base,cmpibg_lit) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (GTSI (FLD (f_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* cmpibge-reg: cmpibge $br_src1, $br_src2, $br_disp */

static SEM_PC
SEM_FN_NAME (i960base,cmpibge_reg) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (GESI (* FLD (i_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* cmpibge-lit: cmpibge $br_lit1, $br_src2, $br_disp */

static SEM_PC
SEM_FN_NAME (i960base,cmpibge_lit) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (GESI (FLD (f_br_src1), * FLD (i_br_src2))) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* bbc-reg: bbc $br_src1, $br_src2, $br_disp */

static SEM_PC
SEM_FN_NAME (i960base,bbc_reg) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (EQSI (ANDSI (SLLSI (1, * FLD (i_br_src1)), * FLD (i_br_src2)), 0)) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* bbc-lit: bbc $br_lit1, $br_src2, $br_disp */

static SEM_PC
SEM_FN_NAME (i960base,bbc_lit) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (EQSI (ANDSI (SLLSI (1, FLD (f_br_src1)), * FLD (i_br_src2)), 0)) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* bbs-reg: bbs $br_src1, $br_src2, $br_disp */

static SEM_PC
SEM_FN_NAME (i960base,bbs_reg) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_reg.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (NESI (ANDSI (SLLSI (1, * FLD (i_br_src1)), * FLD (i_br_src2)), 0)) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* bbs-lit: bbs $br_lit1, $br_src2, $br_disp */

static SEM_PC
SEM_FN_NAME (i960base,bbs_lit) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_cmpobe_lit.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (NESI (ANDSI (SLLSI (1, FLD (f_br_src1)), * FLD (i_br_src2)), 0)) {
  {
    USI opval = FLD (i_br_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* cmpi: cmpi $src1, $src2 */

static SEM_PC
SEM_FN_NAME (i960base,cmpi) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (LTSI (* FLD (i_src1), * FLD (i_src2))) ? (4) : (EQSI (* FLD (i_src1), * FLD (i_src2))) ? (2) : (1);
    CPU (h_cc) = opval;
    TRACE_RESULT (current_cpu, abuf, "cc", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* cmpi1: cmpi $lit1, $src2 */

static SEM_PC
SEM_FN_NAME (i960base,cmpi1) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (LTSI (FLD (f_src1), * FLD (i_src2))) ? (4) : (EQSI (FLD (f_src1), * FLD (i_src2))) ? (2) : (1);
    CPU (h_cc) = opval;
    TRACE_RESULT (current_cpu, abuf, "cc", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* cmpi2: cmpi $src1, $lit2 */

static SEM_PC
SEM_FN_NAME (i960base,cmpi2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (LTSI (* FLD (i_src1), FLD (f_src2))) ? (4) : (EQSI (* FLD (i_src1), FLD (f_src2))) ? (2) : (1);
    CPU (h_cc) = opval;
    TRACE_RESULT (current_cpu, abuf, "cc", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* cmpi3: cmpi $lit1, $lit2 */

static SEM_PC
SEM_FN_NAME (i960base,cmpi3) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (LTSI (FLD (f_src1), FLD (f_src2))) ? (4) : (EQSI (FLD (f_src1), FLD (f_src2))) ? (2) : (1);
    CPU (h_cc) = opval;
    TRACE_RESULT (current_cpu, abuf, "cc", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* cmpo: cmpo $src1, $src2 */

static SEM_PC
SEM_FN_NAME (i960base,cmpo) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (LTUSI (* FLD (i_src1), * FLD (i_src2))) ? (4) : (EQSI (* FLD (i_src1), * FLD (i_src2))) ? (2) : (1);
    CPU (h_cc) = opval;
    TRACE_RESULT (current_cpu, abuf, "cc", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* cmpo1: cmpo $lit1, $src2 */

static SEM_PC
SEM_FN_NAME (i960base,cmpo1) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul1.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (LTUSI (FLD (f_src1), * FLD (i_src2))) ? (4) : (EQSI (FLD (f_src1), * FLD (i_src2))) ? (2) : (1);
    CPU (h_cc) = opval;
    TRACE_RESULT (current_cpu, abuf, "cc", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* cmpo2: cmpo $src1, $lit2 */

static SEM_PC
SEM_FN_NAME (i960base,cmpo2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (LTUSI (* FLD (i_src1), FLD (f_src2))) ? (4) : (EQSI (* FLD (i_src1), FLD (f_src2))) ? (2) : (1);
    CPU (h_cc) = opval;
    TRACE_RESULT (current_cpu, abuf, "cc", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* cmpo3: cmpo $lit1, $lit2 */

static SEM_PC
SEM_FN_NAME (i960base,cmpo3) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul3.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = (LTUSI (FLD (f_src1), FLD (f_src2))) ? (4) : (EQSI (FLD (f_src1), FLD (f_src2))) ? (2) : (1);
    CPU (h_cc) = opval;
    TRACE_RESULT (current_cpu, abuf, "cc", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* testno-reg: testno $br_src1 */

static SEM_PC
SEM_FN_NAME (i960base,testno_reg) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_testno_reg.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = EQSI (CPU (h_cc), 0);
    * FLD (i_br_src1) = opval;
    TRACE_RESULT (current_cpu, abuf, "br_src1", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* testg-reg: testg $br_src1 */

static SEM_PC
SEM_FN_NAME (i960base,testg_reg) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_testno_reg.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = NESI (ANDSI (CPU (h_cc), 1), 0);
    * FLD (i_br_src1) = opval;
    TRACE_RESULT (current_cpu, abuf, "br_src1", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* teste-reg: teste $br_src1 */

static SEM_PC
SEM_FN_NAME (i960base,teste_reg) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_testno_reg.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = NESI (ANDSI (CPU (h_cc), 2), 0);
    * FLD (i_br_src1) = opval;
    TRACE_RESULT (current_cpu, abuf, "br_src1", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* testge-reg: testge $br_src1 */

static SEM_PC
SEM_FN_NAME (i960base,testge_reg) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_testno_reg.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = NESI (ANDSI (CPU (h_cc), 3), 0);
    * FLD (i_br_src1) = opval;
    TRACE_RESULT (current_cpu, abuf, "br_src1", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* testl-reg: testl $br_src1 */

static SEM_PC
SEM_FN_NAME (i960base,testl_reg) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_testno_reg.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = NESI (ANDSI (CPU (h_cc), 4), 0);
    * FLD (i_br_src1) = opval;
    TRACE_RESULT (current_cpu, abuf, "br_src1", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* testne-reg: testne $br_src1 */

static SEM_PC
SEM_FN_NAME (i960base,testne_reg) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_testno_reg.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = NESI (ANDSI (CPU (h_cc), 5), 0);
    * FLD (i_br_src1) = opval;
    TRACE_RESULT (current_cpu, abuf, "br_src1", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* testle-reg: testle $br_src1 */

static SEM_PC
SEM_FN_NAME (i960base,testle_reg) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_testno_reg.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = NESI (ANDSI (CPU (h_cc), 6), 0);
    * FLD (i_br_src1) = opval;
    TRACE_RESULT (current_cpu, abuf, "br_src1", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* testo-reg: testo $br_src1 */

static SEM_PC
SEM_FN_NAME (i960base,testo_reg) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_testno_reg.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = NESI (ANDSI (CPU (h_cc), 7), 0);
    * FLD (i_br_src1) = opval;
    TRACE_RESULT (current_cpu, abuf, "br_src1", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* bno: bno $ctrl_disp */

static SEM_PC
SEM_FN_NAME (i960base,bno) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_bno.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (EQSI (CPU (h_cc), 0)) {
  {
    USI opval = FLD (i_ctrl_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* bg: bg $ctrl_disp */

static SEM_PC
SEM_FN_NAME (i960base,bg) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_bno.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (NESI (ANDSI (CPU (h_cc), 1), 0)) {
  {
    USI opval = FLD (i_ctrl_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* be: be $ctrl_disp */

static SEM_PC
SEM_FN_NAME (i960base,be) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_bno.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (NESI (ANDSI (CPU (h_cc), 2), 0)) {
  {
    USI opval = FLD (i_ctrl_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* bge: bge $ctrl_disp */

static SEM_PC
SEM_FN_NAME (i960base,bge) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_bno.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (NESI (ANDSI (CPU (h_cc), 3), 0)) {
  {
    USI opval = FLD (i_ctrl_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* bl: bl $ctrl_disp */

static SEM_PC
SEM_FN_NAME (i960base,bl) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_bno.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (NESI (ANDSI (CPU (h_cc), 4), 0)) {
  {
    USI opval = FLD (i_ctrl_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* bne: bne $ctrl_disp */

static SEM_PC
SEM_FN_NAME (i960base,bne) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_bno.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (NESI (ANDSI (CPU (h_cc), 5), 0)) {
  {
    USI opval = FLD (i_ctrl_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* ble: ble $ctrl_disp */

static SEM_PC
SEM_FN_NAME (i960base,ble) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_bno.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (NESI (ANDSI (CPU (h_cc), 6), 0)) {
  {
    USI opval = FLD (i_ctrl_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* bo: bo $ctrl_disp */

static SEM_PC
SEM_FN_NAME (i960base,bo) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_bno.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (NESI (ANDSI (CPU (h_cc), 7), 0)) {
  {
    USI opval = FLD (i_ctrl_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* b: b $ctrl_disp */

static SEM_PC
SEM_FN_NAME (i960base,b) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_bno.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = FLD (i_ctrl_disp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }

  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* bx-indirect-offset: bx $offset($abase) */

static SEM_PC
SEM_FN_NAME (i960base,bx_indirect_offset) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = ADDSI (FLD (f_offset), * FLD (i_abase));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }

  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* bx-indirect: bx ($abase) */

static SEM_PC
SEM_FN_NAME (i960base,bx_indirect) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = * FLD (i_abase);
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }

  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* bx-indirect-index: bx ($abase)[$index*S$scale] */

static SEM_PC
SEM_FN_NAME (i960base,bx_indirect_index) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = ADDSI (* FLD (i_abase), MULSI (* FLD (i_index), SLLSI (1, FLD (f_scale))));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }

  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* bx-disp: bx $optdisp */

static SEM_PC
SEM_FN_NAME (i960base,bx_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = FLD (f_optdisp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }

  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* bx-indirect-disp: bx $optdisp($abase) */

static SEM_PC
SEM_FN_NAME (i960base,bx_indirect_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stq_indirect_index_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = ADDSI (FLD (f_optdisp), * FLD (i_abase));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }

  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* callx-disp: callx $optdisp */

static SEM_PC
SEM_FN_NAME (i960base,callx_disp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_callx_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_temp;
  tmp_temp = ANDSI (ADDSI (CPU (h_gr[((UINT) 1)]), 63), INVSI (63));
  {
    SI opval = ADDSI (pc, 8);
    CPU (h_gr[((UINT) 2)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-2", 'x', opval);
  }
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 0), CPU (h_gr[((UINT) 0)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 4), CPU (h_gr[((UINT) 1)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 8), CPU (h_gr[((UINT) 2)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 12), CPU (h_gr[((UINT) 3)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 16), CPU (h_gr[((UINT) 4)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 20), CPU (h_gr[((UINT) 5)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 24), CPU (h_gr[((UINT) 6)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 28), CPU (h_gr[((UINT) 7)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 32), CPU (h_gr[((UINT) 8)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 36), CPU (h_gr[((UINT) 9)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 40), CPU (h_gr[((UINT) 10)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 44), CPU (h_gr[((UINT) 11)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 48), CPU (h_gr[((UINT) 12)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 52), CPU (h_gr[((UINT) 13)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 56), CPU (h_gr[((UINT) 14)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 60), CPU (h_gr[((UINT) 15)]));
  {
    USI opval = FLD (f_optdisp);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
CPU (h_gr[((UINT) 0)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 1)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 2)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 3)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 4)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 5)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 6)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 7)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 8)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 9)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 10)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 11)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 12)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 13)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 14)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 15)]) = 0xdeadbeef;
  {
    SI opval = CPU (h_gr[((UINT) 31)]);
    CPU (h_gr[((UINT) 0)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-0", 'x', opval);
  }
  {
    SI opval = tmp_temp;
    CPU (h_gr[((UINT) 31)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-31", 'x', opval);
  }
  {
    SI opval = ADDSI (tmp_temp, 64);
    CPU (h_gr[((UINT) 1)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-1", 'x', opval);
  }
}

  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* callx-indirect: callx ($abase) */

static SEM_PC
SEM_FN_NAME (i960base,callx_indirect) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_callx_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_temp;
  tmp_temp = ANDSI (ADDSI (CPU (h_gr[((UINT) 1)]), 63), INVSI (63));
  {
    SI opval = ADDSI (pc, 4);
    CPU (h_gr[((UINT) 2)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-2", 'x', opval);
  }
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 0), CPU (h_gr[((UINT) 0)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 4), CPU (h_gr[((UINT) 1)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 8), CPU (h_gr[((UINT) 2)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 12), CPU (h_gr[((UINT) 3)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 16), CPU (h_gr[((UINT) 4)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 20), CPU (h_gr[((UINT) 5)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 24), CPU (h_gr[((UINT) 6)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 28), CPU (h_gr[((UINT) 7)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 32), CPU (h_gr[((UINT) 8)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 36), CPU (h_gr[((UINT) 9)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 40), CPU (h_gr[((UINT) 10)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 44), CPU (h_gr[((UINT) 11)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 48), CPU (h_gr[((UINT) 12)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 52), CPU (h_gr[((UINT) 13)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 56), CPU (h_gr[((UINT) 14)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 60), CPU (h_gr[((UINT) 15)]));
  {
    USI opval = * FLD (i_abase);
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
CPU (h_gr[((UINT) 0)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 1)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 2)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 3)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 4)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 5)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 6)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 7)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 8)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 9)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 10)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 11)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 12)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 13)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 14)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 15)]) = 0xdeadbeef;
  {
    SI opval = CPU (h_gr[((UINT) 31)]);
    CPU (h_gr[((UINT) 0)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-0", 'x', opval);
  }
  {
    SI opval = tmp_temp;
    CPU (h_gr[((UINT) 31)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-31", 'x', opval);
  }
  {
    SI opval = ADDSI (tmp_temp, 64);
    CPU (h_gr[((UINT) 1)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-1", 'x', opval);
  }
}

  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* callx-indirect-offset: callx $offset($abase) */

static SEM_PC
SEM_FN_NAME (i960base,callx_indirect_offset) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_callx_indirect_offset.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_temp;
  tmp_temp = ANDSI (ADDSI (CPU (h_gr[((UINT) 1)]), 63), INVSI (63));
  {
    SI opval = ADDSI (pc, 4);
    CPU (h_gr[((UINT) 2)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-2", 'x', opval);
  }
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 0), CPU (h_gr[((UINT) 0)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 4), CPU (h_gr[((UINT) 1)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 8), CPU (h_gr[((UINT) 2)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 12), CPU (h_gr[((UINT) 3)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 16), CPU (h_gr[((UINT) 4)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 20), CPU (h_gr[((UINT) 5)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 24), CPU (h_gr[((UINT) 6)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 28), CPU (h_gr[((UINT) 7)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 32), CPU (h_gr[((UINT) 8)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 36), CPU (h_gr[((UINT) 9)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 40), CPU (h_gr[((UINT) 10)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 44), CPU (h_gr[((UINT) 11)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 48), CPU (h_gr[((UINT) 12)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 52), CPU (h_gr[((UINT) 13)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 56), CPU (h_gr[((UINT) 14)]));
SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 60), CPU (h_gr[((UINT) 15)]));
  {
    USI opval = ADDSI (FLD (f_offset), * FLD (i_abase));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
CPU (h_gr[((UINT) 0)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 1)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 2)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 3)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 4)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 5)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 6)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 7)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 8)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 9)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 10)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 11)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 12)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 13)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 14)]) = 0xdeadbeef;
CPU (h_gr[((UINT) 15)]) = 0xdeadbeef;
  {
    SI opval = CPU (h_gr[((UINT) 31)]);
    CPU (h_gr[((UINT) 0)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-0", 'x', opval);
  }
  {
    SI opval = tmp_temp;
    CPU (h_gr[((UINT) 31)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-31", 'x', opval);
  }
  {
    SI opval = ADDSI (tmp_temp, 64);
    CPU (h_gr[((UINT) 1)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-1", 'x', opval);
  }
}

  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* ret: ret */

static SEM_PC
SEM_FN_NAME (i960base,ret) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_callx_disp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  {
    SI opval = CPU (h_gr[((UINT) 0)]);
    CPU (h_gr[((UINT) 31)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-31", 'x', opval);
  }
CPU (h_gr[((UINT) 0)]) = GETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 0));
CPU (h_gr[((UINT) 1)]) = GETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 4));
CPU (h_gr[((UINT) 2)]) = GETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 8));
CPU (h_gr[((UINT) 3)]) = GETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 12));
CPU (h_gr[((UINT) 4)]) = GETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 16));
CPU (h_gr[((UINT) 5)]) = GETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 20));
CPU (h_gr[((UINT) 6)]) = GETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 24));
CPU (h_gr[((UINT) 7)]) = GETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 28));
CPU (h_gr[((UINT) 8)]) = GETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 32));
CPU (h_gr[((UINT) 9)]) = GETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 36));
CPU (h_gr[((UINT) 10)]) = GETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 40));
CPU (h_gr[((UINT) 11)]) = GETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 44));
CPU (h_gr[((UINT) 12)]) = GETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 48));
CPU (h_gr[((UINT) 13)]) = GETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 52));
CPU (h_gr[((UINT) 14)]) = GETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 56));
CPU (h_gr[((UINT) 15)]) = GETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 31)]), 60));
  {
    USI opval = CPU (h_gr[((UINT) 2)]);
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* calls: calls $src1 */

static SEM_PC
SEM_FN_NAME (i960base,calls) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_emul2.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = i960_trap (current_cpu, pc, * FLD (i_src1));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }

  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* fmark: fmark */

static SEM_PC
SEM_FN_NAME (i960base,fmark) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.fmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = i960_breakpoint (current_cpu, pc);
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }

  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* flushreg: flushreg */

static SEM_PC
SEM_FN_NAME (i960base,flushreg) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.fmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

((void) 0); /*nop*/

  return vpc;
#undef FLD
}

/* Table of all semantic fns.  */

static const struct sem_fn_desc sem_fns[] = {
  { I960BASE_INSN_X_INVALID, SEM_FN_NAME (i960base,x_invalid) },
  { I960BASE_INSN_X_AFTER, SEM_FN_NAME (i960base,x_after) },
  { I960BASE_INSN_X_BEFORE, SEM_FN_NAME (i960base,x_before) },
  { I960BASE_INSN_X_CTI_CHAIN, SEM_FN_NAME (i960base,x_cti_chain) },
  { I960BASE_INSN_X_CHAIN, SEM_FN_NAME (i960base,x_chain) },
  { I960BASE_INSN_X_BEGIN, SEM_FN_NAME (i960base,x_begin) },
  { I960BASE_INSN_MULO, SEM_FN_NAME (i960base,mulo) },
  { I960BASE_INSN_MULO1, SEM_FN_NAME (i960base,mulo1) },
  { I960BASE_INSN_MULO2, SEM_FN_NAME (i960base,mulo2) },
  { I960BASE_INSN_MULO3, SEM_FN_NAME (i960base,mulo3) },
  { I960BASE_INSN_REMO, SEM_FN_NAME (i960base,remo) },
  { I960BASE_INSN_REMO1, SEM_FN_NAME (i960base,remo1) },
  { I960BASE_INSN_REMO2, SEM_FN_NAME (i960base,remo2) },
  { I960BASE_INSN_REMO3, SEM_FN_NAME (i960base,remo3) },
  { I960BASE_INSN_DIVO, SEM_FN_NAME (i960base,divo) },
  { I960BASE_INSN_DIVO1, SEM_FN_NAME (i960base,divo1) },
  { I960BASE_INSN_DIVO2, SEM_FN_NAME (i960base,divo2) },
  { I960BASE_INSN_DIVO3, SEM_FN_NAME (i960base,divo3) },
  { I960BASE_INSN_REMI, SEM_FN_NAME (i960base,remi) },
  { I960BASE_INSN_REMI1, SEM_FN_NAME (i960base,remi1) },
  { I960BASE_INSN_REMI2, SEM_FN_NAME (i960base,remi2) },
  { I960BASE_INSN_REMI3, SEM_FN_NAME (i960base,remi3) },
  { I960BASE_INSN_DIVI, SEM_FN_NAME (i960base,divi) },
  { I960BASE_INSN_DIVI1, SEM_FN_NAME (i960base,divi1) },
  { I960BASE_INSN_DIVI2, SEM_FN_NAME (i960base,divi2) },
  { I960BASE_INSN_DIVI3, SEM_FN_NAME (i960base,divi3) },
  { I960BASE_INSN_ADDO, SEM_FN_NAME (i960base,addo) },
  { I960BASE_INSN_ADDO1, SEM_FN_NAME (i960base,addo1) },
  { I960BASE_INSN_ADDO2, SEM_FN_NAME (i960base,addo2) },
  { I960BASE_INSN_ADDO3, SEM_FN_NAME (i960base,addo3) },
  { I960BASE_INSN_SUBO, SEM_FN_NAME (i960base,subo) },
  { I960BASE_INSN_SUBO1, SEM_FN_NAME (i960base,subo1) },
  { I960BASE_INSN_SUBO2, SEM_FN_NAME (i960base,subo2) },
  { I960BASE_INSN_SUBO3, SEM_FN_NAME (i960base,subo3) },
  { I960BASE_INSN_NOTBIT, SEM_FN_NAME (i960base,notbit) },
  { I960BASE_INSN_NOTBIT1, SEM_FN_NAME (i960base,notbit1) },
  { I960BASE_INSN_NOTBIT2, SEM_FN_NAME (i960base,notbit2) },
  { I960BASE_INSN_NOTBIT3, SEM_FN_NAME (i960base,notbit3) },
  { I960BASE_INSN_AND, SEM_FN_NAME (i960base,and) },
  { I960BASE_INSN_AND1, SEM_FN_NAME (i960base,and1) },
  { I960BASE_INSN_AND2, SEM_FN_NAME (i960base,and2) },
  { I960BASE_INSN_AND3, SEM_FN_NAME (i960base,and3) },
  { I960BASE_INSN_ANDNOT, SEM_FN_NAME (i960base,andnot) },
  { I960BASE_INSN_ANDNOT1, SEM_FN_NAME (i960base,andnot1) },
  { I960BASE_INSN_ANDNOT2, SEM_FN_NAME (i960base,andnot2) },
  { I960BASE_INSN_ANDNOT3, SEM_FN_NAME (i960base,andnot3) },
  { I960BASE_INSN_SETBIT, SEM_FN_NAME (i960base,setbit) },
  { I960BASE_INSN_SETBIT1, SEM_FN_NAME (i960base,setbit1) },
  { I960BASE_INSN_SETBIT2, SEM_FN_NAME (i960base,setbit2) },
  { I960BASE_INSN_SETBIT3, SEM_FN_NAME (i960base,setbit3) },
  { I960BASE_INSN_NOTAND, SEM_FN_NAME (i960base,notand) },
  { I960BASE_INSN_NOTAND1, SEM_FN_NAME (i960base,notand1) },
  { I960BASE_INSN_NOTAND2, SEM_FN_NAME (i960base,notand2) },
  { I960BASE_INSN_NOTAND3, SEM_FN_NAME (i960base,notand3) },
  { I960BASE_INSN_XOR, SEM_FN_NAME (i960base,xor) },
  { I960BASE_INSN_XOR1, SEM_FN_NAME (i960base,xor1) },
  { I960BASE_INSN_XOR2, SEM_FN_NAME (i960base,xor2) },
  { I960BASE_INSN_XOR3, SEM_FN_NAME (i960base,xor3) },
  { I960BASE_INSN_OR, SEM_FN_NAME (i960base,or) },
  { I960BASE_INSN_OR1, SEM_FN_NAME (i960base,or1) },
  { I960BASE_INSN_OR2, SEM_FN_NAME (i960base,or2) },
  { I960BASE_INSN_OR3, SEM_FN_NAME (i960base,or3) },
  { I960BASE_INSN_NOR, SEM_FN_NAME (i960base,nor) },
  { I960BASE_INSN_NOR1, SEM_FN_NAME (i960base,nor1) },
  { I960BASE_INSN_NOR2, SEM_FN_NAME (i960base,nor2) },
  { I960BASE_INSN_NOR3, SEM_FN_NAME (i960base,nor3) },
  { I960BASE_INSN_XNOR, SEM_FN_NAME (i960base,xnor) },
  { I960BASE_INSN_XNOR1, SEM_FN_NAME (i960base,xnor1) },
  { I960BASE_INSN_XNOR2, SEM_FN_NAME (i960base,xnor2) },
  { I960BASE_INSN_XNOR3, SEM_FN_NAME (i960base,xnor3) },
  { I960BASE_INSN_NOT, SEM_FN_NAME (i960base,not) },
  { I960BASE_INSN_NOT1, SEM_FN_NAME (i960base,not1) },
  { I960BASE_INSN_NOT2, SEM_FN_NAME (i960base,not2) },
  { I960BASE_INSN_NOT3, SEM_FN_NAME (i960base,not3) },
  { I960BASE_INSN_ORNOT, SEM_FN_NAME (i960base,ornot) },
  { I960BASE_INSN_ORNOT1, SEM_FN_NAME (i960base,ornot1) },
  { I960BASE_INSN_ORNOT2, SEM_FN_NAME (i960base,ornot2) },
  { I960BASE_INSN_ORNOT3, SEM_FN_NAME (i960base,ornot3) },
  { I960BASE_INSN_CLRBIT, SEM_FN_NAME (i960base,clrbit) },
  { I960BASE_INSN_CLRBIT1, SEM_FN_NAME (i960base,clrbit1) },
  { I960BASE_INSN_CLRBIT2, SEM_FN_NAME (i960base,clrbit2) },
  { I960BASE_INSN_CLRBIT3, SEM_FN_NAME (i960base,clrbit3) },
  { I960BASE_INSN_SHLO, SEM_FN_NAME (i960base,shlo) },
  { I960BASE_INSN_SHLO1, SEM_FN_NAME (i960base,shlo1) },
  { I960BASE_INSN_SHLO2, SEM_FN_NAME (i960base,shlo2) },
  { I960BASE_INSN_SHLO3, SEM_FN_NAME (i960base,shlo3) },
  { I960BASE_INSN_SHRO, SEM_FN_NAME (i960base,shro) },
  { I960BASE_INSN_SHRO1, SEM_FN_NAME (i960base,shro1) },
  { I960BASE_INSN_SHRO2, SEM_FN_NAME (i960base,shro2) },
  { I960BASE_INSN_SHRO3, SEM_FN_NAME (i960base,shro3) },
  { I960BASE_INSN_SHLI, SEM_FN_NAME (i960base,shli) },
  { I960BASE_INSN_SHLI1, SEM_FN_NAME (i960base,shli1) },
  { I960BASE_INSN_SHLI2, SEM_FN_NAME (i960base,shli2) },
  { I960BASE_INSN_SHLI3, SEM_FN_NAME (i960base,shli3) },
  { I960BASE_INSN_SHRI, SEM_FN_NAME (i960base,shri) },
  { I960BASE_INSN_SHRI1, SEM_FN_NAME (i960base,shri1) },
  { I960BASE_INSN_SHRI2, SEM_FN_NAME (i960base,shri2) },
  { I960BASE_INSN_SHRI3, SEM_FN_NAME (i960base,shri3) },
  { I960BASE_INSN_EMUL, SEM_FN_NAME (i960base,emul) },
  { I960BASE_INSN_EMUL1, SEM_FN_NAME (i960base,emul1) },
  { I960BASE_INSN_EMUL2, SEM_FN_NAME (i960base,emul2) },
  { I960BASE_INSN_EMUL3, SEM_FN_NAME (i960base,emul3) },
  { I960BASE_INSN_MOV, SEM_FN_NAME (i960base,mov) },
  { I960BASE_INSN_MOV1, SEM_FN_NAME (i960base,mov1) },
  { I960BASE_INSN_MOVL, SEM_FN_NAME (i960base,movl) },
  { I960BASE_INSN_MOVL1, SEM_FN_NAME (i960base,movl1) },
  { I960BASE_INSN_MOVT, SEM_FN_NAME (i960base,movt) },
  { I960BASE_INSN_MOVT1, SEM_FN_NAME (i960base,movt1) },
  { I960BASE_INSN_MOVQ, SEM_FN_NAME (i960base,movq) },
  { I960BASE_INSN_MOVQ1, SEM_FN_NAME (i960base,movq1) },
  { I960BASE_INSN_MODPC, SEM_FN_NAME (i960base,modpc) },
  { I960BASE_INSN_MODAC, SEM_FN_NAME (i960base,modac) },
  { I960BASE_INSN_LDA_OFFSET, SEM_FN_NAME (i960base,lda_offset) },
  { I960BASE_INSN_LDA_INDIRECT_OFFSET, SEM_FN_NAME (i960base,lda_indirect_offset) },
  { I960BASE_INSN_LDA_INDIRECT, SEM_FN_NAME (i960base,lda_indirect) },
  { I960BASE_INSN_LDA_INDIRECT_INDEX, SEM_FN_NAME (i960base,lda_indirect_index) },
  { I960BASE_INSN_LDA_DISP, SEM_FN_NAME (i960base,lda_disp) },
  { I960BASE_INSN_LDA_INDIRECT_DISP, SEM_FN_NAME (i960base,lda_indirect_disp) },
  { I960BASE_INSN_LDA_INDEX_DISP, SEM_FN_NAME (i960base,lda_index_disp) },
  { I960BASE_INSN_LDA_INDIRECT_INDEX_DISP, SEM_FN_NAME (i960base,lda_indirect_index_disp) },
  { I960BASE_INSN_LD_OFFSET, SEM_FN_NAME (i960base,ld_offset) },
  { I960BASE_INSN_LD_INDIRECT_OFFSET, SEM_FN_NAME (i960base,ld_indirect_offset) },
  { I960BASE_INSN_LD_INDIRECT, SEM_FN_NAME (i960base,ld_indirect) },
  { I960BASE_INSN_LD_INDIRECT_INDEX, SEM_FN_NAME (i960base,ld_indirect_index) },
  { I960BASE_INSN_LD_DISP, SEM_FN_NAME (i960base,ld_disp) },
  { I960BASE_INSN_LD_INDIRECT_DISP, SEM_FN_NAME (i960base,ld_indirect_disp) },
  { I960BASE_INSN_LD_INDEX_DISP, SEM_FN_NAME (i960base,ld_index_disp) },
  { I960BASE_INSN_LD_INDIRECT_INDEX_DISP, SEM_FN_NAME (i960base,ld_indirect_index_disp) },
  { I960BASE_INSN_LDOB_OFFSET, SEM_FN_NAME (i960base,ldob_offset) },
  { I960BASE_INSN_LDOB_INDIRECT_OFFSET, SEM_FN_NAME (i960base,ldob_indirect_offset) },
  { I960BASE_INSN_LDOB_INDIRECT, SEM_FN_NAME (i960base,ldob_indirect) },
  { I960BASE_INSN_LDOB_INDIRECT_INDEX, SEM_FN_NAME (i960base,ldob_indirect_index) },
  { I960BASE_INSN_LDOB_DISP, SEM_FN_NAME (i960base,ldob_disp) },
  { I960BASE_INSN_LDOB_INDIRECT_DISP, SEM_FN_NAME (i960base,ldob_indirect_disp) },
  { I960BASE_INSN_LDOB_INDEX_DISP, SEM_FN_NAME (i960base,ldob_index_disp) },
  { I960BASE_INSN_LDOB_INDIRECT_INDEX_DISP, SEM_FN_NAME (i960base,ldob_indirect_index_disp) },
  { I960BASE_INSN_LDOS_OFFSET, SEM_FN_NAME (i960base,ldos_offset) },
  { I960BASE_INSN_LDOS_INDIRECT_OFFSET, SEM_FN_NAME (i960base,ldos_indirect_offset) },
  { I960BASE_INSN_LDOS_INDIRECT, SEM_FN_NAME (i960base,ldos_indirect) },
  { I960BASE_INSN_LDOS_INDIRECT_INDEX, SEM_FN_NAME (i960base,ldos_indirect_index) },
  { I960BASE_INSN_LDOS_DISP, SEM_FN_NAME (i960base,ldos_disp) },
  { I960BASE_INSN_LDOS_INDIRECT_DISP, SEM_FN_NAME (i960base,ldos_indirect_disp) },
  { I960BASE_INSN_LDOS_INDEX_DISP, SEM_FN_NAME (i960base,ldos_index_disp) },
  { I960BASE_INSN_LDOS_INDIRECT_INDEX_DISP, SEM_FN_NAME (i960base,ldos_indirect_index_disp) },
  { I960BASE_INSN_LDIB_OFFSET, SEM_FN_NAME (i960base,ldib_offset) },
  { I960BASE_INSN_LDIB_INDIRECT_OFFSET, SEM_FN_NAME (i960base,ldib_indirect_offset) },
  { I960BASE_INSN_LDIB_INDIRECT, SEM_FN_NAME (i960base,ldib_indirect) },
  { I960BASE_INSN_LDIB_INDIRECT_INDEX, SEM_FN_NAME (i960base,ldib_indirect_index) },
  { I960BASE_INSN_LDIB_DISP, SEM_FN_NAME (i960base,ldib_disp) },
  { I960BASE_INSN_LDIB_INDIRECT_DISP, SEM_FN_NAME (i960base,ldib_indirect_disp) },
  { I960BASE_INSN_LDIB_INDEX_DISP, SEM_FN_NAME (i960base,ldib_index_disp) },
  { I960BASE_INSN_LDIB_INDIRECT_INDEX_DISP, SEM_FN_NAME (i960base,ldib_indirect_index_disp) },
  { I960BASE_INSN_LDIS_OFFSET, SEM_FN_NAME (i960base,ldis_offset) },
  { I960BASE_INSN_LDIS_INDIRECT_OFFSET, SEM_FN_NAME (i960base,ldis_indirect_offset) },
  { I960BASE_INSN_LDIS_INDIRECT, SEM_FN_NAME (i960base,ldis_indirect) },
  { I960BASE_INSN_LDIS_INDIRECT_INDEX, SEM_FN_NAME (i960base,ldis_indirect_index) },
  { I960BASE_INSN_LDIS_DISP, SEM_FN_NAME (i960base,ldis_disp) },
  { I960BASE_INSN_LDIS_INDIRECT_DISP, SEM_FN_NAME (i960base,ldis_indirect_disp) },
  { I960BASE_INSN_LDIS_INDEX_DISP, SEM_FN_NAME (i960base,ldis_index_disp) },
  { I960BASE_INSN_LDIS_INDIRECT_INDEX_DISP, SEM_FN_NAME (i960base,ldis_indirect_index_disp) },
  { I960BASE_INSN_LDL_OFFSET, SEM_FN_NAME (i960base,ldl_offset) },
  { I960BASE_INSN_LDL_INDIRECT_OFFSET, SEM_FN_NAME (i960base,ldl_indirect_offset) },
  { I960BASE_INSN_LDL_INDIRECT, SEM_FN_NAME (i960base,ldl_indirect) },
  { I960BASE_INSN_LDL_INDIRECT_INDEX, SEM_FN_NAME (i960base,ldl_indirect_index) },
  { I960BASE_INSN_LDL_DISP, SEM_FN_NAME (i960base,ldl_disp) },
  { I960BASE_INSN_LDL_INDIRECT_DISP, SEM_FN_NAME (i960base,ldl_indirect_disp) },
  { I960BASE_INSN_LDL_INDEX_DISP, SEM_FN_NAME (i960base,ldl_index_disp) },
  { I960BASE_INSN_LDL_INDIRECT_INDEX_DISP, SEM_FN_NAME (i960base,ldl_indirect_index_disp) },
  { I960BASE_INSN_LDT_OFFSET, SEM_FN_NAME (i960base,ldt_offset) },
  { I960BASE_INSN_LDT_INDIRECT_OFFSET, SEM_FN_NAME (i960base,ldt_indirect_offset) },
  { I960BASE_INSN_LDT_INDIRECT, SEM_FN_NAME (i960base,ldt_indirect) },
  { I960BASE_INSN_LDT_INDIRECT_INDEX, SEM_FN_NAME (i960base,ldt_indirect_index) },
  { I960BASE_INSN_LDT_DISP, SEM_FN_NAME (i960base,ldt_disp) },
  { I960BASE_INSN_LDT_INDIRECT_DISP, SEM_FN_NAME (i960base,ldt_indirect_disp) },
  { I960BASE_INSN_LDT_INDEX_DISP, SEM_FN_NAME (i960base,ldt_index_disp) },
  { I960BASE_INSN_LDT_INDIRECT_INDEX_DISP, SEM_FN_NAME (i960base,ldt_indirect_index_disp) },
  { I960BASE_INSN_LDQ_OFFSET, SEM_FN_NAME (i960base,ldq_offset) },
  { I960BASE_INSN_LDQ_INDIRECT_OFFSET, SEM_FN_NAME (i960base,ldq_indirect_offset) },
  { I960BASE_INSN_LDQ_INDIRECT, SEM_FN_NAME (i960base,ldq_indirect) },
  { I960BASE_INSN_LDQ_INDIRECT_INDEX, SEM_FN_NAME (i960base,ldq_indirect_index) },
  { I960BASE_INSN_LDQ_DISP, SEM_FN_NAME (i960base,ldq_disp) },
  { I960BASE_INSN_LDQ_INDIRECT_DISP, SEM_FN_NAME (i960base,ldq_indirect_disp) },
  { I960BASE_INSN_LDQ_INDEX_DISP, SEM_FN_NAME (i960base,ldq_index_disp) },
  { I960BASE_INSN_LDQ_INDIRECT_INDEX_DISP, SEM_FN_NAME (i960base,ldq_indirect_index_disp) },
  { I960BASE_INSN_ST_OFFSET, SEM_FN_NAME (i960base,st_offset) },
  { I960BASE_INSN_ST_INDIRECT_OFFSET, SEM_FN_NAME (i960base,st_indirect_offset) },
  { I960BASE_INSN_ST_INDIRECT, SEM_FN_NAME (i960base,st_indirect) },
  { I960BASE_INSN_ST_INDIRECT_INDEX, SEM_FN_NAME (i960base,st_indirect_index) },
  { I960BASE_INSN_ST_DISP, SEM_FN_NAME (i960base,st_disp) },
  { I960BASE_INSN_ST_INDIRECT_DISP, SEM_FN_NAME (i960base,st_indirect_disp) },
  { I960BASE_INSN_ST_INDEX_DISP, SEM_FN_NAME (i960base,st_index_disp) },
  { I960BASE_INSN_ST_INDIRECT_INDEX_DISP, SEM_FN_NAME (i960base,st_indirect_index_disp) },
  { I960BASE_INSN_STOB_OFFSET, SEM_FN_NAME (i960base,stob_offset) },
  { I960BASE_INSN_STOB_INDIRECT_OFFSET, SEM_FN_NAME (i960base,stob_indirect_offset) },
  { I960BASE_INSN_STOB_INDIRECT, SEM_FN_NAME (i960base,stob_indirect) },
  { I960BASE_INSN_STOB_INDIRECT_INDEX, SEM_FN_NAME (i960base,stob_indirect_index) },
  { I960BASE_INSN_STOB_DISP, SEM_FN_NAME (i960base,stob_disp) },
  { I960BASE_INSN_STOB_INDIRECT_DISP, SEM_FN_NAME (i960base,stob_indirect_disp) },
  { I960BASE_INSN_STOB_INDEX_DISP, SEM_FN_NAME (i960base,stob_index_disp) },
  { I960BASE_INSN_STOB_INDIRECT_INDEX_DISP, SEM_FN_NAME (i960base,stob_indirect_index_disp) },
  { I960BASE_INSN_STOS_OFFSET, SEM_FN_NAME (i960base,stos_offset) },
  { I960BASE_INSN_STOS_INDIRECT_OFFSET, SEM_FN_NAME (i960base,stos_indirect_offset) },
  { I960BASE_INSN_STOS_INDIRECT, SEM_FN_NAME (i960base,stos_indirect) },
  { I960BASE_INSN_STOS_INDIRECT_INDEX, SEM_FN_NAME (i960base,stos_indirect_index) },
  { I960BASE_INSN_STOS_DISP, SEM_FN_NAME (i960base,stos_disp) },
  { I960BASE_INSN_STOS_INDIRECT_DISP, SEM_FN_NAME (i960base,stos_indirect_disp) },
  { I960BASE_INSN_STOS_INDEX_DISP, SEM_FN_NAME (i960base,stos_index_disp) },
  { I960BASE_INSN_STOS_INDIRECT_INDEX_DISP, SEM_FN_NAME (i960base,stos_indirect_index_disp) },
  { I960BASE_INSN_STL_OFFSET, SEM_FN_NAME (i960base,stl_offset) },
  { I960BASE_INSN_STL_INDIRECT_OFFSET, SEM_FN_NAME (i960base,stl_indirect_offset) },
  { I960BASE_INSN_STL_INDIRECT, SEM_FN_NAME (i960base,stl_indirect) },
  { I960BASE_INSN_STL_INDIRECT_INDEX, SEM_FN_NAME (i960base,stl_indirect_index) },
  { I960BASE_INSN_STL_DISP, SEM_FN_NAME (i960base,stl_disp) },
  { I960BASE_INSN_STL_INDIRECT_DISP, SEM_FN_NAME (i960base,stl_indirect_disp) },
  { I960BASE_INSN_STL_INDEX_DISP, SEM_FN_NAME (i960base,stl_index_disp) },
  { I960BASE_INSN_STL_INDIRECT_INDEX_DISP, SEM_FN_NAME (i960base,stl_indirect_index_disp) },
  { I960BASE_INSN_STT_OFFSET, SEM_FN_NAME (i960base,stt_offset) },
  { I960BASE_INSN_STT_INDIRECT_OFFSET, SEM_FN_NAME (i960base,stt_indirect_offset) },
  { I960BASE_INSN_STT_INDIRECT, SEM_FN_NAME (i960base,stt_indirect) },
  { I960BASE_INSN_STT_INDIRECT_INDEX, SEM_FN_NAME (i960base,stt_indirect_index) },
  { I960BASE_INSN_STT_DISP, SEM_FN_NAME (i960base,stt_disp) },
  { I960BASE_INSN_STT_INDIRECT_DISP, SEM_FN_NAME (i960base,stt_indirect_disp) },
  { I960BASE_INSN_STT_INDEX_DISP, SEM_FN_NAME (i960base,stt_index_disp) },
  { I960BASE_INSN_STT_INDIRECT_INDEX_DISP, SEM_FN_NAME (i960base,stt_indirect_index_disp) },
  { I960BASE_INSN_STQ_OFFSET, SEM_FN_NAME (i960base,stq_offset) },
  { I960BASE_INSN_STQ_INDIRECT_OFFSET, SEM_FN_NAME (i960base,stq_indirect_offset) },
  { I960BASE_INSN_STQ_INDIRECT, SEM_FN_NAME (i960base,stq_indirect) },
  { I960BASE_INSN_STQ_INDIRECT_INDEX, SEM_FN_NAME (i960base,stq_indirect_index) },
  { I960BASE_INSN_STQ_DISP, SEM_FN_NAME (i960base,stq_disp) },
  { I960BASE_INSN_STQ_INDIRECT_DISP, SEM_FN_NAME (i960base,stq_indirect_disp) },
  { I960BASE_INSN_STQ_INDEX_DISP, SEM_FN_NAME (i960base,stq_index_disp) },
  { I960BASE_INSN_STQ_INDIRECT_INDEX_DISP, SEM_FN_NAME (i960base,stq_indirect_index_disp) },
  { I960BASE_INSN_CMPOBE_REG, SEM_FN_NAME (i960base,cmpobe_reg) },
  { I960BASE_INSN_CMPOBE_LIT, SEM_FN_NAME (i960base,cmpobe_lit) },
  { I960BASE_INSN_CMPOBNE_REG, SEM_FN_NAME (i960base,cmpobne_reg) },
  { I960BASE_INSN_CMPOBNE_LIT, SEM_FN_NAME (i960base,cmpobne_lit) },
  { I960BASE_INSN_CMPOBL_REG, SEM_FN_NAME (i960base,cmpobl_reg) },
  { I960BASE_INSN_CMPOBL_LIT, SEM_FN_NAME (i960base,cmpobl_lit) },
  { I960BASE_INSN_CMPOBLE_REG, SEM_FN_NAME (i960base,cmpoble_reg) },
  { I960BASE_INSN_CMPOBLE_LIT, SEM_FN_NAME (i960base,cmpoble_lit) },
  { I960BASE_INSN_CMPOBG_REG, SEM_FN_NAME (i960base,cmpobg_reg) },
  { I960BASE_INSN_CMPOBG_LIT, SEM_FN_NAME (i960base,cmpobg_lit) },
  { I960BASE_INSN_CMPOBGE_REG, SEM_FN_NAME (i960base,cmpobge_reg) },
  { I960BASE_INSN_CMPOBGE_LIT, SEM_FN_NAME (i960base,cmpobge_lit) },
  { I960BASE_INSN_CMPIBE_REG, SEM_FN_NAME (i960base,cmpibe_reg) },
  { I960BASE_INSN_CMPIBE_LIT, SEM_FN_NAME (i960base,cmpibe_lit) },
  { I960BASE_INSN_CMPIBNE_REG, SEM_FN_NAME (i960base,cmpibne_reg) },
  { I960BASE_INSN_CMPIBNE_LIT, SEM_FN_NAME (i960base,cmpibne_lit) },
  { I960BASE_INSN_CMPIBL_REG, SEM_FN_NAME (i960base,cmpibl_reg) },
  { I960BASE_INSN_CMPIBL_LIT, SEM_FN_NAME (i960base,cmpibl_lit) },
  { I960BASE_INSN_CMPIBLE_REG, SEM_FN_NAME (i960base,cmpible_reg) },
  { I960BASE_INSN_CMPIBLE_LIT, SEM_FN_NAME (i960base,cmpible_lit) },
  { I960BASE_INSN_CMPIBG_REG, SEM_FN_NAME (i960base,cmpibg_reg) },
  { I960BASE_INSN_CMPIBG_LIT, SEM_FN_NAME (i960base,cmpibg_lit) },
  { I960BASE_INSN_CMPIBGE_REG, SEM_FN_NAME (i960base,cmpibge_reg) },
  { I960BASE_INSN_CMPIBGE_LIT, SEM_FN_NAME (i960base,cmpibge_lit) },
  { I960BASE_INSN_BBC_REG, SEM_FN_NAME (i960base,bbc_reg) },
  { I960BASE_INSN_BBC_LIT, SEM_FN_NAME (i960base,bbc_lit) },
  { I960BASE_INSN_BBS_REG, SEM_FN_NAME (i960base,bbs_reg) },
  { I960BASE_INSN_BBS_LIT, SEM_FN_NAME (i960base,bbs_lit) },
  { I960BASE_INSN_CMPI, SEM_FN_NAME (i960base,cmpi) },
  { I960BASE_INSN_CMPI1, SEM_FN_NAME (i960base,cmpi1) },
  { I960BASE_INSN_CMPI2, SEM_FN_NAME (i960base,cmpi2) },
  { I960BASE_INSN_CMPI3, SEM_FN_NAME (i960base,cmpi3) },
  { I960BASE_INSN_CMPO, SEM_FN_NAME (i960base,cmpo) },
  { I960BASE_INSN_CMPO1, SEM_FN_NAME (i960base,cmpo1) },
  { I960BASE_INSN_CMPO2, SEM_FN_NAME (i960base,cmpo2) },
  { I960BASE_INSN_CMPO3, SEM_FN_NAME (i960base,cmpo3) },
  { I960BASE_INSN_TESTNO_REG, SEM_FN_NAME (i960base,testno_reg) },
  { I960BASE_INSN_TESTG_REG, SEM_FN_NAME (i960base,testg_reg) },
  { I960BASE_INSN_TESTE_REG, SEM_FN_NAME (i960base,teste_reg) },
  { I960BASE_INSN_TESTGE_REG, SEM_FN_NAME (i960base,testge_reg) },
  { I960BASE_INSN_TESTL_REG, SEM_FN_NAME (i960base,testl_reg) },
  { I960BASE_INSN_TESTNE_REG, SEM_FN_NAME (i960base,testne_reg) },
  { I960BASE_INSN_TESTLE_REG, SEM_FN_NAME (i960base,testle_reg) },
  { I960BASE_INSN_TESTO_REG, SEM_FN_NAME (i960base,testo_reg) },
  { I960BASE_INSN_BNO, SEM_FN_NAME (i960base,bno) },
  { I960BASE_INSN_BG, SEM_FN_NAME (i960base,bg) },
  { I960BASE_INSN_BE, SEM_FN_NAME (i960base,be) },
  { I960BASE_INSN_BGE, SEM_FN_NAME (i960base,bge) },
  { I960BASE_INSN_BL, SEM_FN_NAME (i960base,bl) },
  { I960BASE_INSN_BNE, SEM_FN_NAME (i960base,bne) },
  { I960BASE_INSN_BLE, SEM_FN_NAME (i960base,ble) },
  { I960BASE_INSN_BO, SEM_FN_NAME (i960base,bo) },
  { I960BASE_INSN_B, SEM_FN_NAME (i960base,b) },
  { I960BASE_INSN_BX_INDIRECT_OFFSET, SEM_FN_NAME (i960base,bx_indirect_offset) },
  { I960BASE_INSN_BX_INDIRECT, SEM_FN_NAME (i960base,bx_indirect) },
  { I960BASE_INSN_BX_INDIRECT_INDEX, SEM_FN_NAME (i960base,bx_indirect_index) },
  { I960BASE_INSN_BX_DISP, SEM_FN_NAME (i960base,bx_disp) },
  { I960BASE_INSN_BX_INDIRECT_DISP, SEM_FN_NAME (i960base,bx_indirect_disp) },
  { I960BASE_INSN_CALLX_DISP, SEM_FN_NAME (i960base,callx_disp) },
  { I960BASE_INSN_CALLX_INDIRECT, SEM_FN_NAME (i960base,callx_indirect) },
  { I960BASE_INSN_CALLX_INDIRECT_OFFSET, SEM_FN_NAME (i960base,callx_indirect_offset) },
  { I960BASE_INSN_RET, SEM_FN_NAME (i960base,ret) },
  { I960BASE_INSN_CALLS, SEM_FN_NAME (i960base,calls) },
  { I960BASE_INSN_FMARK, SEM_FN_NAME (i960base,fmark) },
  { I960BASE_INSN_FLUSHREG, SEM_FN_NAME (i960base,flushreg) },
  { 0, 0 }
};

/* Add the semantic fns to IDESC_TABLE.  */

void
SEM_FN_NAME (i960base,init_idesc_table) (SIM_CPU *current_cpu)
{
  IDESC *idesc_table = CPU_IDESC (current_cpu);
  const struct sem_fn_desc *sf;
  int mach_num = MACH_NUM (CPU_MACH (current_cpu));

  for (sf = &sem_fns[0]; sf->fn != 0; ++sf)
    {
      const CGEN_INSN *insn = idesc_table[sf->index].idata;
      int valid_p = (CGEN_INSN_VIRTUAL_P (insn)
		     || CGEN_INSN_MACH_HAS_P (insn, mach_num));
#if FAST_P
      if (valid_p)
	idesc_table[sf->index].sem_fast = sf->fn;
      else
	idesc_table[sf->index].sem_fast = SEM_FN_NAME (i960base,x_invalid);
#else
      if (valid_p)
	idesc_table[sf->index].sem_full = sf->fn;
      else
	idesc_table[sf->index].sem_full = SEM_FN_NAME (i960base,x_invalid);
#endif
    }
}

