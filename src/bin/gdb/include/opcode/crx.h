/* crx.h -- Header file for CRX opcode and register tables.
   Copyright 2004 Free Software Foundation, Inc.
   Contributed by Tomer Levi, NSC, Israel.
   Originally written for GAS 2.12 by Tomer Levi, NSC, Israel.
   Updates, BFDizing, GNUifying and ELF support by Tomer Levi.

   This file is part of GAS, GDB and the GNU binutils.

   GAS, GDB, and GNU binutils is free software; you can redistribute it
   and/or modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at your
   option) any later version.

   GAS, GDB, and GNU binutils are distributed in the hope that they will be
   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef _CRX_H_
#define _CRX_H_

/* CRX core/debug Registers :
   The enums are used as indices to CRX registers table (crx_regtab).
   Therefore, order MUST be preserved.  */

typedef enum
  {
    /* 32-bit general purpose registers.  */
    r0, r1, r2, r3, r4, r5, r6, r7, r8, r9,
    r10, r11, r12, r13, r14, r15, ra, sp,
    /* 32-bit user registers.  */
    u0, u1, u2, u3, u4, u5, u6, u7, u8, u9,
    u10, u11, u12, u13, u14, u15, ura, usp,
    /* hi and lo registers.  */
    hi, lo,
    /* hi and lo user registers.  */
    uhi, ulo,
    /* Processor Status Register.  */
    psr,
    /* Configuration Register.  */
    cfg,
    /* Coprocessor Configuration Register.  */
    cpcfg,
    /* Cashe Configuration Register.  */
    ccfg,
    /* Interrupt Base Register.  */
    intbase,
    /* Interrupt Stack Pointer Register.  */
    isp,
    /* Coprocessor Enable Register.  */
    cen,
    /* Program Counter Register.  */
    pc,
    /* Not a register.  */
    nullregister,
    MAX_REG
  }
reg;

/* CRX Coprocessor registers and special registers :
   The enums are used as indices to CRX coprocessor registers table
   (crx_copregtab). Therefore, order MUST be preserved.  */

typedef enum
  {
    /* Coprocessor registers.  */
    c0 = MAX_REG, c1, c2, c3, c4, c5, c6, c7, c8,
    c9, c10, c11, c12, c13, c14, c15,
    /* Coprocessor special registers.  */
    cs0, cs1 ,cs2, cs3, cs4, cs5, cs6, cs7, cs8,
    cs9, cs10, cs11, cs12, cs13, cs14, cs15,
    /* Not a Coprocessor register.  */
    nullcopregister,
    MAX_COPREG
  }
copreg;

/* CRX Register types. */

typedef enum
  {
    CRX_PC_REGTYPE,   /*  pc type */
    CRX_R_REGTYPE,    /*  r<N>	  */
    CRX_U_REGTYPE,    /*  u<N>	  */
    CRX_C_REGTYPE,    /*  c<N>	  */
    CRX_CS_REGTYPE,   /*  cs<N>	  */
    CRX_MTPR_REGTYPE, /*  mtpr	  */
    CRX_CFG_REGTYPE   /*  *hi|lo, *cfg, psr */
  }
reg_type;

/* CRX argument types :
   The argument types correspond to instructions operands

   Argument types :
   r - register
   c - constant
   d - displacement
   ic - immediate
   icr - index register
   rbase - register base
   s - star ('*')
   copr - coprocessor register
   copsr - coprocessor special register.  */

typedef enum
  {
    arg_r, arg_c, arg_cr, arg_dc, arg_dcr, arg_sc,
    arg_ic, arg_icr, arg_rbase, arg_copr, arg_copsr,
    /* Not an argument.  */
    nullargs
  }
argtype;

/* CRX operand types :
   The operand types correspond to instructions operands

   Operand Types :
   cst4 - 4-bit encoded constant
   iN - N-bit immediate field
   d, dispsN - N-bit immediate signed displacement
   dispuN - N-bit immediate unsigned displacement
   absN - N-bit absolute address
   rbase - 4-bit genaral-purpose register specifier
   regr - 4-bit genaral-purpose register specifier
   regr8 - 8-bit register address space
   copregr - coprocessor register
   copsregr - coprocessor special register
   scl2 - 2-bit scaling factor for memory index
   ridx - register index.  */

