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

/*
 * references :
 * http://ftp.netbsd.org/pub/NetBSD/NetBSD-release-4-0/src/sys/arch/amiga/stand/bootblock/
 * http://wandel.ca/homepage/execdis/virus_disassembly.txt
 * http://ciarang.com/wiki/page/Minimal_Amiga_Boot_Code
 * 
 */

#include "amiga_memory_map.h"
#include "rom_calls.h"


// 1 enabled verbose output
//#define DEBUG 1

#define GLOBAL(x) .globl x ; x
#define FUNCTION(x) .global x; .type x,@function; x

#define DRIVE_RETRIES 3
	// when the drive reading fails for some reason, it will
	// retry this many times until it will report a failure


#define SECTSIZE 512

// .text
_bs_entry:
	// Amiga boot sector
_floppy_entry:
	.ascii	"DOS"
	.byte	0
_floppy_checksum:
	.long	0
_floppy_osversion: // ??
	//.long	0x370
	.long	0x09D5A859 // dos private ???

/*
 * floppy boot support code
 */

floppy_start:
	// save some regs just in case
	movem.l		%d1-%d7/%a0-%a6,-(%sp)

	// save the provided IOReq pointer
	lea		sBootIOReq(%pc),%a2
	move.l	%a1,(%a2)

	// seems like a6 is already set to ExecBase when called from the ROM ?
	move.l	4.w,%a6	// exec base
	//jsr	_LVOFindResident(%a6)
	lea	SysBase(%pc),%a2
	move.l	%a6,(%a2)


//	lea	dosname_boot(%pc),%a1
//	move.l	4.w,%a6	// exec base
//	jsr	_LVOFindResident(%a6)
//	lea	DosBase(%pc),%a2
//	tst.l	%d0
//	beq.s	_floppy_err
//	move.l	%d0,(%a2)

	// needed to display alerts
	lea	intname_boot(%pc),%a1
	jsr	_LVOOldOpenLibrary(%a6)
	tst.l	%d0
	beq		_floppy_err
	lea	IntuitionBase(%pc),%a2
	move.l	%d0,(%a2)

	// allocate absolute memory to put the stack & bootloader at,
	// just to make sure AmigaOS won't write there.
	
	move.l	#(AMIGA_ZBEOS_STACK_END-AMIGA_ZBEOS_STACK_BASE+AMIGA_ZBEOS_MAX),%d0
	move.l	#AMIGA_ZBEOS_STACK_BASE,%a1
	jsr		_LVOAllocAbs(%a6)
	tst.l	%d0
	beq.s	_floppy_err_alert
	
	//lea		alert_extra(%pc),%a2
	//addq.b	#1,(%a2)
	move.l	#0xdeadbeef,%d0
	bsr		putx

	// copy haiku_loader to AMIGA_ZBEOS_BASE
	


	lea		sBootIOReq,%a1
	move.l	(%a1),%a1
	move	#CMD_READ,28(%a1)				//io_Command
	//move.l	#0x200,%d0
	//XXX
	//mulu	sNumSectors,%d0
	//move.l	%d0,36(%a1)				//io_Length
	move.l	#0x200,36(%a1)				//io_Length
	move.l	#AMIGA_ZBEOS_BASE,40(%a1)	//io_Data
	move.l	#0,44(%a1)					//io_Offset
	move.w	sNumSectors,%d2

next_sector:
	move.w	%d2,%d0
	bsr		putx

	move.l	%a1,-(%sp)
	jsr		_LVODoIO(%a6)
	move.l	(%sp)+,%a1
	
	tst.l	%d0
	bne	_floppy_err_alert
	
	add.l	#0x200,40(%a1)			//io_Data
	add.l	#0x200,44(%a1)			//io_Offset
	//subq.w	#1,sNumSectors
	subq.w	#1,%d2
	bne		next_sector

	move.l	#0xf100f100,%d0
	bsr		putx

	//lea		alert_extra(%pc),%a2
	//move.b	#'F',(%a2)

//	move.l
//	cmp.l	32(%a1),36(%a1)
//	blt.s	_floppy_err_alert

	//lea		alert_extra(%pc),%a2
	//addq.b	#1,(%a2)

	bra.s	floppy_done
	

