/* CPU data header for i960.

THIS FILE IS MACHINE GENERATED WITH CGEN.

Copyright (C) 1996, 1997, 1998, 1999 Free Software Foundation, Inc.

This file is part of the GNU Binutils and/or GDB, the GNU debugger.

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

#ifndef I960_CPU_H
#define I960_CPU_H

#define CGEN_ARCH i960

/* Given symbol S, return i960_cgen_<S>.  */
#define CGEN_SYM(s) CONCAT3 (i960,_cgen_,s)

/* Selected cpu families.  */
#define HAVE_CPU_I960BASE

#define CGEN_INSN_LSB0_P 0

/* Minimum size of any insn (in bytes).  */
#define CGEN_MIN_INSN_SIZE 4

/* Maximum size of any insn (in bytes).  */
#define CGEN_MAX_INSN_SIZE 8

#define CGEN_INT_INSN_P 0

/* FIXME: Need to compute CGEN_MAX_SYNTAX_BYTES.  */

/* CGEN_MNEMONIC_OPERANDS is defined if mnemonics have operands.
   e.g. In "b,a foo" the ",a" is an operand.  If mnemonics have operands
   we can't hash on everything up to the space.  */
#define CGEN_MNEMONIC_OPERANDS

/* Maximum number of operands any insn or macro-insn has.  */
#define CGEN_MAX_INSN_OPERANDS 16

/* Maximum number of fields in an instruction.  */
#define CGEN_MAX_IFMT_OPERANDS 9

/* Enums.  */

