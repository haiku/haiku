/*
 * Copyright 2008, François Revol, revol@free.fr. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef AMIGA_MEMORY_MAP_H
#define AMIGA_MEMORY_MAP_H


/* the DMA-accessible RAM */
#define AMIGA_CHIPRAM_BASE			0x00000000
//XXX:
/* actually, the first 2kB aren't usable */
//#define AMIGA_CHIPRAM_BASE			0x00001000
#define AMIGA_CHIPRAM_MAX			0x00e00000
#define AMIGA_CHIPRAM_LAST						\
	(AMIGA_CHIPRAM_BASE + (AMIGA_CHIPRAM_MAX - 1))

#define AMIGA_ROM_BASE			0x00f80000
#define AMIGA_ROM_MAX			0x00080000
#define AMIGA_ROM_LAST						\
	(AMIGA_ROM_BASE + (AMIGA_ROM_MAX - 1))

/* some reserved ST I/O there... */

/* cartridge ROM */
#define AMIGA_CARTROM_BASE			0x00fa0000
#define AMIGA_CARTROM_MAX			0x00020000
#define AMIGA_CARTROM_LAST						\
	(AMIGA_CARTROM_BASE + (AMIGA_CARTROM_MAX - 1))

#define AMIGA_SYSROM_BASE			0x00fc0000
#define AMIGA_SYSROM_MAX			0x00030000
#define AMIGA_SYSROM_LAST						\
	(AMIGA_SYSROM_BASE + (AMIGA_SYSROM_MAX - 1))

/* more ST I/O there... */

/* the fast, non-DMA-accessible RAM */
#define AMIGA_FASTRAM_BASE			0x01000000
// max on TT,
// but there is nothing beyond until SHADOW_BASE
//#define AMIGA_FASTRAM_MAX			0x00400000
#define AMIGA_FASTRAM_MAX			0xfe000000
#define AMIGA_FASTRAM_LAST						\
	(AMIGA_FASTRAM_BASE + (AMIGA_FASTRAM_MAX - 1))



/* due to ST legacy (24 bit addressing), IO is actually there */
#define AMIGA_SHADOW_BASE			0xff000000

#define AMIGA_IO_BASE				(AMIGA_SHADOW_BASE + 0x00f00000)
#define AMIGA_IO_MAX				0x00100000
#define AMIGA_IO_LAST							\
	(AMIGA_IO_BASE + (AMIGA_IO_MAX - 1))





/* physical memory layout as used by the bootloader */

//#define AMIGA_ZBEOS_STACK_BASE		0x00040000
#define AMIGA_ZBEOS_STACK_BASE		0x00060000
#define AMIGA_ZBEOS_STACK_END		0x00080000

/* from .prg shell.S will copy itself there
 * must stay in sync with src/system/ldscripts/m68k/boot_prg_amiga_m68k.ld
 */
#define AMIGA_ZBEOS_BASE			0x00080000
#define AMIGA_ZBEOS_MAX				0x00080000
#define AMIGA_ZBEOS_LAST						\
	(AMIGA_ZBEOS_BASE + (AMIGA_ZBEOS_MAX - 1))


#define AMIGA_STRAM_VIRT_BASE		

#endif	/* AMIGA_MEMORY_MAP_H */
