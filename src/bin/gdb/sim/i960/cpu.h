/* CPU family header for i960base.

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

#ifndef CPU_I960BASE_H
#define CPU_I960BASE_H

/* Maximum number of instructions that are fetched at a time.
   This is for LIW type instructions sets (e.g. m32r).  */
#define MAX_LIW_INSNS 1

/* Maximum number of instructions that can be executed in parallel.  */
#define MAX_PARALLEL_INSNS 1

/* CPU state information.  */
typedef struct {
  /* Hardware elements.  */
  struct {
  /* program counter */
  USI h_pc;
#define GET_H_PC() CPU (h_pc)
#define SET_H_PC(x) (CPU (h_pc) = (x))
  /* general registers */
  SI h_gr[32];
#define GET_H_GR(a1) CPU (h_gr)[a1]
#define SET_H_GR(a1, x) (CPU (h_gr)[a1] = (x))
  /* condition code */
  SI h_cc;
#define GET_H_CC() CPU (h_cc)
#define SET_H_CC(x) (CPU (h_cc) = (x))
  } hardware;
#define CPU_CGEN_HW(cpu) (& (cpu)->cpu_data.hardware)
} I960BASE_CPU_DATA;

/* Cover fns for register access.  */
USI i960base_h_pc_get (SIM_CPU *);
void i960base_h_pc_set (SIM_CPU *, USI);
SI i960base_h_gr_get (SIM_CPU *, UINT);
void i960base_h_gr_set (SIM_CPU *, UINT, SI);
SI i960base_h_cc_get (SIM_CPU *);
void i960base_h_cc_set (SIM_CPU *, SI);

/* These must be hand-written.  */
extern CPUREG_FETCH_FN i960base_fetch_register;
extern CPUREG_STORE_FN i960base_store_register;

typedef struct {
  int empty;
} MODEL_I960KA_DATA;

typedef struct {
  int empty;
} MODEL_I960CA_DATA;

/* Instruction argument buffer.  */

