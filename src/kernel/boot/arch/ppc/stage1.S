/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#define BASE 0x100000
#define BOOTDIR_BASE (BASE + 0x1000)
#define STAGE2_BOOTDIR_PAGE	(BOOTDIR_BASE + 0x60)
#define STAGE2_OFFSET (BOOTDIR_BASE + 0x74)

.text
.globl _start
_start:
	lis		1,tempstack_end@ha		/* load the new stack */
	ori		1,1,tempstack_end@l

	/* load the base of the bootdir */
	lis		8,BOOTDIR_BASE@ha
	ori		8,8,BOOTDIR_BASE@l	

	/* load the offset the stage2 will start into the bootdir */
	lis		9,STAGE2_BOOTDIR_PAGE@ha
	ori		9,9,STAGE2_BOOTDIR_PAGE@l
	lwz		9,0(9)
	mulli	9,9,4096
	
	/* load the offset into that page the stage2 entry point will be */
	lis		10,STAGE2_OFFSET@ha
	ori		10,10,STAGE2_OFFSET@l
	lwz		10,0(10)

	/* add them all together */
	add		11,8,9
	add		11,11,10

	/* jump there */
	mtlr	11
	blr

.align 4
tempstack:
	.skip 0x800	
tempstack_end:
	
.data
#include "boot/ppc/final.asminclude"

