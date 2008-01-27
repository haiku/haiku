/*
 * Copyright 2008, François Revol, revol@free.fr. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef ATARI_MEMORY_MAP_H
#define ATARI_MEMORY_MAP_H

#define ATARI_CHIPRAM_TOP_TT		0x00a00000
/* -> e00000 is VME for MegaST*/
#define ATARI_CHIPRAM_TOP_FALCON	0x00e00000

#define ATARI_ROM_BASE				ATARI_CHIPRAM_TOP_FALCON
#define ATARI_ROM_TOP				0x00f00000
#define ATARI_IO_BASE				ATARI_ROM_TOP
#define 	ATARI_CARTRIDGE_BASE	0x00fa0000
#define 	ATARI_OLD_ROM_BASE		0x00fc0000

#define ATARI_IO_TOP				0x01000000
#define ATARI_FASTRAM_TOP			ATARI_IO_TOP

#define ATARI_SHADOW_BASE			0xff000000

/* how we will use it */
//#define ATARI_ZBEOS_STACK_BASE		0x00040000
#define ATARI_ZBEOS_STACK_BASE		0x00060000
#define ATARI_ZBEOS_BASE			0x00080000 /* from .prg shell.S will copy itself there */

#endif	/* ATARI_MEMORY_MAP_H */
