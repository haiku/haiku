/*
** Copyright 2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#define BASE 0x4380000
#define BOOTDIR_BASE (BASE + 0x1000)
#define STAGE2_BOOTDIR_PAGE (BOOTDIR_BASE + 0x60)
#define STAGE2_OFFSET (BOOTDIR_BASE + 0x74)

.globl _start
_start:
	nop

	/* load the base of the bootdir */
	movel	#BOOTDIR_BASE,%a0

	/* load the offset the stage2 will start into the bootdir */
	movel	(STAGE2_BOOTDIR_PAGE),%d0
	mulul	#4096,%d0
	movel	%d0,%a1

	/* load the offset into that page the stage2 entry point will be */
	movel	(STAGE2_OFFSET),%a2

	/* add them together */
	addl	%a2,%a1
	addl	%a1,%a0

	/* look in the vector table and find the monitor vector */
	movec   %vbr,%a3
	movel   %a3@(4),%sp@-

	/* copy the arg we got */
	movel   %sp@(8),%sp@-

	/* make the call */
	jsr		(%a0)

	addql   #8,%sp

	rts

.align 4
tempstack:
    .skip 0x800
	tempstack_end:

.align 0x1000
.data
#include "boot/m68k/final.asminclude"
