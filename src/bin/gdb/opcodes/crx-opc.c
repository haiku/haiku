/* crx-opc.c -- Table of opcodes for the CRX processor.
   Copyright 2004 Free Software Foundation, Inc.
   Contributed by Tomer Levi NSC, Israel.
   Originally written for GAS 2.12 by Tomer Levi.

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

#include <stdio.h>
#include "libiberty.h"
#include "symcat.h"
#include "opcode/crx.h"

const inst crx_instruction[] =
{
/* Create an arithmetic instruction - INST[bw].  */
#define  ARITH_BYTE_INST(NAME, OPC) \
  /* opc8 cst4 r */							   \
  {NAME, 1, OPC,  24, ARITH_BYTE_INS, {{cst4,20}, {regr,16}}},		   \
  /* opc8 i16 r */							   \
  {NAME, 2, (OPC<<4)+0xE, 20, ARITH_BYTE_INS, {{i16,0},	{regr,16}}},	   \
  /* opc8 r r */							   \
  {NAME, 1, OPC+0x40, 24, ARITH_BYTE_INS, {{regr,20}, {regr,16}}}

  ARITH_BYTE_INST ("addub", 0x0),
  ARITH_BYTE_INST ("addb",  0x1),
  ARITH_BYTE_INST ("addcb", 0x2),
  ARITH_BYTE_INST ("andb",  0x3),
  ARITH_BYTE_INST ("cmpb",  0x4),
  ARITH_BYTE_INST ("movb",  0x5),
  ARITH_BYTE_INST ("orb",   0x6),
  ARITH_BYTE_INST ("subb",  0x7),
  ARITH_BYTE_INST ("subcb", 0x8),
  ARITH_BYTE_INST ("xorb",  0x9),
  ARITH_BYTE_INST ("mulb",  0xA),

  ARITH_BYTE_INST ("adduw", 0x10),
  ARITH_BYTE_INST ("addw",  0x11),
  ARITH_BYTE_INST ("addcw", 0x12),
  ARITH_BYTE_INST ("andw",  0x13),
  ARITH_BYTE_INST ("cmpw",  0x14),
  ARITH_BYTE_INST ("movw",  0x15),
  ARITH_BYTE_INST ("orw",   0x16),
  ARITH_BYTE_INST ("subw",  0x17),
  ARITH_BYTE_INST ("subcw", 0x18),
  ARITH_BYTE_INST ("xorw",  0x19),
  ARITH_BYTE_INST ("mulw",  0x1A),

/* Create an arithmetic instruction - INST[d].  */
#define  ARITH_INST(NAME, OPC) \
  /* opc8 cst4 r */						      \
  {NAME, 1, OPC,  24, ARITH_INS, {{cst4,20}, {regr,16}}},	      \
  /* opc8 i16 r */						      \
  {NAME, 2, (OPC<<4)+0xE, 20, ARITH_INS, {{i16,0},   {regr,16}}},     \
  /* opc8 i32 r */						      \
  {NAME, 3, (OPC<<4)+0xF, 20, ARITH_INS, {{i32,0},   {regr,16}}},     \
  /* opc8 r r */						      \
  {NAME, 1, OPC+0x40, 24, ARITH_INS, {{regr,20}, {regr,16}}}

  ARITH_INST ("addud", 0x20),
  ARITH_INST ("addd",  0x21),
  ARITH_INST ("addcd", 0x22),
  ARITH_INST ("andd",  0x23),
  ARITH_INST ("cmpd",  0x24),
  ARITH_INST ("movd",  0x25),
  ARITH_INST ("ord",   0x26),
  ARITH_INST ("subd",  0x27),
  ARITH_INST ("subcd", 0x28),
  ARITH_INST ("xord",  0x29),
  ARITH_INST ("muld",  0x2A),

/* Create a shift instruction.  */
#define  SHIFT_INST(NAME, OPRD, OPC1, SHIFT1, OPC2) \
  /* OPRD=i3 -->> opc9 i3 r */					      \
  /* OPRD=i4 -->> opc8 i4 r */					      \
  /* OPRD=i5 -->> opc7 i5 r */					      \
  {NAME, 1, OPC1, SHIFT1, SHIFT_INS, {{OPRD,20}, {regr,16}}},	      \
  /* opc8 r r */						      \
  {NAME, 1, OPC2, 24, SHIFT_INS, {{regr,20}, {regr,16}}}

  SHIFT_INST ("sllb", i3, 0x1F8, 23, 0x4D),
  SHIFT_INST ("srlb", i3, 0x1F9, 23, 0x4E),
  SHIFT_INST ("srab", i3, 0x1FA, 23, 0x4F),

  SHIFT_INST ("sllw", i4, 0xB6,  24, 0x5D),
  SHIFT_INST ("srlw", i4, 0xB7,  24, 0x5E),
  SHIFT_INST ("sraw", i4, 0xB8,  24, 0x5F),

  SHIFT_INST ("slld", i5, 0x78,  25, 0x6D),
  SHIFT_INST ("srld", i5, 0x79,  25, 0x6E),
  SHIFT_INST ("srad", i5, 0x7A,  25, 0x6F),