union sem_fields {
  struct { /* no operands */
    int empty;
  } fmt_empty;
  struct { /*  */
    IADDR i_ctrl_disp;
  } sfmt_bno;
  struct { /*  */
    SI* i_br_src1;
    unsigned char out_br_src1;
  } sfmt_testno_reg;
  struct { /*  */
    IADDR i_br_disp;
    SI* i_br_src2;
    UINT f_br_src1;
    unsigned char in_br_src2;
  } sfmt_cmpobe_lit;
  struct { /*  */
    IADDR i_br_disp;
    SI* i_br_src1;
    SI* i_br_src2;
    unsigned char in_br_src1;
    unsigned char in_br_src2;
  } sfmt_cmpobe_reg;
  struct { /*  */
    SI* i_dst;
    UINT f_src1;
    UINT f_src2;
    UINT f_srcdst;
    unsigned char out_dst;
    unsigned char out_h_gr_add__DFLT_index_of__DFLT_dst_1;
  } sfmt_emul3;
  struct { /*  */
    SI* i_dst;
    SI* i_src1;
    UINT f_src2;
    UINT f_srcdst;
    unsigned char in_src1;
    unsigned char out_dst;
    unsigned char out_h_gr_add__DFLT_index_of__DFLT_dst_1;
  } sfmt_emul2;
  struct { /*  */
    SI* i_dst;
    SI* i_src2;
    UINT f_src1;
    UINT f_srcdst;
    unsigned char in_src2;
    unsigned char out_dst;
    unsigned char out_h_gr_add__DFLT_index_of__DFLT_dst_1;
  } sfmt_emul1;
  struct { /*  */
    SI* i_dst;
    SI* i_src1;
    SI* i_src2;
    UINT f_srcdst;
    unsigned char in_src1;
    unsigned char in_src2;
    unsigned char out_dst;
    unsigned char out_h_gr_add__DFLT_index_of__DFLT_dst_1;
  } sfmt_emul;
  struct { /*  */
    SI* i_abase;
    SI* i_st_src;
    UINT f_offset;
    UINT f_srcdst;
    unsigned char in_abase;
    unsigned char in_h_gr_add__DFLT_index_of__DFLT_st_src_1;
    unsigned char in_h_gr_add__DFLT_index_of__DFLT_st_src_2;
    unsigned char in_h_gr_add__DFLT_index_of__DFLT_st_src_3;
    unsigned char in_st_src;
  } sfmt_stq_indirect_offset;
  struct { /*  */
    SI* i_abase;
    SI* i_dst;
    UINT f_offset;
    UINT f_srcdst;
    unsigned char in_abase;
    unsigned char out_dst;
    unsigned char out_h_gr_add__DFLT_index_of__DFLT_dst_1;
    unsigned char out_h_gr_add__DFLT_index_of__DFLT_dst_2;
    unsigned char out_h_gr_add__DFLT_index_of__DFLT_dst_3;
  } sfmt_ldq_indirect_offset;
  struct { /*  */
    SI* i_abase;
    SI* i_index;
    SI* i_st_src;
    UINT f_optdisp;
    UINT f_scale;
    UINT f_srcdst;
    unsigned char in_abase;
    unsigned char in_h_gr_add__DFLT_index_of__DFLT_st_src_1;
    unsigned char in_h_gr_add__DFLT_index_of__DFLT_st_src_2;
    unsigned char in_h_gr_add__DFLT_index_of__DFLT_st_src_3;
    unsigned char in_index;
    unsigned char in_st_src;
  } sfmt_stq_indirect_index_disp;
  struct { /*  */
    SI* i_abase;
    SI* i_dst;
    SI* i_index;
    UINT f_optdisp;
    UINT f_scale;
    UINT f_srcdst;
    unsigned char in_abase;
    unsigned char in_index;
    unsigned char out_dst;
    unsigned char out_h_gr_add__DFLT_index_of__DFLT_dst_1;
    unsigned char out_h_gr_add__DFLT_index_of__DFLT_dst_2;
    unsigned char out_h_gr_add__DFLT_index_of__DFLT_dst_3;
  } sfmt_ldq_indirect_index_disp;
  struct { /*  */
    SI* i_dst;
    SI* i_src1;
    UINT f_src1;
    UINT f_srcdst;
    unsigned char in_h_gr_add__DFLT_index_of__DFLT_src1_1;
    unsigned char in_h_gr_add__DFLT_index_of__DFLT_src1_2;
    unsigned char in_h_gr_add__DFLT_index_of__DFLT_src1_3;
    unsigned char in_src1;
    unsigned char out_dst;
    unsigned char out_h_gr_add__DFLT_index_of__DFLT_dst_1;
    unsigned char out_h_gr_add__DFLT_index_of__DFLT_dst_2;
    unsigned char out_h_gr_add__DFLT_index_of__DFLT_dst_3;
  } sfmt_movq;
  struct { /*  */
    UINT f_optdisp;
    unsigned char in_h_gr_0;
    unsigned char in_h_gr_1;
    unsigned char in_h_gr_10;
    unsigned char in_h_gr_11;
    unsigned char in_h_gr_12;
    unsigned char in_h_gr_13;
    unsigned char in_h_gr_14;
    unsigned char in_h_gr_15;
    unsigned char in_h_gr_2;
    unsigned char in_h_gr_3;
    unsigned char in_h_gr_31;
    unsigned char in_h_gr_4;
    unsigned char in_h_gr_5;
    unsigned char in_h_gr_6;
    unsigned char in_h_gr_7;
    unsigned char in_h_gr_8;
    unsigned char in_h_gr_9;
    unsigned char out_h_gr_0;
    unsigned char out_h_gr_1;
    unsigned char out_h_gr_10;
    unsigned char out_h_gr_11;
    unsigned char out_h_gr_12;
    unsigned char out_h_gr_13;
    unsigned char out_h_gr_14;
    unsigned char out_h_gr_15;
    unsigned char out_h_gr_2;
    unsigned char out_h_gr_3;
    unsigned char out_h_gr_31;
    unsigned char out_h_gr_4;
    unsigned char out_h_gr_5;
    unsigned char out_h_gr_6;
    unsigned char out_h_gr_7;
    unsigned char out_h_gr_8;
    unsigned char out_h_gr_9;
  } sfmt_callx_disp;
  struct { /*  */
    SI* i_abase;
    UINT f_offset;
    unsigned char in_abase;
    unsigned char in_h_gr_0;
    unsigned char in_h_gr_1;
    unsigned char in_h_gr_10;
    unsigned char in_h_gr_11;
    unsigned char in_h_gr_12;
    unsigned char in_h_gr_13;
    unsigned char in_h_gr_14;
    unsigned char in_h_gr_15;
    unsigned char in_h_gr_2;
    unsigned char in_h_gr_3;
    unsigned char in_h_gr_31;
    unsigned char in_h_gr_4;
    unsigned char in_h_gr_5;
    unsigned char in_h_gr_6;
    unsigned char in_h_gr_7;
    unsigned char in_h_gr_8;
    unsigned char in_h_gr_9;
    unsigned char out_h_gr_0;
    unsigned char out_h_gr_1;
    unsigned char out_h_gr_10;
    unsigned char out_h_gr_11;
    unsigned char out_h_gr_12;
    unsigned char out_h_gr_13;
    unsigned char out_h_gr_14;
    unsigned char out_h_gr_15;
    unsigned char out_h_gr_2;
    unsigned char out_h_gr_3;
    unsigned char out_h_gr_31;
    unsigned char out_h_gr_4;
    unsigned char out_h_gr_5;
    unsigned char out_h_gr_6;
    unsigned char out_h_gr_7;
    unsigned char out_h_gr_8;
    unsigned char out_h_gr_9;
  } sfmt_callx_indirect_offset;
#if WITH_SCACHE_PBB
  /* Writeback handler.  */
  struct {
    /* Pointer to argbuf entry for insn whose results need writing back.  */
    const struct argbuf *abuf;
  } write;
  /* x-before handler */
  struct {
    /*const SCACHE *insns[MAX_PARALLEL_INSNS];*/
    int first_p;
  } before;
  /* x-after handler */
  struct {
    int empty;
  } after;
  /* This entry is used to terminate each pbb.  */
  struct {
    /* Number of insns in pbb.  */
    int insn_count;
    /* Next pbb to execute.  */
    SCACHE *next;
    SCACHE *branch_target;
  } chain;
#endif
};