//_continue:
//	lea	dosname_boot(%pc),%a1
//	move.l	4.w,%a6	// exec base
//	jsr	_LVOFindResident(%a6)
//	lea	_dosbase(%pc),%a2
//	tst.l	%d0
//	beq.s	_floppy_err
//	move.l	%d0,(%a2)
//	// pop up saved regs
//	movem.l		(%sp)+,%d1-%d7/%a0-%a6
//	move.l	_dosbase,%a0
//	move.l	0x16(%a0),%a0
//	moveq	#0,%d0
//	rts

GLOBAL(_floppy_err_alert):
	lea		IntuitionBase(%pc),%a6
	move.l	(%a6),%a6
	lea		alert_data(%pc),%a0
	moveq	#0,%d0
	move.l	#30,%d1
	jsr		_LVODisplayAlert(%a6)
	
_floppy_err:
	// pop up saved regs
	movem.l		(%sp)+,%d1-%d7/%a0-%a6
	moveq	#-1,%d0
	rts






	
floppy_done:
	// setup stack
	move.l		#AMIGA_ZBEOS_STACK_END,%sp
	//jmp			AMIGA_ZBEOS_BASE+512

	//move.w		TOSVAR_bootdev,%d0
	// XXX:	 use uint16 ??
	//move.b		%d0,AMIGA_ZBEOS_BASE + gBootDriveID - _bs_entry 
	move.b		#1,AMIGA_ZBEOS_BASE + gBootedFromImage - _bs_entry 
	// XXX: copy the rest !

	move.b		#'S',AMIGA_ZBEOS_BASE + alert_extra - _bs_entry 
	// Copy open library handles
	move.l		SysBase(%pc),AMIGA_ZBEOS_BASE + SysBase - _bs_entry 
	move.l		IntuitionBase(%pc),AMIGA_ZBEOS_BASE + IntuitionBase - _bs_entry 

	move.l		#0,%d0

	// jump to C++ code
	jmp		_start


putx:
	movem.l	%d0-%d2/%a0-%a2,-(%sp)
	lea		alert_extra(%pc),%a2
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
	move.b	%d0,(%a2)+
	dbf	%d2,putxloop
//	bsr	putcrlf
	movem.l	(%sp)+,%d0-%d2/%a0-%a2
	rts


	// ATARI
#if 0


floppy_end:
//	.org	FAILURE_STRING
failure_string:
//	.string " Loading failed! Press key to reboot.\r\n"
	.string " Loading failed! Press key.\r\n"
//	.string "FAIL"
	
//	.org	DOT_STRING
//	.string	"."

str:
	.string "Haiku!"
msg_j1:
	.string "Jumping to haiku_loader."

#endif

sNumSectors:
	// this location will contain the length of the boot loader as
	// written by the "makeflop" command in 512 byte blocks
	// 0x180 is the allowed maximum, as the zipped TAR with the
	// kernel and the boot module might start at offset 192 kB
	//.word	0x0300 //0x0180
	.word	BOOT_ARCHIVE_IMAGE_OFFSET*2

sBootIOReq:
	.long	0

dosname_boot:
	.ascii	"dos.library"
	.byte	0

intname_boot:
	.ascii	"intuition.library"
	.byte	0

alert_data:
	.word	10
	.byte	12
	.ascii	"Error loading Haiku: "
alert_extra:
	.ascii	"XXXXXXXX\0"
	.byte	0

GLOBAL(SysBase):
	.long	0
GLOBAL(DosBase):
	.long	0
GLOBAL(IntuitionBase):
	.long	0

end_buff:
// equ *-_floppy_entry
	.dcb.b	(1024)-(end_buff-_floppy_entry),0



//XXX: put bfs_start here

/*
 * \AUTO\HAIKU.PRG and ARAnyM BOOTSTRAP() support code
 */

#if 0

super_done:
	// XXX: copy the rest !
	move.b		#AMIGA_BOOT_DRVAPI_FLOPPY,AMIGA_ZBEOS_BASE + gBootDriveAPI - _bs_entry 
	move.b		#0,AMIGA_ZBEOS_BASE + gBootDriveID - _bs_entry 
	move.b		#1,AMIGA_ZBEOS_BASE + gBootedFromImage - _bs_entry 

	move.l		#0,%d0
#endif

	//jmp			AMIGA_ZBEOS_BASE+512
	jmp			_start

saved_super_stack:
	.long	0

GLOBAL(gBootedFromImage):
	.byte	0

GLOBAL(gBootDriveID):
	.byte	0

GLOBAL(gBootPartitionOffset):
	.long	0

