/*
 * Copyright 2008-2010, François Revol, revol@free.fr. All rights reserved.
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 */

#define FUNCTION(x) .global x; .type x,@function; x

/*
 * stage1 boot code for TOS for use as boot block of HDD partitions.
 * cf.
 * http://www.fortunecity.com/skyscraper/apple/308/html/chap3.htm
 * http://leonard.oxg.free.fr/articles/multi_atari/multi_atari.html
 * http://alive.atari.org/alive10/btmania.php
 * AHDI boot args:
 * http://ftp.netbsd.org/pub/NetBSD/NetBSD-current/src/sys/arch/atari/stand/xxboot/ahdi-xxboot/xxboot.ahdi.S
 * gas stuff:
 * http://www.gnu.org/software/binutils/manual/gas-2.9.1/html_chapter/as_18.html#SEC214
 * 
 * x86:
 * The offset of the partition in 512 byte blocks must be written at
 * position PARTITION_OFFSET_OFFSET (32 bit little endian; makebootable does
 * that) or otherwise the code can't find the partition.
 * The partition must be a BFS formatted. The file "system/haiku_loader"
 * (the stage 2 boot loader) loaded into memory at 0x1000:0x0000 (linear address
 * 0x10000) and entered at 0x:1000:0x0200 with parameters eax - partition offset
 * in 512 byte blocks and dl - BIOS ID of the boot drive.
 * 
 * Compile via:
 * generated.m68k/cross-tools/bin/m68k-unknown-haiku-gcc -nostdlib -fpic -c -o stage1.o src/system/boot/platform/atari_m68k/stage1.S
 * generated.m68k/cross-tools/bin/m68k-unknown-haiku-objcopy -O binary stage1.o stage1
 * 
 * Add to image:
 * dd conv=notrunc if=haiku/trunk/stage1 of=em-20-200.swapped.hd bs=1 count=30
 * set bootable:
 * XXX: that's wrong! echo -en '\x12\x34' | dd conv=notrunc seek=510 bs=1 count=2 of=em-20-200.swapped.hd
 * 
 * .PRG file format:
 * http://mail-index.netbsd.org/tech-userlevel/2007/04/02/0000.html
 * http://pagesperso-orange.fr/patrice.mandin/en/howto-binutils.html
 * http://www.wotsit.org/download.asp?f=atariexe&sc=252874182
 *
 * generated.m68k/cross-tools/bin/m68k-unknown-haiku-gcc -nostdlib -fpic -Wa,--pcrel -c -o stage1.o src/system/boot/platform/atari_m68k/stage1.S
 * generated.m68k/cross-tools/bin/m68k-unknown-haiku-ld -o stage1.prg stage1.o -T src/system/boot/platform/atari_m68k/prg.ld
 * 
 */

#include "toscalls.h"

// 1 enabled verbose output
//#define DEBUG 1


#define	BOOT_BLOCK_START_ADDRESS	0x7c00
#define	STACK_ADDRESS				BOOT_BLOCK_START_ADDRESS
#define	READ_BUFFER_STACK			STACK_ADDRESS - 0x2000
#define	PARTITION_OFFSET_OFFSET		506
#define	BFS_SUPERBLOCK_OFFSET		512


// BFS definitions

#define SUPER_BLOCK_MAGIC1			'1SFB'		; nasm reverses '...' consts
#define SUPER_BLOCK_MAGIC2			0xdd121031
#define SUPER_BLOCK_MAGIC3			0x15b6830e

#define	INODE_MAGIC1				0x3bbe0ad9

#define	NUM_DIRECT_BLOCKS			12

#define	S_IFMT						00000170000o
#define	S_IFDIR						00000040000o


// NOTE: normal programs (.prg) run as user mode, 
// while boot sector is chained in supervisor mode.
// this means using Super(SUP_INQUIRE) we can know 
// from the same entry point if we were run from boot code or prg.


	//Pterm0
	//move.w	#1,%d0
	//trap	#1
	//rts
	
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