/* The ARGBUF struct.  */
struct argbuf {
  /* These are the baseclass definitions.  */
  IADDR addr;
  const IDESC *idesc;
  char trace_p;
  char profile_p;
  /* ??? Temporary hack for skip insns.  */
  char skip_count;
  char unused;
  /* cpu specific data follows */
  union sem semantic;
  int written;
  union sem_fields fields;
};

/* A cached insn.

   ??? SCACHE used to contain more than just argbuf.  We could delete the
   type entirely and always just use ARGBUF, but for future concerns and as
   a level of abstraction it is left in.  */

struct scache {
  struct argbuf argbuf;
};

/* Macros to simplify extraction, reading and semantic code.
   These define and assign the local vars that contain the insn's fields.  */

#define EXTRACT_IFMT_EMPTY_VARS \
  unsigned int length;
#define EXTRACT_IFMT_EMPTY_CODE \
  length = 0; \

#define EXTRACT_IFMT_MULO_VARS \
  UINT f_opcode; \
  UINT f_srcdst; \
  UINT f_src2; \
  UINT f_m3; \
  UINT f_m2; \
  UINT f_m1; \
  UINT f_opcode2; \
  UINT f_zero; \
  UINT f_src1; \
  unsigned int length;
#define EXTRACT_IFMT_MULO_CODE \
  length = 4; \
  f_opcode = EXTRACT_MSB0_UINT (insn, 32, 0, 8); \
  f_srcdst = EXTRACT_MSB0_UINT (insn, 32, 8, 5); \
  f_src2 = EXTRACT_MSB0_UINT (insn, 32, 13, 5); \
  f_m3 = EXTRACT_MSB0_UINT (insn, 32, 18, 1); \
  f_m2 = EXTRACT_MSB0_UINT (insn, 32, 19, 1); \
  f_m1 = EXTRACT_MSB0_UINT (insn, 32, 20, 1); \
  f_opcode2 = EXTRACT_MSB0_UINT (insn, 32, 21, 4); \
  f_zero = EXTRACT_MSB0_UINT (insn, 32, 25, 2); \
  f_src1 = EXTRACT_MSB0_UINT (insn, 32, 27, 5); \

#define EXTRACT_IFMT_MULO1_VARS \
  UINT f_opcode; \
  UINT f_srcdst; \
  UINT f_src2; \
  UINT f_m3; \
  UINT f_m2; \
  UINT f_m1; \
  UINT f_opcode2; \
  UINT f_zero; \
  UINT f_src1; \
  unsigned int length;
