/*
 * Copyright 2008-2011, François Revol, revol@free.fr. All rights reserved.
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 */

/**	This file contains the boot floppy, .PRG and BFS boot block entry points
 *	for the stage 2 boot loader. All 3 entry points load and relocate the
 *	loader so that it ends up being at ATARI_ZBEOS_BASE.
 *	This file is built as PIC so it works wherever it is executed.
 *	Some arithmetic is thus needed to set variables in the relocated code.
 *	The loader .PRG file is created from the bootloader by a custom ldscript.
 *
 *	Known load addresses:
 *	floppy:	0x00a34c (EmuTOS in ARAnyM)
 *	.prg:	0x03543e (depends on the TOS memory)
 *	BFS:	is set in the MBR code.
 *
 *	The floppy and .PRG code is entered at _bs_entry, which jumps further on
 *	to skip some fake FAT descriptor.
 *	It first determines which way it was loaded (floppy boot happens in
 *	supervisor mode, while .PRG are loaded as user code), and jumps to
 *	the specific part.
 *
 *	The floppy code comes first as it must be in the first sector,
 *	and loads the rest of the loader at ATARI_ZBEOS_BASE+0x200,
 *	sets up the new stack and jumps to _start after setting some variables.
 *	The end of the floppy code contains a placeholder for a checksum which
 *	is calculated at build-time by the <build>fixup_tos_floppy_chksum tool.
 *
 *	The .PRG code switches to supervisor mode, copies the whole loader
 *	which is already in memory to ATARI_ZBEOS_BASE. It then sets up the new
 *	stack and some variables and jumps to _start.
 *
 *	The BFS code is yet to be fully written.
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
	// This location will contain the max length of the boot loader
	// as written by the build system in 512 byte blocks and depends
	// on the offset of the zipped TAR with the kernel and the boot
	// modules will start at offset BOOT_ARCHIVE_IMAGE_OFFSET kB.
	.word	BOOT_ARCHIVE_IMAGE_OFFSET*2

real_entry:

// save the regs to return safely, like the NetBSD loader does:
// http://ftp.netbsd.org/pub/NetBSD/NetBSD-release-3-0/src/sys/arch/atari/stand/xxboot/fdboot/fdboot.S
	movem.l		%d1-%d7/%a0-%a6,-(%sp)

//DEBUG: determine load location
//	lea		_bs_entry(%pc),%a0
//	move.l		%a0,%d0
//	bsr		putx

	lea		str,%a0
	bsr		puts

	// first, determine if .prg (user) or bootsect (super)
	// Super(SUP_INQUIRE)
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
	//lea		label_floppy,%a0
	//bsr		puts

	bsr			load_sectors
	tst.w			%d0
	bne			load_failed

floppy_done:
	move.b		#ATARI_BOOT_DRVAPI_FLOPPY,ATARI_ZBEOS_BASE + gBootDriveAPI - _bs_entry 
	move.w		TOSVAR_bootdev,%d0
	// XXX:	 use uint16 ??
	move.b		%d0,ATARI_ZBEOS_BASE + gBootDriveID - _bs_entry 
	move.b		#1,ATARI_ZBEOS_BASE + gBootedFromImage - _bs_entry 


all_done:

	// setup stack
	move.l		#ATARI_ZBEOS_STACK_END,%sp

	// say goodbye to TOS
	lea		msg_j1,%a0
	bsr		puts

	//bra spin
	// say hello to haiku_loader
	jmp		_start

load_failed:
	//bra			_exit

spin:	
	bra			spin
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

	move.w		#1,-(%sp)
	movem.w		%d4-%d6,-(%sp)
	move.w		TOSVAR_bootdev,-(%sp) // devno
	clr.l		-(%sp) // filler
	move.l		%a5,-(%sp)
	move.w		#8,-(%sp)	// floprd
	trap		#XBIOS_TRAP
	add.l		#20,%sp
	tst.l		%d0
	rts

floppy_end:
//	.org	FAILURE_STRING
failure_string:
//	.string " Loading failed! Press key to reboot.\r\n"
	.string " Loading failed! Press key.\r\n"
//	.string "FAIL"
	


putx:
	movem.l	%d0-%d2/%a0-%a2,-(%sp)
	move.l	#8-1,%d2
	move.l	%d0,%d1
putxloop:
	move.l	%d1,%d0
	lsl.l	#4,%d1
	rol.l	#4,%d0
	and.l	#0x0f,%d0
	cmp.b	#9,%d0
	ble	putx0
	add.b	#'a'-'0'-10,%d0
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
//label_prg:
//	.string "P" //"RG boot"
//label_floppy:
//	.string "F" //"loppy boot"
msg_j1:
	.string "Jumping to haiku_loader."

shell_end:
	//.fill	(0x01fe - shell_end), 1, 0x55
	.org	0x01fe
boot_code_checksum:
	.word	0xaa55-1	// will be replaced by the one calculated by the build.
		// we make sure PCs don't try to execute it.
		// this bumps the "start" label to offset 0x0200 as
		// expected by the BFS boot loader, and also marks
		// this block as valid boot block for the BIOS


//XXX: put bfs_start here
bfs_start:
	//FIXME:write BFS code


/*
 * \AUTO\HAIKU.PRG and ARAnyM BOOTSTRAP() support code
 */

prg_start:
	//lea		label_prg,%a0
	//bsr		puts

	// .prg:
	// we need to switch to supervisor mode anyway
	move.l		#SUP_SET,-(%sp)
	move.w		#0x20,-(%sp)
	trap		#1
	addq.l		#6,%sp
	move.l		%d0,saved_super_stack
	
	// disable interrupts
	//or.w		#0x0700,%sr
	
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

super_done:
	// XXX: copy the rest !

	// XXX: force floppy boot for testing to get the kernel tgz for now
	move.b		#ATARI_BOOT_DRVAPI_FLOPPY,ATARI_ZBEOS_BASE + gBootDriveAPI - _bs_entry 
	move.b		#0,ATARI_ZBEOS_BASE + gBootDriveID - _bs_entry 
	move.b		#1,ATARI_ZBEOS_BASE + gBootedFromImage - _bs_entry 

	// all done, go to _start
	bra			all_done

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