typedef enum
  {
    dummy, cst4, disps9,
    i3, i4, i5, i8, i12, i16, i32,
    d5, d9, d17, d25, d33,
    abs16, abs32,
    rbase, rbase_cst4,
    rbase_dispu8, rbase_dispu12, rbase_dispu16, rbase_dispu28, rbase_dispu32,
    rbase_ridx_scl2_dispu6, rbase_ridx_scl2_dispu22,
    regr, regr8, copregr,copregr8,copsregr,
    /* Not an operand.  */
    nulloperand,
    /* Maximum supported operand.  */
    MAX_OPRD
  }
operand_type;

/* CRX instruction types.  */

#define ARITH_INS         1
#define LD_STOR_INS       2
#define BRANCH_INS        3
#define ARITH_BYTE_INS    4
#define CMPBR_INS         5
#define SHIFT_INS         6
#define BRANCH_NEQ_INS    7
#define LD_STOR_INS_INC   8
#define STOR_IMM_INS	  9
#define CSTBIT_INS       10
#define SYS_INS		 11
#define JMP_INS		 12
#define MUL_INS		 13
#define DIV_INS		 14
#define COP_BRANCH_INS   15
#define COP_REG_INS      16
#define COPS_REG_INS     17
#define DCR_BRANCH_INS   18
#define MMC_INS          19
#define MMU_INS          20

/* Maximum value supported for instruction types.  */
#define CRX_INS_MAX	(1 << 5)
/* Mask to record an instruction type.  */
#define CRX_INS_MASK	(CRX_INS_MAX - 1)
/* Return instruction type, given instruction's attributes.  */
#define CRX_INS_TYPE(attr) ((attr) & CRX_INS_MASK)

/* Indicates whether this instruction has a register list as parameter.  */
#define REG_LIST	CRX_INS_MAX
/* The operands in binary and assembly are placed in reverse order.
   load - (REVERSE_MATCH)/store - (! REVERSE_MATCH).  */
#define REVERSE_MATCH  (REG_LIST << 1)

/* Kind of displacement map used DISPU[BWD]4.  */
#define DISPUB4	       (REVERSE_MATCH << 1)
#define DISPUW4	       (DISPUB4 << 1)
#define DISPUD4	       (DISPUW4 << 1)
#define CST4MAP	       (DISPUB4 | DISPUW4 | DISPUD4)

/* Printing formats, where the instruction prefix isn't consecutive.  */
#define FMT_1	       (DISPUD4 << 1) /* 0xF0F00000 */
#define FMT_2	       (FMT_1 << 1)   /* 0xFFF0FF00 */
#define FMT_3	       (FMT_2 << 1)   /* 0xFFF00F00 */
#define FMT_4	       (FMT_3 << 1)   /* 0xFFF0F000 */
#define FMT_5	       (FMT_4 << 1)   /* 0xFFF0FFF0 */
#define FMT_CRX	       (FMT_1 | FMT_2 | FMT_3 | FMT_4 | FMT_5)

#define RELAXABLE      (FMT_5 << 1)

/* Maximum operands per instruction.  */
#define MAX_OPERANDS	  5
/* Maximum words per instruction.  */
#define MAX_WORDS	  3
/* Maximum register name length. */
#define MAX_REGNAME_LEN	  10
/* Maximum instruction length. */
#define MAX_INST_LEN	  256

/* Single operand description.  */

typedef struct
  {
    /* Operand type.  */
    operand_type op_type;
    /* Operand location within the opcode.  */
    unsigned int shift;
  }
operand_desc;

/* Instruction data structure used in instruction table.  */

typedef struct
  {
    /* Name.  */
    const char *mnemonic;
    /* Size (in words).  */
    unsigned int size;
    /* Constant prefix (matched by the disassembler).  */
    unsigned long match;
    /* Match size (in bits).  */
    int match_bits;
    /* Attributes.  */
    unsigned int flags;
    /* Operands (always last, so unreferenced operands are initialized).  */
    operand_desc operands[MAX_OPERANDS];
  }
inst;

/* Data structure for a single instruction's arguments (Operands).  */