#define EXTRACT_IFMT_MULO1_CODE \
  length = 4; \
  f_opcode = EXTRACT_MSB0_UINT (insn, 32, 0, 8); \
  f_srcdst = EXTRACT_MSB0_UINT (insn, 32, 8, 5); \
  f_src2 = EXTRACT_MSB0_UINT (insn, 32, 13, 5); \
  f_m3 = EXTRACT_MSB0_UINT (insn, 32, 18, 1); \
  f_m2 = EXTRACT_MSB0_UINT (insn, 32, 19, 1); \
  f_m1 = EXTRACT_MSB0_UINT (insn, 32, 20, 1); \
  f_opcode2 = EXTRACT_MSB0_UINT (insn, 32, 21, 4); \
  f_zero = EXTRACT_MSB0_UINT (insn, 32, 25, 2); \
  f_src1 = EXTRACT_MSB0_UINT (insn, 32, 27, 5); \

#define EXTRACT_IFMT_MULO2_VARS \
  UINT f_opcode; \
  UINT f_srcdst; \
  UINT f_src2; \
  UINT f_m3; \
  UINT f_m2; \
  UINT f_m1; \
  UINT f_opcode2; \
  UINT f_zero; \
  UINT f_src1; \
  unsigned int length;
#define EXTRACT_IFMT_MULO2_CODE \
  length = 4; \
  f_opcode = EXTRACT_MSB0_UINT (insn, 32, 0, 8); \
  f_srcdst = EXTRACT_MSB0_UINT (insn, 32, 8, 5); \
  f_src2 = EXTRACT_MSB0_UINT (insn, 32, 13, 5); \
  f_m3 = EXTRACT_MSB0_UINT (insn, 32, 18, 1); \
  f_m2 = EXTRACT_MSB0_UINT (insn, 32, 19, 1); \
  f_m1 = EXTRACT_MSB0_UINT (insn, 32, 20, 1); \
  f_opcode2 = EXTRACT_MSB0_UINT (insn, 32, 21, 4); \
  f_zero = EXTRACT_MSB0_UINT (insn, 32, 25, 2); \
  f_src1 = EXTRACT_MSB0_UINT (insn, 32, 27, 5); \

#define EXTRACT_IFMT_MULO3_VARS \
  UINT f_opcode; \
  UINT f_srcdst; \
  UINT f_src2; \
  UINT f_m3; \
  UINT f_m2; \
  UINT f_m1; \
  UINT f_opcode2; \
  UINT f_zero; \
  UINT f_src1; \
  unsigned int length;
#define EXTRACT_IFMT_MULO3_CODE \
  length = 4; \
  f_opcode = EXTRACT_MSB0_UINT (insn, 32, 0, 8); \
  f_srcdst = EXTRACT_MSB0_UINT (insn, 32, 8, 5); \
  f_src2 = EXTRACT_MSB0_UINT (insn, 32, 13, 5); \
  f_m3 = EXTRACT_MSB0_UINT (insn, 32, 18, 1); \
  f_m2 = EXTRACT_MSB0_UINT (insn, 32, 19, 1); \
  f_m1 = EXTRACT_MSB0_UINT (insn, 32, 20, 1); \
  f_opcode2 = EXTRACT_MSB0_UINT (insn, 32, 21, 4); \
  f_zero = EXTRACT_MSB0_UINT (insn, 32, 25, 2); \
  f_src1 = EXTRACT_MSB0_UINT (insn, 32, 27, 5); \

#define EXTRACT_IFMT_LDA_OFFSET_VARS \
  UINT f_opcode; \
  UINT f_srcdst; \
  UINT f_abase; \
  UINT f_modea; \
  UINT f_zeroa; \
  UINT f_offset; \
  unsigned int length;
#define EXTRACT_IFMT_LDA_OFFSET_CODE \
  length = 4; \
  f_opcode = EXTRACT_MSB0_UINT (insn, 32, 0, 8); \
  f_srcdst = EXTRACT_MSB0_UINT (insn, 32, 8, 5); \
  f_abase = EXTRACT_MSB0_UINT (insn, 32, 13, 5); \
  f_modea = EXTRACT_MSB0_UINT (insn, 32, 18, 1); \
  f_zeroa = EXTRACT_MSB0_UINT (insn, 32, 19, 1); \
  f_offset = EXTRACT_MSB0_UINT (insn, 32, 20, 12); \