/* Create a conditional branch instruction.  */
#define  BRANCH_INST(NAME, OPC) \
  /* opc4 c4 dispe9 */						    \
  {NAME,  1, OPC, 24, BRANCH_INS | RELAXABLE, {{d9,16}}},	    \
  /* opc4 c4 disps17 */						    \
  {NAME,  2, (OPC<<8)+0x7E, 16,	BRANCH_INS | RELAXABLE, {{d17,0}}}, \
  /* opc4 c4 disps33 */						    \
  {NAME,  3, (OPC<<8)+0x7F, 16,	BRANCH_INS | RELAXABLE, {{d33,0}}}

  BRANCH_INST ("beq", 0x70),
  BRANCH_INST ("bne", 0x71),
  BRANCH_INST ("bcs", 0x72),
  BRANCH_INST ("bcc", 0x73),
  BRANCH_INST ("bhi", 0x74),
  BRANCH_INST ("bls", 0x75),
  BRANCH_INST ("bgt", 0x76),
  BRANCH_INST ("ble", 0x77),
  BRANCH_INST ("bfs", 0x78),
  BRANCH_INST ("bfc", 0x79),
  BRANCH_INST ("blo", 0x7A),
  BRANCH_INST ("bhs", 0x7B),
  BRANCH_INST ("blt", 0x7C),
  BRANCH_INST ("bge", 0x7D),
  BRANCH_INST ("br",  0x7E),

/* Create a 'Branch if Equal to 0' instruction.  */
#define  BRANCH_NEQ_INST(NAME, OPC) \
  /* opc8 dispu5 r */						\
  {NAME,  1, OPC, 24, BRANCH_NEQ_INS, {{regr,16}, {d5,20}}}

  BRANCH_NEQ_INST ("beq0b",  0xB0),
  BRANCH_NEQ_INST ("bne0b",  0xB1),
  BRANCH_NEQ_INST ("beq0w",  0xB2),
  BRANCH_NEQ_INST ("bne0w",  0xB3),
  BRANCH_NEQ_INST ("beq0d",  0xB4),
  BRANCH_NEQ_INST ("bne0d",  0xB5),

/* Create instruction with no operands.  */
#define  NO_OP_INST(NAME, OPC) \
  /* opc16 */				\
  {NAME,  1, OPC, 16, 0, {{0, 0}}}

  NO_OP_INST ("nop",	0x3002),
  NO_OP_INST ("retx",	0x3003),
  NO_OP_INST ("di",	0x3004),
  NO_OP_INST ("ei",	0x3005),
  NO_OP_INST ("wait",	0x3006),
  NO_OP_INST ("eiwait",	0x3007),

/* Create a 'Compare & Branch' instruction.  */
#define  CMPBR_INST(NAME, OPC1, OPC2, C4) \
  /* opc12 r r c4 disps9 */										\
  {NAME, 2, ((0x300+OPC1)<<12)+C4,  8, CMPBR_INS | FMT_3 | RELAXABLE, {{regr,16}, {regr,12}, {d9,0}}},  \
  /* opc12 r r c4 disps25 */										\
  {NAME, 3, ((0x310+OPC1)<<12)+C4,  8, CMPBR_INS | FMT_3 | RELAXABLE, {{regr,16}, {regr,12}, {d25,0}}}, \
  /* opc12 i4cst4 r c4 disps9 */									\
  {NAME, 2, ((0x300+OPC2)<<12)+C4,  8, CMPBR_INS | FMT_3 | RELAXABLE, {{cst4,16}, {regr,12}, {d9,0}}},  \
  /* opc12 i4cst4 r c4 disps25 */									\
  {NAME, 3, ((0x310+OPC2)<<12)+C4,  8, CMPBR_INS | FMT_3 | RELAXABLE, {{cst4,16}, {regr,12}, {d25,0}}}

  CMPBR_INST ("cmpbeqb", 0x8, 0xC, 0x0),
  CMPBR_INST ("cmpbneb", 0x8, 0xC, 0x1),
  CMPBR_INST ("cmpbhib", 0x8, 0xC, 0x4),
  CMPBR_INST ("cmpblsb", 0x8, 0xC, 0x5),
  CMPBR_INST ("cmpbgtb", 0x8, 0xC, 0x6),
  CMPBR_INST ("cmpbleb", 0x8, 0xC, 0x7),
  CMPBR_INST ("cmpblob", 0x8, 0xC, 0xA),
  CMPBR_INST ("cmpbhsb", 0x8, 0xC, 0xB),
  CMPBR_INST ("cmpbltb", 0x8, 0xC, 0xC),
  CMPBR_INST ("cmpbgeb", 0x8, 0xC, 0xD),

  CMPBR_INST ("cmpbeqw", 0x9, 0xD, 0x0),
  CMPBR_INST ("cmpbnew", 0x9, 0xD, 0x1),
  CMPBR_INST ("cmpbhiw", 0x9, 0xD, 0x4),
  CMPBR_INST ("cmpblsw", 0x9, 0xD, 0x5),
  CMPBR_INST ("cmpbgtw", 0x9, 0xD, 0x6),
  CMPBR_INST ("cmpblew", 0x9, 0xD, 0x7),
  CMPBR_INST ("cmpblow", 0x9, 0xD, 0xA),
  CMPBR_INST ("cmpbhsw", 0x9, 0xD, 0xB),
  CMPBR_INST ("cmpbltw", 0x9, 0xD, 0xC),
  CMPBR_INST ("cmpbgew", 0x9, 0xD, 0xD),

  CMPBR_INST ("cmpbeqd", 0xA, 0xE, 0x0),
  CMPBR_INST ("cmpbned", 0xA, 0xE, 0x1),
  CMPBR_INST ("cmpbhid", 0xA, 0xE, 0x4),
  CMPBR_INST ("cmpblsd", 0xA, 0xE, 0x5),
  CMPBR_INST ("cmpbgtd", 0xA, 0xE, 0x6),
  CMPBR_INST ("cmpbled", 0xA, 0xE, 0x7),
  CMPBR_INST ("cmpblod", 0xA, 0xE, 0xA),
  CMPBR_INST ("cmpbhsd", 0xA, 0xE, 0xB),
  CMPBR_INST ("cmpbltd", 0xA, 0xE, 0xC),
  CMPBR_INST ("cmpbged", 0xA, 0xE, 0xD),

/* Create an instruction using a single register operand.  */
#define  REG1_INST(NAME, OPC) \
  /* opc8 c4 r */			  \
  {NAME,  1, OPC, 20, 0, {{regr,16}}}

  /* JCond instructions	*/
  REG1_INST ("jeq",  0xBA0),
  REG1_INST ("jne",  0xBA1),
  REG1_INST ("jcs",  0xBA2),
  REG1_INST ("jcc",  0xBA3),
  REG1_INST ("jhi",  0xBA4),
  REG1_INST ("jls",  0xBA5),
  REG1_INST ("jgt",  0xBA6),
  REG1_INST ("jle",  0xBA7),
  REG1_INST ("jfs",  0xBA8),
  REG1_INST ("jfc",  0xBA9),
  REG1_INST ("jlo",  0xBAA),
  REG1_INST ("jhs",  0xBAB),
  REG1_INST ("jlt",  0xBAC),
  REG1_INST ("jge",  0xBAD),
  REG1_INST ("jump", 0xBAE),

  /* SCond instructions */
  REG1_INST ("seq",  0xBB0),
  REG1_INST ("sne",  0xBB1),
  REG1_INST ("scs",  0xBB2),
  REG1_INST ("scc",  0xBB3),
  REG1_INST ("shi",  0xBB4),
  REG1_INST ("sls",  0xBB5),
  REG1_INST ("sgt",  0xBB6),
  REG1_INST ("sle",  0xBB7),
  REG1_INST ("sfs",  0xBB8),
  REG1_INST ("sfc",  0xBB9),
  REG1_INST ("slo",  0xBBA),
  REG1_INST ("shs",  0xBBB),
  REG1_INST ("slt",  0xBBC),
  REG1_INST ("sge",  0xBBD),

/* Create an instruction using two register operands.  */
#define  REG2_INST(NAME, OPC) \
  /* opc24 r r  OR  opc20 c4 r r */			      \
  {NAME,  2, 0x300800+OPC,  8, 0, {{regr,4}, {regr,0}}}

  /* MULTIPLY INSTRUCTIONS */
  REG2_INST ("macsb",  0x40),
  REG2_INST ("macub",  0x41),
  REG2_INST ("macqb",  0x42),

  REG2_INST ("macsw",  0x50),
  REG2_INST ("macuw",  0x51),
  REG2_INST ("macqw",  0x52),

  REG2_INST ("macsd",  0x60),
  REG2_INST ("macud",  0x61),
  REG2_INST ("macqd",  0x62),

  REG2_INST ("mullsd", 0x65),
  REG2_INST ("mullud", 0x66),

  REG2_INST ("mulsbw", 0x3B),
  REG2_INST ("mulubw", 0x3C),
  REG2_INST ("mulswd", 0x3D),
  REG2_INST ("muluwd", 0x3E),

  /*  SIGNEXTEND STUFF    */
  REG2_INST ("sextbw", 0x30),
  REG2_INST ("sextbd", 0x31),
  REG2_INST ("sextwd", 0x32),
  REG2_INST ("zextbw", 0x34),
  REG2_INST ("zextbd", 0x35),
  REG2_INST ("zextwd", 0x36),

  REG2_INST ("bswap",  0x3F),

  REG2_INST ("maxsb",  0x80),
  REG2_INST ("minsb",  0x81),
  REG2_INST ("maxub",  0x82),
  REG2_INST ("minub",  0x83),
  REG2_INST ("absb",   0x84),
  REG2_INST ("negb",   0x85),
  REG2_INST ("cntl0b", 0x86),
  REG2_INST ("cntl1b", 0x87),
  REG2_INST ("popcntb",0x88),
  REG2_INST ("rotlb",  0x89),
  REG2_INST ("rotrb",  0x8A),
  REG2_INST ("mulqb",  0x8B),
  REG2_INST ("addqb",  0x8C),
  REG2_INST ("subqb",  0x8D),
  REG2_INST ("cntlsb", 0x8E),

  REG2_INST ("maxsw",  0x90),
  REG2_INST ("minsw",  0x91),
  REG2_INST ("maxuw",  0x92),
  REG2_INST ("minuw",  0x93),
  REG2_INST ("absw",   0x94),
  REG2_INST ("negw",   0x95),
  REG2_INST ("cntl0w", 0x96),
  REG2_INST ("cntl1w", 0x97),
  REG2_INST ("popcntw",0x98),
  REG2_INST ("rotlw",  0x99),
  REG2_INST ("rotrw",  0x9A),
  REG2_INST ("mulqw",  0x9B),
  REG2_INST ("addqw",  0x9C),
  REG2_INST ("subqw",  0x9D),
  REG2_INST ("cntlsw", 0x9E),

  REG2_INST ("maxsd",  0xA0),
  REG2_INST ("minsd",  0xA1),
  REG2_INST ("maxud",  0xA2),
  REG2_INST ("minud",  0xA3),
  REG2_INST ("absd",   0xA4),
  REG2_INST ("negd",   0xA5),
  REG2_INST ("cntl0d", 0xA6),
  REG2_INST ("cntl1d", 0xA7),
  REG2_INST ("popcntd",0xA8),
  REG2_INST ("rotld",  0xA9),
  REG2_INST ("rotrd",  0xAA),
  REG2_INST ("mulqd",  0xAB),
  REG2_INST ("addqd",  0xAC),
  REG2_INST ("subqd",  0xAD),
  REG2_INST ("cntlsd", 0xAE),

/* Conditional move instructions */
  REG2_INST ("cmoveqd", 0x70),
  REG2_INST ("cmovned", 0x71),
  REG2_INST ("cmovcsd", 0x72),
  REG2_INST ("cmovccd", 0x73),
  REG2_INST ("cmovhid", 0x74),
  REG2_INST ("cmovlsd", 0x75),
  REG2_INST ("cmovgtd", 0x76),
  REG2_INST ("cmovled", 0x77),
  REG2_INST ("cmovfsd", 0x78),
  REG2_INST ("cmovfcd", 0x79),
  REG2_INST ("cmovlod", 0x7A),
  REG2_INST ("cmovhsd", 0x7B),
  REG2_INST ("cmovltd", 0x7C),
  REG2_INST ("cmovged", 0x7D),

/* Load instructions (from memory to register).  */
#define  LD_REG_INST(NAME, OPC1, OPC2, DISP) \
  /* opc12 r abs16 */									 \
  {NAME,  2, 0x320+OPC1,  20, LD_STOR_INS | REVERSE_MATCH, {{abs16,0}, {regr,16}}},	 \
  /* opc12 r abs32 */									 \
  {NAME,  3, 0x330+OPC1,  20, LD_STOR_INS | REVERSE_MATCH, {{abs32,0}, {regr,16}}},	 \
  /* opc4 r c4 rbase */									 \
  {NAME,  1, ((0x8+OPC2)<<8),  20, LD_STOR_INS | DISP | FMT_1 | REVERSE_MATCH, {{rbase,20}, {regr,24}}},\
  /* opc4 r rbase dispu[bwd]4 */							 \
  {NAME,  1, 0x8+OPC2,  28, LD_STOR_INS | DISP | REVERSE_MATCH, {{rbase_cst4,16}, {regr,24}}},		 \
  /* opc4 r rbase disps16 */								 \
  {NAME,  2, ((0x8+OPC2)<<8)+0xE,  20, LD_STOR_INS | DISP | FMT_1 | REVERSE_MATCH, {{rbase_dispu16,16}, {regr,24}}}, \
  /* opc4 r rbase disps32 */								 \
  {NAME,  3, ((0x8+OPC2)<<8)+0xF,  20, LD_STOR_INS | FMT_1 | REVERSE_MATCH, {{rbase_dispu32,16}, {regr,24}}}, \
  /* opc12 r rbase */									 \
  {NAME,  2, 0x328+OPC1,  20, LD_STOR_INS_INC | REVERSE_MATCH, {{rbase,12}, {regr,16}}},		 \
  /* opc12 r rbase disps12 */								 \
  {NAME,  2, 0x328+OPC1,  20, LD_STOR_INS_INC | REVERSE_MATCH, {{rbase_dispu12,12}, {regr,16}}},	 \
  /* opc12 r rbase ridx scl2 disps6 */							 \
  {NAME,  2, 0x32C+OPC1,  20, LD_STOR_INS | REVERSE_MATCH, {{rbase_ridx_scl2_dispu6,0}, {regr,16}}},	 \
  /* opc12 r rbase ridx scl2 disps22 */							 \
  {NAME,  3, 0x33C+OPC1,  20, LD_STOR_INS | REVERSE_MATCH, {{rbase_ridx_scl2_dispu22,0}, {regr,16}}}

  LD_REG_INST ("loadb", 0x0, 0x0, DISPUB4),
  LD_REG_INST ("loadw", 0x1, 0x1, DISPUW4),
  LD_REG_INST ("loadd", 0x2, 0x2, DISPUD4),

