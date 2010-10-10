/*
 * Copyright 2009, François Revol, revol@free.fr.
 */

#define FUNCTION(x) .global x; .type x,@function; x

/*
 * stage1 boot code for AmigaDOS for use as boot block of HDD partitions.
 *
 * cf.
 * http://wandel.ca/homepage/execdis/virus_disassembly.txt
 * http://ciarang.com/wiki/page/Minimal_Amiga_Boot_Code
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
 * 
 * jam -q '<build>fixup_amiga_boot_checksum'
 * generated.m68k/cross-tools/bin/m68k-unknown-haiku-gcc -nostdlib -fpic -Wa,--pcrel -c -o stage1.o src/system/boot/platform/amiga_m68k/stage1.S
 * generated-m68k/cross-tools/bin/m68k-unknown-haiku-ld --oformat binary -o stage1.bin stage1.o
 * generated-m68k/objects/linux/x86/release/tools/fixup_amiga_boot_checksum/fixup_amiga_boot_checksum stage1.bin
 * generated.m68k/cross-tools/bin/m68k-unknown-haiku-ld -o stage1.prg stage1.o -T src/system/boot/platform/atari_m68k/prg.ld
 * 
 * Add to image:
 * dd if=/dev/zero bs=512 count=$((80*2*11)) of=df0.adf
 * dd if=stage1.bin bs=512 conv=notrunc of=df0.adf
 * 
 */


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

// AmigaDOS calls
// exec.library
#define _Supervisor				-0x1e
#define _FindResident			-0x60
#define _Alert					-0x6c
#define _SuperState				-0x96
#define _UserState				-0x9c
#define _OldOpenLibrary         -0x198
#define _CloseLibrary           -0x19e
// dos.library
#define Old_mode                        1005
#define New_mode                        1006
#define _Open					-0x1e
#define _Close					-0x24
#define _Read                   -0x2a
#define _Write                  -0x30
#define _Input					-0x36
#define _Output					-0x3c
#define _IoErr					-0x84
#define _Delay					-0xc6
// intuition.library
#define _DisplayAlert			-90


_floppy_entry:
	.ascii	"DOS"
//	.ascii	"BFS"
	.byte	0
_floppy_checksum:
	.long	0
_floppy_osversion: // ??
//_super_stack: // also used as a variable in the code
	//.long	0x370
	.long	0x09D5A859 // dos private ???

_floppy_boot:

	// we get an open IO request in A1

	// seems like a6 is already set to ExecBase when called from the ROM ?
	// copy it
	move.l	4.w,%a6
	lea		_execbase(%pc),%a2
	move.l	%a6,(%a2)

//	jsr		_SuperState(%a6)
//	move.l	%d0,_super_stack



//	bra _continue

/*
	move.l	%d0,%a6

	lea		window_name(%pc),%a0
	move.l	%a0,%d1
	move.l	#Old_mode,%d2
	jsr		_Open(%a6)
	tst.l	%d0
	beq.s	_floppy_err
_loop_1:
	bra		_loop_1


_loop_2:
	bra		_loop_2
*/	

	//lea _display_alert,%a5
	//jsr _Supervisor(%a6)

	jsr _display_alert


	bra	_floppy_err


//	lea	dosname_boot(%pc),%a1
////	jsr	-0x60(%a6)	// FindResident()
//	move.l	4.w,%a6
//	jsr	-96(%a6)


	tst.l	%d0
	beq.s	_floppy_err



_continue:
	move.l	4.w,%a6


	lea	dosname_boot(%pc),%a1
	jsr	_FindResident(%a6)
	lea	_dosbase(%pc),%a2
	tst.l	%d0
	beq.s	_floppy_err
	move.l	%d0,(%a2)

//	move.l	_super_stack,%d0
//	jsr		_UserState(%a6)

	move.l	_dosbase,%a0
	move.l	0x16(%a0),%a0

	moveq	#0,%d0

	rts

_floppy_err:
//	move.l	4.w,%a6
//	move.l	_super_stack,%d0
//	jsr		_UserState(%a6)

	moveq	#-1,%d0
	rts

_display_alert:
	move.l	4.w,%a6
	lea	intname_boot(%pc),%a1
	jsr	_OldOpenLibrary(%a6)
	tst.l	%d0
	beq.s	_floppy_err
	lea	_intbase(%pc),%a2
	move.l	%d0,(%a2)
	move.l	%d0,%a6

	lea		alert_data(%pc),%a0
	moveq	#0,%d0
	move.l	#30,%d1
	jsr		_DisplayAlert(%a6)
	rts

dosname_boot:
	.ascii	"dos.library"
	.byte	0

intname_boot:
	.ascii	"intuition.library"
	.byte	0

alert_data:
	.word	10
	.byte	12
	.ascii	"Welcome to Haiku\0"
alert_extra:
	.ascii	"\0\0"
	.byte	0

window_name:
	.ascii	"CON:0/0/640/255/plop\0"

_execbase:
	.long	0
_dosbase:
	.long	0
_intbase:
	.long	0

end_buff:
// equ *-_floppy_entry
	.dcb.b	(1024)-(end_buff-_floppy_entry),0

