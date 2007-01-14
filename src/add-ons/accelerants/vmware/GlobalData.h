/*
 * Copyright 1999, Be Incorporated.
 * Copyright 2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Be Incorporated
 *		Eric Petit <eric.petit@lapsus.org>
 */
#ifndef GLOBAL_DATA_H
#define GLOBAL_DATA_H


#include <support/Debug.h>
#undef TRACE
#define TRACE(a...) _sPrintf("VMware: " a)

#include "DriverInterface.h"
#include "generic.h"

/* Global variables */
extern int			gFd;
extern SharedInfo	*gSi;
extern area_id		gSharedArea;
extern int			gAccelerantIsClone;
extern thread_id	gUpdateThread;
extern volatile int	gUpdateThreadDie;

/* Fifo.c */
void FifoInit(void);
void FifoSync(void);
void FifoBeginWrite(void);
void FifoWrite(uint32 value);
void FifoEndWrite(void);
void FifoUpdateFullscreen(void);

#endif	// GLOBAL_DATA_H
