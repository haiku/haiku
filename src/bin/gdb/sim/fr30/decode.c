// OBSOLETE /* Simulator instruction decoder for fr30bf.
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
// OBSOLETE #include "sim-assert.h"
// OBSOLETE 
// OBSOLETE /* The instruction descriptor array.
// OBSOLETE    This is computed at runtime.  Space for it is not malloc'd to save a
// OBSOLETE    teensy bit of cpu in the decoder.  Moving it to malloc space is trivial
// OBSOLETE    but won't be done until necessary (we don't currently support the runtime
// OBSOLETE    addition of instructions nor an SMP machine with different cpus).  */
// OBSOLETE static IDESC fr30bf_insn_data[FR30BF_INSN_XCHB + 1];
// OBSOLETE 
// OBSOLETE /* Commas between elements are contained in the macros.
// OBSOLETE    Some of these are conditionally compiled out.  */
// OBSOLETE 
// OBSOLETE static const struct insn_sem fr30bf_insn_sem[] =
// OBSOLETE {
// OBSOLETE   { VIRTUAL_INSN_X_INVALID, FR30BF_INSN_X_INVALID, FR30BF_SFMT_EMPTY },
// OBSOLETE   { VIRTUAL_INSN_X_AFTER, FR30BF_INSN_X_AFTER, FR30BF_SFMT_EMPTY },
// OBSOLETE   { VIRTUAL_INSN_X_BEFORE, FR30BF_INSN_X_BEFORE, FR30BF_SFMT_EMPTY },
// OBSOLETE   { VIRTUAL_INSN_X_CTI_CHAIN, FR30BF_INSN_X_CTI_CHAIN, FR30BF_SFMT_EMPTY },
// OBSOLETE   { VIRTUAL_INSN_X_CHAIN, FR30BF_INSN_X_CHAIN, FR30BF_SFMT_EMPTY },
// OBSOLETE   { VIRTUAL_INSN_X_BEGIN, FR30BF_INSN_X_BEGIN, FR30BF_SFMT_EMPTY },
// OBSOLETE   { FR30_INSN_ADD, FR30BF_INSN_ADD, FR30BF_SFMT_ADD },
// OBSOLETE   { FR30_INSN_ADDI, FR30BF_INSN_ADDI, FR30BF_SFMT_ADDI },
// OBSOLETE   { FR30_INSN_ADD2, FR30BF_INSN_ADD2, FR30BF_SFMT_ADD2 },
// OBSOLETE   { FR30_INSN_ADDC, FR30BF_INSN_ADDC, FR30BF_SFMT_ADDC },
// OBSOLETE   { FR30_INSN_ADDN, FR30BF_INSN_ADDN, FR30BF_SFMT_ADDN },
// OBSOLETE   { FR30_INSN_ADDNI, FR30BF_INSN_ADDNI, FR30BF_SFMT_ADDNI },
// OBSOLETE   { FR30_INSN_ADDN2, FR30BF_INSN_ADDN2, FR30BF_SFMT_ADDN2 },
// OBSOLETE   { FR30_INSN_SUB, FR30BF_INSN_SUB, FR30BF_SFMT_ADD },
// OBSOLETE   { FR30_INSN_SUBC, FR30BF_INSN_SUBC, FR30BF_SFMT_ADDC },
// OBSOLETE   { FR30_INSN_SUBN, FR30BF_INSN_SUBN, FR30BF_SFMT_ADDN },
// OBSOLETE   { FR30_INSN_CMP, FR30BF_INSN_CMP, FR30BF_SFMT_CMP },
// OBSOLETE   { FR30_INSN_CMPI, FR30BF_INSN_CMPI, FR30BF_SFMT_CMPI },
// OBSOLETE   { FR30_INSN_CMP2, FR30BF_INSN_CMP2, FR30BF_SFMT_CMP2 },
// OBSOLETE   { FR30_INSN_AND, FR30BF_INSN_AND, FR30BF_SFMT_AND },
// OBSOLETE   { FR30_INSN_OR, FR30BF_INSN_OR, FR30BF_SFMT_AND },
// OBSOLETE   { FR30_INSN_EOR, FR30BF_INSN_EOR, FR30BF_SFMT_AND },
// OBSOLETE   { FR30_INSN_ANDM, FR30BF_INSN_ANDM, FR30BF_SFMT_ANDM },
// OBSOLETE   { FR30_INSN_ANDH, FR30BF_INSN_ANDH, FR30BF_SFMT_ANDH },
// OBSOLETE   { FR30_INSN_ANDB, FR30BF_INSN_ANDB, FR30BF_SFMT_ANDB },
// OBSOLETE   { FR30_INSN_ORM, FR30BF_INSN_ORM, FR30BF_SFMT_ANDM },
// OBSOLETE   { FR30_INSN_ORH, FR30BF_INSN_ORH, FR30BF_SFMT_ANDH },
// OBSOLETE   { FR30_INSN_ORB, FR30BF_INSN_ORB, FR30BF_SFMT_ANDB },
// OBSOLETE   { FR30_INSN_EORM, FR30BF_INSN_EORM, FR30BF_SFMT_ANDM },
// OBSOLETE   { FR30_INSN_EORH, FR30BF_INSN_EORH, FR30BF_SFMT_ANDH },
// OBSOLETE   { FR30_INSN_EORB, FR30BF_INSN_EORB, FR30BF_SFMT_ANDB },
// OBSOLETE   { FR30_INSN_BANDL, FR30BF_INSN_BANDL, FR30BF_SFMT_BANDL },
// OBSOLETE   { FR30_INSN_BORL, FR30BF_INSN_BORL, FR30BF_SFMT_BANDL },
// OBSOLETE   { FR30_INSN_BEORL, FR30BF_INSN_BEORL, FR30BF_SFMT_BANDL },
// OBSOLETE   { FR30_INSN_BANDH, FR30BF_INSN_BANDH, FR30BF_SFMT_BANDL },
// OBSOLETE   { FR30_INSN_BORH, FR30BF_INSN_BORH, FR30BF_SFMT_BANDL },
// OBSOLETE   { FR30_INSN_BEORH, FR30BF_INSN_BEORH, FR30BF_SFMT_BANDL },
// OBSOLETE   { FR30_INSN_BTSTL, FR30BF_INSN_BTSTL, FR30BF_SFMT_BTSTL },
// OBSOLETE   { FR30_INSN_BTSTH, FR30BF_INSN_BTSTH, FR30BF_SFMT_BTSTL },
// OBSOLETE   { FR30_INSN_MUL, FR30BF_INSN_MUL, FR30BF_SFMT_MUL },
// OBSOLETE   { FR30_INSN_MULU, FR30BF_INSN_MULU, FR30BF_SFMT_MULU },
// OBSOLETE   { FR30_INSN_MULH, FR30BF_INSN_MULH, FR30BF_SFMT_MULH },
// OBSOLETE   { FR30_INSN_MULUH, FR30BF_INSN_MULUH, FR30BF_SFMT_MULH },
// OBSOLETE   { FR30_INSN_DIV0S, FR30BF_INSN_DIV0S, FR30BF_SFMT_DIV0S },
// OBSOLETE   { FR30_INSN_DIV0U, FR30BF_INSN_DIV0U, FR30BF_SFMT_DIV0U },
// OBSOLETE   { FR30_INSN_DIV1, FR30BF_INSN_DIV1, FR30BF_SFMT_DIV1 },
// OBSOLETE   { FR30_INSN_DIV2, FR30BF_INSN_DIV2, FR30BF_SFMT_DIV2 },
// OBSOLETE   { FR30_INSN_DIV3, FR30BF_INSN_DIV3, FR30BF_SFMT_DIV3 },
// OBSOLETE   { FR30_INSN_DIV4S, FR30BF_INSN_DIV4S, FR30BF_SFMT_DIV4S },
// OBSOLETE   { FR30_INSN_LSL, FR30BF_INSN_LSL, FR30BF_SFMT_LSL },
// OBSOLETE   { FR30_INSN_LSLI, FR30BF_INSN_LSLI, FR30BF_SFMT_LSLI },
// OBSOLETE   { FR30_INSN_LSL2, FR30BF_INSN_LSL2, FR30BF_SFMT_LSLI },
// OBSOLETE   { FR30_INSN_LSR, FR30BF_INSN_LSR, FR30BF_SFMT_LSL },
// OBSOLETE   { FR30_INSN_LSRI, FR30BF_INSN_LSRI, FR30BF_SFMT_LSLI },
// OBSOLETE   { FR30_INSN_LSR2, FR30BF_INSN_LSR2, FR30BF_SFMT_LSLI },
// OBSOLETE   { FR30_INSN_ASR, FR30BF_INSN_ASR, FR30BF_SFMT_LSL },
// OBSOLETE   { FR30_INSN_ASRI, FR30BF_INSN_ASRI, FR30BF_SFMT_LSLI },
// OBSOLETE   { FR30_INSN_ASR2, FR30BF_INSN_ASR2, FR30BF_SFMT_LSLI },
// OBSOLETE   { FR30_INSN_LDI8, FR30BF_INSN_LDI8, FR30BF_SFMT_LDI8 },
// OBSOLETE   { FR30_INSN_LDI20, FR30BF_INSN_LDI20, FR30BF_SFMT_LDI20 },
// OBSOLETE   { FR30_INSN_LDI32, FR30BF_INSN_LDI32, FR30BF_SFMT_LDI32 },
// OBSOLETE   { FR30_INSN_LD, FR30BF_INSN_LD, FR30BF_SFMT_LD },
// OBSOLETE   { FR30_INSN_LDUH, FR30BF_INSN_LDUH, FR30BF_SFMT_LDUH },
// OBSOLETE   { FR30_INSN_LDUB, FR30BF_INSN_LDUB, FR30BF_SFMT_LDUB },
// OBSOLETE   { FR30_INSN_LDR13, FR30BF_INSN_LDR13, FR30BF_SFMT_LDR13 },
// OBSOLETE   { FR30_INSN_LDR13UH, FR30BF_INSN_LDR13UH, FR30BF_SFMT_LDR13UH },
// OBSOLETE   { FR30_INSN_LDR13UB, FR30BF_INSN_LDR13UB, FR30BF_SFMT_LDR13UB },
// OBSOLETE   { FR30_INSN_LDR14, FR30BF_INSN_LDR14, FR30BF_SFMT_LDR14 },
// OBSOLETE   { FR30_INSN_LDR14UH, FR30BF_INSN_LDR14UH, FR30BF_SFMT_LDR14UH },
// OBSOLETE   { FR30_INSN_LDR14UB, FR30BF_INSN_LDR14UB, FR30BF_SFMT_LDR14UB },
// OBSOLETE   { FR30_INSN_LDR15, FR30BF_INSN_LDR15, FR30BF_SFMT_LDR15 },
// OBSOLETE   { FR30_INSN_LDR15GR, FR30BF_INSN_LDR15GR, FR30BF_SFMT_LDR15GR },
// OBSOLETE   { FR30_INSN_LDR15DR, FR30BF_INSN_LDR15DR, FR30BF_SFMT_LDR15DR },
// OBSOLETE   { FR30_INSN_LDR15PS, FR30BF_INSN_LDR15PS, FR30BF_SFMT_LDR15PS },
// OBSOLETE   { FR30_INSN_ST, FR30BF_INSN_ST, FR30BF_SFMT_ST },
// OBSOLETE   { FR30_INSN_STH, FR30BF_INSN_STH, FR30BF_SFMT_STH },
// OBSOLETE   { FR30_INSN_STB, FR30BF_INSN_STB, FR30BF_SFMT_STB },
// OBSOLETE   { FR30_INSN_STR13, FR30BF_INSN_STR13, FR30BF_SFMT_STR13 },
// OBSOLETE   { FR30_INSN_STR13H, FR30BF_INSN_STR13H, FR30BF_SFMT_STR13H },
// OBSOLETE   { FR30_INSN_STR13B, FR30BF_INSN_STR13B, FR30BF_SFMT_STR13B },
// OBSOLETE   { FR30_INSN_STR14, FR30BF_INSN_STR14, FR30BF_SFMT_STR14 },
// OBSOLETE   { FR30_INSN_STR14H, FR30BF_INSN_STR14H, FR30BF_SFMT_STR14H },
// OBSOLETE   { FR30_INSN_STR14B, FR30BF_INSN_STR14B, FR30BF_SFMT_STR14B },
// OBSOLETE   { FR30_INSN_STR15, FR30BF_INSN_STR15, FR30BF_SFMT_STR15 },
// OBSOLETE   { FR30_INSN_STR15GR, FR30BF_INSN_STR15GR, FR30BF_SFMT_STR15GR },
// OBSOLETE   { FR30_INSN_STR15DR, FR30BF_INSN_STR15DR, FR30BF_SFMT_STR15DR },
// OBSOLETE   { FR30_INSN_STR15PS, FR30BF_INSN_STR15PS, FR30BF_SFMT_STR15PS },
// OBSOLETE   { FR30_INSN_MOV, FR30BF_INSN_MOV, FR30BF_SFMT_MOV },
// OBSOLETE   { FR30_INSN_MOVDR, FR30BF_INSN_MOVDR, FR30BF_SFMT_MOVDR },
// OBSOLETE   { FR30_INSN_MOVPS, FR30BF_INSN_MOVPS, FR30BF_SFMT_MOVPS },
// OBSOLETE   { FR30_INSN_MOV2DR, FR30BF_INSN_MOV2DR, FR30BF_SFMT_MOV2DR },
// OBSOLETE   { FR30_INSN_MOV2PS, FR30BF_INSN_MOV2PS, FR30BF_SFMT_MOV2PS },
// OBSOLETE   { FR30_INSN_JMP, FR30BF_INSN_JMP, FR30BF_SFMT_JMP },
// OBSOLETE   { FR30_INSN_JMPD, FR30BF_INSN_JMPD, FR30BF_SFMT_JMP },
// OBSOLETE   { FR30_INSN_CALLR, FR30BF_INSN_CALLR, FR30BF_SFMT_CALLR },
// OBSOLETE   { FR30_INSN_CALLRD, FR30BF_INSN_CALLRD, FR30BF_SFMT_CALLR },
// OBSOLETE   { FR30_INSN_CALL, FR30BF_INSN_CALL, FR30BF_SFMT_CALL },
// OBSOLETE   { FR30_INSN_CALLD, FR30BF_INSN_CALLD, FR30BF_SFMT_CALL },
// OBSOLETE   { FR30_INSN_RET, FR30BF_INSN_RET, FR30BF_SFMT_RET },
// OBSOLETE   { FR30_INSN_RET_D, FR30BF_INSN_RET_D, FR30BF_SFMT_RET },
// OBSOLETE   { FR30_INSN_INT, FR30BF_INSN_INT, FR30BF_SFMT_INT },
// OBSOLETE   { FR30_INSN_INTE, FR30BF_INSN_INTE, FR30BF_SFMT_INTE },
// OBSOLETE   { FR30_INSN_RETI, FR30BF_INSN_RETI, FR30BF_SFMT_RETI },
// OBSOLETE   { FR30_INSN_BRAD, FR30BF_INSN_BRAD, FR30BF_SFMT_BRAD },
// OBSOLETE   { FR30_INSN_BRA, FR30BF_INSN_BRA, FR30BF_SFMT_BRAD },
// OBSOLETE   { FR30_INSN_BNOD, FR30BF_INSN_BNOD, FR30BF_SFMT_BNOD },
// OBSOLETE   { FR30_INSN_BNO, FR30BF_INSN_BNO, FR30BF_SFMT_BNOD },
// OBSOLETE   { FR30_INSN_BEQD, FR30BF_INSN_BEQD, FR30BF_SFMT_BEQD },
// OBSOLETE   { FR30_INSN_BEQ, FR30BF_INSN_BEQ, FR30BF_SFMT_BEQD },
// OBSOLETE   { FR30_INSN_BNED, FR30BF_INSN_BNED, FR30BF_SFMT_BEQD },
// OBSOLETE   { FR30_INSN_BNE, FR30BF_INSN_BNE, FR30BF_SFMT_BEQD },
// OBSOLETE   { FR30_INSN_BCD, FR30BF_INSN_BCD, FR30BF_SFMT_BCD },
// OBSOLETE   { FR30_INSN_BC, FR30BF_INSN_BC, FR30BF_SFMT_BCD },
// OBSOLETE   { FR30_INSN_BNCD, FR30BF_INSN_BNCD, FR30BF_SFMT_BCD },
// OBSOLETE   { FR30_INSN_BNC, FR30BF_INSN_BNC, FR30BF_SFMT_BCD },
// OBSOLETE   { FR30_INSN_BND, FR30BF_INSN_BND, FR30BF_SFMT_BND },
// OBSOLETE   { FR30_INSN_BN, FR30BF_INSN_BN, FR30BF_SFMT_BND },
// OBSOLETE   { FR30_INSN_BPD, FR30BF_INSN_BPD, FR30BF_SFMT_BND },
// OBSOLETE   { FR30_INSN_BP, FR30BF_INSN_BP, FR30BF_SFMT_BND },
// OBSOLETE   { FR30_INSN_BVD, FR30BF_INSN_BVD, FR30BF_SFMT_BVD },
// OBSOLETE   { FR30_INSN_BV, FR30BF_INSN_BV, FR30BF_SFMT_BVD },
// OBSOLETE   { FR30_INSN_BNVD, FR30BF_INSN_BNVD, FR30BF_SFMT_BVD },
// OBSOLETE   { FR30_INSN_BNV, FR30BF_INSN_BNV, FR30BF_SFMT_BVD },
// OBSOLETE   { FR30_INSN_BLTD, FR30BF_INSN_BLTD, FR30BF_SFMT_BLTD },
// OBSOLETE   { FR30_INSN_BLT, FR30BF_INSN_BLT, FR30BF_SFMT_BLTD },
// OBSOLETE   { FR30_INSN_BGED, FR30BF_INSN_BGED, FR30BF_SFMT_BLTD },
// OBSOLETE   { FR30_INSN_BGE, FR30BF_INSN_BGE, FR30BF_SFMT_BLTD },
// OBSOLETE   { FR30_INSN_BLED, FR30BF_INSN_BLED, FR30BF_SFMT_BLED },
// OBSOLETE   { FR30_INSN_BLE, FR30BF_INSN_BLE, FR30BF_SFMT_BLED },
// OBSOLETE   { FR30_INSN_BGTD, FR30BF_INSN_BGTD, FR30BF_SFMT_BLED },
// OBSOLETE   { FR30_INSN_BGT, FR30BF_INSN_BGT, FR30BF_SFMT_BLED },
// OBSOLETE   { FR30_INSN_BLSD, FR30BF_INSN_BLSD, FR30BF_SFMT_BLSD },
// OBSOLETE   { FR30_INSN_BLS, FR30BF_INSN_BLS, FR30BF_SFMT_BLSD },
// OBSOLETE   { FR30_INSN_BHID, FR30BF_INSN_BHID, FR30BF_SFMT_BLSD },
// OBSOLETE   { FR30_INSN_BHI, FR30BF_INSN_BHI, FR30BF_SFMT_BLSD },
// OBSOLETE   { FR30_INSN_DMOVR13, FR30BF_INSN_DMOVR13, FR30BF_SFMT_DMOVR13 },
// OBSOLETE   { FR30_INSN_DMOVR13H, FR30BF_INSN_DMOVR13H, FR30BF_SFMT_DMOVR13H },
// OBSOLETE   { FR30_INSN_DMOVR13B, FR30BF_INSN_DMOVR13B, FR30BF_SFMT_DMOVR13B },
// OBSOLETE   { FR30_INSN_DMOVR13PI, FR30BF_INSN_DMOVR13PI, FR30BF_SFMT_DMOVR13PI },
// OBSOLETE   { FR30_INSN_DMOVR13PIH, FR30BF_INSN_DMOVR13PIH, FR30BF_SFMT_DMOVR13PIH },
// OBSOLETE   { FR30_INSN_DMOVR13PIB, FR30BF_INSN_DMOVR13PIB, FR30BF_SFMT_DMOVR13PIB },
// OBSOLETE   { FR30_INSN_DMOVR15PI, FR30BF_INSN_DMOVR15PI, FR30BF_SFMT_DMOVR15PI },
// OBSOLETE   { FR30_INSN_DMOV2R13, FR30BF_INSN_DMOV2R13, FR30BF_SFMT_DMOV2R13 },
// OBSOLETE   { FR30_INSN_DMOV2R13H, FR30BF_INSN_DMOV2R13H, FR30BF_SFMT_DMOV2R13H },
// OBSOLETE   { FR30_INSN_DMOV2R13B, FR30BF_INSN_DMOV2R13B, FR30BF_SFMT_DMOV2R13B },
// OBSOLETE   { FR30_INSN_DMOV2R13PI, FR30BF_INSN_DMOV2R13PI, FR30BF_SFMT_DMOV2R13PI },
// OBSOLETE   { FR30_INSN_DMOV2R13PIH, FR30BF_INSN_DMOV2R13PIH, FR30BF_SFMT_DMOV2R13PIH },
// OBSOLETE   { FR30_INSN_DMOV2R13PIB, FR30BF_INSN_DMOV2R13PIB, FR30BF_SFMT_DMOV2R13PIB },
// OBSOLETE   { FR30_INSN_DMOV2R15PD, FR30BF_INSN_DMOV2R15PD, FR30BF_SFMT_DMOV2R15PD },
// OBSOLETE   { FR30_INSN_LDRES, FR30BF_INSN_LDRES, FR30BF_SFMT_LDRES },
// OBSOLETE   { FR30_INSN_STRES, FR30BF_INSN_STRES, FR30BF_SFMT_LDRES },
// OBSOLETE   { FR30_INSN_COPOP, FR30BF_INSN_COPOP, FR30BF_SFMT_COPOP },
// OBSOLETE   { FR30_INSN_COPLD, FR30BF_INSN_COPLD, FR30BF_SFMT_COPOP },
// OBSOLETE   { FR30_INSN_COPST, FR30BF_INSN_COPST, FR30BF_SFMT_COPOP },
// OBSOLETE   { FR30_INSN_COPSV, FR30BF_INSN_COPSV, FR30BF_SFMT_COPOP },
// OBSOLETE   { FR30_INSN_NOP, FR30BF_INSN_NOP, FR30BF_SFMT_BNOD },
// OBSOLETE   { FR30_INSN_ANDCCR, FR30BF_INSN_ANDCCR, FR30BF_SFMT_ANDCCR },
// OBSOLETE   { FR30_INSN_ORCCR, FR30BF_INSN_ORCCR, FR30BF_SFMT_ANDCCR },
// OBSOLETE   { FR30_INSN_STILM, FR30BF_INSN_STILM, FR30BF_SFMT_STILM },
// OBSOLETE   { FR30_INSN_ADDSP, FR30BF_INSN_ADDSP, FR30BF_SFMT_ADDSP },
// OBSOLETE   { FR30_INSN_EXTSB, FR30BF_INSN_EXTSB, FR30BF_SFMT_EXTSB },
// OBSOLETE   { FR30_INSN_EXTUB, FR30BF_INSN_EXTUB, FR30BF_SFMT_EXTUB },
// OBSOLETE   { FR30_INSN_EXTSH, FR30BF_INSN_EXTSH, FR30BF_SFMT_EXTSH },
// OBSOLETE   { FR30_INSN_EXTUH, FR30BF_INSN_EXTUH, FR30BF_SFMT_EXTUH },
// OBSOLETE   { FR30_INSN_LDM0, FR30BF_INSN_LDM0, FR30BF_SFMT_LDM0 },
// OBSOLETE   { FR30_INSN_LDM1, FR30BF_INSN_LDM1, FR30BF_SFMT_LDM1 },
// OBSOLETE   { FR30_INSN_STM0, FR30BF_INSN_STM0, FR30BF_SFMT_STM0 },
// OBSOLETE   { FR30_INSN_STM1, FR30BF_INSN_STM1, FR30BF_SFMT_STM1 },
// OBSOLETE   { FR30_INSN_ENTER, FR30BF_INSN_ENTER, FR30BF_SFMT_ENTER },
// OBSOLETE   { FR30_INSN_LEAVE, FR30BF_INSN_LEAVE, FR30BF_SFMT_LEAVE },
// OBSOLETE   { FR30_INSN_XCHB, FR30BF_INSN_XCHB, FR30BF_SFMT_XCHB },
// OBSOLETE };
// OBSOLETE 
// OBSOLETE static const struct insn_sem fr30bf_insn_sem_invalid = {
// OBSOLETE   VIRTUAL_INSN_X_INVALID, FR30BF_INSN_X_INVALID, FR30BF_SFMT_EMPTY
// OBSOLETE };
// OBSOLETE 
// OBSOLETE /* Initialize an IDESC from the compile-time computable parts.  */
// OBSOLETE 
// OBSOLETE static INLINE void
// OBSOLETE init_idesc (SIM_CPU *cpu, IDESC *id, const struct insn_sem *t)
// OBSOLETE {
// OBSOLETE   const CGEN_INSN *insn_table = CGEN_CPU_INSN_TABLE (CPU_CPU_DESC (cpu))->init_entries;
// OBSOLETE 
// OBSOLETE   id->num = t->index;
// OBSOLETE   id->sfmt = t->sfmt;
// OBSOLETE   if ((int) t->type <= 0)
// OBSOLETE     id->idata = & cgen_virtual_insn_table[- (int) t->type];
// OBSOLETE   else
// OBSOLETE     id->idata = & insn_table[t->type];
// OBSOLETE   id->attrs = CGEN_INSN_ATTRS (id->idata);
// OBSOLETE   /* Oh my god, a magic number.  */
// OBSOLETE   id->length = CGEN_INSN_BITSIZE (id->idata) / 8;
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   id->timing = & MODEL_TIMING (CPU_MODEL (cpu)) [t->index];
// OBSOLETE   {
// OBSOLETE     SIM_DESC sd = CPU_STATE (cpu);
// OBSOLETE     SIM_ASSERT (t->index == id->timing->num);
// OBSOLETE   }
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE   /* Semantic pointers are initialized elsewhere.  */
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Initialize the instruction descriptor table.  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE fr30bf_init_idesc_table (SIM_CPU *cpu)
// OBSOLETE {
// OBSOLETE   IDESC *id,*tabend;
// OBSOLETE   const struct insn_sem *t,*tend;
// OBSOLETE   int tabsize = sizeof (fr30bf_insn_data) / sizeof (IDESC);
// OBSOLETE   IDESC *table = fr30bf_insn_data;
// OBSOLETE 
// OBSOLETE   memset (table, 0, tabsize * sizeof (IDESC));
// OBSOLETE 
// OBSOLETE   /* First set all entries to the `invalid insn'.  */
// OBSOLETE   t = & fr30bf_insn_sem_invalid;
// OBSOLETE   for (id = table, tabend = table + tabsize; id < tabend; ++id)
// OBSOLETE     init_idesc (cpu, id, t);
// OBSOLETE 
// OBSOLETE   /* Now fill in the values for the chosen cpu.  */
// OBSOLETE   for (t = fr30bf_insn_sem, tend = t + sizeof (fr30bf_insn_sem) / sizeof (*t);
// OBSOLETE        t != tend; ++t)
// OBSOLETE     {
// OBSOLETE       init_idesc (cpu, & table[t->index], t);
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* Link the IDESC table into the cpu.  */
// OBSOLETE   CPU_IDESC (cpu) = table;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Given an instruction, return a pointer to its IDESC entry.  */
// OBSOLETE 
// OBSOLETE const IDESC *
// OBSOLETE fr30bf_decode (SIM_CPU *current_cpu, IADDR pc,
// OBSOLETE               CGEN_INSN_INT base_insn,
// OBSOLETE               ARGBUF *abuf)
// OBSOLETE {
// OBSOLETE   /* Result of decoder.  */
// OBSOLETE   FR30BF_INSN_TYPE itype;
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE 
// OBSOLETE     {
// OBSOLETE       unsigned int val = (((insn >> 8) & (255 << 0)));
// OBSOLETE       switch (val)
// OBSOLETE       {
// OBSOLETE       case 0 : itype = FR30BF_INSN_LDR13;goto extract_sfmt_ldr13;
// OBSOLETE       case 1 : itype = FR30BF_INSN_LDR13UH;goto extract_sfmt_ldr13uh;
// OBSOLETE       case 2 : itype = FR30BF_INSN_LDR13UB;goto extract_sfmt_ldr13ub;
// OBSOLETE       case 3 : itype = FR30BF_INSN_LDR15;goto extract_sfmt_ldr15;
// OBSOLETE       case 4 : itype = FR30BF_INSN_LD;goto extract_sfmt_ld;
// OBSOLETE       case 5 : itype = FR30BF_INSN_LDUH;goto extract_sfmt_lduh;
// OBSOLETE       case 6 : itype = FR30BF_INSN_LDUB;goto extract_sfmt_ldub;
// OBSOLETE       case 7 :
// OBSOLETE         {
// OBSOLETE           unsigned int val = (((insn >> 6) & (1 << 1)) | ((insn >> 4) & (1 << 0)));
// OBSOLETE           switch (val)
// OBSOLETE           {
// OBSOLETE           case 0 : itype = FR30BF_INSN_LDR15GR;goto extract_sfmt_ldr15gr;
// OBSOLETE           case 1 : itype = FR30BF_INSN_MOV2PS;goto extract_sfmt_mov2ps;
// OBSOLETE           case 2 : itype = FR30BF_INSN_LDR15DR;goto extract_sfmt_ldr15dr;
// OBSOLETE           case 3 : itype = FR30BF_INSN_LDR15PS;goto extract_sfmt_ldr15ps;
// OBSOLETE           default : itype = FR30BF_INSN_X_INVALID; goto extract_sfmt_empty;
// OBSOLETE           }
// OBSOLETE         }
// OBSOLETE       case 8 : itype = FR30BF_INSN_DMOV2R13;goto extract_sfmt_dmov2r13;
// OBSOLETE       case 9 : itype = FR30BF_INSN_DMOV2R13H;goto extract_sfmt_dmov2r13h;
// OBSOLETE       case 10 : itype = FR30BF_INSN_DMOV2R13B;goto extract_sfmt_dmov2r13b;
// OBSOLETE       case 11 : itype = FR30BF_INSN_DMOV2R15PD;goto extract_sfmt_dmov2r15pd;
// OBSOLETE       case 12 : itype = FR30BF_INSN_DMOV2R13PI;goto extract_sfmt_dmov2r13pi;
// OBSOLETE       case 13 : itype = FR30BF_INSN_DMOV2R13PIH;goto extract_sfmt_dmov2r13pih;
// OBSOLETE       case 14 : itype = FR30BF_INSN_DMOV2R13PIB;goto extract_sfmt_dmov2r13pib;
// OBSOLETE       case 15 : itype = FR30BF_INSN_ENTER;goto extract_sfmt_enter;
// OBSOLETE       case 16 : itype = FR30BF_INSN_STR13;goto extract_sfmt_str13;
// OBSOLETE       case 17 : itype = FR30BF_INSN_STR13H;goto extract_sfmt_str13h;
// OBSOLETE       case 18 : itype = FR30BF_INSN_STR13B;goto extract_sfmt_str13b;
// OBSOLETE       case 19 : itype = FR30BF_INSN_STR15;goto extract_sfmt_str15;
// OBSOLETE       case 20 : itype = FR30BF_INSN_ST;goto extract_sfmt_st;
// OBSOLETE       case 21 : itype = FR30BF_INSN_STH;goto extract_sfmt_sth;
// OBSOLETE       case 22 : itype = FR30BF_INSN_STB;goto extract_sfmt_stb;
// OBSOLETE       case 23 :
// OBSOLETE         {
// OBSOLETE           unsigned int val = (((insn >> 6) & (1 << 1)) | ((insn >> 4) & (1 << 0)));
// OBSOLETE           switch (val)
// OBSOLETE           {
// OBSOLETE           case 0 : itype = FR30BF_INSN_STR15GR;goto extract_sfmt_str15gr;
// OBSOLETE           case 1 : itype = FR30BF_INSN_MOVPS;goto extract_sfmt_movps;
// OBSOLETE           case 2 : itype = FR30BF_INSN_STR15DR;goto extract_sfmt_str15dr;
// OBSOLETE           case 3 : itype = FR30BF_INSN_STR15PS;goto extract_sfmt_str15ps;
// OBSOLETE           default : itype = FR30BF_INSN_X_INVALID; goto extract_sfmt_empty;
// OBSOLETE           }
// OBSOLETE         }
// OBSOLETE       case 24 : itype = FR30BF_INSN_DMOVR13;goto extract_sfmt_dmovr13;
// OBSOLETE       case 25 : itype = FR30BF_INSN_DMOVR13H;goto extract_sfmt_dmovr13h;
// OBSOLETE       case 26 : itype = FR30BF_INSN_DMOVR13B;goto extract_sfmt_dmovr13b;
// OBSOLETE       case 27 : itype = FR30BF_INSN_DMOVR15PI;goto extract_sfmt_dmovr15pi;
// OBSOLETE       case 28 : itype = FR30BF_INSN_DMOVR13PI;goto extract_sfmt_dmovr13pi;
// OBSOLETE       case 29 : itype = FR30BF_INSN_DMOVR13PIH;goto extract_sfmt_dmovr13pih;
// OBSOLETE       case 30 : itype = FR30BF_INSN_DMOVR13PIB;goto extract_sfmt_dmovr13pib;
// OBSOLETE       case 31 : itype = FR30BF_INSN_INT;goto extract_sfmt_int;
// OBSOLETE       case 32 : /* fall through */
// OBSOLETE       case 33 : /* fall through */
// OBSOLETE       case 34 : /* fall through */
// OBSOLETE       case 35 : /* fall through */
// OBSOLETE       case 36 : /* fall through */
// OBSOLETE       case 37 : /* fall through */
// OBSOLETE       case 38 : /* fall through */
// OBSOLETE       case 39 : /* fall through */
// OBSOLETE       case 40 : /* fall through */
// OBSOLETE       case 41 : /* fall through */
// OBSOLETE       case 42 : /* fall through */
// OBSOLETE       case 43 : /* fall through */
// OBSOLETE       case 44 : /* fall through */
// OBSOLETE       case 45 : /* fall through */
// OBSOLETE       case 46 : /* fall through */
// OBSOLETE       case 47 : itype = FR30BF_INSN_LDR14;goto extract_sfmt_ldr14;
// OBSOLETE       case 48 : /* fall through */
// OBSOLETE       case 49 : /* fall through */
// OBSOLETE       case 50 : /* fall through */
// OBSOLETE       case 51 : /* fall through */
// OBSOLETE       case 52 : /* fall through */
// OBSOLETE       case 53 : /* fall through */
// OBSOLETE       case 54 : /* fall through */
// OBSOLETE       case 55 : /* fall through */
// OBSOLETE       case 56 : /* fall through */
// OBSOLETE       case 57 : /* fall through */
// OBSOLETE       case 58 : /* fall through */
// OBSOLETE       case 59 : /* fall through */
// OBSOLETE       case 60 : /* fall through */
// OBSOLETE       case 61 : /* fall through */
// OBSOLETE       case 62 : /* fall through */
// OBSOLETE       case 63 : itype = FR30BF_INSN_STR14;goto extract_sfmt_str14;
// OBSOLETE       case 64 : /* fall through */
// OBSOLETE       case 65 : /* fall through */
// OBSOLETE       case 66 : /* fall through */
// OBSOLETE       case 67 : /* fall through */
// OBSOLETE       case 68 : /* fall through */
// OBSOLETE       case 69 : /* fall through */
// OBSOLETE       case 70 : /* fall through */
// OBSOLETE       case 71 : /* fall through */
// OBSOLETE       case 72 : /* fall through */
// OBSOLETE       case 73 : /* fall through */
// OBSOLETE       case 74 : /* fall through */
// OBSOLETE       case 75 : /* fall through */
// OBSOLETE       case 76 : /* fall through */
// OBSOLETE       case 77 : /* fall through */
// OBSOLETE       case 78 : /* fall through */
// OBSOLETE       case 79 : itype = FR30BF_INSN_LDR14UH;goto extract_sfmt_ldr14uh;
// OBSOLETE       case 80 : /* fall through */
// OBSOLETE       case 81 : /* fall through */
// OBSOLETE       case 82 : /* fall through */
// OBSOLETE       case 83 : /* fall through */
// OBSOLETE       case 84 : /* fall through */
// OBSOLETE       case 85 : /* fall through */
// OBSOLETE       case 86 : /* fall through */
// OBSOLETE       case 87 : /* fall through */
// OBSOLETE       case 88 : /* fall through */
// OBSOLETE       case 89 : /* fall through */
// OBSOLETE       case 90 : /* fall through */
// OBSOLETE       case 91 : /* fall through */
// OBSOLETE       case 92 : /* fall through */
// OBSOLETE       case 93 : /* fall through */
// OBSOLETE       case 94 : /* fall through */
// OBSOLETE       case 95 : itype = FR30BF_INSN_STR14H;goto extract_sfmt_str14h;
// OBSOLETE       case 96 : /* fall through */
// OBSOLETE       case 97 : /* fall through */
// OBSOLETE       case 98 : /* fall through */
// OBSOLETE       case 99 : /* fall through */
// OBSOLETE       case 100 : /* fall through */
// OBSOLETE       case 101 : /* fall through */
// OBSOLETE       case 102 : /* fall through */
// OBSOLETE       case 103 : /* fall through */
// OBSOLETE       case 104 : /* fall through */
// OBSOLETE       case 105 : /* fall through */
// OBSOLETE       case 106 : /* fall through */
// OBSOLETE       case 107 : /* fall through */
// OBSOLETE       case 108 : /* fall through */
// OBSOLETE       case 109 : /* fall through */
// OBSOLETE       case 110 : /* fall through */
// OBSOLETE       case 111 : itype = FR30BF_INSN_LDR14UB;goto extract_sfmt_ldr14ub;
// OBSOLETE       case 112 : /* fall through */
// OBSOLETE       case 113 : /* fall through */
// OBSOLETE       case 114 : /* fall through */
// OBSOLETE       case 115 : /* fall through */
// OBSOLETE       case 116 : /* fall through */
// OBSOLETE       case 117 : /* fall through */
// OBSOLETE       case 118 : /* fall through */
// OBSOLETE       case 119 : /* fall through */
// OBSOLETE       case 120 : /* fall through */
// OBSOLETE       case 121 : /* fall through */
// OBSOLETE       case 122 : /* fall through */
// OBSOLETE       case 123 : /* fall through */
// OBSOLETE       case 124 : /* fall through */
// OBSOLETE       case 125 : /* fall through */
// OBSOLETE       case 126 : /* fall through */
// OBSOLETE       case 127 : itype = FR30BF_INSN_STR14B;goto extract_sfmt_str14b;
// OBSOLETE       case 128 : itype = FR30BF_INSN_BANDL;goto extract_sfmt_bandl;
// OBSOLETE       case 129 : itype = FR30BF_INSN_BANDH;goto extract_sfmt_bandl;
// OBSOLETE       case 130 : itype = FR30BF_INSN_AND;goto extract_sfmt_and;
// OBSOLETE       case 131 : itype = FR30BF_INSN_ANDCCR;goto extract_sfmt_andccr;
// OBSOLETE       case 132 : itype = FR30BF_INSN_ANDM;goto extract_sfmt_andm;
// OBSOLETE       case 133 : itype = FR30BF_INSN_ANDH;goto extract_sfmt_andh;
// OBSOLETE       case 134 : itype = FR30BF_INSN_ANDB;goto extract_sfmt_andb;
// OBSOLETE       case 135 : itype = FR30BF_INSN_STILM;goto extract_sfmt_stilm;
// OBSOLETE       case 136 : itype = FR30BF_INSN_BTSTL;goto extract_sfmt_btstl;
// OBSOLETE       case 137 : itype = FR30BF_INSN_BTSTH;goto extract_sfmt_btstl;
// OBSOLETE       case 138 : itype = FR30BF_INSN_XCHB;goto extract_sfmt_xchb;
// OBSOLETE       case 139 : itype = FR30BF_INSN_MOV;goto extract_sfmt_mov;
// OBSOLETE       case 140 : itype = FR30BF_INSN_LDM0;goto extract_sfmt_ldm0;
// OBSOLETE       case 141 : itype = FR30BF_INSN_LDM1;goto extract_sfmt_ldm1;
// OBSOLETE       case 142 : itype = FR30BF_INSN_STM0;goto extract_sfmt_stm0;
// OBSOLETE       case 143 : itype = FR30BF_INSN_STM1;goto extract_sfmt_stm1;
// OBSOLETE       case 144 : itype = FR30BF_INSN_BORL;goto extract_sfmt_bandl;
// OBSOLETE       case 145 : itype = FR30BF_INSN_BORH;goto extract_sfmt_bandl;
// OBSOLETE       case 146 : itype = FR30BF_INSN_OR;goto extract_sfmt_and;
// OBSOLETE       case 147 : itype = FR30BF_INSN_ORCCR;goto extract_sfmt_andccr;
// OBSOLETE       case 148 : itype = FR30BF_INSN_ORM;goto extract_sfmt_andm;
// OBSOLETE       case 149 : itype = FR30BF_INSN_ORH;goto extract_sfmt_andh;
// OBSOLETE       case 150 : itype = FR30BF_INSN_ORB;goto extract_sfmt_andb;
// OBSOLETE       case 151 :
// OBSOLETE         {
// OBSOLETE           unsigned int val = (((insn >> 4) & (15 << 0)));
// OBSOLETE           switch (val)
// OBSOLETE           {
// OBSOLETE           case 0 : itype = FR30BF_INSN_JMP;goto extract_sfmt_jmp;
// OBSOLETE           case 1 : itype = FR30BF_INSN_CALLR;goto extract_sfmt_callr;
// OBSOLETE           case 2 : itype = FR30BF_INSN_RET;goto extract_sfmt_ret;
// OBSOLETE           case 3 : itype = FR30BF_INSN_RETI;goto extract_sfmt_reti;
// OBSOLETE           case 4 : itype = FR30BF_INSN_DIV0S;goto extract_sfmt_div0s;
// OBSOLETE           case 5 : itype = FR30BF_INSN_DIV0U;goto extract_sfmt_div0u;
// OBSOLETE           case 6 : itype = FR30BF_INSN_DIV1;goto extract_sfmt_div1;
// OBSOLETE           case 7 : itype = FR30BF_INSN_DIV2;goto extract_sfmt_div2;
// OBSOLETE           case 8 : itype = FR30BF_INSN_EXTSB;goto extract_sfmt_extsb;
// OBSOLETE           case 9 : itype = FR30BF_INSN_EXTUB;goto extract_sfmt_extub;
// OBSOLETE           case 10 : itype = FR30BF_INSN_EXTSH;goto extract_sfmt_extsh;
// OBSOLETE           case 11 : itype = FR30BF_INSN_EXTUH;goto extract_sfmt_extuh;
// OBSOLETE           default : itype = FR30BF_INSN_X_INVALID; goto extract_sfmt_empty;
// OBSOLETE           }
// OBSOLETE         }
// OBSOLETE       case 152 : itype = FR30BF_INSN_BEORL;goto extract_sfmt_bandl;
// OBSOLETE       case 153 : itype = FR30BF_INSN_BEORH;goto extract_sfmt_bandl;
// OBSOLETE       case 154 : itype = FR30BF_INSN_EOR;goto extract_sfmt_and;
// OBSOLETE       case 155 : itype = FR30BF_INSN_LDI20;goto extract_sfmt_ldi20;
// OBSOLETE       case 156 : itype = FR30BF_INSN_EORM;goto extract_sfmt_andm;
// OBSOLETE       case 157 : itype = FR30BF_INSN_EORH;goto extract_sfmt_andh;
// OBSOLETE       case 158 : itype = FR30BF_INSN_EORB;goto extract_sfmt_andb;
// OBSOLETE       case 159 :
// OBSOLETE         {
// OBSOLETE           unsigned int val = (((insn >> 4) & (15 << 0)));
// OBSOLETE           switch (val)
// OBSOLETE           {
// OBSOLETE           case 0 : itype = FR30BF_INSN_JMPD;goto extract_sfmt_jmp;
// OBSOLETE           case 1 : itype = FR30BF_INSN_CALLRD;goto extract_sfmt_callr;
// OBSOLETE           case 2 : itype = FR30BF_INSN_RET_D;goto extract_sfmt_ret;
// OBSOLETE           case 3 : itype = FR30BF_INSN_INTE;goto extract_sfmt_inte;
// OBSOLETE           case 6 : itype = FR30BF_INSN_DIV3;goto extract_sfmt_div3;
// OBSOLETE           case 7 : itype = FR30BF_INSN_DIV4S;goto extract_sfmt_div4s;
// OBSOLETE           case 8 : itype = FR30BF_INSN_LDI32;goto extract_sfmt_ldi32;
// OBSOLETE           case 9 : itype = FR30BF_INSN_LEAVE;goto extract_sfmt_leave;
// OBSOLETE           case 10 : itype = FR30BF_INSN_NOP;goto extract_sfmt_bnod;
// OBSOLETE           case 12 : itype = FR30BF_INSN_COPOP;goto extract_sfmt_copop;
// OBSOLETE           case 13 : itype = FR30BF_INSN_COPLD;goto extract_sfmt_copop;
// OBSOLETE           case 14 : itype = FR30BF_INSN_COPST;goto extract_sfmt_copop;
// OBSOLETE           case 15 : itype = FR30BF_INSN_COPSV;goto extract_sfmt_copop;
// OBSOLETE           default : itype = FR30BF_INSN_X_INVALID; goto extract_sfmt_empty;
// OBSOLETE           }
// OBSOLETE         }
// OBSOLETE       case 160 : itype = FR30BF_INSN_ADDNI;goto extract_sfmt_addni;
// OBSOLETE       case 161 : itype = FR30BF_INSN_ADDN2;goto extract_sfmt_addn2;
// OBSOLETE       case 162 : itype = FR30BF_INSN_ADDN;goto extract_sfmt_addn;
// OBSOLETE       case 163 : itype = FR30BF_INSN_ADDSP;goto extract_sfmt_addsp;
// OBSOLETE       case 164 : itype = FR30BF_INSN_ADDI;goto extract_sfmt_addi;
// OBSOLETE       case 165 : itype = FR30BF_INSN_ADD2;goto extract_sfmt_add2;
// OBSOLETE       case 166 : itype = FR30BF_INSN_ADD;goto extract_sfmt_add;
// OBSOLETE       case 167 : itype = FR30BF_INSN_ADDC;goto extract_sfmt_addc;
// OBSOLETE       case 168 : itype = FR30BF_INSN_CMPI;goto extract_sfmt_cmpi;
// OBSOLETE       case 169 : itype = FR30BF_INSN_CMP2;goto extract_sfmt_cmp2;
// OBSOLETE       case 170 : itype = FR30BF_INSN_CMP;goto extract_sfmt_cmp;
// OBSOLETE       case 171 : itype = FR30BF_INSN_MULU;goto extract_sfmt_mulu;
// OBSOLETE       case 172 : itype = FR30BF_INSN_SUB;goto extract_sfmt_add;
// OBSOLETE       case 173 : itype = FR30BF_INSN_SUBC;goto extract_sfmt_addc;
// OBSOLETE       case 174 : itype = FR30BF_INSN_SUBN;goto extract_sfmt_addn;
// OBSOLETE       case 175 : itype = FR30BF_INSN_MUL;goto extract_sfmt_mul;
// OBSOLETE       case 176 : itype = FR30BF_INSN_LSRI;goto extract_sfmt_lsli;
// OBSOLETE       case 177 : itype = FR30BF_INSN_LSR2;goto extract_sfmt_lsli;
// OBSOLETE       case 178 : itype = FR30BF_INSN_LSR;goto extract_sfmt_lsl;
// OBSOLETE       case 179 : itype = FR30BF_INSN_MOV2DR;goto extract_sfmt_mov2dr;
// OBSOLETE       case 180 : itype = FR30BF_INSN_LSLI;goto extract_sfmt_lsli;
// OBSOLETE       case 181 : itype = FR30BF_INSN_LSL2;goto extract_sfmt_lsli;
// OBSOLETE       case 182 : itype = FR30BF_INSN_LSL;goto extract_sfmt_lsl;
// OBSOLETE       case 183 : itype = FR30BF_INSN_MOVDR;goto extract_sfmt_movdr;
// OBSOLETE       case 184 : itype = FR30BF_INSN_ASRI;goto extract_sfmt_lsli;
// OBSOLETE       case 185 : itype = FR30BF_INSN_ASR2;goto extract_sfmt_lsli;
// OBSOLETE       case 186 : itype = FR30BF_INSN_ASR;goto extract_sfmt_lsl;
// OBSOLETE       case 187 : itype = FR30BF_INSN_MULUH;goto extract_sfmt_mulh;
// OBSOLETE       case 188 : itype = FR30BF_INSN_LDRES;goto extract_sfmt_ldres;
// OBSOLETE       case 189 : itype = FR30BF_INSN_STRES;goto extract_sfmt_ldres;
// OBSOLETE       case 191 : itype = FR30BF_INSN_MULH;goto extract_sfmt_mulh;
// OBSOLETE       case 192 : /* fall through */
// OBSOLETE       case 193 : /* fall through */
// OBSOLETE       case 194 : /* fall through */
// OBSOLETE       case 195 : /* fall through */
// OBSOLETE       case 196 : /* fall through */
// OBSOLETE       case 197 : /* fall through */
// OBSOLETE       case 198 : /* fall through */
// OBSOLETE       case 199 : /* fall through */
// OBSOLETE       case 200 : /* fall through */
// OBSOLETE       case 201 : /* fall through */
// OBSOLETE       case 202 : /* fall through */
// OBSOLETE       case 203 : /* fall through */
// OBSOLETE       case 204 : /* fall through */
// OBSOLETE       case 205 : /* fall through */
// OBSOLETE       case 206 : /* fall through */
// OBSOLETE       case 207 : itype = FR30BF_INSN_LDI8;goto extract_sfmt_ldi8;
// OBSOLETE       case 208 : /* fall through */
// OBSOLETE       case 209 : /* fall through */
// OBSOLETE       case 210 : /* fall through */
// OBSOLETE       case 211 : /* fall through */
// OBSOLETE       case 212 : /* fall through */
// OBSOLETE       case 213 : /* fall through */
// OBSOLETE       case 214 : /* fall through */
// OBSOLETE       case 215 : itype = FR30BF_INSN_CALL;goto extract_sfmt_call;
// OBSOLETE       case 216 : /* fall through */
// OBSOLETE       case 217 : /* fall through */
// OBSOLETE       case 218 : /* fall through */
// OBSOLETE       case 219 : /* fall through */
// OBSOLETE       case 220 : /* fall through */
// OBSOLETE       case 221 : /* fall through */
// OBSOLETE       case 222 : /* fall through */
// OBSOLETE       case 223 : itype = FR30BF_INSN_CALLD;goto extract_sfmt_call;
// OBSOLETE       case 224 : itype = FR30BF_INSN_BRA;goto extract_sfmt_brad;
// OBSOLETE       case 225 : itype = FR30BF_INSN_BNO;goto extract_sfmt_bnod;
// OBSOLETE       case 226 : itype = FR30BF_INSN_BEQ;goto extract_sfmt_beqd;
// OBSOLETE       case 227 : itype = FR30BF_INSN_BNE;goto extract_sfmt_beqd;
// OBSOLETE       case 228 : itype = FR30BF_INSN_BC;goto extract_sfmt_bcd;
// OBSOLETE       case 229 : itype = FR30BF_INSN_BNC;goto extract_sfmt_bcd;
// OBSOLETE       case 230 : itype = FR30BF_INSN_BN;goto extract_sfmt_bnd;
// OBSOLETE       case 231 : itype = FR30BF_INSN_BP;goto extract_sfmt_bnd;
// OBSOLETE       case 232 : itype = FR30BF_INSN_BV;goto extract_sfmt_bvd;
// OBSOLETE       case 233 : itype = FR30BF_INSN_BNV;goto extract_sfmt_bvd;
// OBSOLETE       case 234 : itype = FR30BF_INSN_BLT;goto extract_sfmt_bltd;
// OBSOLETE       case 235 : itype = FR30BF_INSN_BGE;goto extract_sfmt_bltd;
// OBSOLETE       case 236 : itype = FR30BF_INSN_BLE;goto extract_sfmt_bled;
// OBSOLETE       case 237 : itype = FR30BF_INSN_BGT;goto extract_sfmt_bled;
// OBSOLETE       case 238 : itype = FR30BF_INSN_BLS;goto extract_sfmt_blsd;
// OBSOLETE       case 239 : itype = FR30BF_INSN_BHI;goto extract_sfmt_blsd;
// OBSOLETE       case 240 : itype = FR30BF_INSN_BRAD;goto extract_sfmt_brad;
// OBSOLETE       case 241 : itype = FR30BF_INSN_BNOD;goto extract_sfmt_bnod;
// OBSOLETE       case 242 : itype = FR30BF_INSN_BEQD;goto extract_sfmt_beqd;
// OBSOLETE       case 243 : itype = FR30BF_INSN_BNED;goto extract_sfmt_beqd;
// OBSOLETE       case 244 : itype = FR30BF_INSN_BCD;goto extract_sfmt_bcd;
// OBSOLETE       case 245 : itype = FR30BF_INSN_BNCD;goto extract_sfmt_bcd;
// OBSOLETE       case 246 : itype = FR30BF_INSN_BND;goto extract_sfmt_bnd;
// OBSOLETE       case 247 : itype = FR30BF_INSN_BPD;goto extract_sfmt_bnd;
// OBSOLETE       case 248 : itype = FR30BF_INSN_BVD;goto extract_sfmt_bvd;
// OBSOLETE       case 249 : itype = FR30BF_INSN_BNVD;goto extract_sfmt_bvd;
// OBSOLETE       case 250 : itype = FR30BF_INSN_BLTD;goto extract_sfmt_bltd;
// OBSOLETE       case 251 : itype = FR30BF_INSN_BGED;goto extract_sfmt_bltd;
// OBSOLETE       case 252 : itype = FR30BF_INSN_BLED;goto extract_sfmt_bled;
// OBSOLETE       case 253 : itype = FR30BF_INSN_BGTD;goto extract_sfmt_bled;
// OBSOLETE       case 254 : itype = FR30BF_INSN_BLSD;goto extract_sfmt_blsd;
// OBSOLETE       case 255 : itype = FR30BF_INSN_BHID;goto extract_sfmt_blsd;
// OBSOLETE       default : itype = FR30BF_INSN_X_INVALID; goto extract_sfmt_empty;
// OBSOLETE       }
// OBSOLETE     }
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   /* The instruction has been decoded, now extract the fields.  */
// OBSOLETE 
// OBSOLETE  extract_sfmt_empty:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE 
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_empty", (char *) 0));
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_add:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE     UINT f_Rj;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (f_Rj) = f_Rj;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   FLD (i_Rj) = & CPU (h_gr)[f_Rj];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_add", "f_Ri 0x%x", 'x', f_Ri, "f_Rj 0x%x", 'x', f_Rj, "Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE       FLD (in_Rj) = f_Rj;
// OBSOLETE       FLD (out_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_addi:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE     UINT f_u4;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_u4 = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (f_u4) = f_u4;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_addi", "f_Ri 0x%x", 'x', f_Ri, "f_u4 0x%x", 'x', f_u4, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE       FLD (out_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_add2:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add2.f
// OBSOLETE     SI f_m4;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_m4 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 4)) | (((-1) << (4))));
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (f_m4) = f_m4;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_add2", "f_Ri 0x%x", 'x', f_Ri, "f_m4 0x%x", 'x', f_m4, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE       FLD (out_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_addc:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE     UINT f_Rj;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (f_Rj) = f_Rj;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   FLD (i_Rj) = & CPU (h_gr)[f_Rj];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_addc", "f_Ri 0x%x", 'x', f_Ri, "f_Rj 0x%x", 'x', f_Rj, "Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE       FLD (in_Rj) = f_Rj;
// OBSOLETE       FLD (out_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_addn:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE     UINT f_Rj;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (f_Rj) = f_Rj;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   FLD (i_Rj) = & CPU (h_gr)[f_Rj];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_addn", "f_Ri 0x%x", 'x', f_Ri, "f_Rj 0x%x", 'x', f_Rj, "Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE       FLD (in_Rj) = f_Rj;
// OBSOLETE       FLD (out_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_addni:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE     UINT f_u4;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_u4 = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (f_u4) = f_u4;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_addni", "f_Ri 0x%x", 'x', f_Ri, "f_u4 0x%x", 'x', f_u4, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE       FLD (out_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_addn2:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add2.f
// OBSOLETE     SI f_m4;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_m4 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 4)) | (((-1) << (4))));
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (f_m4) = f_m4;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_addn2", "f_Ri 0x%x", 'x', f_Ri, "f_m4 0x%x", 'x', f_m4, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE       FLD (out_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_cmp:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE     UINT f_Rj;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (f_Rj) = f_Rj;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   FLD (i_Rj) = & CPU (h_gr)[f_Rj];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_cmp", "f_Ri 0x%x", 'x', f_Ri, "f_Rj 0x%x", 'x', f_Rj, "Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE       FLD (in_Rj) = f_Rj;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_cmpi:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE     UINT f_u4;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_u4 = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (f_u4) = f_u4;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_cmpi", "f_Ri 0x%x", 'x', f_Ri, "f_u4 0x%x", 'x', f_u4, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_cmp2:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add2.f
// OBSOLETE     SI f_m4;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_m4 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 4)) | (((-1) << (4))));
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (f_m4) = f_m4;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_cmp2", "f_Ri 0x%x", 'x', f_Ri, "f_m4 0x%x", 'x', f_m4, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_and:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE     UINT f_Rj;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (f_Rj) = f_Rj;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   FLD (i_Rj) = & CPU (h_gr)[f_Rj];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_and", "f_Ri 0x%x", 'x', f_Ri, "f_Rj 0x%x", 'x', f_Rj, "Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE       FLD (in_Rj) = f_Rj;
// OBSOLETE       FLD (out_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_andm:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE     UINT f_Rj;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (f_Rj) = f_Rj;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   FLD (i_Rj) = & CPU (h_gr)[f_Rj];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_andm", "f_Ri 0x%x", 'x', f_Ri, "f_Rj 0x%x", 'x', f_Rj, "Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE       FLD (in_Rj) = f_Rj;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_andh:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE     UINT f_Rj;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (f_Rj) = f_Rj;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   FLD (i_Rj) = & CPU (h_gr)[f_Rj];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_andh", "f_Ri 0x%x", 'x', f_Ri, "f_Rj 0x%x", 'x', f_Rj, "Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE       FLD (in_Rj) = f_Rj;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_andb:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE     UINT f_Rj;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (f_Rj) = f_Rj;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   FLD (i_Rj) = & CPU (h_gr)[f_Rj];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_andb", "f_Ri 0x%x", 'x', f_Ri, "f_Rj 0x%x", 'x', f_Rj, "Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE       FLD (in_Rj) = f_Rj;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_bandl:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE     UINT f_u4;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_u4 = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (f_u4) = f_u4;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_bandl", "f_Ri 0x%x", 'x', f_Ri, "f_u4 0x%x", 'x', f_u4, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_btstl:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE     UINT f_u4;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_u4 = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (f_u4) = f_u4;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_btstl", "f_Ri 0x%x", 'x', f_Ri, "f_u4 0x%x", 'x', f_u4, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_mul:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE     UINT f_Rj;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (f_Rj) = f_Rj;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   FLD (i_Rj) = & CPU (h_gr)[f_Rj];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_mul", "f_Ri 0x%x", 'x', f_Ri, "f_Rj 0x%x", 'x', f_Rj, "Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE       FLD (in_Rj) = f_Rj;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_mulu:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE     UINT f_Rj;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (f_Rj) = f_Rj;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   FLD (i_Rj) = & CPU (h_gr)[f_Rj];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_mulu", "f_Ri 0x%x", 'x', f_Ri, "f_Rj 0x%x", 'x', f_Rj, "Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE       FLD (in_Rj) = f_Rj;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_mulh:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE     UINT f_Rj;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (f_Rj) = f_Rj;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   FLD (i_Rj) = & CPU (h_gr)[f_Rj];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_mulh", "f_Ri 0x%x", 'x', f_Ri, "f_Rj 0x%x", 'x', f_Rj, "Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE       FLD (in_Rj) = f_Rj;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_div0s:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_mov2dr.f
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_div0s", "f_Ri 0x%x", 'x', f_Ri, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_div0u:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE 
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_div0u", (char *) 0));
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_div1:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_mov2dr.f
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_div1", "f_Ri 0x%x", 'x', f_Ri, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_div2:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_mov2dr.f
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_div2", "f_Ri 0x%x", 'x', f_Ri, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_div3:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE 
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_div3", (char *) 0));
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_div4s:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE 
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_div4s", (char *) 0));
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_lsl:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE     UINT f_Rj;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (f_Rj) = f_Rj;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   FLD (i_Rj) = & CPU (h_gr)[f_Rj];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_lsl", "f_Ri 0x%x", 'x', f_Ri, "f_Rj 0x%x", 'x', f_Rj, "Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE       FLD (in_Rj) = f_Rj;
// OBSOLETE       FLD (out_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_lsli:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addi.f
// OBSOLETE     UINT f_u4;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_u4 = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (f_u4) = f_u4;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_lsli", "f_Ri 0x%x", 'x', f_Ri, "f_u4 0x%x", 'x', f_u4, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE       FLD (out_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_ldi8:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldi8.f
// OBSOLETE     UINT f_i8;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_i8 = EXTRACT_MSB0_UINT (insn, 16, 4, 8);
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_i8) = f_i8;
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldi8", "f_i8 0x%x", 'x', f_i8, "f_Ri 0x%x", 'x', f_Ri, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (out_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_ldi20:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldi20.f
// OBSOLETE     UINT f_i20_16;
// OBSOLETE     UINT f_i20_4;
// OBSOLETE     UINT f_Ri;
// OBSOLETE     UINT f_i20;
// OBSOLETE     /* Contents of trailing part of insn.  */
// OBSOLETE     UINT word_1;
// OBSOLETE 
// OBSOLETE   word_1 = GETIMEMUHI (current_cpu, pc + 2);
// OBSOLETE     f_i20_16 = (0|(EXTRACT_MSB0_UINT (word_1, 16, 0, 16) << 0));
// OBSOLETE     f_i20_4 = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE {
// OBSOLETE   f_i20 = ((((f_i20_4) << (16))) | (f_i20_16));
// OBSOLETE }
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_i20) = f_i20;
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldi20", "f_i20 0x%x", 'x', f_i20, "f_Ri 0x%x", 'x', f_Ri, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (out_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_ldi32:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldi32.f
// OBSOLETE     UINT f_i32;
// OBSOLETE     UINT f_Ri;
// OBSOLETE     /* Contents of trailing part of insn.  */
// OBSOLETE     UINT word_1;
// OBSOLETE     UINT word_2;
// OBSOLETE 
// OBSOLETE   word_1 = GETIMEMUHI (current_cpu, pc + 2);
// OBSOLETE   word_2 = GETIMEMUHI (current_cpu, pc + 4);
// OBSOLETE     f_i32 = (0|(EXTRACT_MSB0_UINT (word_2, 16, 0, 16) << 0)|(EXTRACT_MSB0_UINT (word_1, 16, 0, 16) << 16));
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_i32) = f_i32;
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldi32", "f_i32 0x%x", 'x', f_i32, "f_Ri 0x%x", 'x', f_Ri, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (out_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_ld:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr13.f
// OBSOLETE     UINT f_Rj;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Rj) = f_Rj;
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (i_Rj) = & CPU (h_gr)[f_Rj];
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ld", "f_Rj 0x%x", 'x', f_Rj, "f_Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Rj) = f_Rj;
// OBSOLETE       FLD (out_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_lduh:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr13.f
// OBSOLETE     UINT f_Rj;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Rj) = f_Rj;
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (i_Rj) = & CPU (h_gr)[f_Rj];
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_lduh", "f_Rj 0x%x", 'x', f_Rj, "f_Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Rj) = f_Rj;
// OBSOLETE       FLD (out_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_ldub:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr13.f
// OBSOLETE     UINT f_Rj;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Rj) = f_Rj;
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (i_Rj) = & CPU (h_gr)[f_Rj];
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldub", "f_Rj 0x%x", 'x', f_Rj, "f_Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Rj) = f_Rj;
// OBSOLETE       FLD (out_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_ldr13:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr13.f
// OBSOLETE     UINT f_Rj;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Rj) = f_Rj;
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (i_Rj) = & CPU (h_gr)[f_Rj];
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldr13", "f_Rj 0x%x", 'x', f_Rj, "f_Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Rj) = f_Rj;
// OBSOLETE       FLD (in_h_gr_SI_13) = 13;
// OBSOLETE       FLD (out_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_ldr13uh:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr13.f
// OBSOLETE     UINT f_Rj;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Rj) = f_Rj;
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (i_Rj) = & CPU (h_gr)[f_Rj];
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldr13uh", "f_Rj 0x%x", 'x', f_Rj, "f_Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Rj) = f_Rj;
// OBSOLETE       FLD (in_h_gr_SI_13) = 13;
// OBSOLETE       FLD (out_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_ldr13ub:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr13.f
// OBSOLETE     UINT f_Rj;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Rj) = f_Rj;
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (i_Rj) = & CPU (h_gr)[f_Rj];
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldr13ub", "f_Rj 0x%x", 'x', f_Rj, "f_Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Rj) = f_Rj;
// OBSOLETE       FLD (in_h_gr_SI_13) = 13;
// OBSOLETE       FLD (out_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_ldr14:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr14.f
// OBSOLETE     SI f_disp10;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_disp10 = ((EXTRACT_MSB0_INT (insn, 16, 4, 8)) << (2));
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_disp10) = f_disp10;
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldr14", "f_disp10 0x%x", 'x', f_disp10, "f_Ri 0x%x", 'x', f_Ri, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_h_gr_SI_14) = 14;
// OBSOLETE       FLD (out_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_ldr14uh:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr14uh.f
// OBSOLETE     SI f_disp9;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_disp9 = ((EXTRACT_MSB0_INT (insn, 16, 4, 8)) << (1));
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_disp9) = f_disp9;
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldr14uh", "f_disp9 0x%x", 'x', f_disp9, "f_Ri 0x%x", 'x', f_Ri, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_h_gr_SI_14) = 14;
// OBSOLETE       FLD (out_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_ldr14ub:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr14ub.f
// OBSOLETE     INT f_disp8;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_disp8 = EXTRACT_MSB0_INT (insn, 16, 4, 8);
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_disp8) = f_disp8;
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldr14ub", "f_disp8 0x%x", 'x', f_disp8, "f_Ri 0x%x", 'x', f_Ri, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_h_gr_SI_14) = 14;
// OBSOLETE       FLD (out_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_ldr15:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr15.f
// OBSOLETE     USI f_udisp6;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_udisp6 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 4)) << (2));
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_udisp6) = f_udisp6;
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldr15", "f_udisp6 0x%x", 'x', f_udisp6, "f_Ri 0x%x", 'x', f_Ri, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_h_gr_SI_15) = 15;
// OBSOLETE       FLD (out_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_ldr15gr:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr15gr.f
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldr15gr", "f_Ri 0x%x", 'x', f_Ri, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_h_gr_SI_15) = 15;
// OBSOLETE       FLD (out_Ri) = f_Ri;
// OBSOLETE       FLD (out_h_gr_SI_15) = 15;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_ldr15dr:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr15dr.f
// OBSOLETE     UINT f_Rs2;
// OBSOLETE 
// OBSOLETE     f_Rs2 = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Rs2) = f_Rs2;
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldr15dr", "f_Rs2 0x%x", 'x', f_Rs2, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_h_gr_SI_15) = 15;
// OBSOLETE       FLD (out_h_gr_SI_15) = 15;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_ldr15ps:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addsp.f
// OBSOLETE 
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldr15ps", (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_h_gr_SI_15) = 15;
// OBSOLETE       FLD (out_h_gr_SI_15) = 15;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_st:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE     UINT f_Rj;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (f_Rj) = f_Rj;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   FLD (i_Rj) = & CPU (h_gr)[f_Rj];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_st", "f_Ri 0x%x", 'x', f_Ri, "f_Rj 0x%x", 'x', f_Rj, "Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE       FLD (in_Rj) = f_Rj;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_sth:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE     UINT f_Rj;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (f_Rj) = f_Rj;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   FLD (i_Rj) = & CPU (h_gr)[f_Rj];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_sth", "f_Ri 0x%x", 'x', f_Ri, "f_Rj 0x%x", 'x', f_Rj, "Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE       FLD (in_Rj) = f_Rj;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_stb:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE     UINT f_Rj;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (f_Rj) = f_Rj;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   FLD (i_Rj) = & CPU (h_gr)[f_Rj];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_stb", "f_Ri 0x%x", 'x', f_Ri, "f_Rj 0x%x", 'x', f_Rj, "Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE       FLD (in_Rj) = f_Rj;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_str13:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE     UINT f_Rj;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (f_Rj) = f_Rj;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   FLD (i_Rj) = & CPU (h_gr)[f_Rj];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_str13", "f_Ri 0x%x", 'x', f_Ri, "f_Rj 0x%x", 'x', f_Rj, "Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE       FLD (in_Rj) = f_Rj;
// OBSOLETE       FLD (in_h_gr_SI_13) = 13;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_str13h:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE     UINT f_Rj;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (f_Rj) = f_Rj;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   FLD (i_Rj) = & CPU (h_gr)[f_Rj];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_str13h", "f_Ri 0x%x", 'x', f_Ri, "f_Rj 0x%x", 'x', f_Rj, "Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE       FLD (in_Rj) = f_Rj;
// OBSOLETE       FLD (in_h_gr_SI_13) = 13;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_str13b:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str13.f
// OBSOLETE     UINT f_Rj;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (f_Rj) = f_Rj;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   FLD (i_Rj) = & CPU (h_gr)[f_Rj];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_str13b", "f_Ri 0x%x", 'x', f_Ri, "f_Rj 0x%x", 'x', f_Rj, "Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE       FLD (in_Rj) = f_Rj;
// OBSOLETE       FLD (in_h_gr_SI_13) = 13;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_str14:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str14.f
// OBSOLETE     SI f_disp10;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_disp10 = ((EXTRACT_MSB0_INT (insn, 16, 4, 8)) << (2));
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (f_disp10) = f_disp10;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_str14", "f_Ri 0x%x", 'x', f_Ri, "f_disp10 0x%x", 'x', f_disp10, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE       FLD (in_h_gr_SI_14) = 14;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_str14h:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str14h.f
// OBSOLETE     SI f_disp9;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_disp9 = ((EXTRACT_MSB0_INT (insn, 16, 4, 8)) << (1));
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (f_disp9) = f_disp9;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_str14h", "f_Ri 0x%x", 'x', f_Ri, "f_disp9 0x%x", 'x', f_disp9, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE       FLD (in_h_gr_SI_14) = 14;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_str14b:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str14b.f
// OBSOLETE     INT f_disp8;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_disp8 = EXTRACT_MSB0_INT (insn, 16, 4, 8);
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (f_disp8) = f_disp8;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_str14b", "f_Ri 0x%x", 'x', f_Ri, "f_disp8 0x%x", 'x', f_disp8, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE       FLD (in_h_gr_SI_14) = 14;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_str15:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str15.f
// OBSOLETE     USI f_udisp6;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_udisp6 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 4)) << (2));
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (f_udisp6) = f_udisp6;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_str15", "f_Ri 0x%x", 'x', f_Ri, "f_udisp6 0x%x", 'x', f_udisp6, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE       FLD (in_h_gr_SI_15) = 15;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_str15gr:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_str15gr.f
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_str15gr", "f_Ri 0x%x", 'x', f_Ri, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE       FLD (in_h_gr_SI_15) = 15;
// OBSOLETE       FLD (out_h_gr_SI_15) = 15;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_str15dr:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr15dr.f
// OBSOLETE     UINT f_Rs2;
// OBSOLETE 
// OBSOLETE     f_Rs2 = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Rs2) = f_Rs2;
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_str15dr", "f_Rs2 0x%x", 'x', f_Rs2, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_h_gr_SI_15) = 15;
// OBSOLETE       FLD (out_h_gr_SI_15) = 15;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_str15ps:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addsp.f
// OBSOLETE 
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_str15ps", (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_h_gr_SI_15) = 15;
// OBSOLETE       FLD (out_h_gr_SI_15) = 15;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_mov:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldr13.f
// OBSOLETE     UINT f_Rj;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Rj) = f_Rj;
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (i_Rj) = & CPU (h_gr)[f_Rj];
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_mov", "f_Rj 0x%x", 'x', f_Rj, "f_Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Rj) = f_Rj;
// OBSOLETE       FLD (out_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_movdr:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_movdr.f
// OBSOLETE     UINT f_Rs1;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Rs1 = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Rs1) = f_Rs1;
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_movdr", "f_Rs1 0x%x", 'x', f_Rs1, "f_Ri 0x%x", 'x', f_Ri, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (out_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_movps:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_movdr.f
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_movps", "f_Ri 0x%x", 'x', f_Ri, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (out_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_mov2dr:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_mov2dr.f
// OBSOLETE     UINT f_Rs1;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Rs1 = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (f_Rs1) = f_Rs1;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_mov2dr", "f_Ri 0x%x", 'x', f_Ri, "f_Rs1 0x%x", 'x', f_Rs1, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_mov2ps:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_mov2dr.f
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_mov2ps", "f_Ri 0x%x", 'x', f_Ri, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_jmp:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_mov2dr.f
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_jmp", "f_Ri 0x%x", 'x', f_Ri, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_callr:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_mov2dr.f
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_callr", "f_Ri 0x%x", 'x', f_Ri, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_call:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_call.f
// OBSOLETE     SI f_rel12;
// OBSOLETE 
// OBSOLETE     f_rel12 = ((((EXTRACT_MSB0_INT (insn, 16, 5, 11)) << (1))) + (((pc) + (2))));
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (i_label12) = f_rel12;
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_call", "label12 0x%x", 'x', f_rel12, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_ret:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE 
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ret", (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_int:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_int.f
// OBSOLETE     UINT f_u8;
// OBSOLETE 
// OBSOLETE     f_u8 = EXTRACT_MSB0_UINT (insn, 16, 8, 8);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_u8) = f_u8;
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_int", "f_u8 0x%x", 'x', f_u8, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_inte:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE 
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_inte", (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_reti:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE 
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_reti", (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_brad:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE     SI f_rel9;
// OBSOLETE 
// OBSOLETE     f_rel9 = ((((EXTRACT_MSB0_INT (insn, 16, 8, 8)) << (1))) + (((pc) + (2))));
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (i_label9) = f_rel9;
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_brad", "label9 0x%x", 'x', f_rel9, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_bnod:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE 
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_bnod", (char *) 0));
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_beqd:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE     SI f_rel9;
// OBSOLETE 
// OBSOLETE     f_rel9 = ((((EXTRACT_MSB0_INT (insn, 16, 8, 8)) << (1))) + (((pc) + (2))));
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (i_label9) = f_rel9;
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_beqd", "label9 0x%x", 'x', f_rel9, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_bcd:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE     SI f_rel9;
// OBSOLETE 
// OBSOLETE     f_rel9 = ((((EXTRACT_MSB0_INT (insn, 16, 8, 8)) << (1))) + (((pc) + (2))));
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (i_label9) = f_rel9;
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_bcd", "label9 0x%x", 'x', f_rel9, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_bnd:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE     SI f_rel9;
// OBSOLETE 
// OBSOLETE     f_rel9 = ((((EXTRACT_MSB0_INT (insn, 16, 8, 8)) << (1))) + (((pc) + (2))));
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (i_label9) = f_rel9;
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_bnd", "label9 0x%x", 'x', f_rel9, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_bvd:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE     SI f_rel9;
// OBSOLETE 
// OBSOLETE     f_rel9 = ((((EXTRACT_MSB0_INT (insn, 16, 8, 8)) << (1))) + (((pc) + (2))));
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (i_label9) = f_rel9;
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_bvd", "label9 0x%x", 'x', f_rel9, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_bltd:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE     SI f_rel9;
// OBSOLETE 
// OBSOLETE     f_rel9 = ((((EXTRACT_MSB0_INT (insn, 16, 8, 8)) << (1))) + (((pc) + (2))));
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (i_label9) = f_rel9;
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_bltd", "label9 0x%x", 'x', f_rel9, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_bled:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE     SI f_rel9;
// OBSOLETE 
// OBSOLETE     f_rel9 = ((((EXTRACT_MSB0_INT (insn, 16, 8, 8)) << (1))) + (((pc) + (2))));
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (i_label9) = f_rel9;
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_bled", "label9 0x%x", 'x', f_rel9, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_blsd:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_brad.f
// OBSOLETE     SI f_rel9;
// OBSOLETE 
// OBSOLETE     f_rel9 = ((((EXTRACT_MSB0_INT (insn, 16, 8, 8)) << (1))) + (((pc) + (2))));
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (i_label9) = f_rel9;
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_blsd", "label9 0x%x", 'x', f_rel9, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_dmovr13:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pi.f
// OBSOLETE     USI f_dir10;
// OBSOLETE 
// OBSOLETE     f_dir10 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 8)) << (2));
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_dir10) = f_dir10;
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_dmovr13", "f_dir10 0x%x", 'x', f_dir10, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_h_gr_SI_13) = 13;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_dmovr13h:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pih.f
// OBSOLETE     USI f_dir9;
// OBSOLETE 
// OBSOLETE     f_dir9 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 8)) << (1));
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_dir9) = f_dir9;
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_dmovr13h", "f_dir9 0x%x", 'x', f_dir9, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_h_gr_SI_13) = 13;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_dmovr13b:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pib.f
// OBSOLETE     UINT f_dir8;
// OBSOLETE 
// OBSOLETE     f_dir8 = EXTRACT_MSB0_UINT (insn, 16, 8, 8);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_dir8) = f_dir8;
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_dmovr13b", "f_dir8 0x%x", 'x', f_dir8, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_h_gr_SI_13) = 13;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_dmovr13pi:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pi.f
// OBSOLETE     USI f_dir10;
// OBSOLETE 
// OBSOLETE     f_dir10 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 8)) << (2));
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_dir10) = f_dir10;
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_dmovr13pi", "f_dir10 0x%x", 'x', f_dir10, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_h_gr_SI_13) = 13;
// OBSOLETE       FLD (out_h_gr_SI_13) = 13;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_dmovr13pih:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pih.f
// OBSOLETE     USI f_dir9;
// OBSOLETE 
// OBSOLETE     f_dir9 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 8)) << (1));
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_dir9) = f_dir9;
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_dmovr13pih", "f_dir9 0x%x", 'x', f_dir9, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_h_gr_SI_13) = 13;
// OBSOLETE       FLD (out_h_gr_SI_13) = 13;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_dmovr13pib:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pib.f
// OBSOLETE     UINT f_dir8;
// OBSOLETE 
// OBSOLETE     f_dir8 = EXTRACT_MSB0_UINT (insn, 16, 8, 8);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_dir8) = f_dir8;
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_dmovr13pib", "f_dir8 0x%x", 'x', f_dir8, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_h_gr_SI_13) = 13;
// OBSOLETE       FLD (out_h_gr_SI_13) = 13;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_dmovr15pi:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr15pi.f
// OBSOLETE     USI f_dir10;
// OBSOLETE 
// OBSOLETE     f_dir10 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 8)) << (2));
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_dir10) = f_dir10;
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_dmovr15pi", "f_dir10 0x%x", 'x', f_dir10, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_h_gr_SI_15) = 15;
// OBSOLETE       FLD (out_h_gr_SI_15) = 15;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_dmov2r13:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pi.f
// OBSOLETE     USI f_dir10;
// OBSOLETE 
// OBSOLETE     f_dir10 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 8)) << (2));
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_dir10) = f_dir10;
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_dmov2r13", "f_dir10 0x%x", 'x', f_dir10, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (out_h_gr_SI_13) = 13;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_dmov2r13h:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pih.f
// OBSOLETE     USI f_dir9;
// OBSOLETE 
// OBSOLETE     f_dir9 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 8)) << (1));
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_dir9) = f_dir9;
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_dmov2r13h", "f_dir9 0x%x", 'x', f_dir9, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (out_h_gr_SI_13) = 13;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_dmov2r13b:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pib.f
// OBSOLETE     UINT f_dir8;
// OBSOLETE 
// OBSOLETE     f_dir8 = EXTRACT_MSB0_UINT (insn, 16, 8, 8);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_dir8) = f_dir8;
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_dmov2r13b", "f_dir8 0x%x", 'x', f_dir8, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (out_h_gr_SI_13) = 13;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_dmov2r13pi:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pi.f
// OBSOLETE     USI f_dir10;
// OBSOLETE 
// OBSOLETE     f_dir10 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 8)) << (2));
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_dir10) = f_dir10;
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_dmov2r13pi", "f_dir10 0x%x", 'x', f_dir10, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_h_gr_SI_13) = 13;
// OBSOLETE       FLD (out_h_gr_SI_13) = 13;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_dmov2r13pih:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pih.f
// OBSOLETE     USI f_dir9;
// OBSOLETE 
// OBSOLETE     f_dir9 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 8)) << (1));
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_dir9) = f_dir9;
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_dmov2r13pih", "f_dir9 0x%x", 'x', f_dir9, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_h_gr_SI_13) = 13;
// OBSOLETE       FLD (out_h_gr_SI_13) = 13;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_dmov2r13pib:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr13pib.f
// OBSOLETE     UINT f_dir8;
// OBSOLETE 
// OBSOLETE     f_dir8 = EXTRACT_MSB0_UINT (insn, 16, 8, 8);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_dir8) = f_dir8;
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_dmov2r13pib", "f_dir8 0x%x", 'x', f_dir8, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_h_gr_SI_13) = 13;
// OBSOLETE       FLD (out_h_gr_SI_13) = 13;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_dmov2r15pd:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_dmovr15pi.f
// OBSOLETE     USI f_dir10;
// OBSOLETE 
// OBSOLETE     f_dir10 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 8)) << (2));
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_dir10) = f_dir10;
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_dmov2r15pd", "f_dir10 0x%x", 'x', f_dir10, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_h_gr_SI_15) = 15;
// OBSOLETE       FLD (out_h_gr_SI_15) = 15;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_ldres:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add2.f
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldres", "f_Ri 0x%x", 'x', f_Ri, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE       FLD (out_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_copop:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE #define FLD(f) abuf->fields.fmt_empty.f
// OBSOLETE     /* Contents of trailing part of insn.  */
// OBSOLETE     UINT word_1;
// OBSOLETE 
// OBSOLETE   word_1 = GETIMEMUHI (current_cpu, pc + 2);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_copop", (char *) 0));
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_andccr:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_int.f
// OBSOLETE     UINT f_u8;
// OBSOLETE 
// OBSOLETE     f_u8 = EXTRACT_MSB0_UINT (insn, 16, 8, 8);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_u8) = f_u8;
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_andccr", "f_u8 0x%x", 'x', f_u8, (char *) 0));
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_stilm:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_int.f
// OBSOLETE     UINT f_u8;
// OBSOLETE 
// OBSOLETE     f_u8 = EXTRACT_MSB0_UINT (insn, 16, 8, 8);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_u8) = f_u8;
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_stilm", "f_u8 0x%x", 'x', f_u8, (char *) 0));
// OBSOLETE 
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_addsp:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_addsp.f
// OBSOLETE     SI f_s10;
// OBSOLETE 
// OBSOLETE     f_s10 = ((EXTRACT_MSB0_INT (insn, 16, 8, 8)) << (2));
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_s10) = f_s10;
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_addsp", "f_s10 0x%x", 'x', f_s10, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_h_gr_SI_15) = 15;
// OBSOLETE       FLD (out_h_gr_SI_15) = 15;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_extsb:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add2.f
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_extsb", "f_Ri 0x%x", 'x', f_Ri, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE       FLD (out_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_extub:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add2.f
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_extub", "f_Ri 0x%x", 'x', f_Ri, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE       FLD (out_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_extsh:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add2.f
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_extsh", "f_Ri 0x%x", 'x', f_Ri, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE       FLD (out_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_extuh:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add2.f
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_extuh", "f_Ri 0x%x", 'x', f_Ri, "Ri 0x%x", 'x', f_Ri, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE       FLD (out_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_ldm0:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldm0.f
// OBSOLETE     UINT f_reglist_low_ld;
// OBSOLETE 
// OBSOLETE     f_reglist_low_ld = EXTRACT_MSB0_UINT (insn, 16, 8, 8);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_reglist_low_ld) = f_reglist_low_ld;
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldm0", "f_reglist_low_ld 0x%x", 'x', f_reglist_low_ld, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_h_gr_SI_15) = 15;
// OBSOLETE       FLD (out_h_gr_SI_0) = 0;
// OBSOLETE       FLD (out_h_gr_SI_1) = 1;
// OBSOLETE       FLD (out_h_gr_SI_15) = 15;
// OBSOLETE       FLD (out_h_gr_SI_2) = 2;
// OBSOLETE       FLD (out_h_gr_SI_3) = 3;
// OBSOLETE       FLD (out_h_gr_SI_4) = 4;
// OBSOLETE       FLD (out_h_gr_SI_5) = 5;
// OBSOLETE       FLD (out_h_gr_SI_6) = 6;
// OBSOLETE       FLD (out_h_gr_SI_7) = 7;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_ldm1:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_ldm1.f
// OBSOLETE     UINT f_reglist_hi_ld;
// OBSOLETE 
// OBSOLETE     f_reglist_hi_ld = EXTRACT_MSB0_UINT (insn, 16, 8, 8);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_reglist_hi_ld) = f_reglist_hi_ld;
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldm1", "f_reglist_hi_ld 0x%x", 'x', f_reglist_hi_ld, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_h_gr_SI_15) = 15;
// OBSOLETE       FLD (out_h_gr_SI_10) = 10;
// OBSOLETE       FLD (out_h_gr_SI_11) = 11;
// OBSOLETE       FLD (out_h_gr_SI_12) = 12;
// OBSOLETE       FLD (out_h_gr_SI_13) = 13;
// OBSOLETE       FLD (out_h_gr_SI_14) = 14;
// OBSOLETE       FLD (out_h_gr_SI_15) = 15;
// OBSOLETE       FLD (out_h_gr_SI_8) = 8;
// OBSOLETE       FLD (out_h_gr_SI_9) = 9;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_stm0:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_stm0.f
// OBSOLETE     UINT f_reglist_low_st;
// OBSOLETE 
// OBSOLETE     f_reglist_low_st = EXTRACT_MSB0_UINT (insn, 16, 8, 8);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_reglist_low_st) = f_reglist_low_st;
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_stm0", "f_reglist_low_st 0x%x", 'x', f_reglist_low_st, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_h_gr_SI_0) = 0;
// OBSOLETE       FLD (in_h_gr_SI_1) = 1;
// OBSOLETE       FLD (in_h_gr_SI_15) = 15;
// OBSOLETE       FLD (in_h_gr_SI_2) = 2;
// OBSOLETE       FLD (in_h_gr_SI_3) = 3;
// OBSOLETE       FLD (in_h_gr_SI_4) = 4;
// OBSOLETE       FLD (in_h_gr_SI_5) = 5;
// OBSOLETE       FLD (in_h_gr_SI_6) = 6;
// OBSOLETE       FLD (in_h_gr_SI_7) = 7;
// OBSOLETE       FLD (out_h_gr_SI_15) = 15;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_stm1:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_stm1.f
// OBSOLETE     UINT f_reglist_hi_st;
// OBSOLETE 
// OBSOLETE     f_reglist_hi_st = EXTRACT_MSB0_UINT (insn, 16, 8, 8);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_reglist_hi_st) = f_reglist_hi_st;
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_stm1", "f_reglist_hi_st 0x%x", 'x', f_reglist_hi_st, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_h_gr_SI_10) = 10;
// OBSOLETE       FLD (in_h_gr_SI_11) = 11;
// OBSOLETE       FLD (in_h_gr_SI_12) = 12;
// OBSOLETE       FLD (in_h_gr_SI_13) = 13;
// OBSOLETE       FLD (in_h_gr_SI_14) = 14;
// OBSOLETE       FLD (in_h_gr_SI_15) = 15;
// OBSOLETE       FLD (in_h_gr_SI_8) = 8;
// OBSOLETE       FLD (in_h_gr_SI_9) = 9;
// OBSOLETE       FLD (out_h_gr_SI_15) = 15;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_enter:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_enter.f
// OBSOLETE     USI f_u10;
// OBSOLETE 
// OBSOLETE     f_u10 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 8)) << (2));
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_u10) = f_u10;
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_enter", "f_u10 0x%x", 'x', f_u10, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_h_gr_SI_14) = 14;
// OBSOLETE       FLD (in_h_gr_SI_15) = 15;
// OBSOLETE       FLD (out_h_gr_SI_14) = 14;
// OBSOLETE       FLD (out_h_gr_SI_15) = 15;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_leave:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE #define FLD(f) abuf->fields.sfmt_enter.f
// OBSOLETE 
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_leave", (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_h_gr_SI_14) = 14;
// OBSOLETE       FLD (in_h_gr_SI_15) = 15;
// OBSOLETE       FLD (out_h_gr_SI_14) = 14;
// OBSOLETE       FLD (out_h_gr_SI_15) = 15;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE  extract_sfmt_xchb:
// OBSOLETE   {
// OBSOLETE     const IDESC *idesc = &fr30bf_insn_data[itype];
// OBSOLETE     CGEN_INSN_INT insn = base_insn;
// OBSOLETE #define FLD(f) abuf->fields.sfmt_add.f
// OBSOLETE     UINT f_Rj;
// OBSOLETE     UINT f_Ri;
// OBSOLETE 
// OBSOLETE     f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
// OBSOLETE     f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
// OBSOLETE 
// OBSOLETE   /* Record the fields for the semantic handler.  */
// OBSOLETE   FLD (f_Ri) = f_Ri;
// OBSOLETE   FLD (f_Rj) = f_Rj;
// OBSOLETE   FLD (i_Ri) = & CPU (h_gr)[f_Ri];
// OBSOLETE   FLD (i_Rj) = & CPU (h_gr)[f_Rj];
// OBSOLETE   TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_xchb", "f_Ri 0x%x", 'x', f_Ri, "f_Rj 0x%x", 'x', f_Rj, "Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, (char *) 0));
// OBSOLETE 
// OBSOLETE #if WITH_PROFILE_MODEL_P
// OBSOLETE   /* Record the fields for profiling.  */
// OBSOLETE   if (PROFILE_MODEL_P (current_cpu))
// OBSOLETE     {
// OBSOLETE       FLD (in_Ri) = f_Ri;
// OBSOLETE       FLD (in_Rj) = f_Rj;
// OBSOLETE       FLD (out_Ri) = f_Ri;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE #undef FLD
// OBSOLETE     return idesc;
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE }
