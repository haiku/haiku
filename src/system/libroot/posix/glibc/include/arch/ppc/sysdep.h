/* Copyright (C) 1999, 2001 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include_next <sysdep.h>

#ifdef __ASSEMBLER__

/* Symbolic names for the registers.  The only portable way to write asm
   code is to use number but this produces really unreadable code.
   Therefore these symbolic names.  */

/* Integer registers.  */
#define r0	0
#define r1	1
#define r2	2
#define r3	3
#define r4	4
#define r5	5
#define r6	6
#define r7	7
#define r8	8
#define r9	9
#define r10	10
#define r11	11
#define r12	12
#define r13	13
#define r14	14
#define r15	15
#define r16	16
#define r17	17
#define r18	18
#define r19	19
#define r20	20
#define r21	21
#define r22	22
#define r23	23
#define r24	24
#define r25	25
#define r26	26
#define r27	27
#define r28	28
#define r29	29
#define r30	30
#define r31	31

/* Floating-point registers.  */
#define fp0	0
#define fp1	1
#define fp2	2
#define fp3	3
#define fp4	4
#define fp5	5
#define fp6	6
#define fp7	7
#define fp8	8
#define fp9	9
#define fp10	10
#define fp11	11
#define fp12	12
#define fp13	13
#define fp14	14
#define fp15	15
#define fp16	16
#define fp17	17
#define fp18	18
#define fp19	19
#define fp20	20
#define fp21	21
#define fp22	22
#define fp23	23
#define fp24	24
#define fp25	25
#define fp26	26
#define fp27	27
#define fp28	28
#define fp29	29
#define fp30	30
#define fp31	31

/* Condition code registers.  */
#define cr0	0
#define cr1	1
#define cr2	2
#define cr3	3
#define cr4	4
#define cr5	5
#define cr6	6
#define cr7	7


#ifdef __ELF__

/* This seems to always be the case on PPC.  */
#define ALIGNARG(log2) log2
/* For ELF we need the `.type' directive to make shared libs work right.  */
#define ASM_TYPE_DIRECTIVE(name,typearg) .type name,typearg;
#define ASM_SIZE_DIRECTIVE(name) .size name,.-name

/* If compiled for profiling, call `_mcount' at the start of each function.  */
#ifdef	PROF
/* The mcount code relies on a the return address being on the stack
   to locate our caller and so it can restore it; so store one just
   for its benefit.  */
#ifdef PIC
#define CALL_MCOUNT							      \
  .pushsection;								      \
  .section ".data";    							      \
  .align ALIGNARG(2);							      \
0:.long 0;								      \
  .previous;								      \
  mflr  r0;								      \
  stw   r0,4(r1);	       						      \
  bl    _GLOBAL_OFFSET_TABLE_@local-4;					      \
  mflr  r11;								      \
  lwz   r0,0b@got(r11);							      \
  bl    JUMPTARGET(_mcount);
#else  /* PIC */
#define CALL_MCOUNT							      \
  .section ".data";							      \
  .align ALIGNARG(2);							      \
0:.long 0;								      \
  .previous;								      \
  mflr  r0;								      \
  lis   r11,0b@ha;		       					      \
  stw   r0,4(r1);	       						      \
  addi  r0,r11,0b@l;							      \
  bl    JUMPTARGET(_mcount);
#endif /* PIC */
#else  /* PROF */
#define CALL_MCOUNT		/* Do nothing.  */
#endif /* PROF */

#define	ENTRY(name)							      \
  ASM_GLOBAL_DIRECTIVE C_SYMBOL_NAME(name);				      \
  ASM_TYPE_DIRECTIVE (C_SYMBOL_NAME(name),@function)			      \
  .align ALIGNARG(2);							      \
  C_LABEL(name)								      \
  CALL_MCOUNT

#define EALIGN_W_0  /* No words to insert.  */
#define EALIGN_W_1  nop
#define EALIGN_W_2  nop;nop
#define EALIGN_W_3  nop;nop;nop
#define EALIGN_W_4  EALIGN_W_3;nop
#define EALIGN_W_5  EALIGN_W_4;nop
#define EALIGN_W_6  EALIGN_W_5;nop
#define EALIGN_W_7  EALIGN_W_6;nop

/* EALIGN is like ENTRY, but does alignment to 'words'*4 bytes
   past a 2^align boundary.  */
#ifdef PROF
#define EALIGN(name, alignt, words)					      \
  ASM_GLOBAL_DIRECTIVE C_SYMBOL_NAME(name);				      \
  ASM_TYPE_DIRECTIVE (C_SYMBOL_NAME(name),@function)			      \
  .align ALIGNARG(2);							      \
  C_LABEL(name)								      \
  CALL_MCOUNT								      \
  b 0f;									      \
  .align ALIGNARG(alignt);						      \
  EALIGN_W_##words;							      \
  0:
#else /* PROF */
#define EALIGN(name, alignt, words)					      \
  ASM_GLOBAL_DIRECTIVE C_SYMBOL_NAME(name);				      \
  ASM_TYPE_DIRECTIVE (C_SYMBOL_NAME(name),@function)			      \
  .align ALIGNARG(alignt);						      \
  EALIGN_W_##words;							      \
  C_LABEL(name)
#endif

#undef	END
#define END(name)							      \
  ASM_SIZE_DIRECTIVE(name)

#define DO_CALL(syscall)				      		      \
    li 0,syscall;						              \
    sc

#ifdef PIC
#define JUMPTARGET(name) name##@plt
#else
#define JUMPTARGET(name) name
#endif

#define PSEUDO(name, syscall_name, args)				      \
  .section ".text";							      \
  ENTRY (name)								      \
    DO_CALL (SYS_ify (syscall_name));

#define PSEUDO_RET							      \
    bnslr;								      \
    b JUMPTARGET(__syscall_error)
#define ret PSEUDO_RET

#undef	PSEUDO_END
#define	PSEUDO_END(name)						      \
  END (name)

/* Local labels stripped out by the linker.  */
#undef L
#define L(x) .L##x

/* Label in text section.  */
#define C_TEXT(name) name

#endif /* __ELF__ */


#endif	/* __ASSEMBLER__ */
