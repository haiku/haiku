/* OBSOLETE /* Mitsubishi Electric Corp. D30V Simulator. */
/* OBSOLETE    Copyright (C) 1997, Free Software Foundation, Inc. */
/* OBSOLETE    Contributed by Cygnus Support. */
/* OBSOLETE  */
/* OBSOLETE This file is part of GDB, the GNU debugger. */
/* OBSOLETE  */
/* OBSOLETE This program is free software; you can redistribute it and/or modify */
/* OBSOLETE it under the terms of the GNU General Public License as published by */
/* OBSOLETE the Free Software Foundation; either version 2, or (at your option) */
/* OBSOLETE any later version. */
/* OBSOLETE  */
/* OBSOLETE This program is distributed in the hope that it will be useful, */
/* OBSOLETE but WITHOUT ANY WARRANTY; without even the implied warranty of */
/* OBSOLETE MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the */
/* OBSOLETE GNU General Public License for more details. */
/* OBSOLETE  */
/* OBSOLETE You should have received a copy of the GNU General Public License along */
/* OBSOLETE with this program; if not, write to the Free Software Foundation, Inc., */
/* OBSOLETE 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */ */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE #ifndef _D30V_ALU_H_ */
/* OBSOLETE #define _D30V_ALU_H_ */
/* OBSOLETE  */
/* OBSOLETE #define ALU_CARRY (PSW_VAL(PSW_C) != 0) */
/* OBSOLETE  */
/* OBSOLETE #include "sim-alu.h" */
/* OBSOLETE  */
/* OBSOLETE #define ALU16_END(TARG, HIGH)						\ */
/* OBSOLETE {									\ */
/* OBSOLETE   unsigned32 mask, value;						\ */
/* OBSOLETE   if (ALU16_HAD_OVERFLOW) {						\ */
/* OBSOLETE     mask = BIT32 (PSW_V) | BIT32 (PSW_VA) | BIT32 (PSW_C);		\ */
/* OBSOLETE     value = BIT32 (PSW_V) | BIT32 (PSW_VA);				\ */
/* OBSOLETE   }									\ */
/* OBSOLETE   else {								\ */
/* OBSOLETE     mask = BIT32 (PSW_V) | BIT32 (PSW_C);				\ */
/* OBSOLETE     value = 0;								\ */
/* OBSOLETE   }									\ */
/* OBSOLETE   if (ALU16_HAD_CARRY_BORROW)						\ */
/* OBSOLETE     value |= BIT32 (PSW_C);						\ */
/* OBSOLETE   if (HIGH)								\ */
/* OBSOLETE     WRITE32_QUEUE_MASK (TARG, ALU16_OVERFLOW_RESULT<<16, 0xffff0000);	\ */
/* OBSOLETE   else									\ */
/* OBSOLETE     WRITE32_QUEUE_MASK (TARG, ALU16_OVERFLOW_RESULT, 0x0000ffff);	\ */
/* OBSOLETE   WRITE32_QUEUE_MASK (&PSW, value, mask);				\ */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE #define ALU32_END(TARG)							\ */
/* OBSOLETE {									\ */
/* OBSOLETE   unsigned32 mask, value;						\ */
/* OBSOLETE   if (ALU32_HAD_OVERFLOW) {						\ */
/* OBSOLETE     mask = BIT32 (PSW_V) | BIT32 (PSW_VA) | BIT32 (PSW_C);		\ */
/* OBSOLETE     value = BIT32 (PSW_V) | BIT32 (PSW_VA);				\ */
/* OBSOLETE   }									\ */
/* OBSOLETE   else {								\ */
/* OBSOLETE     mask = BIT32 (PSW_V) | BIT32 (PSW_C);				\ */
/* OBSOLETE     value = 0;								\ */
/* OBSOLETE   }									\ */
/* OBSOLETE   if (ALU32_HAD_CARRY_BORROW)						\ */
/* OBSOLETE     value |= BIT32 (PSW_C);						\ */
/* OBSOLETE   WRITE32_QUEUE (TARG, ALU32_OVERFLOW_RESULT);				\ */
/* OBSOLETE   WRITE32_QUEUE_MASK (&PSW, value, mask);				\ */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE #define ALU_END(TARG) ALU32_END(TARG) */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE /* PSW & Flag manipulation */ */
/* OBSOLETE  */
/* OBSOLETE #define PSW_SET(BIT,VAL) BLIT32(PSW, (BIT), (VAL)) */
/* OBSOLETE #define PSW_VAL(BIT) EXTRACTED32(PSW, (BIT), (BIT)) */
/* OBSOLETE  */
/* OBSOLETE #define PSW_F(FLAG) (17 + ((FLAG) % 8) * 2) */
/* OBSOLETE #define PSW_FLAG_SET(FLAG,VAL) PSW_SET(PSW_F(FLAG), VAL) */
/* OBSOLETE #define PSW_FLAG_VAL(FLAG) PSW_VAL(PSW_F(FLAG)) */
/* OBSOLETE  */
/* OBSOLETE #define PSW_SET_QUEUE(BIT,VAL)						\ */
/* OBSOLETE do {									\ */
/* OBSOLETE   unsigned32 mask = BIT32 (BIT);					\ */
/* OBSOLETE   unsigned32 bitval = (VAL) ? mask : 0;					\ */
/* OBSOLETE   WRITE32_QUEUE_MASK (&PSW, bitval, mask);				\ */
/* OBSOLETE } while (0) */
/* OBSOLETE  */
/* OBSOLETE #define PSW_FLAG_SET_QUEUE(FLAG,VAL)					\ */
/* OBSOLETE do {									\ */
/* OBSOLETE   unsigned32 mask = BIT32 (PSW_F (FLAG));				\ */
/* OBSOLETE   unsigned32 bitval = (VAL) ? mask : 0;					\ */
/* OBSOLETE   WRITE32_QUEUE_MASK (&PSW, bitval, mask);				\ */
/* OBSOLETE } while (0) */
/* OBSOLETE  */
/* OBSOLETE /* Bring data in from the cold */ */
/* OBSOLETE  */
/* OBSOLETE #define IMEM(EA) \ */
/* OBSOLETE (sim_core_read_8(STATE_CPU (sd, 0), cia, exec_map, (EA))) */
/* OBSOLETE  */
/* OBSOLETE #define MEM(SIGN, EA, NR_BYTES) \ */
/* OBSOLETE ((SIGN##_##NR_BYTES) sim_core_read_unaligned_##NR_BYTES(STATE_CPU (sd, 0), cia, read_map, (EA))) */
/* OBSOLETE  */
/* OBSOLETE #define STORE(EA, NR_BYTES, VAL) \ */
/* OBSOLETE do { \ */
/* OBSOLETE   sim_core_write_unaligned_##NR_BYTES(STATE_CPU (sd, 0), cia, write_map, (EA), (VAL)); \ */
/* OBSOLETE } while (0) */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE #endif */