/* Store instructions (from Register to Memory).  */
#define  ST_REG_INST(NAME, OPC1, OPC2, DISP) \
  /* opc12 r abs16 */									 \
  {NAME,  2, 0x320+OPC1,  20, LD_STOR_INS, {{regr,16}, {abs16,0}}},			 \
  /* opc12 r abs32 */									 \
  {NAME,  3, 0x330+OPC1,  20, LD_STOR_INS, {{regr,16}, {abs32,0}}},			 \
  /* opc4 r c4 rbase */									 \
  {NAME,  1, ((0x8+OPC2)<<8),  20, LD_STOR_INS | DISP | FMT_1, {{regr,24}, {rbase,20}}},\
  /* opc4 r rbase dispu[bwd]4 */							 \
  {NAME,  1, 0x8+OPC2,  28, LD_STOR_INS | DISP, {{regr,24}, {rbase_cst4,16}}},		 \
  /* opc4 r rbase disps16 */								 \
  {NAME,  2, ((0x8+OPC2)<<8)+0xE,  20, LD_STOR_INS | DISP | FMT_1, {{regr,24}, {rbase_dispu16,16}}}, \
  /* opc4 r rbase disps32 */								 \
  {NAME,  3, ((0x8+OPC2)<<8)+0xF,  20, LD_STOR_INS | FMT_1, {{regr,24}, {rbase_dispu32,16}}}, \
  /* opc12 r rbase */									 \
  {NAME,  2, 0x328+OPC1,  20, LD_STOR_INS_INC, {{regr,16}, {rbase,12}}},		 \
  /* opc12 r rbase disps12 */								 \
  {NAME,  2, 0x328+OPC1,  20, LD_STOR_INS_INC, {{regr,16}, {rbase_dispu12,12}}},	 \
  /* opc12 r rbase ridx scl2 disps6 */							 \
  {NAME,  2, 0x32C+OPC1,  20, LD_STOR_INS, {{regr,16}, {rbase_ridx_scl2_dispu6,0}}},	 \
  /* opc12 r rbase ridx scl2 disps22 */							 \
  {NAME,  3, 0x33C+OPC1,  20, LD_STOR_INS, {{regr,16}, {rbase_ridx_scl2_dispu22,0}}}

/* Store instructions (Immediate to Memory).  */
#define  ST_I_INST(NAME, OPC) \
  /* opc12 i4 abs16 */								 \
  {NAME,  2, 0x360+OPC,	20, STOR_IMM_INS, {{i4,16}, {abs16,0}}},		 \
  /* opc12 i4 abs32 */								 \
  {NAME,  3, 0x370+OPC,	20, STOR_IMM_INS, {{i4,16}, {abs32,0}}},		 \
  /* opc12 i4 c4 rbase */							 \
  {NAME,  1, 0x368+OPC,	20, LD_STOR_INS_INC, {{i4,16}, {rbase,12}}},		 \
  /* opc12 i4 rbase disps12 */							 \
  {NAME,  2, 0x368+OPC,	20, LD_STOR_INS_INC, {{i4,16}, {rbase_dispu12,12}}},	 \
  /* opc4 i4 c4 rbase */							 \
  {NAME,  1, 0x364+OPC,	20, STOR_IMM_INS, {{i4,16}, {rbase,12}}},		 \
  /* opc12 i4 rbase disps12 */							 \
  {NAME,  2, 0x364+OPC,	20, STOR_IMM_INS, {{i4,16}, {rbase_dispu12,12}}},	 \
  /* opc12 i4 rbase disps28 */							 \
  {NAME,  3, 0x374+OPC,	20, STOR_IMM_INS, {{i4,16}, {rbase_dispu28,12}}},	 \
  /* opc12 i4 rbase ridx scl2 disps6 */						 \
  {NAME,  2, 0x36C+OPC,	20, STOR_IMM_INS, {{i4,16}, {rbase_ridx_scl2_dispu6,0}}},\
  /* opc12 i4 rbase ridx scl2 disps22 */					 \
  {NAME,  3, 0x37C+OPC,	20, STOR_IMM_INS, {{i4,16}, {rbase_ridx_scl2_dispu22,0}}}

  ST_REG_INST ("storb", 0x20, 0x4, DISPUB4),
  ST_I_INST ("storb",  0x0),

  ST_REG_INST ("storw", 0x21, 0x5, DISPUW4),
  ST_I_INST ("storw",  0x1),

  ST_REG_INST ("stord", 0x22, 0x6, DISPUD4),
  ST_I_INST ("stord",  0x2),

