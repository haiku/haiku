/*
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		François Revol, revol@free.fr.
 */
#ifndef _BIOS_H
#define _BIOS_H

/* 
 * Atari BIOS calls
 */

#define BIOS_TRAP XXX

// BIOSxxx...: x: param type {W|L}

#define BIOS(nr) \
({ uint32 ret; \
asm volatile ( \
"	move	#%0,-(sp)\n" \
"	trap	#" #BIOS_TRAP "\n" \
"	add.l	#2,sp" \
:: "=r"(ret)); \
ret; })

#define BIOSW(nr, w0) \
({ uint32 ret; \
asm volatile ( \
"	move.w	%1,-(sp)\n" \
"	move	#" #nr ",-(sp)\n" \
"	trap	#" #BIOS_TRAP "\n" \
"	add.l	#2,sp" \
:: "=r"(ret)); \
ret; })

#define Bconin(p0) (uint32)BIOSW(2, p0)
//XXX nparams ?
#define Kbshift(p0) (uint32)BIOSW(11, p0)

#endif /* _BIOS_H */
