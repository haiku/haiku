/*
 * Copyright 2008, François Revol, revol@free.fr. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef ATARI_MEMORY_MAP_H
#define ATARI_MEMORY_MAP_H


/* the DMA-accessible RAM */
/*#define ATARI_CHIPRAM_BASE			0x00000000*/
/* actually, the first 2kB aren't usable */
#define ATARI_CHIPRAM_BASE			0x00001000
#define ATARI_CHIPRAM_MAX			0x00e00000
#define ATARI_CHIPRAM_LAST						\
	(ATARI_CHIPRAM_BASE + (ATARI_CHIPRAM_MAX - 1))

#define ATARI_TOSROM_BASE			0x00e00000
#define ATARI_TOSROM_MAX			0x00100000
#define ATARI_TOSROM_LAST						\
	(ATARI_TOSROM_BASE + (ATARI_TOSROM_MAX - 1))

/* some reserved ST I/O there... */

/* cartridge ROM */
#define ATARI_CARTROM_BASE			0x00fa0000
#define ATARI_CARTROM_MAX			0x00020000
#define ATARI_CARTROM_LAST						\
	(ATARI_CARTROM_BASE + (ATARI_CARTROM_MAX - 1))

#define ATARI_SYSROM_BASE			0x00fc0000
#define ATARI_SYSROM_MAX			0x00030000
#define ATARI_SYSROM_LAST						\
	(ATARI_SYSROM_BASE + (ATARI_SYSROM_MAX - 1))

/* more ST I/O there... */

/* the fast, non-DMA-accessible RAM */
#define ATARI_FASTRAM_BASE			0x01000000
// max on TT,
// but there is nothing beyond until SHADOW_BASE
//#define ATARI_FASTRAM_MAX			0x00400000
#define ATARI_FASTRAM_MAX			0xfe000000
#define ATARI_FASTRAM_LAST						\
	(ATARI_FASTRAM_BASE + (ATARI_FASTRAM_MAX - 1))



/* due to ST legacy (24 bit addressing), IO is actually there */
#define ATARI_SHADOW_BASE			0xff000000

#define ATARI_IO_BASE				(ATARI_SHADOW_BASE + 0x00f00000)
#define ATARI_IO_MAX				0x00100000
#define ATARI_IO_LAST							\
	(ATARI_IO_BASE + (ATARI_IO_MAX - 1))





/* physical memory layout as used by the bootloader */

//#define ATARI_ZBEOS_STACK_BASE		0x00040000
#define ATARI_ZBEOS_STACK_BASE		0x00060000
#define ATARI_ZBEOS_STACK_END		0x00080000

/* from .prg shell.S will copy itself there
 * must stay in sync with src/system/ldscripts/m68k/boot_prg_atari_m68k.ld
 */
#define ATARI_ZBEOS_BASE			0x00080000
#define ATARI_ZBEOS_MAX				0x00080000
#define ATARI_ZBEOS_LAST						\
	(ATARI_ZBEOS_BASE + (ATARI_ZBEOS_MAX - 1))


#define ATARI_STRAM_VIRT_BASE		

#endif	/* ATARI_MEMORY_MAP_H */