#define EXTRACT_IFMT_LDA_INDIRECT_VARS \
  UINT f_opcode; \
  UINT f_srcdst; \
  UINT f_abase; \
  UINT f_modeb; \
  UINT f_scale; \
  UINT f_zerob; \
  UINT f_index; \
  unsigned int length;
#define EXTRACT_IFMT_LDA_INDIRECT_CODE \
  length = 4; \
  f_opcode = EXTRACT_MSB0_UINT (insn, 32, 0, 8); \
  f_srcdst = EXTRACT_MSB0_UINT (insn, 32, 8, 5); \
  f_abase = EXTRACT_MSB0_UINT (insn, 32, 13, 5); \
  f_modeb = EXTRACT_MSB0_UINT (insn, 32, 18, 4); \
  f_scale = EXTRACT_MSB0_UINT (insn, 32, 22, 3); \
  f_zerob = EXTRACT_MSB0_UINT (insn, 32, 25, 2); \
  f_index = EXTRACT_MSB0_UINT (insn, 32, 27, 5); \

#define EXTRACT_IFMT_LDA_DISP_VARS \
  UINT f_opcode; \
  UINT f_optdisp; \
  UINT f_srcdst; \
  UINT f_abase; \
  UINT f_modeb; \
  UINT f_scale; \
  UINT f_zerob; \
  UINT f_index; \
  /* Contents of trailing part of insn.  */ \
  UINT word_1; \
  unsigned int length;
#define EXTRACT_IFMT_LDA_DISP_CODE \
  length = 8; \
  word_1 = GETIMEMUSI (current_cpu, pc + 4); \
  f_opcode = EXTRACT_MSB0_UINT (insn, 32, 0, 8); \
  f_optdisp = (0|(EXTRACT_MSB0_UINT (word_1, 32, 0, 32) << 0)); \
  f_srcdst = EXTRACT_MSB0_UINT (insn, 32, 8, 5); \
  f_abase = EXTRACT_MSB0_UINT (insn, 32, 13, 5); \
  f_modeb = EXTRACT_MSB0_UINT (insn, 32, 18, 4); \
  f_scale = EXTRACT_MSB0_UINT (insn, 32, 22, 3); \
  f_zerob = EXTRACT_MSB0_UINT (insn, 32, 25, 2); \
  f_index = EXTRACT_MSB0_UINT (insn, 32, 27, 5); \

#define EXTRACT_IFMT_ST_OFFSET_VARS \
  UINT f_opcode; \
  UINT f_srcdst; \
  UINT f_abase; \
  UINT f_modea; \
  UINT f_zeroa; \
  UINT f_offset; \
  unsigned int length;
#define EXTRACT_IFMT_ST_OFFSET_CODE \
  length = 4; \
  f_opcode = EXTRACT_MSB0_UINT (insn, 32, 0, 8); \
  f_srcdst = EXTRACT_MSB0_UINT (insn, 32, 8, 5); \
  f_abase = EXTRACT_MSB0_UINT (insn, 32, 13, 5); \
  f_modea = EXTRACT_MSB0_UINT (insn, 32, 18, 1); \
  f_zeroa = EXTRACT_MSB0_UINT (insn, 32, 19, 1); \
  f_offset = EXTRACT_MSB0_UINT (insn, 32, 20, 12); \

#define EXTRACT_IFMT_ST_INDIRECT_VARS \
  UINT f_opcode; \
  UINT f_srcdst; \
  UINT f_abase; \
  UINT f_modeb; \
  UINT f_scale; \
  UINT f_zerob; \
  UINT f_index; \
  unsigned int length;
#define EXTRACT_IFMT_ST_INDIRECT_CODE \
  length = 4; \
  f_opcode = EXTRACT_MSB0_UINT (insn, 32, 0, 8); \
  f_srcdst = EXTRACT_MSB0_UINT (insn, 32, 8, 5); \
  f_abase = EXTRACT_MSB0_UINT (insn, 32, 13, 5); \
  f_modeb = EXTRACT_MSB0_UINT (insn, 32, 18, 4); \
  f_scale = EXTRACT_MSB0_UINT (insn, 32, 22, 3); \
  f_zerob = EXTRACT_MSB0_UINT (insn, 32, 25, 2); \
  f_index = EXTRACT_MSB0_UINT (insn, 32, 27, 5); \

