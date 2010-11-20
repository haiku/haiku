/*
 * Copyright 2008-2010, François Revol, revol@free.fr. All rights reserved.
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
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

/*
 * references :
 * http://ftp.netbsd.org/pub/NetBSD/NetBSD-release-3-0/src/sys/arch/atari/stand/xxboot/fdboot/fdboot.S
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


#define SECTSIZE 512

//.text
_bs_entry:
/* main entry point, both from the floppy boot and .prg */
	bra.s	real_entry

//FAT lookalike to avoid nasty things from happening
// http://alive.atari.org/alive10/btmania.php
// MS-DOS values :
// http://support.microsoft.com/kb/75131/en
// http://alumnus.caltech.edu/~pje/dosfiles.html
	.ascii	"Haiku "
	.byte	0xbe, 0x50, 0x38 // id
	//LITTLE ENDIAN!
	.byte	0x00, 0x02	//BPS
	.byte	0x02		//SPC
	//.byte	0x00		//???
	.byte	0x00, 0x02	//RES - number of reserved sectors
	.byte	0x00		//NFATS
	.byte	0x00, 0x00	//NDIRS
	.byte	0x40, 0x0b	//NSECTS
	.byte	0xf0		//MEDIA
	.byte	0x05, 0x00	//SPF
_fat_spt:	
	.byte	0x12, 0x00	//SPT
	.byte	0x02, 0x00	//NSIDES
	.byte	0x00, 0x00	//NHID
	// we're done

sNumSectors:
	// this location will contain the length of the boot loader as
	// written by the "makeflop" command in 512 byte blocks
	// 0x180 is the allowed maximum, as the zipped TAR with the
	// kernel and the boot module might start at offset 192 kB
	//.word	0x0300 //0x0180
	.word	BOOT_ARCHIVE_IMAGE_OFFSET*2

real_entry:

// save the regs to return safely, like the NetBSD loader does:
// http://ftp.netbsd.org/pub/NetBSD/NetBSD-release-3-0/src/sys/arch/atari/stand/xxboot/fdboot/fdboot.S
	movem.l		%d1-%d7/%a0-%a6,-(%sp)

	lea		str,%a0
	bsr		puts

	// first, determine if .prg (user) or bootsect (super)
	// Super()
	move.l		#SUP_INQUIRE,-(%sp)
	move.w		#0x20,-(%sp)
	trap		#GEMDOS_TRAP
	addq.l		#6,%sp

	cmp.l		#SUP_USER,%d0
	bne			floppy_start
	bra			prg_start

/*
 * floppy boot support code
 */

floppy_start:
	lea		label_floppy,%a0
	bsr		puts
	//bra floppy_start

	// no interrupt
	//or.w		#0x0700,%sr

	//XXX: check for enough RAM

	// load the rest
	//move.w		sNumSectors,%d2
	// load at base + this code.
	//move.l		#(ATARI_ZBEOS_BASE+512),%a2
	//move.l		%a2,%d0
	//bsr putx
	bsr			load_sectors
	tst.w			%d0
	bne			load_failed
floppy_done:
	// setup stack
	move.l		#ATARI_ZBEOS_STACK_END,%sp
	//jmp			ATARI_ZBEOS_BASE+512

	move.b		#ATARI_BOOT_DRVAPI_FLOPPY,ATARI_ZBEOS_BASE + gBootDriveAPI - _bs_entry 
	move.w		TOSVAR_bootdev,%d0
	// XXX:	 use uint16 ??
	move.b		%d0,ATARI_ZBEOS_BASE + gBootDriveID - _bs_entry 
	move.b		#1,ATARI_ZBEOS_BASE + gBootedFromImage - _bs_entry 

	lea		msg_j1,%a0
	bsr		puts
	move.l		#0,%d0

	jmp		_start

load_failed:
	//bra			_exit

spin:	
	//bra			spin
_exit:	/*  */
	lea			failure_string,%a0
	bsr			puts
	bsr			getc
	
	movem.l	(%sp)+,%d1-%d7/%a0-%a6
	rts
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
	move.w		#1,-(%sp) // count
	move.w		#0,-(%sp) // sideno
	move.w		#0,-(%sp) // trackno
	move.w		#1,-(%sp) // sectno
	move.w		TOSVAR_bootdev,-(%sp) // devno
	clr.l		-(%sp) // filler
	//move.w		#0,-(%sp)
	//move.l		%a2,-(%sp)
	move.l		#ATARI_ZBEOS_BASE,-(%sp)
	move.w		#8,-(%sp)	// floprd
	trap		#XBIOS_TRAP
	add.l		#20,%sp
#endif

	//bsr	putx
	//rts

	/*
	 * %d3:	remaining sects
	 * %d4:	sectno
	 * %d5:	trackno
	 * %d6:	sideno
	 * %d7:	sect / track
	 * %a5:	buffer
	 *
	 */
	// load the rest

	// XXX:	 the NetBSD loader probes it, but trying to asserts ARAnyM
	clr.l		%d7
	move.b		_fat_spt,%d7	// sect/track
	//move.w		#0,%d7	// sect/track
	move.w		#0,%d6	// sideno
	move.w		#0,%d5	// trackno
	move.w		#1,%d4	// sectno
	move.w		sNumSectors,%d3	// remainder
	move.l		#ATARI_ZBEOS_BASE,%a5
read_sectors_loop:	
	bsr.s		read_sect
	bne		read_sectors_fail
	moveq		#'.',%d0
	bsr		putc
	subq.w		#1,%d3	// remainder--
	bne.s		read_sectors_next
	bsr	putcrlf
	clr.l		%d0
	rts

	// compute next track/side/sector