/* Enum declaration for insn opcode enums.  */
typedef enum insn_opcode {
  OPCODE_00, OPCODE_01, OPCODE_02, OPCODE_03
 , OPCODE_04, OPCODE_05, OPCODE_06, OPCODE_07
 , OPCODE_08, OPCODE_09, OPCODE_0A, OPCODE_0B
 , OPCODE_0C, OPCODE_0D, OPCODE_0E, OPCODE_0F
 , OPCODE_10, OPCODE_11, OPCODE_12, OPCODE_13
 , OPCODE_14, OPCODE_15, OPCODE_16, OPCODE_17
 , OPCODE_18, OPCODE_19, OPCODE_1A, OPCODE_1B
 , OPCODE_1C, OPCODE_1D, OPCODE_1E, OPCODE_1F
 , OPCODE_20, OPCODE_21, OPCODE_22, OPCODE_23
 , OPCODE_24, OPCODE_25, OPCODE_26, OPCODE_27
 , OPCODE_28, OPCODE_29, OPCODE_2A, OPCODE_2B
 , OPCODE_2C, OPCODE_2D, OPCODE_2E, OPCODE_2F
 , OPCODE_30, OPCODE_31, OPCODE_32, OPCODE_33
 , OPCODE_34, OPCODE_35, OPCODE_36, OPCODE_37
 , OPCODE_38, OPCODE_39, OPCODE_3A, OPCODE_3B
 , OPCODE_3C, OPCODE_3D, OPCODE_3E, OPCODE_3F
 , OPCODE_40, OPCODE_41, OPCODE_42, OPCODE_43
 , OPCODE_44, OPCODE_45, OPCODE_46, OPCODE_47
 , OPCODE_48, OPCODE_49, OPCODE_4A, OPCODE_4B
 , OPCODE_4C, OPCODE_4D, OPCODE_4E, OPCODE_4F
 , OPCODE_50, OPCODE_51, OPCODE_52, OPCODE_53
 , OPCODE_54, OPCODE_55, OPCODE_56, OPCODE_57
 , OPCODE_58, OPCODE_59, OPCODE_5A, OPCODE_5B
 , OPCODE_5C, OPCODE_5D, OPCODE_5E, OPCODE_5F
 , OPCODE_60, OPCODE_61, OPCODE_62, OPCODE_63
 , OPCODE_64, OPCODE_65, OPCODE_66, OPCODE_67
 , OPCODE_68, OPCODE_69, OPCODE_6A, OPCODE_6B
 , OPCODE_6C, OPCODE_6D, OPCODE_6E, OPCODE_6F
 , OPCODE_70, OPCODE_71, OPCODE_72, OPCODE_73
 , OPCODE_74, OPCODE_75, OPCODE_76, OPCODE_77
 , OPCODE_78, OPCODE_79, OPCODE_7A, OPCODE_7B
 , OPCODE_7C, OPCODE_7D, OPCODE_7E, OPCODE_7F
 , OPCODE_80, OPCODE_81, OPCODE_82, OPCODE_83
 , OPCODE_84, OPCODE_85, OPCODE_86, OPCODE_87
 , OPCODE_88, OPCODE_89, OPCODE_8A, OPCODE_8B
 , OPCODE_8C, OPCODE_8D, OPCODE_8E, OPCODE_8F
 , OPCODE_90, OPCODE_91, OPCODE_92, OPCODE_93
 , OPCODE_94, OPCODE_95, OPCODE_96, OPCODE_97
 , OPCODE_98, OPCODE_99, OPCODE_9A, OPCODE_9B
 , OPCODE_9C, OPCODE_9D, OPCODE_9E, OPCODE_9F
 , OPCODE_A0, OPCODE_A1, OPCODE_A2, OPCODE_A3
 , OPCODE_A4, OPCODE_A5, OPCODE_A6, OPCODE_A7
 , OPCODE_A8, OPCODE_A9, OPCODE_AA, OPCODE_AB
 , OPCODE_AC, OPCODE_AD, OPCODE_AE, OPCODE_AF
 , OPCODE_B0, OPCODE_B1, OPCODE_B2, OPCODE_B3
 , OPCODE_B4, OPCODE_B5, OPCODE_B6, OPCODE_B7
 , OPCODE_B8, OPCODE_B9, OPCODE_BA, OPCODE_BB
 , OPCODE_BC, OPCODE_BD, OPCODE_BE, OPCODE_BF
 , OPCODE_C0, OPCODE_C1, OPCODE_C2, OPCODE_C3
 , OPCODE_C4, OPCODE_C5, OPCODE_C6, OPCODE_C7
 , OPCODE_C8, OPCODE_C9, OPCODE_CA, OPCODE_CB
 , OPCODE_CC, OPCODE_CD, OPCODE_CE, OPCODE_CF
 , OPCODE_D0, OPCODE_D1, OPCODE_D2, OPCODE_D3
 , OPCODE_D4, OPCODE_D5, OPCODE_D6, OPCODE_D7
 , OPCODE_D8, OPCODE_D9, OPCODE_DA, OPCODE_DB
 , OPCODE_DC, OPCODE_DD, OPCODE_DE, OPCODE_DF
 , OPCODE_E0, OPCODE_E1, OPCODE_E2, OPCODE_E3
 , OPCODE_E4, OPCODE_E5, OPCODE_E6, OPCODE_E7
 , OPCODE_E8, OPCODE_E9, OPCODE_EA, OPCODE_EB
 , OPCODE_EC, OPCODE_ED, OPCODE_EE, OPCODE_EF
 , OPCODE_F0, OPCODE_F1, OPCODE_F2, OPCODE_F3
 , OPCODE_F4, OPCODE_F5, OPCODE_F6, OPCODE_F7
 , OPCODE_F8, OPCODE_F9, OPCODE_FA, OPCODE_FB
 , OPCODE_FC, OPCODE_FD, OPCODE_FE, OPCODE_FF
} INSN_OPCODE;

/* Enum declaration for insn opcode2 enums.  */
typedef enum insn_opcode2 {
  OPCODE2_0, OPCODE2_1, OPCODE2_2, OPCODE2_3
 , OPCODE2_4, OPCODE2_5, OPCODE2_6, OPCODE2_7
 , OPCODE2_8, OPCODE2_9, OPCODE2_A, OPCODE2_B
 , OPCODE2_C, OPCODE2_D, OPCODE2_E, OPCODE2_F
} INSN_OPCODE2;

/* Enum declaration for insn m3 enums.  */
typedef enum insn_m3 {
  M3_0, M3_1
} INSN_M3;

/* Enum declaration for insn m3 enums.  */
typedef enum insn_m2 {
  M2_0, M2_1
} INSN_M2;

/* Enum declaration for insn m1 enums.  */
typedef enum insn_m1 {
  M1_0, M1_1
} INSN_M1;

/* Enum declaration for insn zero enums.  */
typedef enum insn_zero {
  ZERO_0
} INSN_ZERO;

/* Enum declaration for insn mode a enums.  */
typedef enum insn_modea {
  MODEA_OFFSET, MODEA_INDIRECT_OFFSET
} INSN_MODEA;

