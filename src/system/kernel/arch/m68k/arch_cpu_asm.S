/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <asm_defs.h>

.text

.cpu	68020

/*
 * Those two probe hardware register for presence, trapping bus errors.
 * inspired by the Linux version, cf.
 * http://lxr.linux.no/linux+v2.6.27/arch/m68k/mm/hwtest.c
 *
 * though I'm not sure why the tests are on words only.
 * Some regs are bytes only and the even byte is unmapped...
 * but probably ignored.
 */

/* extern bool m68k_is_hw_register_readable(addr_t address);
 */
FUNCTION(m68k_is_hw_register_readable):
	/*	a1: saved_vbr */
	/* save sp */
	move.l	%sp,saved_sp
	/* set our fault vector in the table */
	move.l	#trap_fault_r,fault_vector

	/* swap our table in */
	movec	%vbr,%a1
	move.l	#temp_vectors,%a0
	movec	%a0,%vbr

	/* attempt the access */
	move.l	4(%sp),%a0
	moveq	#0,%d0
	move.w	(%a0),%d1

	nop	/* flush the pipeline */
	moveq	#1,%d0
trap_fault_r:
	/* restore */
	movec	%a1,%vbr
	move.l	saved_sp,%sp
	rts
FUNCTION_END(m68k_is_hw_register_readable)


/* extern bool m68k_is_hw_register_writable(addr_t address, uint32 value);
 */
FUNCTION(m68k_is_hw_register_writable):
	/*	a1: saved_vbr */
	/* save sp */
	move.l	%sp,saved_sp
	/* set our fault vector in the table */
	move.l	#trap_fault_w,fault_vector

	/* swap our table in */
	movec	%vbr,%a1
	move.l	#temp_vectors,%a0
	movec	%a0,%vbr

	/* attempt the access */
	move.l	4(%sp),%a0
	move.l	8(%sp),%d1
	moveq	#0,%d0
	move.w	%d1,(%a0)

	nop	/* flush the pipeline */
	moveq	#1,%d0
trap_fault_w:
	/* restore */
	movec	%a1,%vbr
	move.l	saved_sp,%sp
	rts
FUNCTION_END(m68k_is_hw_register_writable)

/* scratch data for the 2 functions above */
saved_sp:
	.long	0
temp_vectors:
	.long	0	/* reset sp */
	.long	0	/* reset pc */
fault_vector:
	.long	0	/* fault */


