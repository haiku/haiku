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
// OBSOLETE #ifdef DEFINE_LABELS
// OBSOLETE 
// OBSOLETE   /* The labels have the case they have because the enum of insn types
// OBSOLETE      is all uppercase and in the non-stdc case the insn symbol is built
// OBSOLETE      into the enum name.  */
// OBSOLETE 
// OBSOLETE   static struct {
// OBSOLETE     int index;
// OBSOLETE     void *label;
// OBSOLETE   } labels[] = {
// OBSOLETE     { FR30BF_INSN_X_INVALID, && case_sem_INSN_X_INVALID },
// OBSOLETE     { FR30BF_INSN_X_AFTER, && case_sem_INSN_X_AFTER },
// OBSOLETE     { FR30BF_INSN_X_BEFORE, && case_sem_INSN_X_BEFORE },
// OBSOLETE     { FR30BF_INSN_X_CTI_CHAIN, && case_sem_INSN_X_CTI_CHAIN },
// OBSOLETE     { FR30BF_INSN_X_CHAIN, && case_sem_INSN_X_CHAIN },
// OBSOLETE     { FR30BF_INSN_X_BEGIN, && case_sem_INSN_X_BEGIN },
// OBSOLETE     { FR30BF_INSN_ADD, && case_sem_INSN_ADD },
// OBSOLETE     { FR30BF_INSN_ADDI, && case_sem_INSN_ADDI },
// OBSOLETE     { FR30BF_INSN_ADD2, && case_sem_INSN_ADD2 },
// OBSOLETE     { FR30BF_INSN_ADDC, && case_sem_INSN_ADDC },
// OBSOLETE     { FR30BF_INSN_ADDN, && case_sem_INSN_ADDN },
// OBSOLETE     { FR30BF_INSN_ADDNI, && case_sem_INSN_ADDNI },
// OBSOLETE     { FR30BF_INSN_ADDN2, && case_sem_INSN_ADDN2 },
// OBSOLETE     { FR30BF_INSN_SUB, && case_sem_INSN_SUB },
// OBSOLETE     { FR30BF_INSN_SUBC, && case_sem_INSN_SUBC },
// OBSOLETE     { FR30BF_INSN_SUBN, && case_sem_INSN_SUBN },
// OBSOLETE     { FR30BF_INSN_CMP, && case_sem_INSN_CMP },
// OBSOLETE     { FR30BF_INSN_CMPI, && case_sem_INSN_CMPI },
// OBSOLETE     { FR30BF_INSN_CMP2, && case_sem_INSN_CMP2 },
// OBSOLETE     { FR30BF_INSN_AND, && case_sem_INSN_AND },
// OBSOLETE     { FR30BF_INSN_OR, && case_sem_INSN_OR },
// OBSOLETE     { FR30BF_INSN_EOR, && case_sem_INSN_EOR },
// OBSOLETE     { FR30BF_INSN_ANDM, && case_sem_INSN_ANDM },
// OBSOLETE     { FR30BF_INSN_ANDH, && case_sem_INSN_ANDH },
// OBSOLETE     { FR30BF_INSN_ANDB, && case_sem_INSN_ANDB },
// OBSOLETE     { FR30BF_INSN_ORM, && case_sem_INSN_ORM },
// OBSOLETE     { FR30BF_INSN_ORH, && case_sem_INSN_ORH },
// OBSOLETE     { FR30BF_INSN_ORB, && case_sem_INSN_ORB },
// OBSOLETE     { FR30BF_INSN_EORM, && case_sem_INSN_EORM },
// OBSOLETE     { FR30BF_INSN_EORH, && case_sem_INSN_EORH },
// OBSOLETE     { FR30BF_INSN_EORB, && case_sem_INSN_EORB },
// OBSOLETE     { FR30BF_INSN_BANDL, && case_sem_INSN_BANDL },
// OBSOLETE     { FR30BF_INSN_BORL, && case_sem_INSN_BORL },
// OBSOLETE     { FR30BF_INSN_BEORL, && case_sem_INSN_BEORL },
// OBSOLETE     { FR30BF_INSN_BANDH, && case_sem_INSN_BANDH },
// OBSOLETE     { FR30BF_INSN_BORH, && case_sem_INSN_BORH },
// OBSOLETE     { FR30BF_INSN_BEORH, && case_sem_INSN_BEORH },
// OBSOLETE     { FR30BF_INSN_BTSTL, && case_sem_INSN_BTSTL },
// OBSOLETE     { FR30BF_INSN_BTSTH, && case_sem_INSN_BTSTH },
// OBSOLETE     { FR30BF_INSN_MUL, && case_sem_INSN_MUL },
// OBSOLETE     { FR30BF_INSN_MULU, && case_sem_INSN_MULU },
// OBSOLETE     { FR30BF_INSN_MULH, && case_sem_INSN_MULH },
// OBSOLETE     { FR30BF_INSN_MULUH, && case_sem_INSN_MULUH },
// OBSOLETE     { FR30BF_INSN_DIV0S, && case_sem_INSN_DIV0S },
// OBSOLETE     { FR30BF_INSN_DIV0U, && case_sem_INSN_DIV0U },
// OBSOLETE     { FR30BF_INSN_DIV1, && case_sem_INSN_DIV1 },
// OBSOLETE     { FR30BF_INSN_DIV2, && case_sem_INSN_DIV2 },
// OBSOLETE     { FR30BF_INSN_DIV3, && case_sem_INSN_DIV3 },
// OBSOLETE     { FR30BF_INSN_DIV4S, && case_sem_INSN_DIV4S },
// OBSOLETE     { FR30BF_INSN_LSL, && case_sem_INSN_LSL },
// OBSOLETE     { FR30BF_INSN_LSLI, && case_sem_INSN_LSLI },
// OBSOLETE     { FR30BF_INSN_LSL2, && case_sem_INSN_LSL2 },
// OBSOLETE     { FR30BF_INSN_LSR, && case_sem_INSN_LSR },
// OBSOLETE     { FR30BF_INSN_LSRI, && case_sem_INSN_LSRI },
// OBSOLETE     { FR30BF_INSN_LSR2, && case_sem_INSN_LSR2 },
// OBSOLETE     { FR30BF_INSN_ASR, && case_sem_INSN_ASR },
// OBSOLETE     { FR30BF_INSN_ASRI, && case_sem_INSN_ASRI },
// OBSOLETE     { FR30BF_INSN_ASR2, && case_sem_INSN_ASR2 },
// OBSOLETE     { FR30BF_INSN_LDI8, && case_sem_INSN_LDI8 },
// OBSOLETE     { FR30BF_INSN_LDI20, && case_sem_INSN_LDI20 },
// OBSOLETE     { FR30BF_INSN_LDI32, && case_sem_INSN_LDI32 },
// OBSOLETE     { FR30BF_INSN_LD, && case_sem_INSN_LD },
// OBSOLETE     { FR30BF_INSN_LDUH, && case_sem_INSN_LDUH },
// OBSOLETE     { FR30BF_INSN_LDUB, && case_sem_INSN_LDUB },
// OBSOLETE     { FR30BF_INSN_LDR13, && case_sem_INSN_LDR13 },
// OBSOLETE     { FR30BF_INSN_LDR13UH, && case_sem_INSN_LDR13UH },
// OBSOLETE     { FR30BF_INSN_LDR13UB, && case_sem_INSN_LDR13UB },
// OBSOLETE     { FR30BF_INSN_LDR14, && case_sem_INSN_LDR14 },
// OBSOLETE     { FR30BF_INSN_LDR14UH, && case_sem_INSN_LDR14UH },
// OBSOLETE     { FR30BF_INSN_LDR14UB, && case_sem_INSN_LDR14UB },
// OBSOLETE     { FR30BF_INSN_LDR15, && case_sem_INSN_LDR15 },
// OBSOLETE     { FR30BF_INSN_LDR15GR, && case_sem_INSN_LDR15GR },
// OBSOLETE     { FR30BF_INSN_LDR15DR, && case_sem_INSN_LDR15DR },
// OBSOLETE     { FR30BF_INSN_LDR15PS, && case_sem_INSN_LDR15PS },
// OBSOLETE     { FR30BF_INSN_ST, && case_sem_INSN_ST },
// OBSOLETE     { FR30BF_INSN_STH, && case_sem_INSN_STH },
// OBSOLETE     { FR30BF_INSN_STB, && case_sem_INSN_STB },
// OBSOLETE     { FR30BF_INSN_STR13, && case_sem_INSN_STR13 },
// OBSOLETE     { FR30BF_INSN_STR13H, && case_sem_INSN_STR13H },
// OBSOLETE     { FR30BF_INSN_STR13B, && case_sem_INSN_STR13B },
// OBSOLETE     { FR30BF_INSN_STR14, && case_sem_INSN_STR14 },
// OBSOLETE     { FR30BF_INSN_STR14H, && case_sem_INSN_STR14H },
// OBSOLETE     { FR30BF_INSN_STR14B, && case_sem_INSN_STR14B },
// OBSOLETE     { FR30BF_INSN_STR15, && case_sem_INSN_STR15 },
// OBSOLETE     { FR30BF_INSN_STR15GR, && case_sem_INSN_STR15GR },
// OBSOLETE     { FR30BF_INSN_STR15DR, && case_sem_INSN_STR15DR },
// OBSOLETE     { FR30BF_INSN_STR15PS, && case_sem_INSN_STR15PS },
// OBSOLETE     { FR30BF_INSN_MOV, && case_sem_INSN_MOV },
// OBSOLETE     { FR30BF_INSN_MOVDR, && case_sem_INSN_MOVDR },
// OBSOLETE     { FR30BF_INSN_MOVPS, && case_sem_INSN_MOVPS },
// OBSOLETE     { FR30BF_INSN_MOV2DR, && case_sem_INSN_MOV2DR },
// OBSOLETE     { FR30BF_INSN_MOV2PS, && case_sem_INSN_MOV2PS },
// OBSOLETE     { FR30BF_INSN_JMP, && case_sem_INSN_JMP },
// OBSOLETE     { FR30BF_INSN_JMPD, && case_sem_INSN_JMPD },
// OBSOLETE     { FR30BF_INSN_CALLR, && case_sem_INSN_CALLR },
// OBSOLETE     { FR30BF_INSN_CALLRD, && case_sem_INSN_CALLRD },
// OBSOLETE     { FR30BF_INSN_CALL, && case_sem_INSN_CALL },
// OBSOLETE     { FR30BF_INSN_CALLD, && case_sem_INSN_CALLD },
// OBSOLETE     { FR30BF_INSN_RET, && case_sem_INSN_RET },
// OBSOLETE     { FR30BF_INSN_RET_D, && case_sem_INSN_RET_D },
// OBSOLETE     { FR30BF_INSN_INT, && case_sem_INSN_INT },
// OBSOLETE     { FR30BF_INSN_INTE, && case_sem_INSN_INTE },
// OBSOLETE     { FR30BF_INSN_RETI, && case_sem_INSN_RETI },
// OBSOLETE     { FR30BF_INSN_BRAD, && case_sem_INSN_BRAD },
// OBSOLETE     { FR30BF_INSN_BRA, && case_sem_INSN_BRA },
// OBSOLETE     { FR30BF_INSN_BNOD, && case_sem_INSN_BNOD },
// OBSOLETE     { FR30BF_INSN_BNO, && case_sem_INSN_BNO },
// OBSOLETE     { FR30BF_INSN_BEQD, && case_sem_INSN_BEQD },
// OBSOLETE     { FR30BF_INSN_BEQ, && case_sem_INSN_BEQ },
// OBSOLETE     { FR30BF_INSN_BNED, && case_sem_INSN_BNED },
// OBSOLETE     { FR30BF_INSN_BNE, && case_sem_INSN_BNE },
// OBSOLETE     { FR30BF_INSN_BCD, && case_sem_INSN_BCD },
// OBSOLETE     { FR30BF_INSN_BC, && case_sem_INSN_BC },
// OBSOLETE     { FR30BF_INSN_BNCD, && case_sem_INSN_BNCD },
// OBSOLETE     { FR30BF_INSN_BNC, && case_sem_INSN_BNC },
// OBSOLETE     { FR30BF_INSN_BND, && case_sem_INSN_BND },
// OBSOLETE     { FR30BF_INSN_BN, && case_sem_INSN_BN },
// OBSOLETE     { FR30BF_INSN_BPD, && case_sem_INSN_BPD },
// OBSOLETE     { FR30BF_INSN_BP, && case_sem_INSN_BP },
// OBSOLETE     { FR30BF_INSN_BVD, && case_sem_INSN_BVD },
// OBSOLETE     { FR30BF_INSN_BV, && case_sem_INSN_BV },
// OBSOLETE     { FR30BF_INSN_BNVD, && case_sem_INSN_BNVD },
// OBSOLETE     { FR30BF_INSN_BNV, && case_sem_INSN_BNV },
// OBSOLETE     { FR30BF_INSN_BLTD, && case_sem_INSN_BLTD },
// OBSOLETE     { FR30BF_INSN_BLT, && case_sem_INSN_BLT },
// OBSOLETE     { FR30BF_INSN_BGED, && case_sem_INSN_BGED },
// OBSOLETE     { FR30BF_INSN_BGE, && case_sem_INSN_BGE },
// OBSOLETE     { FR30BF_INSN_BLED, && case_sem_INSN_BLED },
// OBSOLETE     { FR30BF_INSN_BLE, && case_sem_INSN_BLE },
// OBSOLETE     { FR30BF_INSN_BGTD, && case_sem_INSN_BGTD },
// OBSOLETE     { FR30BF_INSN_BGT, && case_sem_INSN_BGT },
// OBSOLETE     { FR30BF_INSN_BLSD, && case_sem_INSN_BLSD },
// OBSOLETE     { FR30BF_INSN_BLS, && case_sem_INSN_BLS },
// OBSOLETE     { FR30BF_INSN_BHID, && case_sem_INSN_BHID },
// OBSOLETE     { FR30BF_INSN_BHI, && case_sem_INSN_BHI },
// OBSOLETE     { FR30BF_INSN_DMOVR13, && case_sem_INSN_DMOVR13 },
// OBSOLETE     { FR30BF_INSN_DMOVR13H, && case_sem_INSN_DMOVR13H },
// OBSOLETE     { FR30BF_INSN_DMOVR13B, && case_sem_INSN_DMOVR13B },
// OBSOLETE     { FR30BF_INSN_DMOVR13PI, && case_sem_INSN_DMOVR13PI },
// OBSOLETE     { FR30BF_INSN_DMOVR13PIH, && case_sem_INSN_DMOVR13PIH },
// OBSOLETE     { FR30BF_INSN_DMOVR13PIB, && case_sem_INSN_DMOVR13PIB },
// OBSOLETE     { FR30BF_INSN_DMOVR15PI, && case_sem_INSN_DMOVR15PI },
// OBSOLETE     { FR30BF_INSN_DMOV2R13, && case_sem_INSN_DMOV2R13 },
// OBSOLETE     { FR30BF_INSN_DMOV2R13H, && case_sem_INSN_DMOV2R13H },
// OBSOLETE     { FR30BF_INSN_DMOV2R13B, && case_sem_INSN_DMOV2R13B },
// OBSOLETE     { FR30BF_INSN_DMOV2R13PI, && case_sem_INSN_DMOV2R13PI },
// OBSOLETE     { FR30BF_INSN_DMOV2R13PIH, && case_sem_INSN_DMOV2R13PIH },
// OBSOLETE     { FR30BF_INSN_DMOV2R13PIB, && case_sem_INSN_DMOV2R13PIB },
// OBSOLETE     { FR30BF_INSN_DMOV2R15PD, && case_sem_INSN_DMOV2R15PD },
// OBSOLETE     { FR30BF_INSN_LDRES, && case_sem_INSN_LDRES },
// OBSOLETE     { FR30BF_INSN_STRES, && case_sem_INSN_STRES },
// OBSOLETE     { FR30BF_INSN_COPOP, && case_sem_INSN_COPOP },
// OBSOLETE     { FR30BF_INSN_COPLD, && case_sem_INSN_COPLD },
// OBSOLETE     { FR30BF_INSN_COPST, && case_sem_INSN_COPST },
// OBSOLETE     { FR30BF_INSN_COPSV, && case_sem_INSN_COPSV },
// OBSOLETE     { FR30BF_INSN_NOP, && case_sem_INSN_NOP },
// OBSOLETE     { FR30BF_INSN_ANDCCR, && case_sem_INSN_ANDCCR },
// OBSOLETE     { FR30BF_INSN_ORCCR, && case_sem_INSN_ORCCR },
// OBSOLETE     { FR30BF_INSN_STILM, && case_sem_INSN_STILM },
// OBSOLETE     { FR30BF_INSN_ADDSP, && case_sem_INSN_ADDSP },
// OBSOLETE     { FR30BF_INSN_EXTSB, && case_sem_INSN_EXTSB },
// OBSOLETE     { FR30BF_INSN_EXTUB, && case_sem_INSN_EXTUB },
// OBSOLETE     { FR30BF_INSN_EXTSH, && case_sem_INSN_EXTSH },
// OBSOLETE     { FR30BF_INSN_EXTUH, && case_sem_INSN_EXTUH },
// OBSOLETE     { FR30BF_INSN_LDM0, && case_sem_INSN_LDM0 },
// OBSOLETE     { FR30BF_INSN_LDM1, && case_sem_INSN_LDM1 },
// OBSOLETE     { FR30BF_INSN_STM0, && case_sem_INSN_STM0 },
// OBSOLETE     { FR30BF_INSN_STM1, && case_sem_INSN_STM1 },
// OBSOLETE     { FR30BF_INSN_ENTER, && case_sem_INSN_ENTER },
// OBSOLETE     { FR30BF_INSN_LEAVE, && case_sem_INSN_LEAVE },
// OBSOLETE     { FR30BF_INSN_XCHB, && case_sem_INSN_XCHB },
// OBSOLETE     { 0, 0 }
// OBSOLETE   };
// OBSOLETE   int i;
// OBSOLETE 
// OBSOLETE   for (i = 0; labels[i].label != 0; ++i)
// OBSOLETE     {
// OBSOLETE #if FAST_P
// OBSOLETE       CPU_IDESC (current_cpu) [labels[i].index].sem_fast_lab = labels[i].label;
// OBSOLETE #else
// OBSOLETE       CPU_IDESC (current_cpu) [labels[i].index].sem_full_lab = labels[i].label;
// OBSOLETE #endif
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE #undef DEFINE_LABELS
// OBSOLETE #endif /* DEFINE_LABELS */
// OBSOLETE 
// OBSOLETE #ifdef DEFINE_SWITCH
// OBSOLETE 
// OBSOLETE /* If hyper-fast [well not unnecessarily slow] execution is selected, turn
// OBSOLETE    off frills like tracing and profiling.  */
// OBSOLETE /* FIXME: A better way would be to have TRACE_RESULT check for something
// OBSOLETE    that can cause it to be optimized out.  Another way would be to emit
// OBSOLETE    special handlers into the instruction "stream".  */
// OBSOLETE 
// OBSOLETE #if FAST_P
// OBSOLETE #undef TRACE_RESULT
// OBSOLETE #define TRACE_RESULT(cpu, abuf, name, type, val)
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE #undef GET_ATTR
// OBSOLETE #if defined (__STDC__) || defined (ALMOST_STDC) || defined (HAVE_STRINGIZE)
// OBSOLETE #define GET_ATTR(cpu, num, attr) CGEN_ATTR_VALUE (NULL, abuf->idesc->attrs, CGEN_INSN_##attr)
// OBSOLETE #else
// OBSOLETE #define GET_ATTR(cpu, num, attr) CGEN_ATTR_VALUE (NULL, abuf->idesc->attrs, CGEN_INSN_/**/attr)
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE {
// OBSOLETE 
// OBSOLETE #if WITH_SCACHE_PBB
// OBSOLETE 
// OBSOLETE /* Branch to next handler without going around main loop.  */
// OBSOLETE #define NEXT(vpc) goto * SEM_ARGBUF (vpc) -> semantic.sem_case
// OBSOLETE SWITCH (sem, SEM_ARGBUF (vpc) -> semantic.sem_case)
// OBSOLETE 
// OBSOLETE #else /* ! WITH_SCACHE_PBB */
// OBSOLETE 
// OBSOLETE #define NEXT(vpc) BREAK (sem)
// OBSOLETE #ifdef __GNUC__
// OBSOLETE #if FAST_P
// OBSOLETE   SWITCH (sem, SEM_ARGBUF (sc) -> idesc->sem_fast_lab)
// OBSOLETE #else
// OBSOLETE   SWITCH (sem, SEM_ARGBUF (sc) -> idesc->sem_full_lab)
// OBSOLETE #endif
// OBSOLETE #else
// OBSOLETE   SWITCH (sem, SEM_ARGBUF (sc) -> idesc->num)
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE #endif /* ! WITH_SCACHE_PBB */
// OBSOLETE 
// OBSOLETE     {
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_X_INVALID) : /* --invalid-- */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 0);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_X_AFTER) : /* --after-- */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 0);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE #if WITH_SCACHE_PBB_FR30BF
// OBSOLETE     fr30bf_pbb_after (current_cpu, sem_arg);
// OBSOLETE #endif
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_X_BEFORE) : /* --before-- */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 0);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE #if WITH_SCACHE_PBB_FR30BF
// OBSOLETE     fr30bf_pbb_before (current_cpu, sem_arg);
// OBSOLETE #endif
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_X_CTI_CHAIN) : /* --cti-chain-- */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 0);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_X_CHAIN) : /* --chain-- */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 0);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_X_BEGIN) : /* --begin-- */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 0);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_ADD) : /* add $Rj,$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_ADDI) : /* add $u4,$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_ADD2) : /* add2 $m4,$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add2.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_ADDC) : /* addc $Rj,$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_ADDN) : /* addn $Rj,$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (* FLD (i_Ri), * FLD (i_Rj));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_ADDNI) : /* addn $u4,$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (* FLD (i_Ri), FLD (f_u4));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_ADDN2) : /* addn2 $m4,$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add2.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (* FLD (i_Ri), FLD (f_m4));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_SUB) : /* sub $Rj,$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_SUBC) : /* subc $Rj,$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_SUBN) : /* subn $Rj,$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = SUBSI (* FLD (i_Ri), * FLD (i_Rj));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_CMP) : /* cmp $Rj,$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_CMPI) : /* cmp $u4,$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_CMP2) : /* cmp2 $m4,$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add2.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_AND) : /* and $Rj,$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_OR) : /* or $Rj,$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_EOR) : /* eor $Rj,$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_ANDM) : /* and $Rj,@$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_ANDH) : /* andh $Rj,@$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_ANDB) : /* andb $Rj,@$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_ORM) : /* or $Rj,@$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_ORH) : /* orh $Rj,@$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_ORB) : /* orb $Rj,@$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_EORM) : /* eor $Rj,@$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_EORH) : /* eorh $Rj,@$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_EORB) : /* eorb $Rj,@$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_BANDL) : /* bandl $u4,@$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     QI opval = ANDQI (ORQI (FLD (f_u4), 240), GETMEMQI (current_cpu, pc, * FLD (i_Ri)));
// OBSOLETE     SETMEMQI (current_cpu, pc, * FLD (i_Ri), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_BORL) : /* borl $u4,@$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     QI opval = ORQI (FLD (f_u4), GETMEMQI (current_cpu, pc, * FLD (i_Ri)));
// OBSOLETE     SETMEMQI (current_cpu, pc, * FLD (i_Ri), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_BEORL) : /* beorl $u4,@$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     QI opval = XORQI (FLD (f_u4), GETMEMQI (current_cpu, pc, * FLD (i_Ri)));
// OBSOLETE     SETMEMQI (current_cpu, pc, * FLD (i_Ri), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_BANDH) : /* bandh $u4,@$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     QI opval = ANDQI (ORQI (SLLQI (FLD (f_u4), 4), 15), GETMEMQI (current_cpu, pc, * FLD (i_Ri)));
// OBSOLETE     SETMEMQI (current_cpu, pc, * FLD (i_Ri), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_BORH) : /* borh $u4,@$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     QI opval = ORQI (SLLQI (FLD (f_u4), 4), GETMEMQI (current_cpu, pc, * FLD (i_Ri)));
// OBSOLETE     SETMEMQI (current_cpu, pc, * FLD (i_Ri), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_BEORH) : /* beorh $u4,@$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     QI opval = XORQI (SLLQI (FLD (f_u4), 4), GETMEMQI (current_cpu, pc, * FLD (i_Ri)));
// OBSOLETE     SETMEMQI (current_cpu, pc, * FLD (i_Ri), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_BTSTL) : /* btstl $u4,@$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_BTSTH) : /* btsth $u4,@$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_MUL) : /* mul $Rj,$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_MULU) : /* mulu $Rj,$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_MULH) : /* mulh $Rj,$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_MULUH) : /* muluh $Rj,$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_DIV0S) : /* div0s $Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_mov2dr.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_DIV0U) : /* div0u $Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_DIV1) : /* div1 $Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_mov2dr.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_DIV2) : /* div2 $Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_mov2dr.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_DIV3) : /* div3 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_DIV4S) : /* div4s */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_LSL) : /* lsl $Rj,$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_LSLI) : /* lsl $u4,$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_LSL2) : /* lsl2 $u4,$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_LSR) : /* lsr $Rj,$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_LSRI) : /* lsr $u4,$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_LSR2) : /* lsr2 $u4,$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_ASR) : /* asr $Rj,$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_ASRI) : /* asr $u4,$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_ASR2) : /* asr2 $u4,$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_LDI8) : /* ldi:8 $i8,$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldi8.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = FLD (f_i8);
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_LDI20) : /* ldi:20 $i20,$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldi20.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 4);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = FLD (f_i20);
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_LDI32) : /* ldi:32 $i32,$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldi32.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 6);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = FLD (f_i32);
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_LD) : /* ld @$Rj,$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr13.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMSI (current_cpu, pc, * FLD (i_Rj));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_LDUH) : /* lduh @$Rj,$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr13.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMUHI (current_cpu, pc, * FLD (i_Rj));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_LDUB) : /* ldub @$Rj,$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr13.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMUQI (current_cpu, pc, * FLD (i_Rj));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_LDR13) : /* ld @($R13,$Rj),$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr13.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMSI (current_cpu, pc, ADDSI (* FLD (i_Rj), CPU (h_gr[((UINT) 13)])));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_LDR13UH) : /* lduh @($R13,$Rj),$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr13.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMUHI (current_cpu, pc, ADDSI (* FLD (i_Rj), CPU (h_gr[((UINT) 13)])));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_LDR13UB) : /* ldub @($R13,$Rj),$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr13.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMUQI (current_cpu, pc, ADDSI (* FLD (i_Rj), CPU (h_gr[((UINT) 13)])));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_LDR14) : /* ld @($R14,$disp10),$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr14.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMSI (current_cpu, pc, ADDSI (FLD (f_disp10), CPU (h_gr[((UINT) 14)])));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_LDR14UH) : /* lduh @($R14,$disp9),$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr14uh.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMUHI (current_cpu, pc, ADDSI (FLD (f_disp9), CPU (h_gr[((UINT) 14)])));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_LDR14UB) : /* ldub @($R14,$disp8),$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr14ub.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMUQI (current_cpu, pc, ADDSI (FLD (f_disp8), CPU (h_gr[((UINT) 14)])));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_LDR15) : /* ld @($R15,$udisp6),$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr15.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMSI (current_cpu, pc, ADDSI (FLD (f_udisp6), CPU (h_gr[((UINT) 15)])));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_LDR15GR) : /* ld @$R15+,$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr15gr.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_LDR15DR) : /* ld @$R15+,$Rs2 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr15dr.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_LDR15PS) : /* ld @$R15+,$ps */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addsp.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_ST) : /* st $Ri,@$Rj */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = * FLD (i_Ri);
// OBSOLETE     SETMEMSI (current_cpu, pc, * FLD (i_Rj), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_STH) : /* sth $Ri,@$Rj */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     HI opval = * FLD (i_Ri);
// OBSOLETE     SETMEMHI (current_cpu, pc, * FLD (i_Rj), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_STB) : /* stb $Ri,@$Rj */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     QI opval = * FLD (i_Ri);
// OBSOLETE     SETMEMQI (current_cpu, pc, * FLD (i_Rj), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_STR13) : /* st $Ri,@($R13,$Rj) */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = * FLD (i_Ri);
// OBSOLETE     SETMEMSI (current_cpu, pc, ADDSI (* FLD (i_Rj), CPU (h_gr[((UINT) 13)])), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_STR13H) : /* sth $Ri,@($R13,$Rj) */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     HI opval = * FLD (i_Ri);
// OBSOLETE     SETMEMHI (current_cpu, pc, ADDSI (* FLD (i_Rj), CPU (h_gr[((UINT) 13)])), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_STR13B) : /* stb $Ri,@($R13,$Rj) */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     QI opval = * FLD (i_Ri);
// OBSOLETE     SETMEMQI (current_cpu, pc, ADDSI (* FLD (i_Rj), CPU (h_gr[((UINT) 13)])), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_STR14) : /* st $Ri,@($R14,$disp10) */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str14.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = * FLD (i_Ri);
// OBSOLETE     SETMEMSI (current_cpu, pc, ADDSI (FLD (f_disp10), CPU (h_gr[((UINT) 14)])), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_STR14H) : /* sth $Ri,@($R14,$disp9) */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str14h.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     HI opval = * FLD (i_Ri);
// OBSOLETE     SETMEMHI (current_cpu, pc, ADDSI (FLD (f_disp9), CPU (h_gr[((UINT) 14)])), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_STR14B) : /* stb $Ri,@($R14,$disp8) */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str14b.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     QI opval = * FLD (i_Ri);
// OBSOLETE     SETMEMQI (current_cpu, pc, ADDSI (FLD (f_disp8), CPU (h_gr[((UINT) 14)])), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_STR15) : /* st $Ri,@($R15,$udisp6) */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str15.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = * FLD (i_Ri);
// OBSOLETE     SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 15)]), FLD (f_udisp6)), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_STR15GR) : /* st $Ri,@-$R15 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str15gr.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_STR15DR) : /* st $Rs2,@-$R15 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr15dr.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_STR15PS) : /* st $ps,@-$R15 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addsp.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_MOV) : /* mov $Rj,$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr13.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = * FLD (i_Rj);
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_MOVDR) : /* mov $Rs1,$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_movdr.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = GET_H_DR (FLD (f_Rs1));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_MOVPS) : /* mov $ps,$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_movdr.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = GET_H_PS ();
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_MOV2DR) : /* mov $Ri,$Rs1 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_mov2dr.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = * FLD (i_Ri);
// OBSOLETE     SET_H_DR (FLD (f_Rs1), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "dr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_MOV2PS) : /* mov $Ri,$ps */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_mov2dr.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     USI opval = * FLD (i_Ri);
// OBSOLETE     SET_H_PS (opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "ps", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_JMP) : /* jmp @$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_mov2dr.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     USI opval = * FLD (i_Ri);
// OBSOLETE     SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_JMPD) : /* jmp:d @$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_mov2dr.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_CALLR) : /* call @$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_mov2dr.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_CALLRD) : /* call:d @$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_mov2dr.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_CALL) : /* call $label12 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_call.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_CALLD) : /* call:d $label12 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_call.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_RET) : /* ret */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     USI opval = GET_H_DR (((UINT) 1));
// OBSOLETE     SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_RET_D) : /* ret:d */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_INT) : /* int $u8 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_int.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_INTE) : /* inte */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_RETI) : /* reti */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_BRAD) : /* bra:d $label9 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_BRA) : /* bra $label9 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     USI opval = FLD (i_label9);
// OBSOLETE     SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   SEM_BRANCH_FINI (vpc);
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_BNOD) : /* bno:d $label9 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE {
// OBSOLETE ((void) 0); /*nop*/
// OBSOLETE }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_BNO) : /* bno $label9 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE ((void) 0); /*nop*/
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_BEQD) : /* beq:d $label9 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_BEQ) : /* beq $label9 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_BNED) : /* bne:d $label9 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_BNE) : /* bne $label9 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_BCD) : /* bc:d $label9 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_BC) : /* bc $label9 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_BNCD) : /* bnc:d $label9 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_BNC) : /* bnc $label9 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_BND) : /* bn:d $label9 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_BN) : /* bn $label9 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_BPD) : /* bp:d $label9 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_BP) : /* bp $label9 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_BVD) : /* bv:d $label9 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_BV) : /* bv $label9 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_BNVD) : /* bnv:d $label9 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_BNV) : /* bnv $label9 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_BLTD) : /* blt:d $label9 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_BLT) : /* blt $label9 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_BGED) : /* bge:d $label9 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_BGE) : /* bge $label9 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_BLED) : /* ble:d $label9 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_BLE) : /* ble $label9 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_BGTD) : /* bgt:d $label9 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_BGT) : /* bgt $label9 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_BLSD) : /* bls:d $label9 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_BLS) : /* bls $label9 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_BHID) : /* bhi:d $label9 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_BHI) : /* bhi $label9 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   SEM_BRANCH_INIT
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_DMOVR13) : /* dmov $R13,@$dir10 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pi.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = CPU (h_gr[((UINT) 13)]);
// OBSOLETE     SETMEMSI (current_cpu, pc, FLD (f_dir10), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_DMOVR13H) : /* dmovh $R13,@$dir9 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pih.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     HI opval = CPU (h_gr[((UINT) 13)]);
// OBSOLETE     SETMEMHI (current_cpu, pc, FLD (f_dir9), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_DMOVR13B) : /* dmovb $R13,@$dir8 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pib.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     QI opval = CPU (h_gr[((UINT) 13)]);
// OBSOLETE     SETMEMQI (current_cpu, pc, FLD (f_dir8), opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_DMOVR13PI) : /* dmov @$R13+,@$dir10 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pi.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_DMOVR13PIH) : /* dmovh @$R13+,@$dir9 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pih.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_DMOVR13PIB) : /* dmovb @$R13+,@$dir8 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pib.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_DMOVR15PI) : /* dmov @$R15+,@$dir10 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr15pi.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_DMOV2R13) : /* dmov @$dir10,$R13 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pi.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMSI (current_cpu, pc, FLD (f_dir10));
// OBSOLETE     CPU (h_gr[((UINT) 13)]) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_DMOV2R13H) : /* dmovh @$dir9,$R13 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pih.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMHI (current_cpu, pc, FLD (f_dir9));
// OBSOLETE     CPU (h_gr[((UINT) 13)]) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_DMOV2R13B) : /* dmovb @$dir8,$R13 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pib.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = GETMEMQI (current_cpu, pc, FLD (f_dir8));
// OBSOLETE     CPU (h_gr[((UINT) 13)]) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_DMOV2R13PI) : /* dmov @$dir10,@$R13+ */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pi.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_DMOV2R13PIH) : /* dmovh @$dir9,@$R13+ */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pih.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_DMOV2R13PIB) : /* dmovb @$dir8,@$R13+ */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pib.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_DMOV2R15PD) : /* dmov @$dir10,@-$R15 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr15pi.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_LDRES) : /* ldres @$Ri+,$u4 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add2.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (* FLD (i_Ri), 4);
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_STRES) : /* stres $u4,@$Ri+ */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add2.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (* FLD (i_Ri), 4);
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_COPOP) : /* copop $u4c,$ccc,$CRj,$CRi */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 4);
// OBSOLETE 
// OBSOLETE ((void) 0); /*nop*/
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_COPLD) : /* copld $u4c,$ccc,$Rjc,$CRi */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 4);
// OBSOLETE 
// OBSOLETE ((void) 0); /*nop*/
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_COPST) : /* copst $u4c,$ccc,$CRj,$Ric */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 4);
// OBSOLETE 
// OBSOLETE ((void) 0); /*nop*/
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_COPSV) : /* copsv $u4c,$ccc,$CRj,$Ric */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 4);
// OBSOLETE 
// OBSOLETE ((void) 0); /*nop*/
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_NOP) : /* nop */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE ((void) 0); /*nop*/
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_ANDCCR) : /* andccr $u8 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_int.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     UQI opval = ANDQI (GET_H_CCR (), FLD (f_u8));
// OBSOLETE     SET_H_CCR (opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "ccr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_ORCCR) : /* orccr $u8 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_int.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     UQI opval = ORQI (GET_H_CCR (), FLD (f_u8));
// OBSOLETE     SET_H_CCR (opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "ccr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_STILM) : /* stilm $u8 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_int.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     UQI opval = ANDSI (FLD (f_u8), 31);
// OBSOLETE     SET_H_ILM (opval);
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "ilm", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_ADDSP) : /* addsp $s10 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addsp.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), FLD (f_s10));
// OBSOLETE     CPU (h_gr[((UINT) 15)]) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_EXTSB) : /* extsb $Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add2.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = EXTQISI (ANDQI (* FLD (i_Ri), 255));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_EXTUB) : /* extub $Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add2.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = ZEXTQISI (ANDQI (* FLD (i_Ri), 255));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_EXTSH) : /* extsh $Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add2.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = EXTHISI (ANDHI (* FLD (i_Ri), 65535));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_EXTUH) : /* extuh $Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add2.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     SI opval = ZEXTHISI (ANDHI (* FLD (i_Ri), 65535));
// OBSOLETE     * FLD (i_Ri) = opval;
// OBSOLETE     TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_LDM0) : /* ldm0 ($reglist_low_ld) */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldm0.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_LDM1) : /* ldm1 ($reglist_hi_ld) */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldm1.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_STM0) : /* stm0 ($reglist_low_st) */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_stm0.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_STM1) : /* stm1 ($reglist_hi_st) */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_stm1.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_ENTER) : /* enter $u10 */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_enter.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_LEAVE) : /* leave */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_enter.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE   CASE (sem, INSN_XCHB) : /* xchb @$Rj,$Ri */
// OBSOLETE {
// OBSOLETE   SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
// OBSOLETE   ARGBUF *abuf = SEM_ARGBUF (sem_arg);
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE   int UNUSED written = 0;
// OBSOLETE   IADDR UNUSED pc = abuf->addr;
// OBSOLETE   vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
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
// OBSOLETE #undef FLD
// OBSOLETE }
// OBSOLETE   NEXT (vpc);
// OBSOLETE 
// OBSOLETE 
// OBSOLETE     }
// OBSOLETE   ENDSWITCH (sem) /* End of semantic switch.  */
// OBSOLETE 
// OBSOLETE   /* At this point `vpc' contains the next insn to execute.  */
// OBSOLETE }
// OBSOLETE 
// OBSOLETE #undef DEFINE_SWITCH
// OBSOLETE #endif /* DEFINE_SWITCH */