/* Create a bit instruction.  */
#define  CSTBIT_INST(NAME, OP, OPC1, DIFF, SHIFT, OPC2) \
  /* OP=i3 -->> opc13 i3 */								  \
  /* OP=i4 -->> opc12 i4 */								  \
  /* OP=i5 -->> opc11 i5 */								  \
											  \
  /* opcNN iN abs16 */									  \
  {NAME,  2, OPC1+0*DIFF, SHIFT, CSTBIT_INS, {{OP,16}, {abs16,0}}},			  \
  /* opcNN iN abs32 */									  \
  {NAME,  3, OPC1+1*DIFF, SHIFT, CSTBIT_INS, {{OP,16}, {abs32,0}}},			  \
  /* opcNN iN rbase */									  \
  {NAME,  1, OPC2,  SHIFT+4,  CSTBIT_INS, {{OP,20}, {rbase,16}}},			  \
  /* opcNN iN rbase disps12 */								  \
  {NAME,  2, OPC1+2*DIFF, SHIFT, CSTBIT_INS, {{OP,16}, {rbase_dispu12,12}}},		  \
  /* opcNN iN rbase disps28 */								  \
  {NAME,  3, OPC1+3*DIFF, SHIFT, CSTBIT_INS, {{OP,16}, {rbase_dispu28,12}}},		  \
  /* opcNN iN rbase ridx scl2 disps6 */							  \
  {NAME,  2, OPC1+4*DIFF, SHIFT, CSTBIT_INS, {{OP,16}, {rbase_ridx_scl2_dispu6,0}}},	  \
  /* opcNN iN rbase ridx scl2 disps22 */						  \
  {NAME,  3, OPC1+5*DIFF, SHIFT, CSTBIT_INS, {{OP,16}, {rbase_ridx_scl2_dispu22,0}}}

  CSTBIT_INST ("cbitb", i3, 0x700, 0x20, 19, 0x1FC),
  CSTBIT_INST ("cbitw", i4, 0x382, 0x10, 20, 0xBD),
  CSTBIT_INST ("cbitd", i5, 0x1C3, 0x8,  21, 0x7B),
  {"cbitd",   2, 0x300838,  8, CSTBIT_INS, {{regr,4}, {regr,0}}},
  {"cbitd",   2, 0x18047B,  9, CSTBIT_INS, {{i5,4}, {regr,0}}},

  CSTBIT_INST ("sbitb", i3, 0x701, 0x20, 19, 0x1FD),
  CSTBIT_INST ("sbitw", i4, 0x383, 0x10, 20, 0xBE),
  CSTBIT_INST ("sbitd", i5, 0x1C4, 0x8,  21, 0x7C),
  {"sbitd",   2, 0x300839,  8, CSTBIT_INS, {{regr,4}, {regr,0}}},
  {"sbitd",   2, 0x18047C,  9, CSTBIT_INS, {{i5,4}, {regr,0}}},

  CSTBIT_INST ("tbitb", i3, 0x702, 0x20, 19, 0x1FE),
  CSTBIT_INST ("tbitw", i4, 0x384, 0x10, 20, 0xBF),
  CSTBIT_INST ("tbitd", i5, 0x1C5, 0x8,  21, 0x7D),
  {"tbitd",   2, 0x30083A,  8, CSTBIT_INS, {{regr,4}, {regr,0}}},
  {"tbitd",   2, 0x18047D,  9, CSTBIT_INS, {{i5,4}, {regr,0}}},

/* Instructions including a register list (opcode is represented as a mask).  */
#define  REGLIST_INST(NAME, OPC) \
  /* opc12 r mask16 */					  \
  {NAME,  2, OPC, 20, REG_LIST, {{regr,16}, {i16,0}}}

  REG1_INST ("getrfid",	0xFF9),
  REG1_INST ("setrfid",	0xFFA),

  REGLIST_INST ("push",	 0x346),
  REG1_INST ("push",	 0xFFB),
  REGLIST_INST ("pushx", 0x347),

  REGLIST_INST ("pop",	 0x324),
  REG1_INST ("pop",	 0xFFC),
  REGLIST_INST ("popx",	 0x327),

  REGLIST_INST ("popret", 0x326),
  REG1_INST ("popret",    0xFFD),

  REGLIST_INST ("loadm",  0x324),
  REGLIST_INST ("loadma", 0x325),
  REGLIST_INST ("popa",	  0x325),

  REGLIST_INST ("storm",  0x344),
  REGLIST_INST ("storma", 0x345),

/* Create a branch instruction.  */
#define  BR_INST(NAME, OPC1, OPC2, INS_TYPE) \
  /* opc12 r disps17 */						      \
  {NAME,  2, OPC1,  20, INS_TYPE | RELAXABLE, {{regr,16}, {d17,0}}},  \
  /* opc12 r disps33 */						      \
  {NAME,  3, OPC2,  20, INS_TYPE | RELAXABLE, {{regr,16}, {d33,0}}}

  BR_INST ("bal",   0x307, 0x317, 0),

  /* Decrement and Branch instructions.  */
  BR_INST ("dbnzb", 0x304, 0x314, DCR_BRANCH_INS),
  BR_INST ("dbnzw", 0x305, 0x315, DCR_BRANCH_INS),
  BR_INST ("dbnzd", 0x306, 0x316, DCR_BRANCH_INS),

  /* Jump and link instructions.  */
  REG1_INST ("jal",    0xFF8),
  REG2_INST ("jal",    0x37),
  REG2_INST ("jalid",  0x33),