read_sectors_next:
#if 0	//DEBUG
	// [TRAK][SIDE]
	move.w		%d5,%d0
	swap		%d0
	move.w		%d6,%d0
	bsr		putx
	// [SECT][S/TK]
	move.w		%d4,%d0
	swap		%d0
	move.w		%d7,%d0
	bsr		putx
	//bsr		getc
#endif	//!DEBUG
	add.l		#SECTSIZE,%a5
	addq.w		#1,%d4	// sectno++
	cmp.w		%d7,%d4	// if (sectno == spt)
	bne.s		.rs2	// {
	addq.w		#1,%d6	//	sideno++ ;
	cmp.w		#2,%d6	//	if (sideno == 2) {
	bne		.rs1
	clr.w		%d6	//		sideno = 0 ;
	addq.w		#1,%d5	//		trackno++ ;
	bsr	putcrlf

.rs1:				//	}
	clr.w		%d4	//	sectno = 0 ;
.rs2:				// }
	
	bra.s		read_sectors_loop

read_sectors_fail:
	tst.w		%d7	// s/t
	bne		read_sectors_fail2
	move.w		%d4,%d0
	bsr		putx
	move.w		%d4,%d7
	clr.w		%d4
	//add.w		#1,
	bra.s		read_sectors_next
read_sectors_fail2:
	moveq		#1,%d0
	rts

	
read_sect:	/* read 1 sector */
	/*
	 * %d4:	sectno
	 * %d5:	trackno
	 * %d6:	sideno
	 * %d7:	remaining count
	 * %a5:	buffer
	 *
	 */
#if 1
	//move.w		%d2,-(%sp)
	//move.w		#1,-(%sp) // count
	//move.w		#0,-(%sp) // sideno
	//move.w		#0,-(%sp) // trackno
	//move.w		#2,-(%sp) // sectno
	move.w		#1,-(%sp)
	movem.w		%d4-%d6,-(%sp)
	move.w		TOSVAR_bootdev,-(%sp) // devno
	clr.l		-(%sp) // filler
	move.l		%a5,-(%sp)
	move.w		#8,-(%sp)	// floprd
	trap		#XBIOS_TRAP
	add.l		#20,%sp
	tst.l		%d0
	
#endif
	rts

floppy_end:
//	.org	FAILURE_STRING
failure_string:
//	.string " Loading failed! Press key to reboot.\r\n"
	.string " Loading failed! Press key.\r\n"
//	.string "FAIL"
	
//	.org	DOT_STRING
//	.string	"."




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
	bsr	putcrlf
	movem.l	(%sp)+,%d0-%d2/%a0-%a2
	rts
	

puts:
.loopt:
	move.b	(%a0)+,%d0
	beq	.strout
	bsr	putc
	bra	.loopt
.strout:
putcrlf:	
	move.b	#'\r',%d0
	bsr	putc
	move.b	#'\n',%d0
	bsr	putc
	rts
	

/* prints the char in d0.b to the console */
putc:
	movem.l	%d0-%d2/%a0-%a2,-(%sp)
	move.w	%d0,-(%sp)
	move.w	#DEV_CON,-(%sp)	// DEV_CON
	move.w	#3,-(%sp)	// Bconout
	trap	#BIOS_TRAP
	add.l	#6,%sp
	movem.l	(%sp)+,%d0-%d2/%a0-%a2
	rts

/* waits for a key */
getc:
	movem.l	%d1-%d2/%a0-%a2,-(%sp)
	move.w	#DEV_CON,-(%sp)	// DEV_CON
	move.w	#2,-(%sp)	// Bconin
	trap	#BIOS_TRAP
	add.l	#4,%sp
	movem.l	(%sp)+,%d1-%d2/%a0-%a2
	rts

str:
	.string "Haiku!"
label_prg:
	.string "P" //"RG boot"
label_floppy:
	.string "F" //"loppy boot"
h4:
	.string "H4"
h5:
	.string "H5"
msg_j1:
	.string "Jumping to haiku_loader."

shell_end:
	//.fill	(0x01fe - shell_end), 1, 0x55
	.org	0x01fe
	.word	0xaa55-1	// will be replaced by the one calculated by the build.
		// we make sure PCs don't try to execute it.
		// this bumps the "start" label to offset 0x0200 as
		// expected by the BFS boot loader, and also marks
		// this block as valid boot block for the BIOS

//XXX: put bfs_start here

/*
 * \AUTO\HAIKU.PRG and ARAnyM BOOTSTRAP() support code
 */

prg_start:
	lea		label_prg,%a0
	bsr		puts

	// .prg:
	// we need to switch to supervisor mode anyway
	move.l		#SUP_SET,-(%sp)
	move.w		#0x20,-(%sp)
	trap		#1
	addq.l		#6,%sp
	move.l		%d0,saved_super_stack
	
#if 0
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
super_done:
	// XXX: copy the rest !
#if 1
	// XXX: force floppy boot for testing to get the kernel tgz for now
	move.b		#ATARI_BOOT_DRVAPI_FLOPPY,ATARI_ZBEOS_BASE + gBootDriveAPI - _bs_entry 
	move.b		#0,ATARI_ZBEOS_BASE + gBootDriveID - _bs_entry 
	move.b		#1,ATARI_ZBEOS_BASE + gBootedFromImage - _bs_entry 
#endif

	move.l		#0,%d0

	//jmp			ATARI_ZBEOS_BASE+512
	jmp			_start

saved_super_stack:
	.long	0

GLOBAL(gBootedFromImage):
	.byte	0

GLOBAL(gBootDriveAPI):
	.byte	ATARI_BOOT_DRVAPI_UNKNOWN

GLOBAL(gBootDriveID):
	.byte	0

GLOBAL(gBootPartitionOffset):
	.long	0

