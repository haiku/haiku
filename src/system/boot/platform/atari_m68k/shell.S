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

/*
 * generate boot floppy:
 * cd src/system/boot/platform/atari_m68k/ ; make fixup_tos_floppy_chksum; cd -
 * dd if=generated/objects/haiku/m68k/release/system/boot/haiku_loader of=~/floppy.img bs=512 count=20 conv=notrunc
 * src/system/boot/platform/atari_m68k/fixup_tos_floppy_chksum ~/floppy.img
 * generate .prg:
 * generated.m68k/cross-tools/bin/m68k-unknown-haiku-ld -o haiku.prg -T src/system/ldscripts/m68k/boot_prg_atari_m68k.ld generated/objects/haiku/m68k/release/system/boot/boot_loader_atari_m68k
 */

#include "atari_memory_map.h"
#include "toscalls.h"

// 1 enabled verbose output
//#define DEBUG 1

#define GLOBAL(x) .globl x ; x
#define FUNCTION(x) .global x; .type x,@function; x

#define DRIVE_RETRIES 3
	// when the drive reading fails for some reason, it will
	// retry this many times until it will report a failure

//.text
_bs_entry:
/* main entry point, both from the floppy boot and .prg */
	bra.s	real_entry

//FAT lookalike to avoid nasty things from happening
// http://alive.atari.org/alive10/btmania.php
	.ascii	"Haiku0"
	.byte	0xbe, 0x50, 0x38 // id
	//LE
	.byte	0x00, 0x02	//BPS
	.byte	0x02		//SPC
	//.byte	0x00		//???
	.byte	0x00, 0x02	//RES - number of reserved sectors
	.byte	0x00//0x02	//NFATS
	.byte	0x00/*70*/, 0x00	//NDIRS
	.byte	0xa0, 0x05		//NSECTS
	.byte	0xf9			//MEDIA
	.byte	0x05, 0x00		//SPF
	.byte	0x09, 0x00		//SPT
	.byte	0x02, 0x00		//NSIDES
	.byte	0x00, 0x00		//NHID
	// we're done

sNumSectors:
	// this location will contain the length of the boot loader as
	// written by the "makeflop" command in 512 byte blocks
	// 0x180 is the allowed maximum, as the zipped TAR with the
	// kernel and the boot module might start at offset 192 kB
	.word	0x0300 //0x0180

real_entry:

#if 0
	lea		h1,%a0
	bsr		puts
	move.l		#0x1234aa55,%d0
	bsr		putx
#endif


	// first, determine if .prg (user) or bootsect (super)
	// Super()
	move.l		#SUP_INQUIRE,-(%sp)
	move.w		#0x20,-(%sp)
	trap		#1
	addq.l		#6,%sp

	bsr		putx

	cmp.l		#SUP_USER,%d0
	bne			floppy_start
	lea		h2,%a0
	bsr		puts

	bra			prg_start
	//bra			bootsect_start

/*
 * floppy boot support code
 */

floppy_start:
	lea		h3,%a0
	bsr		puts
	lea		str,%a0
	bsr		puts
	//bra floppy_start

	// no interrupt
	or.w		#0x0700,%sr

	//XXX: check for enough RAM

	// load the rest
	move.w		sNumSectors,%d2
	// load at base + this code.
	move.l		#(ATARI_ZBEOS_BASE+512),%a2
	//move.l		%a2,%d0
	//bsr putx
	bsr			load_sectors
	tst.w			%d0
	beq			floppy_done
	lea			failure_string,%a0
	bsr			puts
	bra			spin
floppy_done:
	lea		h4,%a0
	bsr		puts
	// setup stack
	move.l		#ATARI_ZBEOS_STACK_END,%sp
	lea		h5,%a0
	bsr		puts
	move.l		#0,%d0
	//jmp			ATARI_ZBEOS_BASE+512
	jmp		_start

	//rts

/**	Loads %d2 sectors from floppy disk, starting at head XXX %dh, sector %cx.
 *	The data is loaded to %a2.
 */

load_sectors:
	
#if 0
	/* it seems to skip 9 every 9 sectors, buggy side handling ? */
	// Rwabs
	//move.l		#1,-(%sp)
	move.w		#0,-(%sp)	// A:
	//move.w		#2,-(%sp)	// C:
	//move.w		#-1,-(%sp)	// 2nd sector
	move.w		#1,-(%sp)	// 2nd sector
	move.w		%d2,-(%sp)
	move.l		%a2,-(%sp)
	//move.w		#RW_READ+RW_NOTRANSLATE,-(%sp)
	move.w		#RW_READ+RW_NOMEDIACH,-(%sp)
	move.w		#4,-(%sp)
	trap		#13
	add.l		#14,%sp
