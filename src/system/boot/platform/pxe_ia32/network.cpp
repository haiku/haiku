/*
 * Copyright 2006, Marcus Overhagen <marcus@overhagen.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <OS.h>
#include <KernelExport.h>

#include <boot/net/Ethernet.h>
#include <boot/net/NetStack.h>

#include "pxe_undi.h"

#define TRACE_NETWORK
#ifdef TRACE_NETWORK
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...)
#endif


status_t
platform_net_stack_init()
{
	TRACE("platform_net_stack_init\n");

	pxe_undi_init();

	return B_OK;
}