/* Enum declaration for insn zero a enums.  */
typedef enum insn_zeroa {
  ZEROA_0
} INSN_ZEROA;

/* Enum declaration for insn mode b enums.  */
typedef enum insn_modeb {
  MODEB_ILL0, MODEB_ILL1, MODEB_ILL2, MODEB_ILL3
 , MODEB_INDIRECT, MODEB_IP_DISP, MODEB_RES6, MODEB_INDIRECT_INDEX
 , MODEB_ILL8, MODEB_ILL9, MODEB_ILL10, MODEB_ILL11
 , MODEB_DISP, MODEB_INDIRECT_DISP, MODEB_INDEX_DISP, MODEB_INDIRECT_INDEX_DISP
} INSN_MODEB;

/* Enum declaration for insn zero b enums.  */
typedef enum insn_zerob {
  ZEROB_0
} INSN_ZEROB;

/* Enum declaration for insn branch m1 enums.  */
typedef enum insn_br_m1 {
  BR_M1_0, BR_M1_1
} INSN_BR_M1;

/* Enum declaration for insn branch zero enums.  */
typedef enum insn_br_zero {
  BR_ZERO_0
} INSN_BR_ZERO;

/* Enum declaration for insn ctrl zero enums.  */
typedef enum insn_ctrl_zero {
  CTRL_ZERO_0
} INSN_CTRL_ZERO;

/* Attributes.  */

/* Enum declaration for machine type selection.  */
typedef enum mach_attr {
  MACH_BASE, MACH_I960_KA_SA, MACH_I960_CA, MACH_MAX
} MACH_ATTR;

/* Enum declaration for instruction set selection.  */
typedef enum isa_attr {
  ISA_I960, ISA_MAX
} ISA_ATTR;

/* Number of architecture variants.  */
#define MAX_ISAS  1
#define MAX_MACHS ((int) MACH_MAX)

/* Ifield support.  */

extern const struct cgen_ifld i960_cgen_ifld_table[];

/* Ifield attribute indices.  */

/* Enum declaration for cgen_ifld attrs.  */
typedef enum cgen_ifld_attr {
  CGEN_IFLD_VIRTUAL, CGEN_IFLD_PCREL_ADDR, CGEN_IFLD_ABS_ADDR, CGEN_IFLD_RESERVED
 , CGEN_IFLD_SIGN_OPT, CGEN_IFLD_SIGNED, CGEN_IFLD_END_BOOLS, CGEN_IFLD_START_NBOOLS = 31
 , CGEN_IFLD_MACH, CGEN_IFLD_END_NBOOLS
} CGEN_IFLD_ATTR;

/* Number of non-boolean elements in cgen_ifld_attr.  */
#define CGEN_IFLD_NBOOL_ATTRS (CGEN_IFLD_END_NBOOLS - CGEN_IFLD_START_NBOOLS - 1)

/* Enum declaration for i960 ifield types.  */
typedef enum ifield_type {
  I960_F_NIL, I960_F_OPCODE, I960_F_SRCDST, I960_F_SRC2
 , I960_F_M3, I960_F_M2, I960_F_M1, I960_F_OPCODE2
 , I960_F_ZERO, I960_F_SRC1, I960_F_ABASE, I960_F_MODEA
 , I960_F_ZEROA, I960_F_OFFSET, I960_F_MODEB, I960_F_SCALE
 , I960_F_ZEROB, I960_F_INDEX, I960_F_OPTDISP, I960_F_BR_SRC1
 , I960_F_BR_SRC2, I960_F_BR_M1, I960_F_BR_DISP, I960_F_BR_ZERO
 , I960_F_CTRL_DISP, I960_F_CTRL_ZERO, I960_F_MAX
} IFIELD_TYPE;

#define MAX_IFLD ((int) I960_F_MAX)

/* Hardware attribute indices.  */

/* Enum declaration for cgen_hw attrs.  */
typedef enum cgen_hw_attr {
  CGEN_HW_VIRTUAL, CGEN_HW_CACHE_ADDR, CGEN_HW_PC, CGEN_HW_PROFILE
 , CGEN_HW_END_BOOLS, CGEN_HW_START_NBOOLS = 31, CGEN_HW_MACH, CGEN_HW_END_NBOOLS
} CGEN_HW_ATTR;

