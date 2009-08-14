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
#include <boot/images.h>
#include <board_config.h>
#include <util/list.h>
#include <drivers/driver_settings.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct fb_description gFrameBuffer;

#define TRACE_VIDEO
#ifdef TRACE_VIDEO
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

#define write_io_32(a, v) ((*(vuint32 *)a) = v)
#define read_io_32(a) (*(vuint32 *)a)

#define dumpr(a) dprintf("LCC:%s:0x%lx\n", #a, read_io_32(a))

//	#pragma mark -


#if BOARD_CPU_PXA270
static struct pxa27x_lcd_dma_descriptor sVideoDMADesc;
static uint32 scratch[128] __attribute__((aligned(16)));
status_t
arch_init_video(void)
{
	void *fb;
	//fb = malloc(800*600*4 + 16 - 1);
	//fb = (void *)(((uint32)fb) & ~(0x0f));
	//fb = (void *)0xa0000000;
	fb = scratch - 800;

	dprintf("fb @ %p\n", fb);

	
	sVideoDMADesc.fdadr = ((uint32)&sVideoDMADesc & ~0x0f) | 0x01;
	sVideoDMADesc.fsadr = (uint32)(fb) & ~0x0f;
	sVideoDMADesc.fidr = 0;
	sVideoDMADesc.ldcmd = (800*600*4);

	// if not already enabled, set a default mode
	if (!(read_io_32(LCCR0) & 0x00000001)) {
		dprintf("Setting default video mode 800x600\n");
		int bpp = 0x09; // 24 bpp
		int pdfor = 0x3; // Format 4: RGB888 (no alpha bit)
		write_io_32(LCCR1, (0 << 0) | (800));
		write_io_32(LCCR2, (0 << 0) | (600-1));
		write_io_32(LCCR3, (pdfor << 30) | ((bpp >> 3) << 29) | ((bpp & 0x07) << 24));
		write_io_32(FDADR0, sVideoDMADesc.fdadr);
		write_io_32(LCCR0, read_io_32(LCCR0) | 0x01800001);     // no ints +ENB
		write_io_32(FBR0, sVideoDMADesc.fdadr);
	dumpr(LCCR0);
	dumpr(LCCR1);
	dumpr(LCCR2);
	dumpr(LCCR3);
	dumpr(LCCR4);
	
	for (int i = 0; i < 128; i++)
		//((uint32 *)fb)[i+16] = 0x000000ff << ((i%4) * 8);
		scratch[i] = 0x000000ff << ((i%4) * 8);
	}
	return B_OK;
}

#elif BOARD_CPU_OMAP3

status_t
arch_init_video(void)
{
	return B_OK;
}

#else


status_t
arch_init_video(void)
{
	return B_OK;
}

#endif


//	#pragma mark -


