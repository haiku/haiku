/* Intel PRO/1000 Family Driver
 * Copyright (C) 2004 Marcus Overhagen <marcus@overhagen.de>. All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its 
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies, and that both the
 * copyright notice and this permission notice appear in supporting documentation.
 *
 * Marcus Overhagen makes no representations about the suitability of this software
 * for any purpose. It is provided "as is" without express or implied warranty.
 *
 * MARCUS OVERHAGEN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL MARCUS
 * OVERHAGEN BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef __DEBUG_H
#define __DEBUG_H

#include <KernelExport.h>

#ifdef DEBUG

	/* Set these to 1 to enable debugging */
	#define DEBUG_DEVICE		0
	#define DEBUG_INIT  		0
	#define DEBUG_IOCTL			0
	#define DEBUG_HW			0
	#define DEBUG_FUNC_CALLS	0
	#define DEBUG_DISPLAY_STATS	1

	#define ASSERT(a)						if (a) ; else panic("ipro1000: ASSERT failed, " #a)
	#define	DEBUGFUNC(S)					if (DEBUG_FUNC_CALLS) dprintf("ipro1000: " S "\n")
	#define DEVICE_DEBUGOUT(S)				if (DEBUG_DEVICE) dprintf("ipro1000: " S "\n")
	#define DEVICE_DEBUGOUT1(S,A)			if (DEBUG_DEVICE) dprintf("ipro1000: " S "\n", A)
	#define DEVICE_DEBUGOUT2(S,A,B)			if (DEBUG_DEVICE) dprintf("ipro1000: " S "\n", A, B)
	#define INIT_DEBUGOUT(S)            	if (DEBUG_INIT)  dprintf("ipro1000: " S "\n")
	#define INIT_DEBUGOUT1(S,A)     	   	if (DEBUG_INIT)  dprintf("ipro1000: " S "\n", A)
	#define INIT_DEBUGOUT2(S,A,B)   	  	if (DEBUG_INIT)  dprintf("ipro1000: " S "\n", A, B)
	#define INIT_DEBUGOUT3(S,A,B,C)     	if (DEBUG_INIT)  dprintf("ipro1000: " S "\n", A, B, C)
	#define INIT_DEBUGOUT7(S,A,B,C,D,E,F,G)	if (DEBUG_INIT)  dprintf("ipro1000: " S "\n", A,B,C,D,E,F,G)
	#define IOCTL_DEBUGOUT(S)           	if (DEBUG_IOCTL) dprintf("ipro1000: " S "\n")
	#define IOCTL_DEBUGOUT1(S,A)  	     	if (DEBUG_IOCTL) dprintf("ipro1000: " S "\n", A)
	#define IOCTL_DEBUGOUT2(S,A,B)	    	if (DEBUG_IOCTL) dprintf("ipro1000: " S "\n", A, B)
	#define HW_DEBUGOUT(S)					if (DEBUG_HW) dprintf("ipro1000: " S "\n")
	#define HW_DEBUGOUT1(S,A)				if (DEBUG_HW) dprintf("ipro1000: " S "\n", A)
	#define HW_DEBUGOUT2(S,A,B)				if (DEBUG_HW) dprintf("ipro1000: " S "\n", A, B)
	#define HW_DEBUGOUT7(S,A,B,C,D,E,F,G)	if (DEBUG_HW) dprintf("ipro1000: " S "\n", A,B,C,D,E,F,G)
#else
	#define DEBUG_DISPLAY_STATS	0
	#define	DEBUGFUNC(S)
	#define DEVICE_DEBUGOUT(S)
	#define DEVICE_DEBUGOUT1(S,A)
	#define DEVICE_DEBUGOUT2(S,A,B)
	#define INIT_DEBUGOUT(S)
	#define INIT_DEBUGOUT1(S,A)
	#define INIT_DEBUGOUT2(S,A,B)
	#define INIT_DEBUGOUT3(S,A,B,C)
	#define INIT_DEBUGOUT7(S,A,B,C,D,E,F,G)
	#define IOCTL_DEBUGOUT(S)
	#define IOCTL_DEBUGOUT1(S,A)
	#define IOCTL_DEBUGOUT2(S,A,B)
	#define HW_DEBUGOUT(S)
	#define HW_DEBUGOUT1(S,A)
	#define HW_DEBUGOUT2(S,A,B)
	#define HW_DEBUGOUT7(S,A,B,C,D,E,F,G)
	#define ASSERT(a)
#endif

#define ERROROUT(S)						dprintf("ipro1000: ERROR " S "\n")
#define ERROROUT1(S,A)					dprintf("ipro1000: ERROR " S "\n", A)
#define ERROROUT3(S,A,B,C)				dprintf("ipro1000: ERROR " S "\n", A,B,C)

#endif