/* Create a CO-processor instruction.  */
#define  COP_INST(NAME, OPC, TYPE, REG1, REG2) \
  /* opc12 c4 opc8 REG1 REG2 */		       \
  {NAME,  2, 0x301030+OPC,  8, TYPE | FMT_2, {{i4,16}, {REG1,4}, {REG2,0}}}

  COP_INST ("mtcr",   0, COP_REG_INS,	regr,	  copregr),
  COP_INST ("mfcr",   1, COP_REG_INS,	copregr,  regr),
  COP_INST ("mtcsr",  2, COPS_REG_INS,	regr,	  copsregr),
  COP_INST ("mfcsr",  3, COPS_REG_INS,	copsregr, regr),
  COP_INST ("ldcr",   4, COP_REG_INS,	regr,	  copregr),
  COP_INST ("stcr",   5, COP_REG_INS,	regr,	  copregr),
  COP_INST ("ldcsr",  6, COPS_REG_INS,	regr,	  copsregr),
  COP_INST ("stcsr",  7, COPS_REG_INS,	regr,	  copsregr),

/* Create a memory-related CO-processor instruction.  */
#define  COPMEM_INST(NAME, OPC, TYPE) \
  /* opc12 c4 opc12 r mask16 */	      \
  {NAME,  3, 0x3110300+OPC,  4, TYPE | REG_LIST | FMT_5, {{i4,16}, {regr,0}, {i16,0}}}

  COPMEM_INST("loadmcr",  0,  COP_REG_INS),
  COPMEM_INST("stormcr",  1,  COP_REG_INS),
  COPMEM_INST("loadmcsr", 2,  COPS_REG_INS),
  COPMEM_INST("stormcsr", 3,  COPS_REG_INS),

  /* CO-processor extensions.  */
  /* opc12 c4 opc4 i4 disps9 */
  {"bcop",    2, 0x30107, 12, COP_BRANCH_INS | FMT_4, {{i4,16}, {i4,8}, {d9,0}}},
  /* opc12 c4 opc4 i4 disps25 */
  {"bcop",    3, 0x31107, 12, COP_BRANCH_INS | FMT_4, {{i4,16}, {i4,8}, {d25,0}}},
  /* opc12 c4 opc4 cpdo r r */
  {"cpdop",   2, 0x3010B, 12, COP_REG_INS | FMT_4, {{i4,16}, {i4,8}, {regr,4}, {regr,0}}},
  /* opc12 c4 opc4 cpdo r r cpdo16 */
  {"cpdop",   3, 0x3110B, 12, COP_REG_INS | FMT_4, {{i4,16}, {i4,8}, {regr,4}, {regr,0}, {i16,0}}},
  /* esc16 r procreg */
  {"mtpr",    2, 0x3009,  16, 0, {{regr8,8}, {regr8,0}}},
  /* esc16 procreg r */
  {"mfpr",    2, 0x300A,  16, 0, {{regr8,8}, {regr8,0}}},

  /* Miscellaneous.  */
  /* opc12 i4 */
  {"excp",    1, 0xFFF,	20, 0, {{i4,16}}},
  /* opc28 i4 */
  {"cinv",    2, 0x3010000, 4,	0, {{i4,0}}},

  /* opc9 i5 i5 i5 r r */
  {"ram",     2, 0x7C,	23, 0, {{i5,18}, {i5,13}, {i5,8}, {regr,4}, {regr,0}}},
  {"rim",     2, 0x7D,	23, 0, {{i5,18}, {i5,13}, {i5,8}, {regr,4}, {regr,0}}},

  /* opc9 i3 r */
  {"rotb",    1, 0x1FB,	23, 0, {{i3,20}, {regr,16}}},
  /* opc8 i4 r */
  {"rotw",    1, 0xB9,	24, 0, {{i4,20}, {regr,16}}},
  /* opc23 i5 r */
  {"rotd",    2, 0x180478,  9, 0, {{i5,4}, {regr,0}}},

  {NULL,      0, 0, 0,	0, {{0, 0}}}
};

const int crx_num_opcodes = ARRAY_SIZE (crx_instruction);

/* Macro to build a reg_entry, which have an opcode image :
   For example :
      REG(u4, 0x84, CRX_U_REGTYPE)
   is interpreted as :
      {"u4",  u4, 0x84, CRX_U_REGTYPE}  */
#define REG(NAME, N, TYPE)    {STRINGX(NAME), {NAME}, N, TYPE}

