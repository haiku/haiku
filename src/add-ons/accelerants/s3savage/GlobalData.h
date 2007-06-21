/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors:
	Gerald Zajac 2006-2007
*/

#ifndef GLOBALDATA_H
#define GLOBALDATA_H

#include "DriverInterface.h"

extern int			 driverFileDesc;	// file descriptor of kernel driver
extern SharedInfo*	 si;				// address of info shared between accelerants
extern area_id		 sharedInfoArea;	// shared info area ID
extern uint8*		 regs;				// base address of MMIO register area
extern area_id		 regsArea;			// MMIO register area ID
extern display_mode* modeList;			// list of standard display modes
extern area_id		 modeListArea;		// mode list area ID
extern bool			 bAccelerantIsClone;// true if this is a cloned accelerant


void TraceLog(const char* fmt, ...);

#ifdef TRACE_S3SAVAGE
#	define TRACE(a)	TraceLog a
#else
#	define TRACE(a)
#endif

#endif	// GLOBALDATA_H
