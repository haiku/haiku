/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Copyright 2003, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

#include <asm_defs.h>

#include "asm_offsets.h"


/*	General exception handling concept:

	Starting with 68020 the vector offset (=vector number * 4) is part 
	of the all exception frame types, so we can easily have a common
	asm stub for all of them, which calls m68k_exception_entry in 
	arch_int.cpp.
	Also, m68k doesn't disable the mmu on interrupt as ppc does, 
	which makes things simpler.

	----ppc
	
	The PPC architecture specifies entry point offsets for the various
	exceptions in the first two physical pages. We put a short piece of code
	(VEC_ENTRY()) into each exception vector. It calls exception_vector_common,
	which is defined in the unused space at the beginning of the first physical
	page. It re-enables address translation and calls ppc_exception_tail which
	lies in the kernel. It dumps an iframe and invokes ppc_exception_entry()
	(arch_int.cpp), which handles the exception and returns eventually.
	The registers are restored from the iframe and we return from the
	interrupt.

	algorithm overview:

	* VEC_ENTRY
	* exception_vector_common
	* ppc_exception_tail
		- dump iframe
		- ppc_exception_entry()
		- restore registers and return from interrupt

	Here we use the following SPRG registers, which are at the disposal of the
	operating system:
	* SPRG0: Physical address pointer to a struct cpu_exception_context
			 for the current CPU. The structure contains helpful pointers
			 as well as some scratch memory for temporarily saving registers.
	* SPRG1: Scratch.

	struct cpu_exception_context (defined in arch_int.h):
	offset 0:  virtual address of the exception handler routine in the kernel
	offset 4:  virtual address of the exception context
	offset 8:  kernel stack for the current thread
	offset 12: start of scratch memory for saving registers etc.

	algorithm in detail:

	* VEC_ENTRY
		- save r1 in SPRG1 and load cpu_exception_context into r1
		- save r0, save LR in r0
	* exception_vector_common
		- params:
			. r0: old LR
			. r1: exception context (physical address)
			. SPRG1: original r1
		- save r0-3
		- load virtual exception context address to r1
		- turn on BAT for exception vector code
		- turn on address translation
		- get exception vector offset from LR
	* ppc_exception_tail
		- params:
			. r1: exception context (virtual address)
			. r3: exception vector offset
			. SPRG1: original r1
		- turn off BAT
		- get kernel stack pointer
		- dump iframe
		- ppc_exception_entry()
		- restore registers and return from interrupt
 */


/* exception vector definitions */

/* this one just returns */
FUNCTION(__m68k_exception_noop):
	rte
FUNCTION_END(__m68k_exception_noop)

/* see arch_asm.S for ctx switch */

FUNCTION(__m68k_exception_common):
	/* save regs */
	movem.l		%d0-%d7/%a0-%a6,-(%sp)	/* push the iframe address */
	/* save fp */
	sub.l		#FPU_STATE_sizeof,%sp
	fsave		(%sp)
	tst.b		(%sp)			/* check for a null state */
	beq		null_sav_1		/* yes */
	
	fmovem		%fpcr/%fpsr/%fpiar,-(%sp)
	fmovem		%fp0-%fp7,-(%sp)
	bra		null_sav_2
null_sav_1:
	sub.l		#IFRAME_fpu-IFRAME_fp,%sp
null_sav_2:	

	move.l		%sp,%fp			/* have stack_trace() find the iframe */
	move.l		%sp,-(%sp)		/* push address of iframe */
	bsr		m68k_exception_entry	/* call C entry */
	add.l		#4,%sp

	/* restore fp */
	tst.b		IFRAME_fpu-IFRAME_fp(%sp) /* check for a null state */
	beq		null_res_1		/* yes */
	fmovem		(%sp)+,%fp0-%fp7
	fmovem		(%sp)+,%fpcr/%fpsr/%fpiar
	bra		null_res_2
null_res_1:
	add.l		#IFRAME_fpu-IFRAME_fp,%sp
null_res_2:	
	/* restore fp */
	frestore	(%sp)
	add.l		#FPU_STATE_sizeof,%sp
	/* restore regs */
	movem.l		(%sp)+,%d0-%d7/%a0-%a6
	
	rte
FUNCTION_END(__m68k_exception_common)


