/* Bounded-pointer definitions for x86-64 assembler.
   Copyright (C) 2001 Free Software Foundation, Inc.

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

#ifndef _bp_asm_h_
# define _bp_asm_h_ 1

# if __ASSEMBLER__

#  if __BOUNDED_POINTERS__

/* Bounded pointers occupy three words.  */
#   define PTR_SIZE 24
/* Bounded pointer return values are passed back through a hidden
   argument that points to caller-allocate space.  The hidden arg
   occupies one word on the stack.  */
#   define RTN_SIZE 6
/* Although the caller pushes the hidden arg, the callee is
   responsible for popping it.  */
#   define RET_PTR ret $RTN_SIZE
/* Maintain frame pointer chain in leaf assembler functions for the benefit
   of debugging stack traces when bounds violations occur.  */
#   define ENTER pushq %rbp; movq %rsp, %rbp
#   define LEAVE movq %rbp, %rsp; popq %rbp
/* Stack space overhead of procedure-call linkage: return address and
   frame pointer.  */
#   define LINKAGE 16
/* Stack offset of return address after calling ENTER.  */
#   define PCOFF 8

/* Int 5 is the "bound range" exception also raised by the "bound"
   instruction.  */
#   define BOUNDS_VIOLATED int $5

#   define CHECK_BOUNDS_LOW(VAL_REG, BP_MEM)	\
	cmpq 8+BP_MEM, VAL_REG;			\
	jae 0f; /* continue if value >= low */	\
	BOUNDS_VIOLATED;			\
    0:

#   define CHECK_BOUNDS_HIGH(VAL_REG, BP_MEM, Jcc)	\
	cmpq 16+BP_MEM, VAL_REG;			\
	Jcc 0f; /* continue if value < high */		\
	BOUNDS_VIOLATED;				\
    0:

#   define CHECK_BOUNDS_BOTH(VAL_REG, BP_MEM)	\
	cmpq 8+BP_MEM, VAL_REG;			\
	jb 1f; /* die if value < low */		\
	cmpq 16+BP_MEM, VAL_REG;		\
	jb 0f; /* continue if value < high */	\
    1:	BOUNDS_VIOLATED;			\
    0:

#   define CHECK_BOUNDS_BOTH_WIDE(VAL_REG, BP_MEM, LENGTH)	\
	CHECK_BOUNDS_LOW(VAL_REG, BP_MEM);			\
	addl LENGTH, VAL_REG;					\
	cmpq 16+BP_MEM, VAL_REG;					\
	jbe 0f; /* continue if value <= high */			\
	BOUNDS_VIOLATED;					\
    0:	subq LENGTH, VAL_REG /* restore value */

/* Take bounds from BP_MEM and affix them to the pointer
   value in %rax, stuffing all into memory at RTN(%esp).
   Use %rdx as a scratch register.  */

#   define RETURN_BOUNDED_POINTER(BP_MEM)	\
	movq RTN(%rsp), %rdx;			\
	movq %rax, 0(%rdx);			\
	movq 8+BP_MEM, %rax;			\
	movq %rax, 4(%rdx);			\
	movq 16+BP_MEM, %rax;			\
	movq %rax, 8(%rdx)

#   define RETURN_NULL_BOUNDED_POINTER		\
	movl RTN(%rsp), %rdx;			\
	movl %rax, 0(%rdx);			\
	movl %rax, 4(%rdx);			\
	movl %rax, 8(%rdx)

/* The caller of __errno_location is responsible for allocating space
   for the three-word BP return-value and passing pushing its address
   as an implicit first argument.  */
#   define PUSH_ERRNO_LOCATION_RETURN		\
	subl $16, %esp;				\
	subl $8, %esp;				\
	pushq %rsp

/* __errno_location is responsible for popping the implicit first
   argument, but we must pop the space for the BP itself.  We also
   dereference the return value in order to dig out the pointer value.  */
#   define POP_ERRNO_LOCATION_RETURN		\
	popq %rax;				\
	addq $16, %rsp

#  else /* !__BOUNDED_POINTERS__ */

/* Unbounded pointers occupy one word.  */
#   define PTR_SIZE 8
/* Unbounded pointer return values are passed back in the register %rax.  */
#   define RTN_SIZE 0
/* Use simple return instruction for unbounded pointer values.  */
#   define RET_PTR ret
/* Don't maintain frame pointer chain for leaf assembler functions.  */
#   define ENTER
#   define LEAVE
/* Stack space overhead of procedure-call linkage: return address only.  */
#   define LINKAGE 8
/* Stack offset of return address after calling ENTER.  */
#   define PCOFF 0

#   define CHECK_BOUNDS_LOW(VAL_REG, BP_MEM)
#   define CHECK_BOUNDS_HIGH(VAL_REG, BP_MEM, Jcc)
#   define CHECK_BOUNDS_BOTH(VAL_REG, BP_MEM)
#   define CHECK_BOUNDS_BOTH_WIDE(VAL_REG, BP_MEM, LENGTH)
#   define RETURN_BOUNDED_POINTER(BP_MEM)

#   define RETURN_NULL_BOUNDED_POINTER

#   define PUSH_ERRNO_LOCATION_RETURN
#   define POP_ERRNO_LOCATION_RETURN

#  endif /* !__BOUNDED_POINTERS__ */

# endif /* __ASSEMBLER__ */

#endif /* _bp_asm_h_ */
