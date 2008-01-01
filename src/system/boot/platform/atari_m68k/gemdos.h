/*
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		François Revol, revol@free.fr.
 */
#ifndef _GEMDOS_H
#define _GEMDOS_H

/* 
 * Atari GEMDOS calls
 */

#define GEMDOS_TRAP 1

// official names
#define Cconin() GEMDOS(1)

// check for names
#define supexec() GEMDOS(0x20)
#define terminate() GEMDOS(0)

#endif /* _GEMDOS_H */
