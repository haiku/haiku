// OBSOLETE /* Simulator instruction semantics for fr30bf.
// OBSOLETE 
// OBSOLETE THIS FILE IS MACHINE GENERATED WITH CGEN.
// OBSOLETE 
// OBSOLETE Copyright 1996, 1997, 1998, 1999, 2000, 2001 Free Software Foundation, Inc.
// OBSOLETE 
// OBSOLETE This file is part of the GNU simulators.
// OBSOLETE 
// OBSOLETE This program is free software; you can redistribute it and/or modify
// OBSOLETE it under the terms of the GNU General Public License as published by
// OBSOLETE the Free Software Foundation; either version 2, or (at your option)
// OBSOLETE any later version.
// OBSOLETE 
// OBSOLETE This program is distributed in the hope that it will be useful,
// OBSOLETE but WITHOUT ANY WARRANTY; without even the implied warranty of
// OBSOLETE MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// OBSOLETE GNU General Public License for more details.
// OBSOLETE 
// OBSOLETE You should have received a copy of the GNU General Public License along
// OBSOLETE with this program; if not, write to the Free Software Foundation, Inc.,
// OBSOLETE 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
// OBSOLETE 
// OBSOLETE */
// OBSOLETE 
// OBSOLETE #define WANT_CPU fr30bf
// OBSOLETE #define WANT_CPU_FR30BF
// OBSOLETE 
// OBSOLETE #include "sim-main.h"
// OBSOLETE #include "cgen-mem.h"
// OBSOLETE #include "cgen-ops.h"
// OBSOLETE 
// OBSOLETE #undef GET_ATTR
// OBSOLETE #if defined (__STDC__) || defined (ALMOST_STDC) || defined (HAVE_STRINGIZE)
// OBSOLETE #define GET_ATTR(cpu, num, attr) CGEN_ATTR_VALUE (NULL, abuf->idesc->attrs, CGEN_INSN_##attr)
// OBSOLETE #else
// OBSOLETE #define GET_ATTR(cpu, num, attr) CGEN_ATTR_VALUE (NULL, abuf->idesc->attrs, CGEN_INSN_/**/attr)
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE /* This is used so that we can compile two copies of the semantic code,
// OBSOLETE    one with full feature support and one without that runs fast(er).
// OBSOLETE    FAST_P, when desired, is defined on the command line, -DFAST_P=1.  */
// OBSOLETE #if FAST_P
// OBSOLETE #define SEM_FN_NAME(cpu,fn) XCONCAT3 (cpu,_semf_,fn)
// OBSOLETE #undef TRACE_RESULT
// OBSOLETE #define TRACE_RESULT(cpu, abuf, name, type, val)
// OBSOLETE #else
// OBSOLETE #define SEM_FN_NAME(cpu,fn) XCONCAT3 (cpu,_sem_,fn)
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE /* x-invalid: --invalid-- */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,x_invalid) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 0);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     /* Update the recorded pc in the cpu state struct.
// OBSOLETE        Only necessary for WITH_SCACHE case, but to avoid the
// OBSOLETE        conditional compilation ....  */
// OBSOLETE     SET_H_PC (pc);
// OBSOLETE     /* Virtual insns have zero size.  Overwrite vpc with address of next insn
// OBSOLETE        using the default-insn-bitsize spec.  When executing insns in parallel
// OBSOLETE        we may want to queue the fault and continue execution.  */
// OBSOLETE     vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE     vpc = sim_engine_invalid_insn (current_cpu, pc, vpc);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* x-after: --after-- */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,x_after) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 0);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE #if WITH_SCACHE_PBB_FR30BF
// OBSOLETE     fr30bf_pbb_after (current_cpu, sem_arg);
// OBSOLETE #endif
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* x-before: --before-- */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,x_before) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 0);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE #if WITH_SCACHE_PBB_FR30BF
// OBSOLETE     fr30bf_pbb_before (current_cpu, sem_arg);
// OBSOLETE #endif
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* x-cti-chain: --cti-chain-- */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,x_cti_chain) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 0);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE #if WITH_SCACHE_PBB_FR30BF
// OBSOLETE #ifdef DEFINE_SWITCH
// OBSOLETE     vpc = fr30bf_pbb_cti_chain (current_cpu, sem_arg,
// OBSOLETE 			       pbb_br_type, pbb_br_npc);
// OBSOLETE     BREAK (sem);
// OBSOLETE #else
// OBSOLETE     /* FIXME: Allow provision of explicit ifmt spec in insn spec.  */
// OBSOLETE     vpc = fr30bf_pbb_cti_chain (current_cpu, sem_arg,
// OBSOLETE 			       CPU_PBB_BR_TYPE (current_cpu),
// OBSOLETE 			       CPU_PBB_BR_NPC (current_cpu));
// OBSOLETE #endif
// OBSOLETE #endif
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* x-chain: --chain-- */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,x_chain) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 0);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE #if WITH_SCACHE_PBB_FR30BF
// OBSOLETE     vpc = fr30bf_pbb_chain (current_cpu, sem_arg);
// OBSOLETE #ifdef DEFINE_SWITCH
// OBSOLETE     BREAK (sem);
// OBSOLETE #endif
// OBSOLETE #endif
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* x-begin: --begin-- */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,x_begin) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 0);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE #if WITH_SCACHE_PBB_FR30BF
// OBSOLETE #if defined DEFINE_SWITCH || defined FAST_P
// OBSOLETE     /* In the switch case FAST_P is a constant, allowing several optimizations
// OBSOLETE        in any called inline functions.  */
// OBSOLETE     vpc = fr30bf_pbb_begin (current_cpu, FAST_P);
// OBSOLETE #else
// OBSOLETE #if 0 /* cgen engine can't handle dynamic fast/full switching yet.  */
// OBSOLETE     vpc = fr30bf_pbb_begin (current_cpu, STATE_RUN_FAST_P (CPU_STATE (current_cpu)));
// OBSOLETE #else
// OBSOLETE     vpc = fr30bf_pbb_begin (current_cpu, 0);
// OBSOLETE #endif
// OBSOLETE #endif
// OBSOLETE #endif
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* add: add $Rj,$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,add) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     BI opval = ADDOFSI (* FLD (i_Ri), * FLD (i_Rj), 0);
// OBSOLETE     CPU (h_vbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "vbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = ADDCFSI (* FLD (i_Ri), * FLD (i_Rj), 0);
// OBSOLETE     CPU (h_cbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (* FLD (i_Ri), * FLD (i_Rj));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     BI opval = EQSI (* FLD (i_Ri), 0);
// OBSOLETE     CPU (h_zbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = LTSI (* FLD (i_Ri), 0);
// OBSOLETE     CPU (h_nbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* addi: add $u4,$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,addi) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     BI opval = ADDOFSI (* FLD (i_Ri), FLD (f_u4), 0);
// OBSOLETE     CPU (h_vbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "vbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = ADDCFSI (* FLD (i_Ri), FLD (f_u4), 0);
// OBSOLETE     CPU (h_cbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (* FLD (i_Ri), FLD (f_u4));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     BI opval = EQSI (* FLD (i_Ri), 0);
// OBSOLETE     CPU (h_zbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = LTSI (* FLD (i_Ri), 0);
// OBSOLETE     CPU (h_nbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* add2: add2 $m4,$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,add2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add2.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     BI opval = ADDOFSI (* FLD (i_Ri), FLD (f_m4), 0);
// OBSOLETE     CPU (h_vbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "vbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = ADDCFSI (* FLD (i_Ri), FLD (f_m4), 0);
// OBSOLETE     CPU (h_cbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (* FLD (i_Ri), FLD (f_m4));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     BI opval = EQSI (* FLD (i_Ri), 0);
// OBSOLETE     CPU (h_zbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = LTSI (* FLD (i_Ri), 0);
// OBSOLETE     CPU (h_nbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* addc: addc $Rj,$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,addc) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   SI tmp_tmp;
// OBSOLETE   tmp_tmp = ADDCSI (* FLD (i_Ri), * FLD (i_Rj), CPU (h_cbit));
// OBSOLETE   {
// OBSOLETE     BI opval = ADDOFSI (* FLD (i_Ri), * FLD (i_Rj), CPU (h_cbit));
// OBSOLETE     CPU (h_vbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "vbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = ADDCFSI (* FLD (i_Ri), * FLD (i_Rj), CPU (h_cbit));
// OBSOLETE     CPU (h_cbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = tmp_tmp;
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     BI opval = EQSI (* FLD (i_Ri), 0);
// OBSOLETE     CPU (h_zbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = LTSI (* FLD (i_Ri), 0);
// OBSOLETE     CPU (h_nbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* addn: addn $Rj,$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,addn) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (* FLD (i_Ri), * FLD (i_Rj));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* addni: addn $u4,$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,addni) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (* FLD (i_Ri), FLD (f_u4));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* addn2: addn2 $m4,$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,addn2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add2.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (* FLD (i_Ri), FLD (f_m4));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* sub: sub $Rj,$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,sub) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     BI opval = SUBOFSI (* FLD (i_Ri), * FLD (i_Rj), 0);
// OBSOLETE     CPU (h_vbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "vbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = SUBCFSI (* FLD (i_Ri), * FLD (i_Rj), 0);
// OBSOLETE     CPU (h_cbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = SUBSI (* FLD (i_Ri), * FLD (i_Rj));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     BI opval = EQSI (* FLD (i_Ri), 0);
// OBSOLETE     CPU (h_zbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = LTSI (* FLD (i_Ri), 0);
// OBSOLETE     CPU (h_nbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* subc: subc $Rj,$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,subc) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   SI tmp_tmp;
// OBSOLETE   tmp_tmp = SUBCSI (* FLD (i_Ri), * FLD (i_Rj), CPU (h_cbit));
// OBSOLETE   {
// OBSOLETE     BI opval = SUBOFSI (* FLD (i_Ri), * FLD (i_Rj), CPU (h_cbit));
// OBSOLETE     CPU (h_vbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "vbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = SUBCFSI (* FLD (i_Ri), * FLD (i_Rj), CPU (h_cbit));
// OBSOLETE     CPU (h_cbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = tmp_tmp;
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     BI opval = EQSI (* FLD (i_Ri), 0);
// OBSOLETE     CPU (h_zbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = LTSI (* FLD (i_Ri), 0);
// OBSOLETE     CPU (h_nbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* subn: subn $Rj,$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,subn) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = SUBSI (* FLD (i_Ri), * FLD (i_Rj));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* cmp: cmp $Rj,$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,cmp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   SI tmp_tmp1;
// OBSOLETE   {
// OBSOLETE     BI opval = SUBOFSI (* FLD (i_Ri), * FLD (i_Rj), 0);
// OBSOLETE     CPU (h_vbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "vbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = SUBCFSI (* FLD (i_Ri), * FLD (i_Rj), 0);
// OBSOLETE     CPU (h_cbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   tmp_tmp1 = SUBSI (* FLD (i_Ri), * FLD (i_Rj));
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     BI opval = EQSI (tmp_tmp1, 0);
// OBSOLETE     CPU (h_zbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = LTSI (tmp_tmp1, 0);
// OBSOLETE     CPU (h_nbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* cmpi: cmp $u4,$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,cmpi) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   SI tmp_tmp1;
// OBSOLETE   {
// OBSOLETE     BI opval = SUBOFSI (* FLD (i_Ri), FLD (f_u4), 0);
// OBSOLETE     CPU (h_vbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "vbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = SUBCFSI (* FLD (i_Ri), FLD (f_u4), 0);
// OBSOLETE     CPU (h_cbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   tmp_tmp1 = SUBSI (* FLD (i_Ri), FLD (f_u4));
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     BI opval = EQSI (tmp_tmp1, 0);
// OBSOLETE     CPU (h_zbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = LTSI (tmp_tmp1, 0);
// OBSOLETE     CPU (h_nbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* cmp2: cmp2 $m4,$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,cmp2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add2.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   SI tmp_tmp1;
// OBSOLETE   {
// OBSOLETE     BI opval = SUBOFSI (* FLD (i_Ri), FLD (f_m4), 0);
// OBSOLETE     CPU (h_vbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "vbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = SUBCFSI (* FLD (i_Ri), FLD (f_m4), 0);
// OBSOLETE     CPU (h_cbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   tmp_tmp1 = SUBSI (* FLD (i_Ri), FLD (f_m4));
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     BI opval = EQSI (tmp_tmp1, 0);
// OBSOLETE     CPU (h_zbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = LTSI (tmp_tmp1, 0);
// OBSOLETE     CPU (h_nbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* and: and $Rj,$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,and) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = ANDSI (* FLD (i_Ri), * FLD (i_Rj));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     BI opval = EQSI (* FLD (i_Ri), 0);
// OBSOLETE     CPU (h_zbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = LTSI (* FLD (i_Ri), 0);
// OBSOLETE     CPU (h_nbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* or: or $Rj,$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,or) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = ORSI (* FLD (i_Ri), * FLD (i_Rj));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     BI opval = EQSI (* FLD (i_Ri), 0);
// OBSOLETE     CPU (h_zbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = LTSI (* FLD (i_Ri), 0);
// OBSOLETE     CPU (h_nbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* eor: eor $Rj,$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,eor) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = XORSI (* FLD (i_Ri), * FLD (i_Rj));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     BI opval = EQSI (* FLD (i_Ri), 0);
// OBSOLETE     CPU (h_zbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = LTSI (* FLD (i_Ri), 0);
// OBSOLETE     CPU (h_nbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* andm: and $Rj,@$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,andm) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   SI tmp_tmp;
// OBSOLETE   tmp_tmp = ANDSI (GETMEMSI (current_cpu, pc, * FLD (i_Ri)), * FLD (i_Rj));
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     BI opval = EQSI (tmp_tmp, 0);
// OBSOLETE     CPU (h_zbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = LTSI (tmp_tmp, 0);
// OBSOLETE     CPU (h_nbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE   {
// OBSOLETE     SI opval = tmp_tmp;
// OBSOLETE     SETMEMSI (current_cpu, pc, * FLD (i_Ri), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* andh: andh $Rj,@$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,andh) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   HI tmp_tmp;
// OBSOLETE   tmp_tmp = ANDHI (GETMEMHI (current_cpu, pc, * FLD (i_Ri)), * FLD (i_Rj));
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     BI opval = EQHI (tmp_tmp, 0);
// OBSOLETE     CPU (h_zbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = LTHI (tmp_tmp, 0);
// OBSOLETE     CPU (h_nbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE   {
// OBSOLETE     HI opval = tmp_tmp;
// OBSOLETE     SETMEMHI (current_cpu, pc, * FLD (i_Ri), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* andb: andb $Rj,@$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,andb) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   QI tmp_tmp;
// OBSOLETE   tmp_tmp = ANDQI (GETMEMQI (current_cpu, pc, * FLD (i_Ri)), * FLD (i_Rj));
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     BI opval = EQQI (tmp_tmp, 0);
// OBSOLETE     CPU (h_zbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = LTQI (tmp_tmp, 0);
// OBSOLETE     CPU (h_nbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE   {
// OBSOLETE     QI opval = tmp_tmp;
// OBSOLETE     SETMEMQI (current_cpu, pc, * FLD (i_Ri), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* orm: or $Rj,@$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,orm) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   SI tmp_tmp;
// OBSOLETE   tmp_tmp = ORSI (GETMEMSI (current_cpu, pc, * FLD (i_Ri)), * FLD (i_Rj));
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     BI opval = EQSI (tmp_tmp, 0);
// OBSOLETE     CPU (h_zbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = LTSI (tmp_tmp, 0);
// OBSOLETE     CPU (h_nbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE   {
// OBSOLETE     SI opval = tmp_tmp;
// OBSOLETE     SETMEMSI (current_cpu, pc, * FLD (i_Ri), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* orh: orh $Rj,@$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,orh) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   HI tmp_tmp;
// OBSOLETE   tmp_tmp = ORHI (GETMEMHI (current_cpu, pc, * FLD (i_Ri)), * FLD (i_Rj));
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     BI opval = EQHI (tmp_tmp, 0);
// OBSOLETE     CPU (h_zbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = LTHI (tmp_tmp, 0);
// OBSOLETE     CPU (h_nbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE   {
// OBSOLETE     HI opval = tmp_tmp;
// OBSOLETE     SETMEMHI (current_cpu, pc, * FLD (i_Ri), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* orb: orb $Rj,@$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,orb) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   QI tmp_tmp;
// OBSOLETE   tmp_tmp = ORQI (GETMEMQI (current_cpu, pc, * FLD (i_Ri)), * FLD (i_Rj));
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     BI opval = EQQI (tmp_tmp, 0);
// OBSOLETE     CPU (h_zbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = LTQI (tmp_tmp, 0);
// OBSOLETE     CPU (h_nbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE   {
// OBSOLETE     QI opval = tmp_tmp;
// OBSOLETE     SETMEMQI (current_cpu, pc, * FLD (i_Ri), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* eorm: eor $Rj,@$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,eorm) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   SI tmp_tmp;
// OBSOLETE   tmp_tmp = XORSI (GETMEMSI (current_cpu, pc, * FLD (i_Ri)), * FLD (i_Rj));
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     BI opval = EQSI (tmp_tmp, 0);
// OBSOLETE     CPU (h_zbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = LTSI (tmp_tmp, 0);
// OBSOLETE     CPU (h_nbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE   {
// OBSOLETE     SI opval = tmp_tmp;
// OBSOLETE     SETMEMSI (current_cpu, pc, * FLD (i_Ri), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* eorh: eorh $Rj,@$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,eorh) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   HI tmp_tmp;
// OBSOLETE   tmp_tmp = XORHI (GETMEMHI (current_cpu, pc, * FLD (i_Ri)), * FLD (i_Rj));
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     BI opval = EQHI (tmp_tmp, 0);
// OBSOLETE     CPU (h_zbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = LTHI (tmp_tmp, 0);
// OBSOLETE     CPU (h_nbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE   {
// OBSOLETE     HI opval = tmp_tmp;
// OBSOLETE     SETMEMHI (current_cpu, pc, * FLD (i_Ri), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* eorb: eorb $Rj,@$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,eorb) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   QI tmp_tmp;
// OBSOLETE   tmp_tmp = XORQI (GETMEMQI (current_cpu, pc, * FLD (i_Ri)), * FLD (i_Rj));
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     BI opval = EQQI (tmp_tmp, 0);
// OBSOLETE     CPU (h_zbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = LTQI (tmp_tmp, 0);
// OBSOLETE     CPU (h_nbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE   {
// OBSOLETE     QI opval = tmp_tmp;
// OBSOLETE     SETMEMQI (current_cpu, pc, * FLD (i_Ri), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* bandl: bandl $u4,@$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,bandl) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     QI opval = ANDQI (ORQI (FLD (f_u4), 240), GETMEMQI (current_cpu, pc, * FLD (i_Ri)));
// OBSOLETE     SETMEMQI (current_cpu, pc, * FLD (i_Ri), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* borl: borl $u4,@$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,borl) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     QI opval = ORQI (FLD (f_u4), GETMEMQI (current_cpu, pc, * FLD (i_Ri)));
// OBSOLETE     SETMEMQI (current_cpu, pc, * FLD (i_Ri), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* beorl: beorl $u4,@$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,beorl) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     QI opval = XORQI (FLD (f_u4), GETMEMQI (current_cpu, pc, * FLD (i_Ri)));
// OBSOLETE     SETMEMQI (current_cpu, pc, * FLD (i_Ri), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* bandh: bandh $u4,@$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,bandh) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     QI opval = ANDQI (ORQI (SLLQI (FLD (f_u4), 4), 15), GETMEMQI (current_cpu, pc, * FLD (i_Ri)));
// OBSOLETE     SETMEMQI (current_cpu, pc, * FLD (i_Ri), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* borh: borh $u4,@$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,borh) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     QI opval = ORQI (SLLQI (FLD (f_u4), 4), GETMEMQI (current_cpu, pc, * FLD (i_Ri)));
// OBSOLETE     SETMEMQI (current_cpu, pc, * FLD (i_Ri), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* beorh: beorh $u4,@$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,beorh) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     QI opval = XORQI (SLLQI (FLD (f_u4), 4), GETMEMQI (current_cpu, pc, * FLD (i_Ri)));
// OBSOLETE     SETMEMQI (current_cpu, pc, * FLD (i_Ri), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* btstl: btstl $u4,@$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,btstl) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   QI tmp_tmp;
// OBSOLETE   tmp_tmp = ANDQI (FLD (f_u4), GETMEMQI (current_cpu, pc, * FLD (i_Ri)));
// OBSOLETE   {
// OBSOLETE     BI opval = EQQI (tmp_tmp, 0);
// OBSOLETE     CPU (h_zbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = 0;
// OBSOLETE     CPU (h_nbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* btsth: btsth $u4,@$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,btsth) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   QI tmp_tmp;
// OBSOLETE   tmp_tmp = ANDQI (SLLQI (FLD (f_u4), 4), GETMEMQI (current_cpu, pc, * FLD (i_Ri)));
// OBSOLETE   {
// OBSOLETE     BI opval = EQQI (tmp_tmp, 0);
// OBSOLETE     CPU (h_zbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = LTQI (tmp_tmp, 0);
// OBSOLETE     CPU (h_nbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* mul: mul $Rj,$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,mul) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   DI tmp_tmp;
// OBSOLETE   tmp_tmp = MULDI (EXTSIDI (* FLD (i_Rj)), EXTSIDI (* FLD (i_Ri)));
// OBSOLETE   {
// OBSOLETE     SI opval = TRUNCDISI (tmp_tmp);
// OBSOLETE     SET_H_DR (((UINT) 5), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "dr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = TRUNCDISI (SRLDI (tmp_tmp, 32));
// OBSOLETE     SET_H_DR (((UINT) 4), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "dr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = LTSI (GET_H_DR (((UINT) 5)), 0);
// OBSOLETE     CPU (h_nbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = EQDI (tmp_tmp, MAKEDI (0, 0));
// OBSOLETE     CPU (h_zbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = ORIF (GTDI (tmp_tmp, MAKEDI (0, 2147483647)), LTDI (tmp_tmp, NEGDI (MAKEDI (0, 0x80000000))));
// OBSOLETE     CPU (h_vbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "vbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* mulu: mulu $Rj,$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,mulu) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   DI tmp_tmp;
// OBSOLETE   tmp_tmp = MULDI (ZEXTSIDI (* FLD (i_Rj)), ZEXTSIDI (* FLD (i_Ri)));
// OBSOLETE   {
// OBSOLETE     SI opval = TRUNCDISI (tmp_tmp);
// OBSOLETE     SET_H_DR (((UINT) 5), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "dr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = TRUNCDISI (SRLDI (tmp_tmp, 32));
// OBSOLETE     SET_H_DR (((UINT) 4), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "dr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = LTSI (GET_H_DR (((UINT) 4)), 0);
// OBSOLETE     CPU (h_nbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = EQSI (GET_H_DR (((UINT) 5)), 0);
// OBSOLETE     CPU (h_zbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = NESI (GET_H_DR (((UINT) 4)), 0);
// OBSOLETE     CPU (h_vbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "vbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* mulh: mulh $Rj,$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,mulh) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = MULHI (TRUNCSIHI (* FLD (i_Rj)), TRUNCSIHI (* FLD (i_Ri)));
// OBSOLETE     SET_H_DR (((UINT) 5), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "dr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = LTSI (GET_H_DR (((UINT) 5)), 0);
// OBSOLETE     CPU (h_nbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = GESI (GET_H_DR (((UINT) 5)), 0);
// OBSOLETE     CPU (h_zbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* muluh: muluh $Rj,$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,muluh) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = MULSI (ANDSI (* FLD (i_Rj), 65535), ANDSI (* FLD (i_Ri), 65535));
// OBSOLETE     SET_H_DR (((UINT) 5), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "dr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = LTSI (GET_H_DR (((UINT) 5)), 0);
// OBSOLETE     CPU (h_nbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = GESI (GET_H_DR (((UINT) 5)), 0);
// OBSOLETE     CPU (h_zbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* div0s: div0s $Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,div0s) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_mov2dr.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     BI opval = LTSI (GET_H_DR (((UINT) 5)), 0);
// OBSOLETE     CPU (h_d0bit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "d0bit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = XORBI (CPU (h_d0bit), LTSI (* FLD (i_Ri), 0));
// OBSOLETE     CPU (h_d1bit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "d1bit", 'x', opval);
// OBSOLETE   }
// OBSOLETE if (NEBI (CPU (h_d0bit), 0)) {
// OBSOLETE   {
// OBSOLETE     SI opval = 0xffffffff;
// OBSOLETE     SET_H_DR (((UINT) 4), opval);
// OBSOLETE     written |= (1 << 5);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "dr", 'x', opval);
// OBSOLETE   }
// OBSOLETE } else {
// OBSOLETE   {
// OBSOLETE     SI opval = 0;
// OBSOLETE     SET_H_DR (((UINT) 4), opval);
// OBSOLETE     written |= (1 << 5);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "dr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* div0u: div0u $Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,div0u) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     BI opval = 0;
// OBSOLETE     CPU (h_d0bit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "d0bit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = 0;
// OBSOLETE     CPU (h_d1bit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "d1bit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = 0;
// OBSOLETE     SET_H_DR (((UINT) 4), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "dr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* div1: div1 $Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,div1) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_mov2dr.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   SI tmp_tmp;
// OBSOLETE   {
// OBSOLETE     SI opval = SLLSI (GET_H_DR (((UINT) 4)), 1);
// OBSOLETE     SET_H_DR (((UINT) 4), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "dr", 'x', opval);
// OBSOLETE   }
// OBSOLETE if (LTSI (GET_H_DR (((UINT) 5)), 0)) {
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (GET_H_DR (((UINT) 4)), 1);
// OBSOLETE     SET_H_DR (((UINT) 4), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "dr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE   {
// OBSOLETE     SI opval = SLLSI (GET_H_DR (((UINT) 5)), 1);
// OBSOLETE     SET_H_DR (((UINT) 5), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "dr", 'x', opval);
// OBSOLETE   }
// OBSOLETE if (EQBI (CPU (h_d1bit), 1)) {
// OBSOLETE {
// OBSOLETE   tmp_tmp = ADDSI (GET_H_DR (((UINT) 4)), * FLD (i_Ri));
// OBSOLETE   {
// OBSOLETE     BI opval = ADDCFSI (GET_H_DR (((UINT) 4)), * FLD (i_Ri), 0);
// OBSOLETE     CPU (h_cbit) = opval;
// OBSOLETE     written |= (1 << 6);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE } else {
// OBSOLETE {
// OBSOLETE   tmp_tmp = SUBSI (GET_H_DR (((UINT) 4)), * FLD (i_Ri));
// OBSOLETE   {
// OBSOLETE     BI opval = SUBCFSI (GET_H_DR (((UINT) 4)), * FLD (i_Ri), 0);
// OBSOLETE     CPU (h_cbit) = opval;
// OBSOLETE     written |= (1 << 6);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE if (NOTBI (XORBI (XORBI (CPU (h_d0bit), CPU (h_d1bit)), CPU (h_cbit)))) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = tmp_tmp;
// OBSOLETE     SET_H_DR (((UINT) 4), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "dr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = ORSI (GET_H_DR (((UINT) 5)), 1);
// OBSOLETE     SET_H_DR (((UINT) 5), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "dr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE   {
// OBSOLETE     BI opval = EQSI (GET_H_DR (((UINT) 4)), 0);
// OBSOLETE     CPU (h_zbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* div2: div2 $Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,div2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_mov2dr.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   SI tmp_tmp;
// OBSOLETE if (EQBI (CPU (h_d1bit), 1)) {
// OBSOLETE {
// OBSOLETE   tmp_tmp = ADDSI (GET_H_DR (((UINT) 4)), * FLD (i_Ri));
// OBSOLETE   {
// OBSOLETE     BI opval = ADDCFSI (GET_H_DR (((UINT) 4)), * FLD (i_Ri), 0);
// OBSOLETE     CPU (h_cbit) = opval;
// OBSOLETE     written |= (1 << 3);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE } else {
// OBSOLETE {
// OBSOLETE   tmp_tmp = SUBSI (GET_H_DR (((UINT) 4)), * FLD (i_Ri));
// OBSOLETE   {
// OBSOLETE     BI opval = SUBCFSI (GET_H_DR (((UINT) 4)), * FLD (i_Ri), 0);
// OBSOLETE     CPU (h_cbit) = opval;
// OBSOLETE     written |= (1 << 3);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE if (EQSI (tmp_tmp, 0)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     BI opval = 1;
// OBSOLETE     CPU (h_zbit) = opval;
// OBSOLETE     written |= (1 << 5);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = 0;
// OBSOLETE     SET_H_DR (((UINT) 4), opval);
// OBSOLETE     written |= (1 << 4);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "dr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE } else {
// OBSOLETE   {
// OBSOLETE     BI opval = 0;
// OBSOLETE     CPU (h_zbit) = opval;
// OBSOLETE     written |= (1 << 5);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* div3: div3 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,div3) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE if (EQBI (CPU (h_zbit), 1)) {
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (GET_H_DR (((UINT) 5)), 1);
// OBSOLETE     SET_H_DR (((UINT) 5), opval);
// OBSOLETE     written |= (1 << 2);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "dr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* div4s: div4s */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,div4s) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE if (EQBI (CPU (h_d1bit), 1)) {
// OBSOLETE   {
// OBSOLETE     SI opval = NEGSI (GET_H_DR (((UINT) 5)));
// OBSOLETE     SET_H_DR (((UINT) 5), opval);
// OBSOLETE     written |= (1 << 2);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "dr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* lsl: lsl $Rj,$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,lsl) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   SI tmp_shift;
// OBSOLETE   tmp_shift = ANDSI (* FLD (i_Rj), 31);
// OBSOLETE if (NESI (tmp_shift, 0)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     BI opval = NESI (ANDSI (* FLD (i_Ri), SLLSI (1, SUBSI (32, tmp_shift))), 0);
// OBSOLETE     CPU (h_cbit) = opval;
// OBSOLETE     written |= (1 << 3);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = SLLSI (* FLD (i_Ri), tmp_shift);
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     written |= (1 << 2);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE } else {
// OBSOLETE   {
// OBSOLETE     BI opval = 0;
// OBSOLETE     CPU (h_cbit) = opval;
// OBSOLETE     written |= (1 << 3);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE   {
// OBSOLETE     BI opval = LTSI (* FLD (i_Ri), 0);
// OBSOLETE     CPU (h_nbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = EQSI (* FLD (i_Ri), 0);
// OBSOLETE     CPU (h_zbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* lsli: lsl $u4,$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,lsli) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   SI tmp_shift;
// OBSOLETE   tmp_shift = FLD (f_u4);
// OBSOLETE if (NESI (tmp_shift, 0)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     BI opval = NESI (ANDSI (* FLD (i_Ri), SLLSI (1, SUBSI (32, tmp_shift))), 0);
// OBSOLETE     CPU (h_cbit) = opval;
// OBSOLETE     written |= (1 << 3);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = SLLSI (* FLD (i_Ri), tmp_shift);
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     written |= (1 << 2);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE } else {
// OBSOLETE   {
// OBSOLETE     BI opval = 0;
// OBSOLETE     CPU (h_cbit) = opval;
// OBSOLETE     written |= (1 << 3);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE   {
// OBSOLETE     BI opval = LTSI (* FLD (i_Ri), 0);
// OBSOLETE     CPU (h_nbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = EQSI (* FLD (i_Ri), 0);
// OBSOLETE     CPU (h_zbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* lsl2: lsl2 $u4,$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,lsl2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   SI tmp_shift;
// OBSOLETE   tmp_shift = ADDSI (FLD (f_u4), 16);
// OBSOLETE if (NESI (tmp_shift, 0)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     BI opval = NESI (ANDSI (* FLD (i_Ri), SLLSI (1, SUBSI (32, tmp_shift))), 0);
// OBSOLETE     CPU (h_cbit) = opval;
// OBSOLETE     written |= (1 << 3);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = SLLSI (* FLD (i_Ri), tmp_shift);
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     written |= (1 << 2);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE } else {
// OBSOLETE   {
// OBSOLETE     BI opval = 0;
// OBSOLETE     CPU (h_cbit) = opval;
// OBSOLETE     written |= (1 << 3);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE   {
// OBSOLETE     BI opval = LTSI (* FLD (i_Ri), 0);
// OBSOLETE     CPU (h_nbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = EQSI (* FLD (i_Ri), 0);
// OBSOLETE     CPU (h_zbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* lsr: lsr $Rj,$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,lsr) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   SI tmp_shift;
// OBSOLETE   tmp_shift = ANDSI (* FLD (i_Rj), 31);
// OBSOLETE if (NESI (tmp_shift, 0)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     BI opval = NESI (ANDSI (* FLD (i_Ri), SLLSI (1, SUBSI (tmp_shift, 1))), 0);
// OBSOLETE     CPU (h_cbit) = opval;
// OBSOLETE     written |= (1 << 3);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = SRLSI (* FLD (i_Ri), tmp_shift);
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     written |= (1 << 2);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE } else {
// OBSOLETE   {
// OBSOLETE     BI opval = 0;
// OBSOLETE     CPU (h_cbit) = opval;
// OBSOLETE     written |= (1 << 3);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE   {
// OBSOLETE     BI opval = LTSI (* FLD (i_Ri), 0);
// OBSOLETE     CPU (h_nbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = EQSI (* FLD (i_Ri), 0);
// OBSOLETE     CPU (h_zbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* lsri: lsr $u4,$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,lsri) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   SI tmp_shift;
// OBSOLETE   tmp_shift = FLD (f_u4);
// OBSOLETE if (NESI (tmp_shift, 0)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     BI opval = NESI (ANDSI (* FLD (i_Ri), SLLSI (1, SUBSI (tmp_shift, 1))), 0);
// OBSOLETE     CPU (h_cbit) = opval;
// OBSOLETE     written |= (1 << 3);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = SRLSI (* FLD (i_Ri), tmp_shift);
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     written |= (1 << 2);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE } else {
// OBSOLETE   {
// OBSOLETE     BI opval = 0;
// OBSOLETE     CPU (h_cbit) = opval;
// OBSOLETE     written |= (1 << 3);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE   {
// OBSOLETE     BI opval = LTSI (* FLD (i_Ri), 0);
// OBSOLETE     CPU (h_nbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = EQSI (* FLD (i_Ri), 0);
// OBSOLETE     CPU (h_zbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* lsr2: lsr2 $u4,$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,lsr2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   SI tmp_shift;
// OBSOLETE   tmp_shift = ADDSI (FLD (f_u4), 16);
// OBSOLETE if (NESI (tmp_shift, 0)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     BI opval = NESI (ANDSI (* FLD (i_Ri), SLLSI (1, SUBSI (tmp_shift, 1))), 0);
// OBSOLETE     CPU (h_cbit) = opval;
// OBSOLETE     written |= (1 << 3);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = SRLSI (* FLD (i_Ri), tmp_shift);
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     written |= (1 << 2);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE } else {
// OBSOLETE   {
// OBSOLETE     BI opval = 0;
// OBSOLETE     CPU (h_cbit) = opval;
// OBSOLETE     written |= (1 << 3);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE   {
// OBSOLETE     BI opval = LTSI (* FLD (i_Ri), 0);
// OBSOLETE     CPU (h_nbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = EQSI (* FLD (i_Ri), 0);
// OBSOLETE     CPU (h_zbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* asr: asr $Rj,$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,asr) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   SI tmp_shift;
// OBSOLETE   tmp_shift = ANDSI (* FLD (i_Rj), 31);
// OBSOLETE if (NESI (tmp_shift, 0)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     BI opval = NESI (ANDSI (* FLD (i_Ri), SLLSI (1, SUBSI (tmp_shift, 1))), 0);
// OBSOLETE     CPU (h_cbit) = opval;
// OBSOLETE     written |= (1 << 3);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = SRASI (* FLD (i_Ri), tmp_shift);
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     written |= (1 << 2);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE } else {
// OBSOLETE   {
// OBSOLETE     BI opval = 0;
// OBSOLETE     CPU (h_cbit) = opval;
// OBSOLETE     written |= (1 << 3);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE   {
// OBSOLETE     BI opval = LTSI (* FLD (i_Ri), 0);
// OBSOLETE     CPU (h_nbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = EQSI (* FLD (i_Ri), 0);
// OBSOLETE     CPU (h_zbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* asri: asr $u4,$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,asri) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   SI tmp_shift;
// OBSOLETE   tmp_shift = FLD (f_u4);
// OBSOLETE if (NESI (tmp_shift, 0)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     BI opval = NESI (ANDSI (* FLD (i_Ri), SLLSI (1, SUBSI (tmp_shift, 1))), 0);
// OBSOLETE     CPU (h_cbit) = opval;
// OBSOLETE     written |= (1 << 3);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = SRASI (* FLD (i_Ri), tmp_shift);
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     written |= (1 << 2);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE } else {
// OBSOLETE   {
// OBSOLETE     BI opval = 0;
// OBSOLETE     CPU (h_cbit) = opval;
// OBSOLETE     written |= (1 << 3);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE   {
// OBSOLETE     BI opval = LTSI (* FLD (i_Ri), 0);
// OBSOLETE     CPU (h_nbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = EQSI (* FLD (i_Ri), 0);
// OBSOLETE     CPU (h_zbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* asr2: asr2 $u4,$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,asr2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   SI tmp_shift;
// OBSOLETE   tmp_shift = ADDSI (FLD (f_u4), 16);
// OBSOLETE if (NESI (tmp_shift, 0)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     BI opval = NESI (ANDSI (* FLD (i_Ri), SLLSI (1, SUBSI (tmp_shift, 1))), 0);
// OBSOLETE     CPU (h_cbit) = opval;
// OBSOLETE     written |= (1 << 3);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = SRASI (* FLD (i_Ri), tmp_shift);
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     written |= (1 << 2);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE } else {
// OBSOLETE   {
// OBSOLETE     BI opval = 0;
// OBSOLETE     CPU (h_cbit) = opval;
// OBSOLETE     written |= (1 << 3);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE   {
// OBSOLETE     BI opval = LTSI (* FLD (i_Ri), 0);
// OBSOLETE     CPU (h_nbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     BI opval = EQSI (* FLD (i_Ri), 0);
// OBSOLETE     CPU (h_zbit) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* ldi8: ldi:8 $i8,$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,ldi8) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldi8.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = FLD (f_i8);
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* ldi20: ldi:20 $i20,$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,ldi20) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldi20.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = FLD (f_i20);
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* ldi32: ldi:32 $i32,$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,ldi32) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldi32.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 6);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = FLD (f_i32);
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* ld: ld @$Rj,$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,ld) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr13.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMSI (current_cpu, pc, * FLD (i_Rj));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* lduh: lduh @$Rj,$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,lduh) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr13.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMUHI (current_cpu, pc, * FLD (i_Rj));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* ldub: ldub @$Rj,$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,ldub) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr13.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMUQI (current_cpu, pc, * FLD (i_Rj));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* ldr13: ld @($R13,$Rj),$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,ldr13) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr13.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMSI (current_cpu, pc, ADDSI (* FLD (i_Rj), CPU (h_gr[((UINT) 13)])));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* ldr13uh: lduh @($R13,$Rj),$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,ldr13uh) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr13.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMUHI (current_cpu, pc, ADDSI (* FLD (i_Rj), CPU (h_gr[((UINT) 13)])));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* ldr13ub: ldub @($R13,$Rj),$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,ldr13ub) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr13.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMUQI (current_cpu, pc, ADDSI (* FLD (i_Rj), CPU (h_gr[((UINT) 13)])));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* ldr14: ld @($R14,$disp10),$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,ldr14) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr14.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMSI (current_cpu, pc, ADDSI (FLD (f_disp10), CPU (h_gr[((UINT) 14)])));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* ldr14uh: lduh @($R14,$disp9),$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,ldr14uh) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr14uh.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMUHI (current_cpu, pc, ADDSI (FLD (f_disp9), CPU (h_gr[((UINT) 14)])));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* ldr14ub: ldub @($R14,$disp8),$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,ldr14ub) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr14ub.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMUQI (current_cpu, pc, ADDSI (FLD (f_disp8), CPU (h_gr[((UINT) 14)])));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* ldr15: ld @($R15,$udisp6),$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,ldr15) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr15.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMSI (current_cpu, pc, ADDSI (FLD (f_udisp6), CPU (h_gr[((UINT) 15)])));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* ldr15gr: ld @$R15+,$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,ldr15gr) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr15gr.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE if (NESI (FLD (f_Ri), 15)) {
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     written |= (1 << 4);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* ldr15dr: ld @$R15+,$Rs2 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,ldr15dr) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr15dr.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   SI tmp_tmp;
// OBSOLETE   tmp_tmp = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]));
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = tmp_tmp;
// OBSOLETE     SET_H_DR (FLD (f_Rs2), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "dr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* ldr15ps: ld @$R15+,$ps */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,ldr15ps) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addsp.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     USI opval = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]));
// OBSOLETE     SET_H_PS (opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "ps", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* st: st $Ri,@$Rj */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,st) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = * FLD (i_Ri);
// OBSOLETE     SETMEMSI (current_cpu, pc, * FLD (i_Rj), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* sth: sth $Ri,@$Rj */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,sth) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     HI opval = * FLD (i_Ri);
// OBSOLETE     SETMEMHI (current_cpu, pc, * FLD (i_Rj), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* stb: stb $Ri,@$Rj */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,stb) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     QI opval = * FLD (i_Ri);
// OBSOLETE     SETMEMQI (current_cpu, pc, * FLD (i_Rj), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* str13: st $Ri,@($R13,$Rj) */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,str13) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = * FLD (i_Ri);
// OBSOLETE     SETMEMSI (current_cpu, pc, ADDSI (* FLD (i_Rj), CPU (h_gr[((UINT) 13)])), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* str13h: sth $Ri,@($R13,$Rj) */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,str13h) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     HI opval = * FLD (i_Ri);
// OBSOLETE     SETMEMHI (current_cpu, pc, ADDSI (* FLD (i_Rj), CPU (h_gr[((UINT) 13)])), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* str13b: stb $Ri,@($R13,$Rj) */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,str13b) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     QI opval = * FLD (i_Ri);
// OBSOLETE     SETMEMQI (current_cpu, pc, ADDSI (* FLD (i_Rj), CPU (h_gr[((UINT) 13)])), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* str14: st $Ri,@($R14,$disp10) */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,str14) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str14.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = * FLD (i_Ri);
// OBSOLETE     SETMEMSI (current_cpu, pc, ADDSI (FLD (f_disp10), CPU (h_gr[((UINT) 14)])), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* str14h: sth $Ri,@($R14,$disp9) */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,str14h) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str14h.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     HI opval = * FLD (i_Ri);
// OBSOLETE     SETMEMHI (current_cpu, pc, ADDSI (FLD (f_disp9), CPU (h_gr[((UINT) 14)])), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* str14b: stb $Ri,@($R14,$disp8) */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,str14b) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str14b.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     QI opval = * FLD (i_Ri);
// OBSOLETE     SETMEMQI (current_cpu, pc, ADDSI (FLD (f_disp8), CPU (h_gr[((UINT) 14)])), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* str15: st $Ri,@($R15,$udisp6) */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,str15) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str15.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = * FLD (i_Ri);
// OBSOLETE     SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 15)]), FLD (f_udisp6)), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* str15gr: st $Ri,@-$R15 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,str15gr) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str15gr.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   SI tmp_tmp;
// OBSOLETE   tmp_tmp = * FLD (i_Ri);
// OBSOLETE   {
// OBSOLETE     SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = tmp_tmp;
// OBSOLETE     SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* str15dr: st $Rs2,@-$R15 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,str15dr) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr15dr.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   SI tmp_tmp;
// OBSOLETE   tmp_tmp = GET_H_DR (FLD (f_Rs2));
// OBSOLETE   {
// OBSOLETE     SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = tmp_tmp;
// OBSOLETE     SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* str15ps: st $ps,@-$R15 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,str15ps) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addsp.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = GET_H_PS ();
// OBSOLETE     SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* mov: mov $Rj,$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,mov) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr13.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = * FLD (i_Rj);
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* movdr: mov $Rs1,$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,movdr) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_movdr.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = GET_H_DR (FLD (f_Rs1));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* movps: mov $ps,$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,movps) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_movdr.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = GET_H_PS ();
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* mov2dr: mov $Ri,$Rs1 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,mov2dr) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_mov2dr.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = * FLD (i_Ri);
// OBSOLETE     SET_H_DR (FLD (f_Rs1), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "dr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* mov2ps: mov $Ri,$ps */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,mov2ps) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_mov2dr.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     USI opval = * FLD (i_Ri);
// OBSOLETE     SET_H_PS (opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "ps", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* jmp: jmp @$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,jmp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_mov2dr.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     USI opval = * FLD (i_Ri);
// OBSOLETE     SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* jmpd: jmp:d @$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,jmpd) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_mov2dr.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     USI opval = * FLD (i_Ri);
// OBSOLETE     SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* callr: call @$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,callr) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_mov2dr.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (pc, 2);
// OBSOLETE     SET_H_DR (((UINT) 1), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "dr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     USI opval = * FLD (i_Ri);
// OBSOLETE     SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* callrd: call:d @$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,callrd) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_mov2dr.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (pc, 4);
// OBSOLETE     SET_H_DR (((UINT) 1), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "dr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     USI opval = * FLD (i_Ri);
// OBSOLETE     SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* call: call $label12 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,call) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_call.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (pc, 2);
// OBSOLETE     SET_H_DR (((UINT) 1), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "dr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     USI opval = FLD (i_label12);
// OBSOLETE     SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* calld: call:d $label12 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,calld) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_call.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (pc, 4);
// OBSOLETE     SET_H_DR (((UINT) 1), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "dr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     USI opval = FLD (i_label12);
// OBSOLETE     SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* ret: ret */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,ret) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     USI opval = GET_H_DR (((UINT) 1));
// OBSOLETE     SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* ret:d: ret:d */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,ret_d) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     USI opval = GET_H_DR (((UINT) 1));
// OBSOLETE     SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* int: int $u8 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,int) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_int.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE ; /*clobber*/
// OBSOLETE ; /*clobber*/
// OBSOLETE ; /*clobber*/
// OBSOLETE   {
// OBSOLETE     SI opval = fr30_int (current_cpu, pc, FLD (f_u8));
// OBSOLETE     SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* inte: inte */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,inte) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE ; /*clobber*/
// OBSOLETE ; /*clobber*/
// OBSOLETE ; /*clobber*/
// OBSOLETE   {
// OBSOLETE     SI opval = fr30_inte (current_cpu, pc);
// OBSOLETE     SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* reti: reti */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,reti) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE if (EQBI (GET_H_SBIT (), 0)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMSI (current_cpu, pc, GET_H_DR (((UINT) 2)));
// OBSOLETE     SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     written |= (1 << 7);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (GET_H_DR (((UINT) 2)), 4);
// OBSOLETE     SET_H_DR (((UINT) 2), opval);
// OBSOLETE     written |= (1 << 5);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "dr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMSI (current_cpu, pc, GET_H_DR (((UINT) 2)));
// OBSOLETE     SET_H_PS (opval);
// OBSOLETE     written |= (1 << 8);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "ps", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (GET_H_DR (((UINT) 2)), 4);
// OBSOLETE     SET_H_DR (((UINT) 2), opval);
// OBSOLETE     written |= (1 << 5);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "dr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE } else {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMSI (current_cpu, pc, GET_H_DR (((UINT) 3)));
// OBSOLETE     SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     written |= (1 << 7);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (GET_H_DR (((UINT) 3)), 4);
// OBSOLETE     SET_H_DR (((UINT) 3), opval);
// OBSOLETE     written |= (1 << 6);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "dr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMSI (current_cpu, pc, GET_H_DR (((UINT) 3)));
// OBSOLETE     SET_H_PS (opval);
// OBSOLETE     written |= (1 << 8);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "ps", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (GET_H_DR (((UINT) 3)), 4);
// OBSOLETE     SET_H_DR (((UINT) 3), opval);
// OBSOLETE     written |= (1 << 6);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "dr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* brad: bra:d $label9 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,brad) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     USI opval = FLD (i_label9);
// OBSOLETE     SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* bra: bra $label9 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,bra) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     USI opval = FLD (i_label9);
// OBSOLETE     SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* bnod: bno:d $label9 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,bnod) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE ((void) 0); /*nop*/
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* bno: bno $label9 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,bno) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE ((void) 0); /*nop*/
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* beqd: beq:d $label9 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,beqd) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE if (CPU (h_zbit)) {
// OBSOLETE   {
// OBSOLETE     USI opval = FLD (i_label9);
// OBSOLETE     SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     written |= (1 << 2);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* beq: beq $label9 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,beq) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE if (CPU (h_zbit)) {
// OBSOLETE   {
// OBSOLETE     USI opval = FLD (i_label9);
// OBSOLETE     SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     written |= (1 << 2);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* bned: bne:d $label9 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,bned) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE if (NOTBI (CPU (h_zbit))) {
// OBSOLETE   {
// OBSOLETE     USI opval = FLD (i_label9);
// OBSOLETE     SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     written |= (1 << 2);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* bne: bne $label9 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,bne) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE if (NOTBI (CPU (h_zbit))) {
// OBSOLETE   {
// OBSOLETE     USI opval = FLD (i_label9);
// OBSOLETE     SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     written |= (1 << 2);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* bcd: bc:d $label9 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,bcd) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE if (CPU (h_cbit)) {
// OBSOLETE   {
// OBSOLETE     USI opval = FLD (i_label9);
// OBSOLETE     SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     written |= (1 << 2);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* bc: bc $label9 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,bc) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE if (CPU (h_cbit)) {
// OBSOLETE   {
// OBSOLETE     USI opval = FLD (i_label9);
// OBSOLETE     SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     written |= (1 << 2);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* bncd: bnc:d $label9 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,bncd) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE if (NOTBI (CPU (h_cbit))) {
// OBSOLETE   {
// OBSOLETE     USI opval = FLD (i_label9);
// OBSOLETE     SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     written |= (1 << 2);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* bnc: bnc $label9 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,bnc) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE if (NOTBI (CPU (h_cbit))) {
// OBSOLETE   {
// OBSOLETE     USI opval = FLD (i_label9);
// OBSOLETE     SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     written |= (1 << 2);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* bnd: bn:d $label9 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,bnd) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE if (CPU (h_nbit)) {
// OBSOLETE   {
// OBSOLETE     USI opval = FLD (i_label9);
// OBSOLETE     SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     written |= (1 << 2);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* bn: bn $label9 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,bn) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE if (CPU (h_nbit)) {
// OBSOLETE   {
// OBSOLETE     USI opval = FLD (i_label9);
// OBSOLETE     SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     written |= (1 << 2);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* bpd: bp:d $label9 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,bpd) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE if (NOTBI (CPU (h_nbit))) {
// OBSOLETE   {
// OBSOLETE     USI opval = FLD (i_label9);
// OBSOLETE     SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     written |= (1 << 2);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* bp: bp $label9 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,bp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE if (NOTBI (CPU (h_nbit))) {
// OBSOLETE   {
// OBSOLETE     USI opval = FLD (i_label9);
// OBSOLETE     SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     written |= (1 << 2);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* bvd: bv:d $label9 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,bvd) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE if (CPU (h_vbit)) {
// OBSOLETE   {
// OBSOLETE     USI opval = FLD (i_label9);
// OBSOLETE     SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     written |= (1 << 2);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* bv: bv $label9 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,bv) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE if (CPU (h_vbit)) {
// OBSOLETE   {
// OBSOLETE     USI opval = FLD (i_label9);
// OBSOLETE     SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     written |= (1 << 2);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* bnvd: bnv:d $label9 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,bnvd) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE if (NOTBI (CPU (h_vbit))) {
// OBSOLETE   {
// OBSOLETE     USI opval = FLD (i_label9);
// OBSOLETE     SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     written |= (1 << 2);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* bnv: bnv $label9 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,bnv) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE if (NOTBI (CPU (h_vbit))) {
// OBSOLETE   {
// OBSOLETE     USI opval = FLD (i_label9);
// OBSOLETE     SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     written |= (1 << 2);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* bltd: blt:d $label9 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,bltd) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE if (XORBI (CPU (h_vbit), CPU (h_nbit))) {
// OBSOLETE   {
// OBSOLETE     USI opval = FLD (i_label9);
// OBSOLETE     SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     written |= (1 << 3);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* blt: blt $label9 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,blt) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE if (XORBI (CPU (h_vbit), CPU (h_nbit))) {
// OBSOLETE   {
// OBSOLETE     USI opval = FLD (i_label9);
// OBSOLETE     SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     written |= (1 << 3);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* bged: bge:d $label9 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,bged) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE if (NOTBI (XORBI (CPU (h_vbit), CPU (h_nbit)))) {
// OBSOLETE   {
// OBSOLETE     USI opval = FLD (i_label9);
// OBSOLETE     SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     written |= (1 << 3);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* bge: bge $label9 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,bge) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE if (NOTBI (XORBI (CPU (h_vbit), CPU (h_nbit)))) {
// OBSOLETE   {
// OBSOLETE     USI opval = FLD (i_label9);
// OBSOLETE     SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     written |= (1 << 3);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* bled: ble:d $label9 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,bled) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE if (ORBI (XORBI (CPU (h_vbit), CPU (h_nbit)), CPU (h_zbit))) {
// OBSOLETE   {
// OBSOLETE     USI opval = FLD (i_label9);
// OBSOLETE     SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     written |= (1 << 4);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* ble: ble $label9 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,ble) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE if (ORBI (XORBI (CPU (h_vbit), CPU (h_nbit)), CPU (h_zbit))) {
// OBSOLETE   {
// OBSOLETE     USI opval = FLD (i_label9);
// OBSOLETE     SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     written |= (1 << 4);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* bgtd: bgt:d $label9 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,bgtd) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE if (NOTBI (ORBI (XORBI (CPU (h_vbit), CPU (h_nbit)), CPU (h_zbit)))) {
// OBSOLETE   {
// OBSOLETE     USI opval = FLD (i_label9);
// OBSOLETE     SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     written |= (1 << 4);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* bgt: bgt $label9 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,bgt) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE if (NOTBI (ORBI (XORBI (CPU (h_vbit), CPU (h_nbit)), CPU (h_zbit)))) {
// OBSOLETE   {
// OBSOLETE     USI opval = FLD (i_label9);
// OBSOLETE     SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     written |= (1 << 4);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* blsd: bls:d $label9 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,blsd) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE if (ORBI (CPU (h_cbit), CPU (h_zbit))) {
// OBSOLETE   {
// OBSOLETE     USI opval = FLD (i_label9);
// OBSOLETE     SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     written |= (1 << 3);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* bls: bls $label9 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,bls) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE if (ORBI (CPU (h_cbit), CPU (h_zbit))) {
// OBSOLETE   {
// OBSOLETE     USI opval = FLD (i_label9);
// OBSOLETE     SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     written |= (1 << 3);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* bhid: bhi:d $label9 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,bhid) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE if (NOTBI (ORBI (CPU (h_cbit), CPU (h_zbit)))) {
// OBSOLETE   {
// OBSOLETE     USI opval = FLD (i_label9);
// OBSOLETE     SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     written |= (1 << 3);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* bhi: bhi $label9 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,bhi) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE if (NOTBI (ORBI (CPU (h_cbit), CPU (h_zbit)))) {
// OBSOLETE   {
// OBSOLETE     USI opval = FLD (i_label9);
// OBSOLETE     SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     written |= (1 << 3);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* dmovr13: dmov $R13,@$dir10 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,dmovr13) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pi.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = CPU (h_gr[((UINT) 13)]);
// OBSOLETE     SETMEMSI (current_cpu, pc, FLD (f_dir10), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* dmovr13h: dmovh $R13,@$dir9 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,dmovr13h) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pih.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     HI opval = CPU (h_gr[((UINT) 13)]);
// OBSOLETE     SETMEMHI (current_cpu, pc, FLD (f_dir9), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* dmovr13b: dmovb $R13,@$dir8 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,dmovr13b) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pib.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     QI opval = CPU (h_gr[((UINT) 13)]);
// OBSOLETE     SETMEMQI (current_cpu, pc, FLD (f_dir8), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* dmovr13pi: dmov @$R13+,@$dir10 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,dmovr13pi) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pi.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 13)]));
// OBSOLETE     SETMEMSI (current_cpu, pc, FLD (f_dir10), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (CPU (h_gr[((UINT) 13)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 13)]) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* dmovr13pih: dmovh @$R13+,@$dir9 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,dmovr13pih) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pih.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     HI opval = GETMEMHI (current_cpu, pc, CPU (h_gr[((UINT) 13)]));
// OBSOLETE     SETMEMHI (current_cpu, pc, FLD (f_dir9), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (CPU (h_gr[((UINT) 13)]), 2);
// OBSOLETE     CPU (h_gr[((UINT) 13)]) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* dmovr13pib: dmovb @$R13+,@$dir8 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,dmovr13pib) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pib.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     QI opval = GETMEMQI (current_cpu, pc, CPU (h_gr[((UINT) 13)]));
// OBSOLETE     SETMEMQI (current_cpu, pc, FLD (f_dir8), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (CPU (h_gr[((UINT) 13)]), 1);
// OBSOLETE     CPU (h_gr[((UINT) 13)]) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* dmovr15pi: dmov @$R15+,@$dir10 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,dmovr15pi) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr15pi.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]));
// OBSOLETE     SETMEMSI (current_cpu, pc, FLD (f_dir10), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* dmov2r13: dmov @$dir10,$R13 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,dmov2r13) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pi.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMSI (current_cpu, pc, FLD (f_dir10));
// OBSOLETE     CPU (h_gr[((UINT) 13)]) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* dmov2r13h: dmovh @$dir9,$R13 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,dmov2r13h) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pih.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMHI (current_cpu, pc, FLD (f_dir9));
// OBSOLETE     CPU (h_gr[((UINT) 13)]) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* dmov2r13b: dmovb @$dir8,$R13 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,dmov2r13b) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pib.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMQI (current_cpu, pc, FLD (f_dir8));
// OBSOLETE     CPU (h_gr[((UINT) 13)]) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* dmov2r13pi: dmov @$dir10,@$R13+ */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,dmov2r13pi) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pi.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMSI (current_cpu, pc, FLD (f_dir10));
// OBSOLETE     SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 13)]), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (CPU (h_gr[((UINT) 13)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 13)]) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* dmov2r13pih: dmovh @$dir9,@$R13+ */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,dmov2r13pih) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pih.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     HI opval = GETMEMHI (current_cpu, pc, FLD (f_dir9));
// OBSOLETE     SETMEMHI (current_cpu, pc, CPU (h_gr[((UINT) 13)]), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (CPU (h_gr[((UINT) 13)]), 2);
// OBSOLETE     CPU (h_gr[((UINT) 13)]) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* dmov2r13pib: dmovb @$dir8,@$R13+ */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,dmov2r13pib) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pib.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     QI opval = GETMEMQI (current_cpu, pc, FLD (f_dir8));
// OBSOLETE     SETMEMQI (current_cpu, pc, CPU (h_gr[((UINT) 13)]), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (CPU (h_gr[((UINT) 13)]), 1);
// OBSOLETE     CPU (h_gr[((UINT) 13)]) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* dmov2r15pd: dmov @$dir10,@-$R15 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,dmov2r15pd) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr15pi.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMSI (current_cpu, pc, FLD (f_dir10));
// OBSOLETE     SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* ldres: ldres @$Ri+,$u4 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,ldres) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add2.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (* FLD (i_Ri), 4);
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* stres: stres $u4,@$Ri+ */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,stres) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add2.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (* FLD (i_Ri), 4);
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* copop: copop $u4c,$ccc,$CRj,$CRi */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,copop) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);
// OBSOLETE 
// OBSOLETE ((void) 0); /*nop*/
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* copld: copld $u4c,$ccc,$Rjc,$CRi */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,copld) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);
// OBSOLETE 
// OBSOLETE ((void) 0); /*nop*/
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* copst: copst $u4c,$ccc,$CRj,$Ric */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,copst) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);
// OBSOLETE 
// OBSOLETE ((void) 0); /*nop*/
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* copsv: copsv $u4c,$ccc,$CRj,$Ric */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,copsv) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);
// OBSOLETE 
// OBSOLETE ((void) 0); /*nop*/
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* nop: nop */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,nop) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE ((void) 0); /*nop*/
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* andccr: andccr $u8 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,andccr) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_int.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     UQI opval = ANDQI (GET_H_CCR (), FLD (f_u8));
// OBSOLETE     SET_H_CCR (opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "ccr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* orccr: orccr $u8 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,orccr) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_int.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     UQI opval = ORQI (GET_H_CCR (), FLD (f_u8));
// OBSOLETE     SET_H_CCR (opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "ccr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* stilm: stilm $u8 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,stilm) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_int.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     UQI opval = ANDSI (FLD (f_u8), 31);
// OBSOLETE     SET_H_ILM (opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "ilm", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* addsp: addsp $s10 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,addsp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addsp.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), FLD (f_s10));
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* extsb: extsb $Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,extsb) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add2.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = EXTQISI (ANDQI (* FLD (i_Ri), 255));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* extub: extub $Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,extub) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add2.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = ZEXTQISI (ANDQI (* FLD (i_Ri), 255));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* extsh: extsh $Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,extsh) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add2.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = EXTHISI (ANDHI (* FLD (i_Ri), 65535));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* extuh: extuh $Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,extuh) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add2.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = ZEXTHISI (ANDHI (* FLD (i_Ri), 65535));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* ldm0: ldm0 ($reglist_low_ld) */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,ldm0) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldm0.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE if (ANDSI (FLD (f_reglist_low_ld), 1)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]));
// OBSOLETE     CPU (h_gr[((UINT) 0)]) = opval;
// OBSOLETE     written |= (1 << 3);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     written |= (1 << 5);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE if (ANDSI (FLD (f_reglist_low_ld), 2)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]));
// OBSOLETE     CPU (h_gr[((UINT) 1)]) = opval;
// OBSOLETE     written |= (1 << 4);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     written |= (1 << 5);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE if (ANDSI (FLD (f_reglist_low_ld), 4)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]));
// OBSOLETE     CPU (h_gr[((UINT) 2)]) = opval;
// OBSOLETE     written |= (1 << 6);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     written |= (1 << 5);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE if (ANDSI (FLD (f_reglist_low_ld), 8)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]));
// OBSOLETE     CPU (h_gr[((UINT) 3)]) = opval;
// OBSOLETE     written |= (1 << 7);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     written |= (1 << 5);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE if (ANDSI (FLD (f_reglist_low_ld), 16)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]));
// OBSOLETE     CPU (h_gr[((UINT) 4)]) = opval;
// OBSOLETE     written |= (1 << 8);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     written |= (1 << 5);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE if (ANDSI (FLD (f_reglist_low_ld), 32)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]));
// OBSOLETE     CPU (h_gr[((UINT) 5)]) = opval;
// OBSOLETE     written |= (1 << 9);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     written |= (1 << 5);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE if (ANDSI (FLD (f_reglist_low_ld), 64)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]));
// OBSOLETE     CPU (h_gr[((UINT) 6)]) = opval;
// OBSOLETE     written |= (1 << 10);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     written |= (1 << 5);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE if (ANDSI (FLD (f_reglist_low_ld), 128)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]));
// OBSOLETE     CPU (h_gr[((UINT) 7)]) = opval;
// OBSOLETE     written |= (1 << 11);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     written |= (1 << 5);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* ldm1: ldm1 ($reglist_hi_ld) */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,ldm1) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldm1.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE if (ANDSI (FLD (f_reglist_hi_ld), 1)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]));
// OBSOLETE     CPU (h_gr[((UINT) 8)]) = opval;
// OBSOLETE     written |= (1 << 9);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     written |= (1 << 8);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE if (ANDSI (FLD (f_reglist_hi_ld), 2)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]));
// OBSOLETE     CPU (h_gr[((UINT) 9)]) = opval;
// OBSOLETE     written |= (1 << 10);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     written |= (1 << 8);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE if (ANDSI (FLD (f_reglist_hi_ld), 4)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]));
// OBSOLETE     CPU (h_gr[((UINT) 10)]) = opval;
// OBSOLETE     written |= (1 << 3);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     written |= (1 << 8);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE if (ANDSI (FLD (f_reglist_hi_ld), 8)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]));
// OBSOLETE     CPU (h_gr[((UINT) 11)]) = opval;
// OBSOLETE     written |= (1 << 4);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     written |= (1 << 8);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE if (ANDSI (FLD (f_reglist_hi_ld), 16)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]));
// OBSOLETE     CPU (h_gr[((UINT) 12)]) = opval;
// OBSOLETE     written |= (1 << 5);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     written |= (1 << 8);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE if (ANDSI (FLD (f_reglist_hi_ld), 32)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]));
// OBSOLETE     CPU (h_gr[((UINT) 13)]) = opval;
// OBSOLETE     written |= (1 << 6);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     written |= (1 << 8);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE if (ANDSI (FLD (f_reglist_hi_ld), 64)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]));
// OBSOLETE     CPU (h_gr[((UINT) 14)]) = opval;
// OBSOLETE     written |= (1 << 7);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     written |= (1 << 8);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE if (ANDSI (FLD (f_reglist_hi_ld), 128)) {
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]));
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     written |= (1 << 8);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* stm0: stm0 ($reglist_low_st) */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,stm0) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_stm0.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE if (ANDSI (FLD (f_reglist_low_st), 1)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     written |= (1 << 10);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = CPU (h_gr[((UINT) 7)]);
// OBSOLETE     SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]), opval);
// OBSOLETE     written |= (1 << 11);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE if (ANDSI (FLD (f_reglist_low_st), 2)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     written |= (1 << 10);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = CPU (h_gr[((UINT) 6)]);
// OBSOLETE     SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]), opval);
// OBSOLETE     written |= (1 << 11);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE if (ANDSI (FLD (f_reglist_low_st), 4)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     written |= (1 << 10);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = CPU (h_gr[((UINT) 5)]);
// OBSOLETE     SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]), opval);
// OBSOLETE     written |= (1 << 11);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE if (ANDSI (FLD (f_reglist_low_st), 8)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     written |= (1 << 10);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = CPU (h_gr[((UINT) 4)]);
// OBSOLETE     SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]), opval);
// OBSOLETE     written |= (1 << 11);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE if (ANDSI (FLD (f_reglist_low_st), 16)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     written |= (1 << 10);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = CPU (h_gr[((UINT) 3)]);
// OBSOLETE     SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]), opval);
// OBSOLETE     written |= (1 << 11);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE if (ANDSI (FLD (f_reglist_low_st), 32)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     written |= (1 << 10);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = CPU (h_gr[((UINT) 2)]);
// OBSOLETE     SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]), opval);
// OBSOLETE     written |= (1 << 11);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE if (ANDSI (FLD (f_reglist_low_st), 64)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     written |= (1 << 10);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = CPU (h_gr[((UINT) 1)]);
// OBSOLETE     SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]), opval);
// OBSOLETE     written |= (1 << 11);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE if (ANDSI (FLD (f_reglist_low_st), 128)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     written |= (1 << 10);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = CPU (h_gr[((UINT) 0)]);
// OBSOLETE     SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]), opval);
// OBSOLETE     written |= (1 << 11);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* stm1: stm1 ($reglist_hi_st) */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,stm1) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_stm1.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE if (ANDSI (FLD (f_reglist_hi_st), 1)) {
// OBSOLETE {
// OBSOLETE   SI tmp_save_r15;
// OBSOLETE   tmp_save_r15 = CPU (h_gr[((UINT) 15)]);
// OBSOLETE   {
// OBSOLETE     SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     written |= (1 << 9);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = tmp_save_r15;
// OBSOLETE     SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]), opval);
// OBSOLETE     written |= (1 << 10);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE if (ANDSI (FLD (f_reglist_hi_st), 2)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     written |= (1 << 9);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = CPU (h_gr[((UINT) 14)]);
// OBSOLETE     SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]), opval);
// OBSOLETE     written |= (1 << 10);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE if (ANDSI (FLD (f_reglist_hi_st), 4)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     written |= (1 << 9);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = CPU (h_gr[((UINT) 13)]);
// OBSOLETE     SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]), opval);
// OBSOLETE     written |= (1 << 10);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE if (ANDSI (FLD (f_reglist_hi_st), 8)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     written |= (1 << 9);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = CPU (h_gr[((UINT) 12)]);
// OBSOLETE     SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]), opval);
// OBSOLETE     written |= (1 << 10);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE if (ANDSI (FLD (f_reglist_hi_st), 16)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     written |= (1 << 9);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = CPU (h_gr[((UINT) 11)]);
// OBSOLETE     SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]), opval);
// OBSOLETE     written |= (1 << 10);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE if (ANDSI (FLD (f_reglist_hi_st), 32)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     written |= (1 << 9);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = CPU (h_gr[((UINT) 10)]);
// OBSOLETE     SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]), opval);
// OBSOLETE     written |= (1 << 10);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE if (ANDSI (FLD (f_reglist_hi_st), 64)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     written |= (1 << 9);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = CPU (h_gr[((UINT) 9)]);
// OBSOLETE     SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]), opval);
// OBSOLETE     written |= (1 << 10);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE if (ANDSI (FLD (f_reglist_hi_st), 128)) {
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     written |= (1 << 9);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = CPU (h_gr[((UINT) 8)]);
// OBSOLETE     SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]), opval);
// OBSOLETE     written |= (1 << 10);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   abuf->written = written;
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* enter: enter $u10 */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,enter) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_enter.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   SI tmp_tmp;
// OBSOLETE   tmp_tmp = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
// OBSOLETE   {
// OBSOLETE     SI opval = CPU (h_gr[((UINT) 14)]);
// OBSOLETE     SETMEMSI (current_cpu, pc, tmp_tmp, opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = tmp_tmp;
// OBSOLETE     CPU (h_gr[((UINT) 14)]) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), FLD (f_u10));
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* leave: leave */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,leave) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_enter.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (CPU (h_gr[((UINT) 14)]), 4);
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMSI (current_cpu, pc, SUBSI (CPU (h_gr[((UINT) 15)]), 4));
// OBSOLETE     CPU (h_gr[((UINT) 14)]) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* xchb: xchb @$Rj,$Ri */
// OBSOLETE 
// OBSOLETE static SEM_PC
// OBSOLETE SEM_FN_NAME (fr30bf,xchb) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
// OBSOLETE {
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE   SI tmp_tmp;
// OBSOLETE   tmp_tmp = * FLD (i_Ri);
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMUQI (current_cpu, pc, * FLD (i_Rj));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE   {
// OBSOLETE     UQI opval = tmp_tmp;
// OBSOLETE     SETMEMUQI (current_cpu, pc, * FLD (i_Rj), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   return vpc;
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Table of all semantic fns.  */
// OBSOLETE 
// OBSOLETE static const struct sem_fn_desc sem_fns[] = {
// OBSOLETE   { FR30BF_INSN_X_INVALID, SEM_FN_NAME (fr30bf,x_invalid) },
// OBSOLETE   { FR30BF_INSN_X_AFTER, SEM_FN_NAME (fr30bf,x_after) },
// OBSOLETE   { FR30BF_INSN_X_BEFORE, SEM_FN_NAME (fr30bf,x_before) },
// OBSOLETE   { FR30BF_INSN_X_CTI_CHAIN, SEM_FN_NAME (fr30bf,x_cti_chain) },
// OBSOLETE   { FR30BF_INSN_X_CHAIN, SEM_FN_NAME (fr30bf,x_chain) },
// OBSOLETE   { FR30BF_INSN_X_BEGIN, SEM_FN_NAME (fr30bf,x_begin) },
// OBSOLETE   { FR30BF_INSN_ADD, SEM_FN_NAME (fr30bf,add) },
// OBSOLETE   { FR30BF_INSN_ADDI, SEM_FN_NAME (fr30bf,addi) },
// OBSOLETE   { FR30BF_INSN_ADD2, SEM_FN_NAME (fr30bf,add2) },
// OBSOLETE   { FR30BF_INSN_ADDC, SEM_FN_NAME (fr30bf,addc) },
// OBSOLETE   { FR30BF_INSN_ADDN, SEM_FN_NAME (fr30bf,addn) },
// OBSOLETE   { FR30BF_INSN_ADDNI, SEM_FN_NAME (fr30bf,addni) },
// OBSOLETE   { FR30BF_INSN_ADDN2, SEM_FN_NAME (fr30bf,addn2) },
// OBSOLETE   { FR30BF_INSN_SUB, SEM_FN_NAME (fr30bf,sub) },
// OBSOLETE   { FR30BF_INSN_SUBC, SEM_FN_NAME (fr30bf,subc) },
// OBSOLETE   { FR30BF_INSN_SUBN, SEM_FN_NAME (fr30bf,subn) },
// OBSOLETE   { FR30BF_INSN_CMP, SEM_FN_NAME (fr30bf,cmp) },
// OBSOLETE   { FR30BF_INSN_CMPI, SEM_FN_NAME (fr30bf,cmpi) },
// OBSOLETE   { FR30BF_INSN_CMP2, SEM_FN_NAME (fr30bf,cmp2) },
// OBSOLETE   { FR30BF_INSN_AND, SEM_FN_NAME (fr30bf,and) },
// OBSOLETE   { FR30BF_INSN_OR, SEM_FN_NAME (fr30bf,or) },
// OBSOLETE   { FR30BF_INSN_EOR, SEM_FN_NAME (fr30bf,eor) },
// OBSOLETE   { FR30BF_INSN_ANDM, SEM_FN_NAME (fr30bf,andm) },
// OBSOLETE   { FR30BF_INSN_ANDH, SEM_FN_NAME (fr30bf,andh) },
// OBSOLETE   { FR30BF_INSN_ANDB, SEM_FN_NAME (fr30bf,andb) },
// OBSOLETE   { FR30BF_INSN_ORM, SEM_FN_NAME (fr30bf,orm) },
// OBSOLETE   { FR30BF_INSN_ORH, SEM_FN_NAME (fr30bf,orh) },
// OBSOLETE   { FR30BF_INSN_ORB, SEM_FN_NAME (fr30bf,orb) },
// OBSOLETE   { FR30BF_INSN_EORM, SEM_FN_NAME (fr30bf,eorm) },
// OBSOLETE   { FR30BF_INSN_EORH, SEM_FN_NAME (fr30bf,eorh) },
// OBSOLETE   { FR30BF_INSN_EORB, SEM_FN_NAME (fr30bf,eorb) },
// OBSOLETE   { FR30BF_INSN_BANDL, SEM_FN_NAME (fr30bf,bandl) },
// OBSOLETE   { FR30BF_INSN_BORL, SEM_FN_NAME (fr30bf,borl) },
// OBSOLETE   { FR30BF_INSN_BEORL, SEM_FN_NAME (fr30bf,beorl) },
// OBSOLETE   { FR30BF_INSN_BANDH, SEM_FN_NAME (fr30bf,bandh) },
// OBSOLETE   { FR30BF_INSN_BORH, SEM_FN_NAME (fr30bf,borh) },
// OBSOLETE   { FR30BF_INSN_BEORH, SEM_FN_NAME (fr30bf,beorh) },
// OBSOLETE   { FR30BF_INSN_BTSTL, SEM_FN_NAME (fr30bf,btstl) },
// OBSOLETE   { FR30BF_INSN_BTSTH, SEM_FN_NAME (fr30bf,btsth) },
// OBSOLETE   { FR30BF_INSN_MUL, SEM_FN_NAME (fr30bf,mul) },
// OBSOLETE   { FR30BF_INSN_MULU, SEM_FN_NAME (fr30bf,mulu) },
// OBSOLETE   { FR30BF_INSN_MULH, SEM_FN_NAME (fr30bf,mulh) },
// OBSOLETE   { FR30BF_INSN_MULUH, SEM_FN_NAME (fr30bf,muluh) },
// OBSOLETE   { FR30BF_INSN_DIV0S, SEM_FN_NAME (fr30bf,div0s) },
// OBSOLETE   { FR30BF_INSN_DIV0U, SEM_FN_NAME (fr30bf,div0u) },
// OBSOLETE   { FR30BF_INSN_DIV1, SEM_FN_NAME (fr30bf,div1) },
// OBSOLETE   { FR30BF_INSN_DIV2, SEM_FN_NAME (fr30bf,div2) },
// OBSOLETE   { FR30BF_INSN_DIV3, SEM_FN_NAME (fr30bf,div3) },
// OBSOLETE   { FR30BF_INSN_DIV4S, SEM_FN_NAME (fr30bf,div4s) },
// OBSOLETE   { FR30BF_INSN_LSL, SEM_FN_NAME (fr30bf,lsl) },
// OBSOLETE   { FR30BF_INSN_LSLI, SEM_FN_NAME (fr30bf,lsli) },
// OBSOLETE   { FR30BF_INSN_LSL2, SEM_FN_NAME (fr30bf,lsl2) },
// OBSOLETE   { FR30BF_INSN_LSR, SEM_FN_NAME (fr30bf,lsr) },
// OBSOLETE   { FR30BF_INSN_LSRI, SEM_FN_NAME (fr30bf,lsri) },
// OBSOLETE   { FR30BF_INSN_LSR2, SEM_FN_NAME (fr30bf,lsr2) },
// OBSOLETE   { FR30BF_INSN_ASR, SEM_FN_NAME (fr30bf,asr) },
// OBSOLETE   { FR30BF_INSN_ASRI, SEM_FN_NAME (fr30bf,asri) },
// OBSOLETE   { FR30BF_INSN_ASR2, SEM_FN_NAME (fr30bf,asr2) },
// OBSOLETE   { FR30BF_INSN_LDI8, SEM_FN_NAME (fr30bf,ldi8) },
// OBSOLETE   { FR30BF_INSN_LDI20, SEM_FN_NAME (fr30bf,ldi20) },
// OBSOLETE   { FR30BF_INSN_LDI32, SEM_FN_NAME (fr30bf,ldi32) },
// OBSOLETE   { FR30BF_INSN_LD, SEM_FN_NAME (fr30bf,ld) },
// OBSOLETE   { FR30BF_INSN_LDUH, SEM_FN_NAME (fr30bf,lduh) },
// OBSOLETE   { FR30BF_INSN_LDUB, SEM_FN_NAME (fr30bf,ldub) },
// OBSOLETE   { FR30BF_INSN_LDR13, SEM_FN_NAME (fr30bf,ldr13) },
// OBSOLETE   { FR30BF_INSN_LDR13UH, SEM_FN_NAME (fr30bf,ldr13uh) },
// OBSOLETE   { FR30BF_INSN_LDR13UB, SEM_FN_NAME (fr30bf,ldr13ub) },
// OBSOLETE   { FR30BF_INSN_LDR14, SEM_FN_NAME (fr30bf,ldr14) },
// OBSOLETE   { FR30BF_INSN_LDR14UH, SEM_FN_NAME (fr30bf,ldr14uh) },
// OBSOLETE   { FR30BF_INSN_LDR14UB, SEM_FN_NAME (fr30bf,ldr14ub) },
// OBSOLETE   { FR30BF_INSN_LDR15, SEM_FN_NAME (fr30bf,ldr15) },
// OBSOLETE   { FR30BF_INSN_LDR15GR, SEM_FN_NAME (fr30bf,ldr15gr) },
// OBSOLETE   { FR30BF_INSN_LDR15DR, SEM_FN_NAME (fr30bf,ldr15dr) },
// OBSOLETE   { FR30BF_INSN_LDR15PS, SEM_FN_NAME (fr30bf,ldr15ps) },
// OBSOLETE   { FR30BF_INSN_ST, SEM_FN_NAME (fr30bf,st) },
// OBSOLETE   { FR30BF_INSN_STH, SEM_FN_NAME (fr30bf,sth) },
// OBSOLETE   { FR30BF_INSN_STB, SEM_FN_NAME (fr30bf,stb) },
// OBSOLETE   { FR30BF_INSN_STR13, SEM_FN_NAME (fr30bf,str13) },
// OBSOLETE   { FR30BF_INSN_STR13H, SEM_FN_NAME (fr30bf,str13h) },
// OBSOLETE   { FR30BF_INSN_STR13B, SEM_FN_NAME (fr30bf,str13b) },
// OBSOLETE   { FR30BF_INSN_STR14, SEM_FN_NAME (fr30bf,str14) },
// OBSOLETE   { FR30BF_INSN_STR14H, SEM_FN_NAME (fr30bf,str14h) },
// OBSOLETE   { FR30BF_INSN_STR14B, SEM_FN_NAME (fr30bf,str14b) },
// OBSOLETE   { FR30BF_INSN_STR15, SEM_FN_NAME (fr30bf,str15) },
// OBSOLETE   { FR30BF_INSN_STR15GR, SEM_FN_NAME (fr30bf,str15gr) },
// OBSOLETE   { FR30BF_INSN_STR15DR, SEM_FN_NAME (fr30bf,str15dr) },
// OBSOLETE   { FR30BF_INSN_STR15PS, SEM_FN_NAME (fr30bf,str15ps) },
// OBSOLETE   { FR30BF_INSN_MOV, SEM_FN_NAME (fr30bf,mov) },
// OBSOLETE   { FR30BF_INSN_MOVDR, SEM_FN_NAME (fr30bf,movdr) },
// OBSOLETE   { FR30BF_INSN_MOVPS, SEM_FN_NAME (fr30bf,movps) },
// OBSOLETE   { FR30BF_INSN_MOV2DR, SEM_FN_NAME (fr30bf,mov2dr) },
// OBSOLETE   { FR30BF_INSN_MOV2PS, SEM_FN_NAME (fr30bf,mov2ps) },
// OBSOLETE   { FR30BF_INSN_JMP, SEM_FN_NAME (fr30bf,jmp) },
// OBSOLETE   { FR30BF_INSN_JMPD, SEM_FN_NAME (fr30bf,jmpd) },
// OBSOLETE   { FR30BF_INSN_CALLR, SEM_FN_NAME (fr30bf,callr) },
// OBSOLETE   { FR30BF_INSN_CALLRD, SEM_FN_NAME (fr30bf,callrd) },
// OBSOLETE   { FR30BF_INSN_CALL, SEM_FN_NAME (fr30bf,call) },
// OBSOLETE   { FR30BF_INSN_CALLD, SEM_FN_NAME (fr30bf,calld) },
// OBSOLETE   { FR30BF_INSN_RET, SEM_FN_NAME (fr30bf,ret) },
// OBSOLETE   { FR30BF_INSN_RET_D, SEM_FN_NAME (fr30bf,ret_d) },
// OBSOLETE   { FR30BF_INSN_INT, SEM_FN_NAME (fr30bf,int) },
// OBSOLETE   { FR30BF_INSN_INTE, SEM_FN_NAME (fr30bf,inte) },
// OBSOLETE   { FR30BF_INSN_RETI, SEM_FN_NAME (fr30bf,reti) },
// OBSOLETE   { FR30BF_INSN_BRAD, SEM_FN_NAME (fr30bf,brad) },
// OBSOLETE   { FR30BF_INSN_BRA, SEM_FN_NAME (fr30bf,bra) },
// OBSOLETE   { FR30BF_INSN_BNOD, SEM_FN_NAME (fr30bf,bnod) },
// OBSOLETE   { FR30BF_INSN_BNO, SEM_FN_NAME (fr30bf,bno) },
// OBSOLETE   { FR30BF_INSN_BEQD, SEM_FN_NAME (fr30bf,beqd) },
// OBSOLETE   { FR30BF_INSN_BEQ, SEM_FN_NAME (fr30bf,beq) },
// OBSOLETE   { FR30BF_INSN_BNED, SEM_FN_NAME (fr30bf,bned) },
// OBSOLETE   { FR30BF_INSN_BNE, SEM_FN_NAME (fr30bf,bne) },
// OBSOLETE   { FR30BF_INSN_BCD, SEM_FN_NAME (fr30bf,bcd) },
// OBSOLETE   { FR30BF_INSN_BC, SEM_FN_NAME (fr30bf,bc) },
// OBSOLETE   { FR30BF_INSN_BNCD, SEM_FN_NAME (fr30bf,bncd) },
// OBSOLETE   { FR30BF_INSN_BNC, SEM_FN_NAME (fr30bf,bnc) },
// OBSOLETE   { FR30BF_INSN_BND, SEM_FN_NAME (fr30bf,bnd) },
// OBSOLETE   { FR30BF_INSN_BN, SEM_FN_NAME (fr30bf,bn) },
// OBSOLETE   { FR30BF_INSN_BPD, SEM_FN_NAME (fr30bf,bpd) },
// OBSOLETE   { FR30BF_INSN_BP, SEM_FN_NAME (fr30bf,bp) },
// OBSOLETE   { FR30BF_INSN_BVD, SEM_FN_NAME (fr30bf,bvd) },
// OBSOLETE   { FR30BF_INSN_BV, SEM_FN_NAME (fr30bf,bv) },
// OBSOLETE   { FR30BF_INSN_BNVD, SEM_FN_NAME (fr30bf,bnvd) },
// OBSOLETE   { FR30BF_INSN_BNV, SEM_FN_NAME (fr30bf,bnv) },
// OBSOLETE   { FR30BF_INSN_BLTD, SEM_FN_NAME (fr30bf,bltd) },
// OBSOLETE   { FR30BF_INSN_BLT, SEM_FN_NAME (fr30bf,blt) },
// OBSOLETE   { FR30BF_INSN_BGED, SEM_FN_NAME (fr30bf,bged) },
// OBSOLETE   { FR30BF_INSN_BGE, SEM_FN_NAME (fr30bf,bge) },
// OBSOLETE   { FR30BF_INSN_BLED, SEM_FN_NAME (fr30bf,bled) },
// OBSOLETE   { FR30BF_INSN_BLE, SEM_FN_NAME (fr30bf,ble) },
// OBSOLETE   { FR30BF_INSN_BGTD, SEM_FN_NAME (fr30bf,bgtd) },
// OBSOLETE   { FR30BF_INSN_BGT, SEM_FN_NAME (fr30bf,bgt) },
// OBSOLETE   { FR30BF_INSN_BLSD, SEM_FN_NAME (fr30bf,blsd) },
// OBSOLETE   { FR30BF_INSN_BLS, SEM_FN_NAME (fr30bf,bls) },
// OBSOLETE   { FR30BF_INSN_BHID, SEM_FN_NAME (fr30bf,bhid) },
// OBSOLETE   { FR30BF_INSN_BHI, SEM_FN_NAME (fr30bf,bhi) },
// OBSOLETE   { FR30BF_INSN_DMOVR13, SEM_FN_NAME (fr30bf,dmovr13) },
// OBSOLETE   { FR30BF_INSN_DMOVR13H, SEM_FN_NAME (fr30bf,dmovr13h) },
// OBSOLETE   { FR30BF_INSN_DMOVR13B, SEM_FN_NAME (fr30bf,dmovr13b) },
// OBSOLETE   { FR30BF_INSN_DMOVR13PI, SEM_FN_NAME (fr30bf,dmovr13pi) },
// OBSOLETE   { FR30BF_INSN_DMOVR13PIH, SEM_FN_NAME (fr30bf,dmovr13pih) },
// OBSOLETE   { FR30BF_INSN_DMOVR13PIB, SEM_FN_NAME (fr30bf,dmovr13pib) },
// OBSOLETE   { FR30BF_INSN_DMOVR15PI, SEM_FN_NAME (fr30bf,dmovr15pi) },
// OBSOLETE   { FR30BF_INSN_DMOV2R13, SEM_FN_NAME (fr30bf,dmov2r13) },
// OBSOLETE   { FR30BF_INSN_DMOV2R13H, SEM_FN_NAME (fr30bf,dmov2r13h) },
// OBSOLETE   { FR30BF_INSN_DMOV2R13B, SEM_FN_NAME (fr30bf,dmov2r13b) },
// OBSOLETE   { FR30BF_INSN_DMOV2R13PI, SEM_FN_NAME (fr30bf,dmov2r13pi) },
// OBSOLETE   { FR30BF_INSN_DMOV2R13PIH, SEM_FN_NAME (fr30bf,dmov2r13pih) },
// OBSOLETE   { FR30BF_INSN_DMOV2R13PIB, SEM_FN_NAME (fr30bf,dmov2r13pib) },
// OBSOLETE   { FR30BF_INSN_DMOV2R15PD, SEM_FN_NAME (fr30bf,dmov2r15pd) },
// OBSOLETE   { FR30BF_INSN_LDRES, SEM_FN_NAME (fr30bf,ldres) },
// OBSOLETE   { FR30BF_INSN_STRES, SEM_FN_NAME (fr30bf,stres) },
// OBSOLETE   { FR30BF_INSN_COPOP, SEM_FN_NAME (fr30bf,copop) },
// OBSOLETE   { FR30BF_INSN_COPLD, SEM_FN_NAME (fr30bf,copld) },
// OBSOLETE   { FR30BF_INSN_COPST, SEM_FN_NAME (fr30bf,copst) },
// OBSOLETE   { FR30BF_INSN_COPSV, SEM_FN_NAME (fr30bf,copsv) },
// OBSOLETE   { FR30BF_INSN_NOP, SEM_FN_NAME (fr30bf,nop) },
// OBSOLETE   { FR30BF_INSN_ANDCCR, SEM_FN_NAME (fr30bf,andccr) },
// OBSOLETE   { FR30BF_INSN_ORCCR, SEM_FN_NAME (fr30bf,orccr) },
// OBSOLETE   { FR30BF_INSN_STILM, SEM_FN_NAME (fr30bf,stilm) },
// OBSOLETE   { FR30BF_INSN_ADDSP, SEM_FN_NAME (fr30bf,addsp) },
// OBSOLETE   { FR30BF_INSN_EXTSB, SEM_FN_NAME (fr30bf,extsb) },
// OBSOLETE   { FR30BF_INSN_EXTUB, SEM_FN_NAME (fr30bf,extub) },
// OBSOLETE   { FR30BF_INSN_EXTSH, SEM_FN_NAME (fr30bf,extsh) },
// OBSOLETE   { FR30BF_INSN_EXTUH, SEM_FN_NAME (fr30bf,extuh) },
// OBSOLETE   { FR30BF_INSN_LDM0, SEM_FN_NAME (fr30bf,ldm0) },
// OBSOLETE   { FR30BF_INSN_LDM1, SEM_FN_NAME (fr30bf,ldm1) },
// OBSOLETE   { FR30BF_INSN_STM0, SEM_FN_NAME (fr30bf,stm0) },
// OBSOLETE   { FR30BF_INSN_STM1, SEM_FN_NAME (fr30bf,stm1) },
// OBSOLETE   { FR30BF_INSN_ENTER, SEM_FN_NAME (fr30bf,enter) },
// OBSOLETE   { FR30BF_INSN_LEAVE, SEM_FN_NAME (fr30bf,leave) },
// OBSOLETE   { FR30BF_INSN_XCHB, SEM_FN_NAME (fr30bf,xchb) },
// OBSOLETE   { 0, 0 }
// OBSOLETE };
// OBSOLETE 
// OBSOLETE /* Add the semantic fns to IDESC_TABLE.  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE SEM_FN_NAME (fr30bf,init_idesc_table) (SIM_CPU *current_cpu)
// OBSOLETE {
// OBSOLETE   IDESC *idesc_table = CPU_IDESC (current_cpu);
// OBSOLETE   const struct sem_fn_desc *sf;
// OBSOLETE   int mach_num = MACH_NUM (CPU_MACH (current_cpu));
// OBSOLETE 
// OBSOLETE   for (sf = &sem_fns[0]; sf->fn != 0; ++sf)
// OBSOLETE     {
// OBSOLETE       const CGEN_INSN *insn = idesc_table[sf->index].idata;
// OBSOLETE       int valid_p = (CGEN_INSN_VIRTUAL_P (insn)
// OBSOLETE 		     || CGEN_INSN_MACH_HAS_P (insn, mach_num));
// OBSOLETE #if FAST_P
// OBSOLETE       if (valid_p)
// OBSOLETE 	idesc_table[sf->index].sem_fast = sf->fn;
// OBSOLETE       else
// OBSOLETE 	idesc_table[sf->index].sem_fast = SEM_FN_NAME (fr30bf,x_invalid);
// OBSOLETE #else
// OBSOLETE       if (valid_p)
// OBSOLETE 	idesc_table[sf->index].sem_full = sf->fn;
// OBSOLETE       else
// OBSOLETE 	idesc_table[sf->index].sem_full = SEM_FN_NAME (fr30bf,x_invalid);
// OBSOLETE #endif
// OBSOLETE     }
// OBSOLETE }