#endif
#if 0
	// d2:	

	//move.w		%d2,-(%sp)
	move.w		#9,-(%sp)
	move.w		#0,-(%sp)
	move.w		#0,-(%sp)
	move.w		#1,-(%sp)
	move.w		#0,-(%sp)
	move.l		#0,-(%sp)
	//move.w		#0,-(%sp)
	//move.l		%a2,-(%sp)
	move.l		#ATARI_ZBEOS_BASE,-(%sp)
	move.w		#8,-(%sp)	// floprd
	trap		#XBIOS_TRAP
	add.l		#20,%sp
#endif

	bsr	putx
	rts

floppy_end:
//	.org	FAILURE_STRING
failure_string:
//	.string " Loading failed! Press key to reboot.\r\n"
	
//	.org	DOT_STRING
	.string	"."

/*
 * \AUTO\HAIKU.PRG and ARAnyM BOOTSTRAP() support code
 */

prg_start:
	//lea		h4,%a0
	//bsr		puts

	// .prg:
	// we need to switch to supervisor mode anyway
	move.l		#SUP_SET,-(%sp)
	move.w		#0x20,-(%sp)
	trap		#1
	addq.l		#6,%sp
	move.l		%d0,saved_super_stack
	
#if 1
			//_membot
	move.l		#0x432,%a0
	move.l		(%a0),%d0
	bsr		putx

			//_memtop
	move.l		#0x436,%a0
	move.l		(%a0),%d0
	bsr		putx

			//_v_bas_ad
	move.l		#0x44e,%a0
	move.l		(%a0),%d0
	bsr		putx
#endif
	// disable interrupts
	//or.w		#0x0700,%sr
	
	// setup stack
	move.l		#ATARI_ZBEOS_STACK_END,%sp

	lea		h5,%a0
	bsr		puts
	lea		_bs_entry,%a0
	move.l		%a0,%d0
	bsr		putx

	// copy the rest of the prg

	// load counter
	clr.l		%d0
	move.w		sNumSectors,%d0
	sub.w		#1,%d0
	// load addresses
	lea		_bs_entry,%a0
	move.l		#ATARI_ZBEOS_BASE,%a1


nextsect:
	move.l		#512/4-1,%d1
copysect_loop:
	move.l		(%a0)+,(%a1)+
	dbf		%d1,copysect_loop
	//bsr		putx
	dbf		%d0,nextsect

	lea		msg_j1,%a0
	bsr		puts
	// all done
spin:
	//bra			spin

super_done:
	// XXX: copy the rest !
	move.l		#0,%d0
	//jmp			ATARI_ZBEOS_BASE+512
	jmp			_start

saved_super_stack:
	.long	0

GLOBAL(gBootedFromImage):
	.byte	0

GLOBAL(gBootDriveID):
	.byte	0

GLOBAL(gBootPartitionOffset):
	.long	0



putx:
	movem.l	%d0-%d2/%a0-%a2,-(%sp)
	move.l	#8-1,%d2
	move.l	%d0,%d1
putxloop:
	move.l	%d1,%d0
	lsl.l	#4,%d1
	//swap	%d0
	//lsr.l	#8,%d0
	//lsr.l	#4,%d0
	rol.l	#4,%d0
	and.l	#0x0f,%d0
	cmp.b	#9,%d0
	ble	putx0
	add.b	#'a'-'0'-10,%d0
	//bra	putxdisp
putx0:
	add.b	#'0',%d0
putxdisp:
	bsr	putc
	dbf	%d2,putxloop
	move.b	#'\r',%d0
	bsr	putc
	move.b	#'\n',%d0
	bsr	putc
	movem.l	(%sp)+,%d0-%d2/%a0-%a2
	rts
	
	lea.l	str,%a0
puts:
.loopt:
	move.b	(%a0)+,%d0
	beq	.strout
	bsr	putc
	bra	.loopt
.strout:
	move.b	#'\r',%d0
	bsr	putc
	move.b	#'\n',%d0
	bsr	putc
	rts
	
.loop:
	move	#'.',%d0
	bsr		putc
	bra		.loop
	rts


/* prints the char in d0.b to the console */
putc:
	movem.l	%d0-%d2/%a0-%a2,-(%sp)
	move.w	%d0,-(%sp)
	move.w	#DEV_CON,-(%sp)	// DEV_CON
	move.w	#3,-(%sp)	// Bconout
	trap	#13
	add.l	#6,%sp
	movem.l	(%sp)+,%d0-%d2/%a0-%a2
	rts

str:
	.string "Haiku!"
h1:
	.string "H1"
h2:
	.string "H2"
h3:
	.string "H3"
h4:
	.string "H4"
h5:
	.string "H5"
unimpl:
	.string "Unimplemented."
msg_j1:
	.string "jumping to haiku_loader."


shell_end:
	//.fill	(0x01fe - shell_end), 1, 0x55
	.org	0x01fe
	.word	0xaa55-1	// will be replaced by the one calculated by the build.
				// we make sure we PCs don't try to execute it.
		// this bumps the "start" label to offset 0x0200 as
		// expected by the BFS boot loader, and also marks
		// this block as valid boot block for the BIOS
		// XXX: actually TOS wants a real checksum here so sum is 0x1234!

