/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		François Revol, revol@free.fr.
 */

/**	This file contains the boot floppy and BFS boot block entry points for
 *	the stage 2 boot loader.
 * x86 ahead:
 *	The floppy entry point is at offset 0. It's loaded at 0x07c0:0x000. It
 *	will load the rest of the loader to 0x1000:0x0200 and execute it.
 *	The BFS boot block will load the whole stage 2 loader to 0x1000:0x0000
 *	and will then jump to 0x1000:0x0200 as its entry point.
 *	This code will then switch to protected mode and will directly call
 *	the entry function of the embedded ELF part of the loader.
 */



#include "toscalls.h"

// 1 enabled verbose output
//#define DEBUG 1

#define GLOBAL(x) .globl x ; x
#define FUNCTION(x) .global x; .type x,@function; x

#define DRIVE_RETRIES 3
	// when the drive reading fails for some reason, it will
	// retry this many times until it will report a failure

.text

/* main entry point, both from the floppy boot and .prg */

	// first, determine if .prg (user) or bootsect (super)
	// Super()
	move.l		SUP_INQUIRE,-(%sp)
	move.w		#0x20,%d0
	trap		#1
	addq.l		#6,%sp
	cmp.w		#SUP_SUPER,%d0
	beq			floppy_start
	bra			prg_start
	bra			bootsect_start
	rts
floppy_start:

floppy_end:
	.org	FAILURE_STRING
	.string " Loading failed! Press key to reboot.\r\n"
	.org	DOT_STRING
	.string	"."

	.org	0x01fe
	.word	0xaa55
		// this bumps the "start" label to offset 0x0200 as
		// expected by the BFS boot loader, and also marks
		// this block as valid boot block for the BIOS

prg_start:
	// .prg:
	// we need to switch to supervisor mode anyway
	move.l		SUP_SET,-(%sp)
	move.w		#0x20,%d0
	trap		#1
	addq.l		#6,%sp
	move.l		%d0,saved_super_stack
	// all done


super_done:
	
	jsr			_start

saved_super_stack:
	.long	0

GLOBAL(gBootedFromImage):
	.byte	0

GLOBAL(gBootDriveID):
	.byte	0

GLOBAL(gBootPartitionOffset):
	.long	0



	
	lea.l	str,%a0
.loopt:
	move.b	(%a0)+,%d0
	beq	.strout
	bsr	putc
	bra	.loopt
.strout:
	//Pterm0
	move.w	#1,%d0
	trap	#1

	rts
	
.loop:
	move	#'.',%d0
	bsr		putc
	bra		.loop
	rts

/* prints the char in d0.b to the console */
putc:
	movem.l	%a0,-(%sp)
	move.w	%d0,-(%sp)
	move.w	#DEV_CON,-(%sp)	// DEV_CON
	move.w	#3,-(%sp)	// Bconout
	trap	#13
	add.l	#6,%sp
	movem.l	(%sp)+,%a0
	rts
str:
	.ascii "Haiku!"
	.byte 0