#define EXTRACT_IFMT_ST_DISP_VARS \
  UINT f_opcode; \
  UINT f_optdisp; \
  UINT f_srcdst; \
  UINT f_abase; \
  UINT f_modeb; \
  UINT f_scale; \
  UINT f_zerob; \
  UINT f_index; \
  /* Contents of trailing part of insn.  */ \
  UINT word_1; \
  unsigned int length;
#define EXTRACT_IFMT_ST_DISP_CODE \
  length = 8; \
  word_1 = GETIMEMUSI (current_cpu, pc + 4); \
  f_opcode = EXTRACT_MSB0_UINT (insn, 32, 0, 8); \
  f_optdisp = (0|(EXTRACT_MSB0_UINT (word_1, 32, 0, 32) << 0)); \
  f_srcdst = EXTRACT_MSB0_UINT (insn, 32, 8, 5); \
  f_abase = EXTRACT_MSB0_UINT (insn, 32, 13, 5); \
  f_modeb = EXTRACT_MSB0_UINT (insn, 32, 18, 4); \
  f_scale = EXTRACT_MSB0_UINT (insn, 32, 22, 3); \
  f_zerob = EXTRACT_MSB0_UINT (insn, 32, 25, 2); \
  f_index = EXTRACT_MSB0_UINT (insn, 32, 27, 5); \

#define EXTRACT_IFMT_CMPOBE_REG_VARS \
  UINT f_opcode; \
  UINT f_br_src1; \
  UINT f_br_src2; \
  UINT f_br_m1; \
  SI f_br_disp; \
  UINT f_br_zero; \
  unsigned int length;
#define EXTRACT_IFMT_CMPOBE_REG_CODE \
  length = 4; \
  f_opcode = EXTRACT_MSB0_UINT (insn, 32, 0, 8); \
  f_br_src1 = EXTRACT_MSB0_UINT (insn, 32, 8, 5); \
  f_br_src2 = EXTRACT_MSB0_UINT (insn, 32, 13, 5); \
  f_br_m1 = EXTRACT_MSB0_UINT (insn, 32, 18, 1); \
  f_br_disp = ((((EXTRACT_MSB0_INT (insn, 32, 19, 11)) << (2))) + (pc)); \
  f_br_zero = EXTRACT_MSB0_UINT (insn, 32, 30, 2); \

#define EXTRACT_IFMT_CMPOBE_LIT_VARS \
  UINT f_opcode; \
  UINT f_br_src1; \
  UINT f_br_src2; \
  UINT f_br_m1; \
  SI f_br_disp; \
  UINT f_br_zero; \
  unsigned int length;
#define EXTRACT_IFMT_CMPOBE_LIT_CODE \
  length = 4; \
  f_opcode = EXTRACT_MSB0_UINT (insn, 32, 0, 8); \
  f_br_src1 = EXTRACT_MSB0_UINT (insn, 32, 8, 5); \
  f_br_src2 = EXTRACT_MSB0_UINT (insn, 32, 13, 5); \
  f_br_m1 = EXTRACT_MSB0_UINT (insn, 32, 18, 1); \
  f_br_disp = ((((EXTRACT_MSB0_INT (insn, 32, 19, 11)) << (2))) + (pc)); \
  f_br_zero = EXTRACT_MSB0_UINT (insn, 32, 30, 2); \

#define EXTRACT_IFMT_BNO_VARS \
  UINT f_opcode; \
  SI f_ctrl_disp; \
  UINT f_ctrl_zero; \
  unsigned int length;
#define EXTRACT_IFMT_BNO_CODE \
  length = 4; \
  f_opcode = EXTRACT_MSB0_UINT (insn, 32, 0, 8); \
  f_ctrl_disp = ((((EXTRACT_MSB0_INT (insn, 32, 8, 22)) << (2))) + (pc)); \
  f_ctrl_zero = EXTRACT_MSB0_UINT (insn, 32, 30, 2); \

/* Collection of various things for the trace handler to use.  */

typedef struct trace_record {
  IADDR pc;
  /* FIXME:wip */
} TRACE_RECORD;

#endif /* CPU_I960BASE_H */
