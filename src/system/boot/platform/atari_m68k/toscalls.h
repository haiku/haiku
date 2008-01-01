/*
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		François Revol, revol@free.fr.
 */

#ifndef _TOSCALLS_H
#define _TOSCALLS_H

#ifdef __cplusplus
extern "C" {
#endif

/* 
 * Atari BIOS calls
 */

extern int32 bios(uint16 nr, ...);

#define Bconin(p0) bios(2, p0)
//XXX nparams ?
#define Kbshift(p0) bios(11, p0)

/* 
 * Atari XBIOS calls
 */

extern int32 bios(uint16 nr, ...);



/* 
 * Atari GEMDOS calls
 */

extern int32 gemdos(uint16 nr, ...);

// official names
#define Cconin() gemdos(1)
#define Super(a) gemdos(0x20, (uint32)a)

// check for names
#define terminate() GEMDOS(0)

/*
 * error mapping
 * in debug.c
 */

extern status_t toserror(int32 err);

#ifdef __cplusplus
}
#endif

#endif /* _TOSCALLS_H */
