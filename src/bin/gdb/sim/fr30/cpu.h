// OBSOLETE /* CPU family header for fr30bf.
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
// OBSOLETE #ifndef CPU_FR30BF_H
// OBSOLETE #define CPU_FR30BF_H
// OBSOLETE 
// OBSOLETE /* Maximum number of instructions that are fetched at a time.
// OBSOLETE    This is for LIW type instructions sets (e.g. m32r).  */
// OBSOLETE #define MAX_LIW_INSNS 1
// OBSOLETE 
// OBSOLETE /* Maximum number of instructions that can be executed in parallel.  */
// OBSOLETE #define MAX_PARALLEL_INSNS 1
// OBSOLETE 
// OBSOLETE /* CPU state information.  */
// OBSOLETE typedef struct {
// OBSOLETE   /* Hardware elements.  */
// OBSOLETE   struct {
// OBSOLETE   /* program counter */
// OBSOLETE   USI h_pc;
// OBSOLETE #define GET_H_PC() CPU (h_pc)
// OBSOLETE #define SET_H_PC(x) (CPU (h_pc) = (x))
// OBSOLETE   /* general registers */
// OBSOLETE   SI h_gr[16];
// OBSOLETE #define GET_H_GR(a1) CPU (h_gr)[a1]
// OBSOLETE #define SET_H_GR(a1, x) (CPU (h_gr)[a1] = (x))
// OBSOLETE   /* coprocessor registers */
// OBSOLETE   SI h_cr[16];
// OBSOLETE #define GET_H_CR(a1) CPU (h_cr)[a1]
// OBSOLETE #define SET_H_CR(a1, x) (CPU (h_cr)[a1] = (x))
// OBSOLETE   /* dedicated registers */
// OBSOLETE   SI h_dr[6];
// OBSOLETE #define GET_H_DR(index) fr30bf_h_dr_get_handler (current_cpu, index)
// OBSOLETE #define SET_H_DR(index, x) \
// OBSOLETE do { \
// OBSOLETE fr30bf_h_dr_set_handler (current_cpu, (index), (x));\
// OBSOLETE ;} while (0)
// OBSOLETE   /* processor status */
// OBSOLETE   USI h_ps;
// OBSOLETE #define GET_H_PS() fr30bf_h_ps_get_handler (current_cpu)
// OBSOLETE #define SET_H_PS(x) \
// OBSOLETE do { \
// OBSOLETE fr30bf_h_ps_set_handler (current_cpu, (x));\
// OBSOLETE ;} while (0)
// OBSOLETE   /* General Register 13 explicitly required */
// OBSOLETE   SI h_r13;
// OBSOLETE #define GET_H_R13() CPU (h_r13)
// OBSOLETE #define SET_H_R13(x) (CPU (h_r13) = (x))
// OBSOLETE   /* General Register 14 explicitly required */
// OBSOLETE   SI h_r14;
// OBSOLETE #define GET_H_R14() CPU (h_r14)
// OBSOLETE #define SET_H_R14(x) (CPU (h_r14) = (x))
// OBSOLETE   /* General Register 15 explicitly required */
// OBSOLETE   SI h_r15;
// OBSOLETE #define GET_H_R15() CPU (h_r15)
// OBSOLETE #define SET_H_R15(x) (CPU (h_r15) = (x))
// OBSOLETE   /* negative         bit */
// OBSOLETE   BI h_nbit;
// OBSOLETE #define GET_H_NBIT() CPU (h_nbit)
// OBSOLETE #define SET_H_NBIT(x) (CPU (h_nbit) = (x))
// OBSOLETE   /* zero             bit */
// OBSOLETE   BI h_zbit;
// OBSOLETE #define GET_H_ZBIT() CPU (h_zbit)
// OBSOLETE #define SET_H_ZBIT(x) (CPU (h_zbit) = (x))
// OBSOLETE   /* overflow         bit */
// OBSOLETE   BI h_vbit;
// OBSOLETE #define GET_H_VBIT() CPU (h_vbit)
// OBSOLETE #define SET_H_VBIT(x) (CPU (h_vbit) = (x))
// OBSOLETE   /* carry            bit */
// OBSOLETE   BI h_cbit;
// OBSOLETE #define GET_H_CBIT() CPU (h_cbit)
// OBSOLETE #define SET_H_CBIT(x) (CPU (h_cbit) = (x))
// OBSOLETE   /* interrupt enable bit */
// OBSOLETE   BI h_ibit;
// OBSOLETE #define GET_H_IBIT() CPU (h_ibit)
// OBSOLETE #define SET_H_IBIT(x) (CPU (h_ibit) = (x))
// OBSOLETE   /* stack bit */
// OBSOLETE   BI h_sbit;
// OBSOLETE #define GET_H_SBIT() fr30bf_h_sbit_get_handler (current_cpu)
// OBSOLETE #define SET_H_SBIT(x) \
// OBSOLETE do { \
// OBSOLETE fr30bf_h_sbit_set_handler (current_cpu, (x));\
// OBSOLETE ;} while (0)
// OBSOLETE   /* trace trap       bit */
// OBSOLETE   BI h_tbit;
// OBSOLETE #define GET_H_TBIT() CPU (h_tbit)
// OBSOLETE #define SET_H_TBIT(x) (CPU (h_tbit) = (x))
// OBSOLETE   /* division 0       bit */
// OBSOLETE   BI h_d0bit;
// OBSOLETE #define GET_H_D0BIT() CPU (h_d0bit)
// OBSOLETE #define SET_H_D0BIT(x) (CPU (h_d0bit) = (x))
// OBSOLETE   /* division 1       bit */
// OBSOLETE   BI h_d1bit;
// OBSOLETE #define GET_H_D1BIT() CPU (h_d1bit)
// OBSOLETE #define SET_H_D1BIT(x) (CPU (h_d1bit) = (x))
// OBSOLETE   /* condition code bits */
// OBSOLETE   UQI h_ccr;
// OBSOLETE #define GET_H_CCR() fr30bf_h_ccr_get_handler (current_cpu)
// OBSOLETE #define SET_H_CCR(x) \
// OBSOLETE do { \
// OBSOLETE fr30bf_h_ccr_set_handler (current_cpu, (x));\
// OBSOLETE ;} while (0)
// OBSOLETE   /* system condition bits */
// OBSOLETE   UQI h_scr;
// OBSOLETE #define GET_H_SCR() fr30bf_h_scr_get_handler (current_cpu)
// OBSOLETE #define SET_H_SCR(x) \
// OBSOLETE do { \
// OBSOLETE fr30bf_h_scr_set_handler (current_cpu, (x));\
// OBSOLETE ;} while (0)
// OBSOLETE   /* interrupt level mask */
// OBSOLETE   UQI h_ilm;
// OBSOLETE #define GET_H_ILM() fr30bf_h_ilm_get_handler (current_cpu)
// OBSOLETE #define SET_H_ILM(x) \
// OBSOLETE do { \
// OBSOLETE fr30bf_h_ilm_set_handler (current_cpu, (x));\
// OBSOLETE ;} while (0)
// OBSOLETE   } hardware;
// OBSOLETE #define CPU_CGEN_HW(cpu) (& (cpu)->cpu_data.hardware)
// OBSOLETE } FR30BF_CPU_DATA;
// OBSOLETE 
// OBSOLETE /* Cover fns for register access.  */
// OBSOLETE USI fr30bf_h_pc_get (SIM_CPU *);
// OBSOLETE void fr30bf_h_pc_set (SIM_CPU *, USI);
// OBSOLETE SI fr30bf_h_gr_get (SIM_CPU *, UINT);
// OBSOLETE void fr30bf_h_gr_set (SIM_CPU *, UINT, SI);
// OBSOLETE SI fr30bf_h_cr_get (SIM_CPU *, UINT);
// OBSOLETE void fr30bf_h_cr_set (SIM_CPU *, UINT, SI);
// OBSOLETE SI fr30bf_h_dr_get (SIM_CPU *, UINT);
// OBSOLETE void fr30bf_h_dr_set (SIM_CPU *, UINT, SI);
// OBSOLETE USI fr30bf_h_ps_get (SIM_CPU *);
// OBSOLETE void fr30bf_h_ps_set (SIM_CPU *, USI);
// OBSOLETE SI fr30bf_h_r13_get (SIM_CPU *);
// OBSOLETE void fr30bf_h_r13_set (SIM_CPU *, SI);
// OBSOLETE SI fr30bf_h_r14_get (SIM_CPU *);
// OBSOLETE void fr30bf_h_r14_set (SIM_CPU *, SI);
// OBSOLETE SI fr30bf_h_r15_get (SIM_CPU *);
// OBSOLETE void fr30bf_h_r15_set (SIM_CPU *, SI);
// OBSOLETE BI fr30bf_h_nbit_get (SIM_CPU *);
// OBSOLETE void fr30bf_h_nbit_set (SIM_CPU *, BI);
// OBSOLETE BI fr30bf_h_zbit_get (SIM_CPU *);
// OBSOLETE void fr30bf_h_zbit_set (SIM_CPU *, BI);
// OBSOLETE BI fr30bf_h_vbit_get (SIM_CPU *);
// OBSOLETE void fr30bf_h_vbit_set (SIM_CPU *, BI);
// OBSOLETE BI fr30bf_h_cbit_get (SIM_CPU *);
// OBSOLETE void fr30bf_h_cbit_set (SIM_CPU *, BI);
// OBSOLETE BI fr30bf_h_ibit_get (SIM_CPU *);
// OBSOLETE void fr30bf_h_ibit_set (SIM_CPU *, BI);
// OBSOLETE BI fr30bf_h_sbit_get (SIM_CPU *);
// OBSOLETE void fr30bf_h_sbit_set (SIM_CPU *, BI);
// OBSOLETE BI fr30bf_h_tbit_get (SIM_CPU *);
// OBSOLETE void fr30bf_h_tbit_set (SIM_CPU *, BI);
// OBSOLETE BI fr30bf_h_d0bit_get (SIM_CPU *);
// OBSOLETE void fr30bf_h_d0bit_set (SIM_CPU *, BI);
// OBSOLETE BI fr30bf_h_d1bit_get (SIM_CPU *);
// OBSOLETE void fr30bf_h_d1bit_set (SIM_CPU *, BI);
// OBSOLETE UQI fr30bf_h_ccr_get (SIM_CPU *);
// OBSOLETE void fr30bf_h_ccr_set (SIM_CPU *, UQI);
// OBSOLETE UQI fr30bf_h_scr_get (SIM_CPU *);
// OBSOLETE void fr30bf_h_scr_set (SIM_CPU *, UQI);
// OBSOLETE UQI fr30bf_h_ilm_get (SIM_CPU *);
// OBSOLETE void fr30bf_h_ilm_set (SIM_CPU *, UQI);
// OBSOLETE 
// OBSOLETE /* These must be hand-written.  */
// OBSOLETE extern CPUREG_FETCH_FN fr30bf_fetch_register;
// OBSOLETE extern CPUREG_STORE_FN fr30bf_store_register;
// OBSOLETE 
// OBSOLETE typedef struct {
// OBSOLETE   UINT load_regs;
// OBSOLETE   UINT load_regs_pending;
// OBSOLETE } MODEL_FR30_1_DATA;
// OBSOLETE 
// OBSOLETE /* Instruction argument buffer.  */
// OBSOLETE 
// OBSOLETE union sem_fields {
// OBSOLETE   struct { /* no operands */
// OBSOLETE     int empty;
// OBSOLETE   } fmt_empty;
// OBSOLETE   struct { /*  */
// OBSOLETE     IADDR i_label9;
// OBSOLETE   } sfmt_brad;
// OBSOLETE   struct { /*  */
// OBSOLETE     UINT f_u8;
// OBSOLETE   } sfmt_int;
// OBSOLETE   struct { /*  */
// OBSOLETE     IADDR i_label12;
// OBSOLETE   } sfmt_call;
// OBSOLETE   struct { /*  */
// OBSOLETE     SI f_s10;
// OBSOLETE     unsigned char in_h_gr_SI_15;
// OBSOLETE     unsigned char out_h_gr_SI_15;
// OBSOLETE   } sfmt_addsp;
// OBSOLETE   struct { /*  */
// OBSOLETE     USI f_dir10;
// OBSOLETE     unsigned char in_h_gr_SI_15;
// OBSOLETE     unsigned char out_h_gr_SI_15;
// OBSOLETE   } sfmt_dmovr15pi;
// OBSOLETE   struct { /*  */
// OBSOLETE     UINT f_dir8;
// OBSOLETE     unsigned char in_h_gr_SI_13;
// OBSOLETE     unsigned char out_h_gr_SI_13;
// OBSOLETE   } sfmt_dmovr13pib;
// OBSOLETE   struct { /*  */
// OBSOLETE     USI f_dir9;
// OBSOLETE     unsigned char in_h_gr_SI_13;
// OBSOLETE     unsigned char out_h_gr_SI_13;
// OBSOLETE   } sfmt_dmovr13pih;
// OBSOLETE   struct { /*  */
// OBSOLETE     USI f_dir10;
// OBSOLETE     unsigned char in_h_gr_SI_13;
// OBSOLETE     unsigned char out_h_gr_SI_13;
// OBSOLETE   } sfmt_dmovr13pi;
// OBSOLETE   struct { /*  */
// OBSOLETE     UINT f_Rs2;
// OBSOLETE     unsigned char in_h_gr_SI_15;
// OBSOLETE     unsigned char out_h_gr_SI_15;
// OBSOLETE   } sfmt_ldr15dr;
// OBSOLETE   struct { /*  */
// OBSOLETE     SI* i_Ri;
// OBSOLETE     UINT f_Ri;
// OBSOLETE     UINT f_Rs1;
// OBSOLETE     unsigned char in_Ri;
// OBSOLETE   } sfmt_mov2dr;
// OBSOLETE   struct { /*  */
// OBSOLETE     SI* i_Ri;
// OBSOLETE     UINT f_Ri;
// OBSOLETE     UINT f_Rs1;
// OBSOLETE     unsigned char out_Ri;
// OBSOLETE   } sfmt_movdr;
// OBSOLETE   struct { /*  */
// OBSOLETE     SI* i_Ri;
// OBSOLETE     UINT f_Ri;
// OBSOLETE     UINT f_i32;
// OBSOLETE     unsigned char out_Ri;
// OBSOLETE   } sfmt_ldi32;
// OBSOLETE   struct { /*  */
// OBSOLETE     SI* i_Ri;
// OBSOLETE     UINT f_Ri;
// OBSOLETE     UINT f_i20;
// OBSOLETE     unsigned char out_Ri;
// OBSOLETE   } sfmt_ldi20;
// OBSOLETE   struct { /*  */
// OBSOLETE     SI* i_Ri;
// OBSOLETE     UINT f_Ri;
// OBSOLETE     UINT f_i8;
// OBSOLETE     unsigned char out_Ri;
// OBSOLETE   } sfmt_ldi8;
// OBSOLETE   struct { /*  */
// OBSOLETE     USI f_u10;
// OBSOLETE     unsigned char in_h_gr_SI_14;
// OBSOLETE     unsigned char in_h_gr_SI_15;
// OBSOLETE     unsigned char out_h_gr_SI_14;
// OBSOLETE     unsigned char out_h_gr_SI_15;
// OBSOLETE   } sfmt_enter;
// OBSOLETE   struct { /*  */
// OBSOLETE     SI* i_Ri;
// OBSOLETE     UINT f_Ri;
// OBSOLETE     unsigned char in_Ri;
// OBSOLETE     unsigned char in_h_gr_SI_15;
// OBSOLETE     unsigned char out_h_gr_SI_15;
// OBSOLETE   } sfmt_str15gr;
// OBSOLETE   struct { /*  */
// OBSOLETE     SI* i_Ri;
// OBSOLETE     UINT f_Ri;
// OBSOLETE     USI f_udisp6;
// OBSOLETE     unsigned char in_Ri;
// OBSOLETE     unsigned char in_h_gr_SI_15;
// OBSOLETE   } sfmt_str15;
// OBSOLETE   struct { /*  */
// OBSOLETE     SI* i_Ri;
// OBSOLETE     INT f_disp8;
// OBSOLETE     UINT f_Ri;
// OBSOLETE     unsigned char in_Ri;
// OBSOLETE     unsigned char in_h_gr_SI_14;
// OBSOLETE   } sfmt_str14b;
// OBSOLETE   struct { /*  */
// OBSOLETE     SI* i_Ri;
// OBSOLETE     SI f_disp9;
// OBSOLETE     UINT f_Ri;
// OBSOLETE     unsigned char in_Ri;
// OBSOLETE     unsigned char in_h_gr_SI_14;
// OBSOLETE   } sfmt_str14h;
// OBSOLETE   struct { /*  */
// OBSOLETE     SI* i_Ri;
// OBSOLETE     SI f_disp10;
// OBSOLETE     UINT f_Ri;
// OBSOLETE     unsigned char in_Ri;
// OBSOLETE     unsigned char in_h_gr_SI_14;
// OBSOLETE   } sfmt_str14;
// OBSOLETE   struct { /*  */
// OBSOLETE     SI* i_Ri;
// OBSOLETE     UINT f_Ri;
// OBSOLETE     unsigned char in_h_gr_SI_15;
// OBSOLETE     unsigned char out_Ri;
// OBSOLETE     unsigned char out_h_gr_SI_15;
// OBSOLETE   } sfmt_ldr15gr;
// OBSOLETE   struct { /*  */
// OBSOLETE     SI* i_Ri;
// OBSOLETE     UINT f_Ri;
// OBSOLETE     USI f_udisp6;
// OBSOLETE     unsigned char in_h_gr_SI_15;
// OBSOLETE     unsigned char out_Ri;
// OBSOLETE   } sfmt_ldr15;
// OBSOLETE   struct { /*  */
// OBSOLETE     SI* i_Ri;
// OBSOLETE     INT f_disp8;
// OBSOLETE     UINT f_Ri;
// OBSOLETE     unsigned char in_h_gr_SI_14;
// OBSOLETE     unsigned char out_Ri;
// OBSOLETE   } sfmt_ldr14ub;
// OBSOLETE   struct { /*  */
// OBSOLETE     SI* i_Ri;
// OBSOLETE     SI f_disp9;
// OBSOLETE     UINT f_Ri;
// OBSOLETE     unsigned char in_h_gr_SI_14;
// OBSOLETE     unsigned char out_Ri;
// OBSOLETE   } sfmt_ldr14uh;
// OBSOLETE   struct { /*  */
// OBSOLETE     SI* i_Ri;
// OBSOLETE     SI f_disp10;
// OBSOLETE     UINT f_Ri;
// OBSOLETE     unsigned char in_h_gr_SI_14;
// OBSOLETE     unsigned char out_Ri;
// OBSOLETE   } sfmt_ldr14;
// OBSOLETE   struct { /*  */
// OBSOLETE     SI* i_Ri;
// OBSOLETE     SI f_m4;
// OBSOLETE     UINT f_Ri;
// OBSOLETE     unsigned char in_Ri;
// OBSOLETE     unsigned char out_Ri;
// OBSOLETE   } sfmt_add2;
// OBSOLETE   struct { /*  */
// OBSOLETE     SI* i_Ri;
// OBSOLETE     UINT f_Ri;
// OBSOLETE     UINT f_u4;
// OBSOLETE     unsigned char in_Ri;
// OBSOLETE     unsigned char out_Ri;
// OBSOLETE   } sfmt_addi;
// OBSOLETE   struct { /*  */
// OBSOLETE     SI* i_Ri;
// OBSOLETE     SI* i_Rj;
// OBSOLETE     UINT f_Ri;
// OBSOLETE     UINT f_Rj;
// OBSOLETE     unsigned char in_Ri;
// OBSOLETE     unsigned char in_Rj;
// OBSOLETE     unsigned char in_h_gr_SI_13;
// OBSOLETE   } sfmt_str13;
// OBSOLETE   struct { /*  */
// OBSOLETE     SI* i_Ri;
// OBSOLETE     SI* i_Rj;
// OBSOLETE     UINT f_Ri;
// OBSOLETE     UINT f_Rj;
// OBSOLETE     unsigned char in_Rj;
// OBSOLETE     unsigned char in_h_gr_SI_13;
// OBSOLETE     unsigned char out_Ri;
// OBSOLETE   } sfmt_ldr13;
// OBSOLETE   struct { /*  */
// OBSOLETE     SI* i_Ri;
// OBSOLETE     SI* i_Rj;
// OBSOLETE     UINT f_Ri;
// OBSOLETE     UINT f_Rj;
// OBSOLETE     unsigned char in_Ri;
// OBSOLETE     unsigned char in_Rj;
// OBSOLETE     unsigned char out_Ri;
// OBSOLETE   } sfmt_add;
// OBSOLETE   struct { /*  */
// OBSOLETE     UINT f_reglist_hi_st;
// OBSOLETE     unsigned char in_h_gr_SI_10;
// OBSOLETE     unsigned char in_h_gr_SI_11;
// OBSOLETE     unsigned char in_h_gr_SI_12;
// OBSOLETE     unsigned char in_h_gr_SI_13;
// OBSOLETE     unsigned char in_h_gr_SI_14;
// OBSOLETE     unsigned char in_h_gr_SI_15;
// OBSOLETE     unsigned char in_h_gr_SI_8;
// OBSOLETE     unsigned char in_h_gr_SI_9;
// OBSOLETE     unsigned char out_h_gr_SI_15;
// OBSOLETE   } sfmt_stm1;
// OBSOLETE   struct { /*  */
// OBSOLETE     UINT f_reglist_hi_ld;
// OBSOLETE     unsigned char in_h_gr_SI_15;
// OBSOLETE     unsigned char out_h_gr_SI_10;
// OBSOLETE     unsigned char out_h_gr_SI_11;
// OBSOLETE     unsigned char out_h_gr_SI_12;
// OBSOLETE     unsigned char out_h_gr_SI_13;
// OBSOLETE     unsigned char out_h_gr_SI_14;
// OBSOLETE     unsigned char out_h_gr_SI_15;
// OBSOLETE     unsigned char out_h_gr_SI_8;
// OBSOLETE     unsigned char out_h_gr_SI_9;
// OBSOLETE   } sfmt_ldm1;
// OBSOLETE   struct { /*  */
// OBSOLETE     UINT f_reglist_low_st;
// OBSOLETE     unsigned char in_h_gr_SI_0;
// OBSOLETE     unsigned char in_h_gr_SI_1;
// OBSOLETE     unsigned char in_h_gr_SI_15;
// OBSOLETE     unsigned char in_h_gr_SI_2;
// OBSOLETE     unsigned char in_h_gr_SI_3;
// OBSOLETE     unsigned char in_h_gr_SI_4;
// OBSOLETE     unsigned char in_h_gr_SI_5;
// OBSOLETE     unsigned char in_h_gr_SI_6;
// OBSOLETE     unsigned char in_h_gr_SI_7;
// OBSOLETE     unsigned char out_h_gr_SI_15;
// OBSOLETE   } sfmt_stm0;
// OBSOLETE   struct { /*  */
// OBSOLETE     UINT f_reglist_low_ld;
// OBSOLETE     unsigned char in_h_gr_SI_15;
// OBSOLETE     unsigned char out_h_gr_SI_0;
// OBSOLETE     unsigned char out_h_gr_SI_1;
// OBSOLETE     unsigned char out_h_gr_SI_15;
// OBSOLETE     unsigned char out_h_gr_SI_2;
// OBSOLETE     unsigned char out_h_gr_SI_3;
// OBSOLETE     unsigned char out_h_gr_SI_4;
// OBSOLETE     unsigned char out_h_gr_SI_5;
// OBSOLETE     unsigned char out_h_gr_SI_6;
// OBSOLETE     unsigned char out_h_gr_SI_7;
// OBSOLETE   } sfmt_ldm0;
// OBSOLETE #if WITH_SCACHE_PBB
// OBSOLETE   /* Writeback handler.  */
// OBSOLETE   struct {
// OBSOLETE     /* Pointer to argbuf entry for insn whose results need writing back.  */
// OBSOLETE     const struct argbuf *abuf;
// OBSOLETE   } write;
// OBSOLETE   /* x-before handler */
// OBSOLETE   struct {
// OBSOLETE     /*const SCACHE *insns[MAX_PARALLEL_INSNS];*/
// OBSOLETE     int first_p;
// OBSOLETE   } before;
// OBSOLETE   /* x-after handler */
// OBSOLETE   struct {
// OBSOLETE     int empty;
// OBSOLETE   } after;
// OBSOLETE   /* This entry is used to terminate each pbb.  */
// OBSOLETE   struct {
// OBSOLETE     /* Number of insns in pbb.  */
// OBSOLETE     int insn_count;
// OBSOLETE     /* Next pbb to execute.  */
// OBSOLETE     SCACHE *next;
// OBSOLETE     SCACHE *branch_target;
// OBSOLETE   } chain;
// OBSOLETE #endif
// OBSOLETE };
// OBSOLETE 
// OBSOLETE /* The ARGBUF struct.  */
// OBSOLETE struct argbuf {
// OBSOLETE   /* These are the baseclass definitions.  */
// OBSOLETE   IADDR addr;
// OBSOLETE   const IDESC *idesc;
// OBSOLETE   char trace_p;
// OBSOLETE   char profile_p;
// OBSOLETE   /* ??? Temporary hack for skip insns.  */
// OBSOLETE   char skip_count;
// OBSOLETE   char unused;
// OBSOLETE   /* cpu specific data follows */
// OBSOLETE   union sem semantic;
// OBSOLETE   int written;
// OBSOLETE   union sem_fields fields;
// OBSOLETE };
// OBSOLETE 
// OBSOLETE /* A cached insn.
// OBSOLETE 
// OBSOLETE    ??? SCACHE used to contain more than just argbuf.  We could delete the
// OBSOLETE    type entirely and always just use ARGBUF, but for future concerns and as
// OBSOLETE    a level of abstraction it is left in.  */
// OBSOLETE 
// OBSOLETE struct scache {
// OBSOLETE   struct argbuf argbuf;
// OBSOLETE };
// OBSOLETE 
// OBSOLETE /* Macros to simplify extraction, reading and semantic code.
// OBSOLETE    These define and assign the local vars that contain the insn's fields.  */
// OBSOLETE 
// OBSOLETE #define EXTRACT_IFMT_EMPTY_VARS \
// OBSOLETE   unsigned int length;
// OBSOLETE #define EXTRACT_IFMT_EMPTY_CODE \
// OBSOLETE   length = 0; \
// OBSOLETE 
// OBSOLETE #define EXTRACT_IFMT_ADD_VARS \
// OBSOLETE   UINT f_op1; \
// OBSOLETE   UINT f_op2; \
// OBSOLETE   UINT f_Rj; \
// OBSOLETE   UINT f_Ri; \
// OBSOLETE   unsigned int length;
// OBSOLETE #define EXTRACT_IFMT_ADD_CODE \
// OBSOLETE   length = 2; \
// OBSOLETE   f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
// OBSOLETE   f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
// OBSOLETE   f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4); \
// OBSOLETE   f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4); \
// OBSOLETE 
// OBSOLETE #define EXTRACT_IFMT_ADDI_VARS \
// OBSOLETE   UINT f_op1; \
// OBSOLETE   UINT f_op2; \
// OBSOLETE   UINT f_u4; \
// OBSOLETE   UINT f_Ri; \
// OBSOLETE   unsigned int length;
// OBSOLETE #define EXTRACT_IFMT_ADDI_CODE \
// OBSOLETE   length = 2; \
// OBSOLETE   f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
// OBSOLETE   f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
// OBSOLETE   f_u4 = EXTRACT_MSB0_UINT (insn, 16, 8, 4); \
// OBSOLETE   f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4); \
// OBSOLETE 
// OBSOLETE #define EXTRACT_IFMT_ADD2_VARS \
// OBSOLETE   UINT f_op1; \
// OBSOLETE   UINT f_op2; \
// OBSOLETE   SI f_m4; \
// OBSOLETE   UINT f_Ri; \
// OBSOLETE   unsigned int length;
// OBSOLETE #define EXTRACT_IFMT_ADD2_CODE \
// OBSOLETE   length = 2; \
// OBSOLETE   f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
// OBSOLETE   f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
// OBSOLETE   f_m4 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 4)) | (((-1) << (4)))); \
// OBSOLETE   f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4); \
// OBSOLETE 
// OBSOLETE #define EXTRACT_IFMT_DIV0S_VARS \
// OBSOLETE   UINT f_op1; \
// OBSOLETE   UINT f_op2; \
// OBSOLETE   UINT f_op3; \
// OBSOLETE   UINT f_Ri; \
// OBSOLETE   unsigned int length;
// OBSOLETE #define EXTRACT_IFMT_DIV0S_CODE \
// OBSOLETE   length = 2; \
// OBSOLETE   f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
// OBSOLETE   f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
// OBSOLETE   f_op3 = EXTRACT_MSB0_UINT (insn, 16, 8, 4); \
// OBSOLETE   f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4); \
// OBSOLETE 
// OBSOLETE #define EXTRACT_IFMT_DIV3_VARS \
// OBSOLETE   UINT f_op1; \
// OBSOLETE   UINT f_op2; \
// OBSOLETE   UINT f_op3; \
// OBSOLETE   UINT f_op4; \
// OBSOLETE   unsigned int length;
// OBSOLETE #define EXTRACT_IFMT_DIV3_CODE \
// OBSOLETE   length = 2; \
// OBSOLETE   f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
// OBSOLETE   f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
// OBSOLETE   f_op3 = EXTRACT_MSB0_UINT (insn, 16, 8, 4); \
// OBSOLETE   f_op4 = EXTRACT_MSB0_UINT (insn, 16, 12, 4); \
// OBSOLETE 
// OBSOLETE #define EXTRACT_IFMT_LDI8_VARS \
// OBSOLETE   UINT f_op1; \
// OBSOLETE   UINT f_i8; \
// OBSOLETE   UINT f_Ri; \
// OBSOLETE   unsigned int length;
// OBSOLETE #define EXTRACT_IFMT_LDI8_CODE \
// OBSOLETE   length = 2; \
// OBSOLETE   f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
// OBSOLETE   f_i8 = EXTRACT_MSB0_UINT (insn, 16, 4, 8); \
// OBSOLETE   f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4); \
// OBSOLETE 
// OBSOLETE #define EXTRACT_IFMT_LDI20_VARS \
// OBSOLETE   UINT f_op1; \
// OBSOLETE   UINT f_i20_4; \
// OBSOLETE   UINT f_i20_16; \
// OBSOLETE   UINT f_i20; \
// OBSOLETE   UINT f_op2; \
// OBSOLETE   UINT f_Ri; \
// OBSOLETE   /* Contents of trailing part of insn.  */ \
// OBSOLETE   UINT word_1; \
// OBSOLETE   unsigned int length;
// OBSOLETE #define EXTRACT_IFMT_LDI20_CODE \
// OBSOLETE   length = 4; \
// OBSOLETE   word_1 = GETIMEMUHI (current_cpu, pc + 2); \
// OBSOLETE   f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
// OBSOLETE   f_i20_4 = EXTRACT_MSB0_UINT (insn, 16, 8, 4); \
// OBSOLETE   f_i20_16 = (0|(EXTRACT_MSB0_UINT (word_1, 16, 0, 16) << 0)); \
// OBSOLETE {\
// OBSOLETE   f_i20 = ((((f_i20_4) << (16))) | (f_i20_16));\
// OBSOLETE }\
// OBSOLETE   f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
// OBSOLETE   f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4); \
// OBSOLETE 
// OBSOLETE #define EXTRACT_IFMT_LDI32_VARS \
// OBSOLETE   UINT f_op1; \
// OBSOLETE   UINT f_i32; \
// OBSOLETE   UINT f_op2; \
// OBSOLETE   UINT f_op3; \
// OBSOLETE   UINT f_Ri; \
// OBSOLETE   /* Contents of trailing part of insn.  */ \
// OBSOLETE   UINT word_1; \
// OBSOLETE   UINT word_2; \
// OBSOLETE   unsigned int length;
// OBSOLETE #define EXTRACT_IFMT_LDI32_CODE \
// OBSOLETE   length = 6; \
// OBSOLETE   word_1 = GETIMEMUHI (current_cpu, pc + 2); \
// OBSOLETE   word_2 = GETIMEMUHI (current_cpu, pc + 4); \
// OBSOLETE   f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
// OBSOLETE   f_i32 = (0|(EXTRACT_MSB0_UINT (word_2, 16, 0, 16) << 0)|(EXTRACT_MSB0_UINT (word_1, 16, 0, 16) << 16)); \
// OBSOLETE   f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
// OBSOLETE   f_op3 = EXTRACT_MSB0_UINT (insn, 16, 8, 4); \
// OBSOLETE   f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4); \
// OBSOLETE 
// OBSOLETE #define EXTRACT_IFMT_LDR14_VARS \
// OBSOLETE   UINT f_op1; \
// OBSOLETE   SI f_disp10; \
// OBSOLETE   UINT f_Ri; \
// OBSOLETE   unsigned int length;
// OBSOLETE #define EXTRACT_IFMT_LDR14_CODE \
// OBSOLETE   length = 2; \
// OBSOLETE   f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
// OBSOLETE   f_disp10 = ((EXTRACT_MSB0_INT (insn, 16, 4, 8)) << (2)); \
// OBSOLETE   f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4); \
// OBSOLETE 
// OBSOLETE #define EXTRACT_IFMT_LDR14UH_VARS \
// OBSOLETE   UINT f_op1; \
// OBSOLETE   SI f_disp9; \
// OBSOLETE   UINT f_Ri; \
// OBSOLETE   unsigned int length;
// OBSOLETE #define EXTRACT_IFMT_LDR14UH_CODE \
// OBSOLETE   length = 2; \
// OBSOLETE   f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
// OBSOLETE   f_disp9 = ((EXTRACT_MSB0_INT (insn, 16, 4, 8)) << (1)); \
// OBSOLETE   f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4); \
// OBSOLETE 
// OBSOLETE #define EXTRACT_IFMT_LDR14UB_VARS \
// OBSOLETE   UINT f_op1; \
// OBSOLETE   INT f_disp8; \
// OBSOLETE   UINT f_Ri; \
// OBSOLETE   unsigned int length;
// OBSOLETE #define EXTRACT_IFMT_LDR14UB_CODE \
// OBSOLETE   length = 2; \
// OBSOLETE   f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
// OBSOLETE   f_disp8 = EXTRACT_MSB0_INT (insn, 16, 4, 8); \
// OBSOLETE   f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4); \
// OBSOLETE 
// OBSOLETE #define EXTRACT_IFMT_LDR15_VARS \
// OBSOLETE   UINT f_op1; \
// OBSOLETE   UINT f_op2; \
// OBSOLETE   USI f_udisp6; \
// OBSOLETE   UINT f_Ri; \
// OBSOLETE   unsigned int length;
// OBSOLETE #define EXTRACT_IFMT_LDR15_CODE \
// OBSOLETE   length = 2; \
// OBSOLETE   f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
// OBSOLETE   f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
// OBSOLETE   f_udisp6 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 4)) << (2)); \
// OBSOLETE   f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4); \
// OBSOLETE 
// OBSOLETE #define EXTRACT_IFMT_LDR15DR_VARS \
// OBSOLETE   UINT f_op1; \
// OBSOLETE   UINT f_op2; \
// OBSOLETE   UINT f_op3; \
// OBSOLETE   UINT f_Rs2; \
// OBSOLETE   unsigned int length;
// OBSOLETE #define EXTRACT_IFMT_LDR15DR_CODE \
// OBSOLETE   length = 2; \
// OBSOLETE   f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
// OBSOLETE   f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
// OBSOLETE   f_op3 = EXTRACT_MSB0_UINT (insn, 16, 8, 4); \
// OBSOLETE   f_Rs2 = EXTRACT_MSB0_UINT (insn, 16, 12, 4); \
// OBSOLETE 
// OBSOLETE #define EXTRACT_IFMT_MOVDR_VARS \
// OBSOLETE   UINT f_op1; \
// OBSOLETE   UINT f_op2; \
// OBSOLETE   UINT f_Rs1; \
// OBSOLETE   UINT f_Ri; \
// OBSOLETE   unsigned int length;
// OBSOLETE #define EXTRACT_IFMT_MOVDR_CODE \
// OBSOLETE   length = 2; \
// OBSOLETE   f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
// OBSOLETE   f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
// OBSOLETE   f_Rs1 = EXTRACT_MSB0_UINT (insn, 16, 8, 4); \
// OBSOLETE   f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4); \
// OBSOLETE 
// OBSOLETE #define EXTRACT_IFMT_CALL_VARS \
// OBSOLETE   UINT f_op1; \
// OBSOLETE   UINT f_op5; \
// OBSOLETE   SI f_rel12; \
// OBSOLETE   unsigned int length;
// OBSOLETE #define EXTRACT_IFMT_CALL_CODE \
// OBSOLETE   length = 2; \
// OBSOLETE   f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
// OBSOLETE   f_op5 = EXTRACT_MSB0_UINT (insn, 16, 4, 1); \
// OBSOLETE   f_rel12 = ((((EXTRACT_MSB0_INT (insn, 16, 5, 11)) << (1))) + (((pc) + (2)))); \
// OBSOLETE 
// OBSOLETE #define EXTRACT_IFMT_INT_VARS \
// OBSOLETE   UINT f_op1; \
// OBSOLETE   UINT f_op2; \
// OBSOLETE   UINT f_u8; \
// OBSOLETE   unsigned int length;
// OBSOLETE #define EXTRACT_IFMT_INT_CODE \
// OBSOLETE   length = 2; \
// OBSOLETE   f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
// OBSOLETE   f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
// OBSOLETE   f_u8 = EXTRACT_MSB0_UINT (insn, 16, 8, 8); \
// OBSOLETE 
// OBSOLETE #define EXTRACT_IFMT_BRAD_VARS \
// OBSOLETE   UINT f_op1; \
// OBSOLETE   UINT f_cc; \
// OBSOLETE   SI f_rel9; \
// OBSOLETE   unsigned int length;
// OBSOLETE #define EXTRACT_IFMT_BRAD_CODE \
// OBSOLETE   length = 2; \
// OBSOLETE   f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
// OBSOLETE   f_cc = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
// OBSOLETE   f_rel9 = ((((EXTRACT_MSB0_INT (insn, 16, 8, 8)) << (1))) + (((pc) + (2)))); \
// OBSOLETE 
// OBSOLETE #define EXTRACT_IFMT_DMOVR13_VARS \
// OBSOLETE   UINT f_op1; \
// OBSOLETE   UINT f_op2; \
// OBSOLETE   USI f_dir10; \
// OBSOLETE   unsigned int length;
// OBSOLETE #define EXTRACT_IFMT_DMOVR13_CODE \
// OBSOLETE   length = 2; \
// OBSOLETE   f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
// OBSOLETE   f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
// OBSOLETE   f_dir10 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 8)) << (2)); \
// OBSOLETE 
// OBSOLETE #define EXTRACT_IFMT_DMOVR13H_VARS \
// OBSOLETE   UINT f_op1; \
// OBSOLETE   UINT f_op2; \
// OBSOLETE   USI f_dir9; \
// OBSOLETE   unsigned int length;
// OBSOLETE #define EXTRACT_IFMT_DMOVR13H_CODE \
// OBSOLETE   length = 2; \
// OBSOLETE   f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
// OBSOLETE   f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
// OBSOLETE   f_dir9 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 8)) << (1)); \
// OBSOLETE 
// OBSOLETE #define EXTRACT_IFMT_DMOVR13B_VARS \
// OBSOLETE   UINT f_op1; \
// OBSOLETE   UINT f_op2; \
// OBSOLETE   UINT f_dir8; \
// OBSOLETE   unsigned int length;
// OBSOLETE #define EXTRACT_IFMT_DMOVR13B_CODE \
// OBSOLETE   length = 2; \
// OBSOLETE   f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
// OBSOLETE   f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
// OBSOLETE   f_dir8 = EXTRACT_MSB0_UINT (insn, 16, 8, 8); \
// OBSOLETE 
// OBSOLETE #define EXTRACT_IFMT_COPOP_VARS \
// OBSOLETE   UINT f_op1; \
// OBSOLETE   UINT f_ccc; \
// OBSOLETE   UINT f_op2; \
// OBSOLETE   UINT f_op3; \
// OBSOLETE   UINT f_CRj; \
// OBSOLETE   UINT f_u4c; \
// OBSOLETE   UINT f_CRi; \
// OBSOLETE   /* Contents of trailing part of insn.  */ \
// OBSOLETE   UINT word_1; \
// OBSOLETE   unsigned int length;
// OBSOLETE #define EXTRACT_IFMT_COPOP_CODE \
// OBSOLETE   length = 4; \
// OBSOLETE   word_1 = GETIMEMUHI (current_cpu, pc + 2); \
// OBSOLETE   f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
// OBSOLETE   f_ccc = (0|(EXTRACT_MSB0_UINT (word_1, 16, 0, 8) << 0)); \
// OBSOLETE   f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
// OBSOLETE   f_op3 = EXTRACT_MSB0_UINT (insn, 16, 8, 4); \
// OBSOLETE   f_CRj = (0|(EXTRACT_MSB0_UINT (word_1, 16, 8, 4) << 0)); \
// OBSOLETE   f_u4c = EXTRACT_MSB0_UINT (insn, 16, 12, 4); \
// OBSOLETE   f_CRi = (0|(EXTRACT_MSB0_UINT (word_1, 16, 12, 4) << 0)); \
// OBSOLETE 
// OBSOLETE #define EXTRACT_IFMT_COPLD_VARS \
// OBSOLETE   UINT f_op1; \
// OBSOLETE   UINT f_ccc; \
// OBSOLETE   UINT f_op2; \
// OBSOLETE   UINT f_op3; \
// OBSOLETE   UINT f_Rjc; \
// OBSOLETE   UINT f_u4c; \
// OBSOLETE   UINT f_CRi; \
// OBSOLETE   /* Contents of trailing part of insn.  */ \
// OBSOLETE   UINT word_1; \
// OBSOLETE   unsigned int length;
// OBSOLETE #define EXTRACT_IFMT_COPLD_CODE \
// OBSOLETE   length = 4; \
// OBSOLETE   word_1 = GETIMEMUHI (current_cpu, pc + 2); \
// OBSOLETE   f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
// OBSOLETE   f_ccc = (0|(EXTRACT_MSB0_UINT (word_1, 16, 0, 8) << 0)); \
// OBSOLETE   f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
// OBSOLETE   f_op3 = EXTRACT_MSB0_UINT (insn, 16, 8, 4); \
// OBSOLETE   f_Rjc = (0|(EXTRACT_MSB0_UINT (word_1, 16, 8, 4) << 0)); \
// OBSOLETE   f_u4c = EXTRACT_MSB0_UINT (insn, 16, 12, 4); \
// OBSOLETE   f_CRi = (0|(EXTRACT_MSB0_UINT (word_1, 16, 12, 4) << 0)); \
// OBSOLETE 
// OBSOLETE #define EXTRACT_IFMT_COPST_VARS \
// OBSOLETE   UINT f_op1; \
// OBSOLETE   UINT f_ccc; \
// OBSOLETE   UINT f_op2; \
// OBSOLETE   UINT f_op3; \
// OBSOLETE   UINT f_CRj; \
// OBSOLETE   UINT f_u4c; \
// OBSOLETE   UINT f_Ric; \
// OBSOLETE   /* Contents of trailing part of insn.  */ \
// OBSOLETE   UINT word_1; \
// OBSOLETE   unsigned int length;
// OBSOLETE #define EXTRACT_IFMT_COPST_CODE \
// OBSOLETE   length = 4; \
// OBSOLETE   word_1 = GETIMEMUHI (current_cpu, pc + 2); \
// OBSOLETE   f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
// OBSOLETE   f_ccc = (0|(EXTRACT_MSB0_UINT (word_1, 16, 0, 8) << 0)); \
// OBSOLETE   f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
// OBSOLETE   f_op3 = EXTRACT_MSB0_UINT (insn, 16, 8, 4); \
// OBSOLETE   f_CRj = (0|(EXTRACT_MSB0_UINT (word_1, 16, 8, 4) << 0)); \
// OBSOLETE   f_u4c = EXTRACT_MSB0_UINT (insn, 16, 12, 4); \
// OBSOLETE   f_Ric = (0|(EXTRACT_MSB0_UINT (word_1, 16, 12, 4) << 0)); \
// OBSOLETE 
// OBSOLETE #define EXTRACT_IFMT_ADDSP_VARS \
// OBSOLETE   UINT f_op1; \
// OBSOLETE   UINT f_op2; \
// OBSOLETE   SI f_s10; \
// OBSOLETE   unsigned int length;
// OBSOLETE #define EXTRACT_IFMT_ADDSP_CODE \
// OBSOLETE   length = 2; \
// OBSOLETE   f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
// OBSOLETE   f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
// OBSOLETE   f_s10 = ((EXTRACT_MSB0_INT (insn, 16, 8, 8)) << (2)); \
// OBSOLETE 
// OBSOLETE #define EXTRACT_IFMT_LDM0_VARS \
// OBSOLETE   UINT f_op1; \
// OBSOLETE   UINT f_op2; \
// OBSOLETE   UINT f_reglist_low_ld; \
// OBSOLETE   unsigned int length;
// OBSOLETE #define EXTRACT_IFMT_LDM0_CODE \
// OBSOLETE   length = 2; \
// OBSOLETE   f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
// OBSOLETE   f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
// OBSOLETE   f_reglist_low_ld = EXTRACT_MSB0_UINT (insn, 16, 8, 8); \
// OBSOLETE 
// OBSOLETE #define EXTRACT_IFMT_LDM1_VARS \
// OBSOLETE   UINT f_op1; \
// OBSOLETE   UINT f_op2; \
// OBSOLETE   UINT f_reglist_hi_ld; \
// OBSOLETE   unsigned int length;
// OBSOLETE #define EXTRACT_IFMT_LDM1_CODE \
// OBSOLETE   length = 2; \
// OBSOLETE   f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
// OBSOLETE   f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
// OBSOLETE   f_reglist_hi_ld = EXTRACT_MSB0_UINT (insn, 16, 8, 8); \
// OBSOLETE 
// OBSOLETE #define EXTRACT_IFMT_STM0_VARS \
// OBSOLETE   UINT f_op1; \
// OBSOLETE   UINT f_op2; \
// OBSOLETE   UINT f_reglist_low_st; \
// OBSOLETE   unsigned int length;
// OBSOLETE #define EXTRACT_IFMT_STM0_CODE \
// OBSOLETE   length = 2; \
// OBSOLETE   f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
// OBSOLETE   f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
// OBSOLETE   f_reglist_low_st = EXTRACT_MSB0_UINT (insn, 16, 8, 8); \
// OBSOLETE 
// OBSOLETE #define EXTRACT_IFMT_STM1_VARS \
// OBSOLETE   UINT f_op1; \
// OBSOLETE   UINT f_op2; \
// OBSOLETE   UINT f_reglist_hi_st; \
// OBSOLETE   unsigned int length;
// OBSOLETE #define EXTRACT_IFMT_STM1_CODE \
// OBSOLETE   length = 2; \
// OBSOLETE   f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
// OBSOLETE   f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
// OBSOLETE   f_reglist_hi_st = EXTRACT_MSB0_UINT (insn, 16, 8, 8); \
// OBSOLETE 
// OBSOLETE #define EXTRACT_IFMT_ENTER_VARS \
// OBSOLETE   UINT f_op1; \
// OBSOLETE   UINT f_op2; \
// OBSOLETE   USI f_u10; \
// OBSOLETE   unsigned int length;
// OBSOLETE #define EXTRACT_IFMT_ENTER_CODE \
// OBSOLETE   length = 2; \
// OBSOLETE   f_op1 = EXTRACT_MSB0_UINT (insn, 16, 0, 4); \
// OBSOLETE   f_op2 = EXTRACT_MSB0_UINT (insn, 16, 4, 4); \
// OBSOLETE   f_u10 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 8)) << (2)); \
// OBSOLETE 
// OBSOLETE /* Collection of various things for the trace handler to use.  */
// OBSOLETE 
// OBSOLETE typedef struct trace_record {
// OBSOLETE   IADDR pc;
// OBSOLETE   /* FIXME:wip */
// OBSOLETE } TRACE_RECORD;
// OBSOLETE 
// OBSOLETE #endif /* CPU_FR30BF_H */