typedef struct
  {
    /* Register or base register.  */
    reg r;
    /* Index register.  */
    reg i_r;
    /* Coprocessor register.  */
    copreg cr;
    /* Constant/immediate/absolute value.  */
    unsigned long int constant;
    /* Scaled index mode.  */
    unsigned int scale;
    /* Argument type.  */
    argtype type;
    /* Size of the argument (in bits) required to represent.  */
    int size;
    /* Indicates whether a constant is positive or negative.  */
    int signflag;
  }
argument;

/* Internal structure to hold the various entities
   corresponding to the current assembling instruction.  */

typedef struct
  {
    /* Number of arguments.  */
    int nargs;
    /* The argument data structure for storing args (operands).  */
    argument arg[MAX_OPERANDS];
/* The following fields are required only by CRX-assembler.  */
#ifdef TC_CRX
    /* Expression used for setting the fixups (if any).  */
    expressionS exp;
    bfd_reloc_code_real_type rtype;
#endif /* TC_CRX */
    /* Instruction size (in bytes).  */
    int size;
  }
ins;

/* Structure to hold information about predefined operands.  */

typedef struct
  {
    /* Size (in bits).  */
    unsigned int bit_size;
    /* Argument type.  */
    argtype arg_type;
  }
operand_entry;

/* Structure to hold trap handler information.  */

typedef struct
  {
    /* Trap name.  */
    char *name;
    /* Index in dispatch table.  */
    unsigned int entry;
  }
trap_entry;

/* Structure to hold information about predefined registers.  */

typedef struct
  {
    /* Name (string representation).  */
    char *name;
    /* Value (enum representation).  */
    union
    {
      /* Register.  */
      reg reg_val;
      /* Coprocessor register.  */
      copreg copreg_val;
    } value;
    /* Register image.  */
    int image;
    /* Register type.  */
    reg_type type;
  }
reg_entry;

/* Structure to hold a cst4 operand mapping.  */

typedef struct
  {
    /* The binary value which is written to the object file.  */
    int binary;
    /* The value which is mapped.  */
    int value;
  }
cst4_entry;

/* CRX opcode table.  */
extern const inst crx_instruction[];
extern const int crx_num_opcodes;
#define NUMOPCODES crx_num_opcodes

/* CRX operands table.  */
extern const operand_entry crx_optab[];

/* CRX registers table.  */
extern const reg_entry crx_regtab[];
extern const int crx_num_regs;
#define NUMREGS crx_num_regs

/* CRX coprocessor registers table.  */
extern const reg_entry crx_copregtab[];
extern const int crx_num_copregs;
#define NUMCOPREGS crx_num_copregs

/* CRX trap/interrupt table.  */
extern const trap_entry crx_traps[];
extern const int crx_num_traps;
#define NUMTRAPS crx_num_traps

/* cst4 operand mapping.  */
extern const cst4_entry cst4_map[];
extern const int cst4_maps;

/* Current instruction we're assembling.  */
extern const inst *instruction;

/* A macro for representing the instruction "constant" opcode, that is,
   the FIXED part of the instruction. The "constant" opcode is represented
   as a 32-bit unsigned long, where OPC is expanded (by a left SHIFT)
   over that range.  */
#define BIN(OPC,SHIFT)	(OPC << SHIFT)

/* Is the current instruction type is TYPE ?  */
#define IS_INSN_TYPE(TYPE)	      \
  (CRX_INS_TYPE(instruction->flags) == TYPE)

/* Is the current instruction mnemonic is MNEMONIC ?  */
#define IS_INSN_MNEMONIC(MNEMONIC)    \
  (strcmp(instruction->mnemonic,MNEMONIC) == 0)

/* Does the current instruction has register list ?  */
#define INST_HAS_REG_LIST	      \
  (instruction->flags & REG_LIST)

/* Long long type handling.  */
/* Replace all appearances of 'long long int' with LONGLONG.  */
typedef long long int LONGLONG;
typedef unsigned long long ULONGLONG;
/* A mask for the upper 31 bits of a 64 bits type.  */
#define UPPER31_MASK	0xFFFFFFFE00000000LL

#endif /* _CRX_H_ */
