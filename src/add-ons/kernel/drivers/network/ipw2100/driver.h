/*
	Driver for Intel(R) PRO/Wireless 2100 devices.
	Copyright (C) 2006 Michael Lotz <mmlr@mlotz.ch>
	Released under the terms of the MIT license.
*/

#ifndef _DRIVER_H_
#define _DRIVER_H_

#include <SupportDefs.h>
#include <KernelExport.h>

#define	DRIVER_NAME			"ipw2100"
#define	DRIVER_VERSION		1.0.0
#define	DRIVER_DESCRIPTION	"Intel(R) PRO/Wireless 2100 Driver"

#define MAX_INSTANCES		3
#define VENDOR_ID_INTEL		0x8086

//#define TRACE_IPW2100
#ifdef TRACE_IPW2100
#define TRACE(x)			dprintf x
#define TRACE_ALWAYS(x)		dprintf x
#else
#define TRACE(x)			/* nothing */
#define TRACE_ALWAYS(x)		dprintf x
#endif

#endif // _DRIVER_H_
