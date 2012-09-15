/*
 * Copyright 2009, François Revol, revol@free.fr.
 * Distributed under the terms of the MIT License.
 */


#include "arch_video.h"

#include <arch/cpu.h>
#include <boot/stage2.h>
#include <boot/platform.h>
#include <boot/menu.h>
#include <boot/kernel_args.h>
#include <boot/platform/generic/video.h>
#include <board_config.h>
#include <util/list.h>
#include <drivers/driver_settings.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//XXX
extern "C" addr_t mmu_map_physical_memory(addr_t physicalAddress, size_t size,
	uint32 flags);


#define TRACE_VIDEO
#ifdef TRACE_VIDEO
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

#define write_io_32(a, v) ((*(vuint32 *)a) = v)
#define read_io_32(a) (*(vuint32 *)a)

#define dumpr(a) dprintf("LCC:%s:0x%lx\n", #a, read_io_32(a))


#if !BOARD_CPU_PXA270 && !BOARD_CPU_OMAP3 && !BOARD_CPU_ARM920T
//	#pragma mark -


status_t
arch_probe_video_mode(void)
{
	return B_ERROR;
}


status_t
arch_set_video_mode(int width, int height, int depth)
{
	return B_ERROR;
}


status_t
arch_set_default_video_mode()
{
	return arch_set_video_mode(800, 600, 32);
}


#endif