/* Number of non-boolean elements in cgen_hw_attr.  */
#define CGEN_HW_NBOOL_ATTRS (CGEN_HW_END_NBOOLS - CGEN_HW_START_NBOOLS - 1)

/* Enum declaration for i960 hardware types.  */
typedef enum cgen_hw_type {
  HW_H_MEMORY, HW_H_SINT, HW_H_UINT, HW_H_ADDR
 , HW_H_IADDR, HW_H_PC, HW_H_GR, HW_H_CC
 , HW_MAX
} CGEN_HW_TYPE;

#define MAX_HW ((int) HW_MAX)

/* Operand attribute indices.  */

/* Enum declaration for cgen_operand attrs.  */
typedef enum cgen_operand_attr {
  CGEN_OPERAND_VIRTUAL, CGEN_OPERAND_PCREL_ADDR, CGEN_OPERAND_ABS_ADDR, CGEN_OPERAND_SIGN_OPT
 , CGEN_OPERAND_SIGNED, CGEN_OPERAND_NEGATIVE, CGEN_OPERAND_RELAX, CGEN_OPERAND_SEM_ONLY
 , CGEN_OPERAND_END_BOOLS, CGEN_OPERAND_START_NBOOLS = 31, CGEN_OPERAND_MACH, CGEN_OPERAND_END_NBOOLS
} CGEN_OPERAND_ATTR;

/* Number of non-boolean elements in cgen_operand_attr.  */
#define CGEN_OPERAND_NBOOL_ATTRS (CGEN_OPERAND_END_NBOOLS - CGEN_OPERAND_START_NBOOLS - 1)

/* Enum declaration for i960 operand types.  */
typedef enum cgen_operand_type {
  I960_OPERAND_PC, I960_OPERAND_SRC1, I960_OPERAND_SRC2, I960_OPERAND_DST
 , I960_OPERAND_LIT1, I960_OPERAND_LIT2, I960_OPERAND_ST_SRC, I960_OPERAND_ABASE
 , I960_OPERAND_OFFSET, I960_OPERAND_SCALE, I960_OPERAND_INDEX, I960_OPERAND_OPTDISP
 , I960_OPERAND_BR_SRC1, I960_OPERAND_BR_SRC2, I960_OPERAND_BR_DISP, I960_OPERAND_BR_LIT1
 , I960_OPERAND_CTRL_DISP, I960_OPERAND_MAX
} CGEN_OPERAND_TYPE;

/* Number of operands types.  */
#define MAX_OPERANDS ((int) I960_OPERAND_MAX)

/* Maximum number of operands referenced by any insn.  */
#define MAX_OPERAND_INSTANCES 8

/* Insn attribute indices.  */

/* Enum declaration for cgen_insn attrs.  */
typedef enum cgen_insn_attr {
  CGEN_INSN_ALIAS, CGEN_INSN_VIRTUAL, CGEN_INSN_UNCOND_CTI, CGEN_INSN_COND_CTI
 , CGEN_INSN_SKIP_CTI, CGEN_INSN_DELAY_SLOT, CGEN_INSN_RELAXABLE, CGEN_INSN_RELAX
 , CGEN_INSN_NO_DIS, CGEN_INSN_PBB, CGEN_INSN_END_BOOLS, CGEN_INSN_START_NBOOLS = 31
 , CGEN_INSN_MACH, CGEN_INSN_END_NBOOLS
} CGEN_INSN_ATTR;

/* Number of non-boolean elements in cgen_insn_attr.  */
#define CGEN_INSN_NBOOL_ATTRS (CGEN_INSN_END_NBOOLS - CGEN_INSN_START_NBOOLS - 1)

/* cgen.h uses things we just defined.  */
#include "opcode/cgen.h"

/* Attributes.  */
extern const CGEN_ATTR_TABLE i960_cgen_hardware_attr_table[];
extern const CGEN_ATTR_TABLE i960_cgen_ifield_attr_table[];
extern const CGEN_ATTR_TABLE i960_cgen_operand_attr_table[];
extern const CGEN_ATTR_TABLE i960_cgen_insn_attr_table[];

/* Hardware decls.  */

extern CGEN_KEYWORD i960_cgen_opval_h_gr;
extern CGEN_KEYWORD i960_cgen_opval_h_cc;




#endif /* I960_CPU_H */
