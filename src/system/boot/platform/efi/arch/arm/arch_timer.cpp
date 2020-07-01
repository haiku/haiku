/*
 * Copyright, 2019-2020, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 	Alexander von Gluck IV <kallisti5@unixzen.com>
*/


#include "arch_timer.h"

#include <KernelExport.h>

#include <safemode.h>
#include <boot/stage2.h>
#include <boot/menu.h>

#include <string.h>

//#define TRACE_TIMER
#ifdef TRACE_TIMER
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


void
arch_timer_init(void)
{
	// Stub
}
