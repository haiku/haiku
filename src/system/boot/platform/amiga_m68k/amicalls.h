/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		François Revol, revol@free.fr.
 * 
 */
#ifndef _AMICALLS_H
#define _AMICALLS_H


#ifdef __cplusplus
extern "C" {
#endif

#ifndef __ASSEMBLER__
#include <OS.h>
#include <SupportDefs.h>
#endif /* __ASSEMBLER__ */


#ifndef __ASSEMBLER__

// <exec/types.h>


// <exec/nodes.h>


// <exec/lists.h>


// <exec/interrupts.h>


// <exec/library.h>


// <exec/execbase.h>


#endif /* __ASSEMBLER__ */


#define AFF_68010	(0x01)
#define AFF_68020	(0x02)
#define AFF_68030	(0x04)
#define AFF_68040	(0x08)
#define AFF_68881	(0x10)
#define AFF_68882	(0x20)
#define AFF_FPU40	(0x40)


#ifndef __ASSEMBLER__

// <exec/ports.h>



// <exec/io.h>


#endif /* __ASSEMBLER__ */

// io_Flags
#define IOB_QUICK	0
#define IOF_QUICK	0x01


#define CMD_INVALID	0
#define CMD_RESET	1
#define CMD_READ	2
#define CMD_WRITE	3
#define CMD_UPDATE	4
#define CMD_CLEAR	5
#define CMD_STOP	6
#define CMD_START	7
#define CMD_FLUSH	8
#define CMD_NONSTD	9


#ifndef __ASSEMBLER__

// <exec/devices.h>


#endif /* __ASSEMBLER__ */


// <exec/errors.h>

#define IOERR_OPENFAIL		(-1)
#define IOERR_ABORTED		(-2)
#define IOERR_NOCMD			(-3)
#define IOERR_BADLENGTH		(-4)
#define IOERR_BADADDRESS	(-5)
#define IOERR_UNITBUSY		(-6)
#define IOERR_SELFTEST		(-7)


#ifndef __ASSEMBLER__
extern "C" status_t exec_error(int32 err);
#endif /* __ASSEMBLER__ */


// <intuition/intuition.h>


#define ALERT_TYPE		0x80000000
#define RECOVERY_ALERT	0x00000000
#define DEADEND_ALERT	0x80000000


#ifdef __cplusplus
}
#endif

#endif /* _AMICALLS_H */