const reg_entry crx_regtab[] =
{
/* Build a general purpose register r<N>.  */
#define REG_R(N)    REG(CONCAT2(r,N), N, CRX_R_REGTYPE)

  REG_R(0),  REG_R(1),	REG_R(2),  REG_R(3),
  REG_R(4),  REG_R(5),	REG_R(6),  REG_R(7),
  REG_R(8),  REG_R(9),	REG_R(10), REG_R(11),
  REG_R(12), REG_R(13), REG_R(14), REG_R(15),
  REG(ra, 0xe, CRX_R_REGTYPE),
  REG(sp, 0xf, CRX_R_REGTYPE),

/* Build a user register u<N>.  */
#define REG_U(N)    REG(CONCAT2(u,N), 0x80 + N, CRX_U_REGTYPE)

  REG_U(0),  REG_U(1),  REG_U(2),  REG_U(3),
  REG_U(4),  REG_U(5),  REG_U(6),  REG_U(7),
  REG_U(8),  REG_U(9),  REG_U(10), REG_U(11),
  REG_U(12), REG_U(13), REG_U(14), REG_U(15),
  REG(ura, 0x8e, CRX_U_REGTYPE),
  REG(usp, 0x8f, CRX_U_REGTYPE),

/* Build a configuration register.  */
#define REG_CFG(NAME, N)    REG(NAME, N, CRX_CFG_REGTYPE)

  REG_CFG(hi,    0x10),
  REG_CFG(lo,    0x11),
  REG_CFG(uhi,   0x90),
  REG_CFG(ulo,   0x91),
  REG_CFG(psr,   0x12),
  REG_CFG(cfg,   0x15),
  REG_CFG(cpcfg, 0x16),
  REG_CFG(ccfg,	 0x1b),

/* Build a mptr register.  */
#define REG_MPTR(NAME, N)    REG(NAME, N, CRX_MTPR_REGTYPE)

  REG_MPTR(intbase, 0x13),
  REG_MPTR(isp,     0x14),
  REG_MPTR(cen,     0x17),

/* Build a pc register.  */
#define REG_PC(NAME, N)    REG(NAME, N, CRX_PC_REGTYPE)

  REG_PC(pc,  0x0)
};

const int crx_num_regs = ARRAY_SIZE (crx_regtab);

const reg_entry crx_copregtab[] =
{
/* Build a Coprocessor register c<N>.  */
#define REG_C(N)    REG(CONCAT2(c,N), N, CRX_C_REGTYPE)

  REG_C(0),  REG_C(1),	REG_C(2),  REG_C(3),
  REG_C(4),  REG_C(5),	REG_C(6),  REG_C(7),
  REG_C(8),  REG_C(9),	REG_C(10), REG_C(11),
  REG_C(12), REG_C(13), REG_C(14), REG_C(15),

/* Build a Coprocessor Special register cs<N>.  */
#define REG_CS(N)    REG(CONCAT2(cs,N), N, CRX_CS_REGTYPE)

  REG_CS(0),  REG_CS(1),  REG_CS(2),  REG_CS(3),
  REG_CS(4),  REG_CS(5),  REG_CS(6),  REG_CS(7),
  REG_CS(8),  REG_CS(9),  REG_CS(10), REG_CS(11),
  REG_CS(12), REG_CS(13), REG_CS(14), REG_CS(15)
};

const int crx_num_copregs = ARRAY_SIZE (crx_copregtab);

/* CRX operands table.  */
const operand_entry crx_optab[] =
{
  /* Index 0 is dummy, so we can count the instruction's operands.  */
  {0,	nullargs},  /* dummy */
  {4,	arg_ic},    /* cst4 */
  {8,	arg_c},	    /* disps9 */
  {3,	arg_ic},    /* i3 */
  {4,	arg_ic},    /* i4 */
  {5,	arg_ic},    /* i5 */
  {8,	arg_ic},    /* i8 */
  {12,	arg_ic},    /* i12 */
  {16,	arg_ic},    /* i16 */
  {32,	arg_ic},    /* i32 */
  {4,	arg_c},	    /* d5 */
  {8,	arg_c},	    /* d9 */
  {16,	arg_c},	    /* d17 */
  {24,	arg_c},	    /* d25 */
  {32,	arg_c},	    /* d33 */
  {16,	arg_c},	    /* abs16 */
  {32,	arg_c},	    /* abs32 */
  {4,	arg_rbase}, /* rbase */
  {4,	arg_cr},    /* rbase_cst4 */
  {8,	arg_cr},    /* rbase_dispu8 */
  {12,	arg_cr},    /* rbase_dispu12 */
  {16,	arg_cr},    /* rbase_dispu16 */
  {28,	arg_cr},    /* rbase_dispu28 */
  {32,	arg_cr},    /* rbase_dispu32 */
  {6,	arg_icr},   /* rbase_ridx_scl2_dispu6 */
  {22,  arg_icr},   /* rbase_ridx_scl2_dispu22 */
  {4,	arg_r},	    /* regr */
  {8,	arg_r},	    /* regr8 */
  {4,	arg_copr},  /* copregr */
  {8,	arg_copr},  /* copregr8 */
  {4,	arg_copsr}  /* copsregr */
};

/* CRX traps/interrupts.  */
const trap_entry crx_traps[] =
{
  {"nmi", 1}, {"svc", 5}, {"dvz", 6}, {"flg", 7},
  {"bpt", 8}, {"und", 10}, {"prv", 11}, {"iberr", 12}
};

const int crx_num_traps = ARRAY_SIZE (crx_traps);

/* cst4 operand mapping.  */
const cst4_entry cst4_map[] =
{
  {0,0}, {1,1}, {2,2}, {3,3}, {4,4}, {5,-4}, {6,-1},
  {7,7}, {8,8}, {9,16}, {10,32}, {11,20}, {12,12}, {13,48}
};

const int cst4_maps = ARRAY_SIZE (cst4_map);
