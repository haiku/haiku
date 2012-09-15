/*
 * Copyright 2009-2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      François Revol, revol@free.fr
 *      Alexander von Gluck IV, kallisti5@unixzen.com
 */


#include "arch_framebuffer.h"

#include <arch/arm/omap3.h>
#include <arch/cpu.h>
#include <boot/stage2.h>
#include <boot/platform.h>
#include <boot/menu.h>
#include <boot/kernel_args.h>
#include <boot/platform/generic/video.h>
#include <util/list.h>
#include <drivers/driver_settings.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "graphics/omap/omap3_regs.h"


//XXX
extern "C" addr_t mmu_map_physical_memory(addr_t physicalAddress, size_t size,
	uint32 flags);


#define write_io_32(a, v) ((*(vuint32 *)a) = v)
#define read_io_32(a) (*(vuint32 *)a)

#define dumpr(a) dprintf("LCC:%s:0x%lx\n", #a, read_io_32(a))


class ArchFBArmOmap3 : public ArchFramebuffer {
public:
							ArchFBArmOmap3(addr_t base);
							~ArchFBArmOmap3();
			status_t		Init();
			status_t		Probe();
			status_t		SetDefaultMode();
			status_t		SetVideoMode(int width, int height, int depth);
};

ArchFBArmOmap3 *arch_get_fb_arm_omap3(addr_t base);


//	#pragma mark -


struct video_mode {
	short width, height;
	const char *name;
	uint32 dispc_timing_h;
	uint32 dispc_timing_v;
	uint32 dispc_divisor;
	uint32 dss_divisor;
};

// Master clock (PLL4) is 864 Mhz, and changing it is a pita since it
// cascades to other devices. 
// Pixel clock is 864 / cm_clksel_dss.dss1_alwan_fclk / dispc_divisor.divisor.pcd
// So most of these modes are just approximate (1280x1024 is correct)
// List must be in ascending order.
struct video_mode modes[] = {
	{
		640, 480, "640x480-71",
		(128 << DISPCB_HBP) | (24 << DISPCB_HFP) | (40 << DISPCB_HSW),
		(28 << DISPCB_VBP) | (9 << DISPCB_VFP) | (3 << DISPCB_VSW),
		2, 14
	},
	{
		800, 600, "800x600-59",
		(88 << DISPCB_HBP) | (40 << DISPCB_HFP) | (128 << DISPCB_HSW),
		(23 << DISPCB_VBP) | (1 << DISPCB_VFP) | (4 << DISPCB_VSW),
		2, 11
	},       	  
	{
		1024, 768, "1024x768-61",
		(160 << DISPCB_HBP) | (24 << DISPCB_HFP) | (136 << DISPCB_HSW),
		(29 << DISPCB_VBP) | (3 << DISPCB_VFP) | (6 << DISPCB_VSW),
		1, 13
	},
	{
		1280, 1024, "1280x1024-60",
		(248 << DISPCB_HBP) | (48 << DISPCB_HFP) | (112 << DISPCB_HSW),
		(38 << DISPCB_VBP) | (1 << DISPCB_VFP) | (3 << DISPCB_VSW),
		1, 8
	},
};


static inline void
setaddr(uint32 reg, unsigned int v)
{
	*((volatile uint32 *)(reg)) = v;
}


static inline void
modaddr(unsigned int reg, unsigned int m, unsigned int v)
{
	uint32 o;

	o = *((volatile uint32 *)(reg));
	o &= ~m;
	o |= v;
	*((volatile uint32 *)(reg)) = o;
}


static inline
void setreg(uint32 base, unsigned int reg, unsigned int v)
{
	*((volatile uint32 *)(base + reg)) = v;
}


static inline
uint32 readreg(uint32 base, unsigned int reg)
{
	return *((volatile uint32 *)(base + reg));
}


static inline void
modreg(uint32 base, unsigned int reg, unsigned int m, unsigned int v)
{
	uint32 o;

	o = *((volatile uint32 *)(base + reg));
	o &= ~m;
	o |= v;
	*((volatile uint32 *)(base + reg)) = o;
}


// init beagle gpio for video
static void
omap_beagle_init(void)
{
	// setup GPIO stuff, i can't find any references to these
	setreg(GPIO1_BASE, GPIO_OE, 0xfefffedf);
	setreg(GPIO1_BASE, GPIO_SETDATAOUT, 0x01000120);
	// DVI-D is enabled by GPIO 170?
}


static void
omap_clock_init(void)
{
	// sets pixel clock to 72MHz

	// sys_clk = 26.0 Mhz
	// DPLL4 = sys_clk * 432 / 13 = 864
	// DSS1_ALWON_FCLK = 864 / 6 = 144
	// Pixel clock (DISPC_DIVISOR) = 144 / 2 = 72Mhz
	// and also VENC clock = 864 / 16 = 54MHz

	// The clock multiplier/divider cannot be changed
	// without affecting other system clocks - do don't.

	// pll4 clock multiplier/divider
	setaddr(CM_CLKSEL2_PLL, (432 << 8) | 12);
	// tv clock divider, dss1 alwon fclk divider
	setaddr(CM_CLKSEL_DSS, (16 << 8) | 6);
	// core/peripheral PLL to 1MHz
	setaddr(CM_CLKEN_PLL, 0x00370037);
}


static void
omap_dss_init(void)
{
	setreg(DSS_BASE, DSS_SYSCONFIG, DSS_AUTOIDLE);
	// Select DSS1 ALWON as clock source
	setreg(DSS_BASE, DSS_CONTROL, DSS_VENC_OUT_SEL | DSS_DAC_POWERDN_BGZ
		| DSS_DAC_DEMEN | DSS_VENC_CLOCK_4X_ENABLE);
}


static void
omap_dispc_init(void)
{
	uint32 DISPC = DISPC_BASE;

	setreg(DISPC, DISPC_SYSCONFIG,
		DISPC_MIDLEMODE_SMART
		| DISPC_SIDLEMODE_SMART
		| DISPC_ENWAKEUP
		| DISPC_AUTOIDLE);

	setreg(DISPC, DISPC_CONFIG, DISPC_LOADMODE_FRAME);

	// LCD default colour = black
	setreg(DISPC, DISPC_DEFAULT_COLOR0, 0x000000);

	setreg(DISPC, DISPC_POL_FREQ,
		DISPC_POL_IPC
		| DISPC_POL_IHS
		| DISPC_POL_IVS
		| (2<<DISPCB_POL_ACBI)
		| (8<<DISPCB_POL_ACB));

	// Set pixel clock divisor = 2
	setreg(DISPC, DISPC_DIVISOR,
		(1<<DISPCB_DIVISOR_LCD)
		| (2<<DISPCB_DIVISOR_PCD));

	// Disable graphical output
	setreg(DISPC, DISPC_GFX_ATTRIBUTES, 0);

	// Turn on the LCD output
	setreg(DISPC, DISPC_CONTROL,
		DISPC_GPOUT1
		| DISPC_GPOUT0
		| DISPC_TFTDATALINES_24
		| DISPC_STDITHERENABLE
		| DISPC_GOLCD
		| DISPC_STNTFT
		| DISPC_LCDENABLE
		);

	while ((readreg(DISPC, DISPC_CONTROL) & DISPC_GOLCD))
		;
}


static void
omap_set_lcd_mode(int w, int h)
{
	uint32 DISPC = DISPC_BASE;
	unsigned int i;
	struct video_mode *m;

	dprintf("omap3: set_lcd_mode %d,%d\n", w, h);

	for (i = 0; i < sizeof(modes) / sizeof(modes[0]); i++) {
		if (w <= modes[i].width
			&& h <= modes[i].height)
		goto found;
	}
	i -= 1;
found:
	m = &modes[i];

	dprintf("omap3: found mode[%s]\n", m->name);

	setreg(DISPC, DISPC_SIZE_LCD, (m->width - 1) | ((m->height - 1) << 16));
	setreg(DISPC, DISPC_TIMING_H, m->dispc_timing_h);
	setreg(DISPC, DISPC_TIMING_V, m->dispc_timing_v);

	modreg(DISPC, DISPC_DIVISOR, 0xffff, m->dispc_divisor);
	modaddr(CM_CLKSEL_DSS, 0xffff, m->dss_divisor);

	// Tell hardware to update, and wait for it
	modreg(DISPC, DISPC_CONTROL,
		DISPC_GOLCD,
		DISPC_GOLCD);

	while ((readreg(DISPC, DISPC_CONTROL) & DISPC_GOLCD))
		;
}


static void
omap_attach_framebuffer(addr_t data, int width, int height, int depth)
{
	uint32 DISPC = DISPC_BASE;
	uint32 gsize = ((height - 1) << 16) | (width - 1);

	dprintf("omap3: attach bitmap (%d,%d) to screen\n", width, height);

	setreg(DISPC, DISPC_GFX_BA0, (uint32)data);
	setreg(DISPC, DISPC_GFX_BA1, (uint32)data);
	setreg(DISPC, DISPC_GFX_POSITION, 0);
	setreg(DISPC, DISPC_GFX_SIZE, gsize);
	setreg(DISPC, DISPC_GFX_FIFO_THRESHOLD, (0x3ff << 16) | 0x3c0);
	setreg(DISPC, DISPC_GFX_ROW_INC, 1);
	setreg(DISPC, DISPC_GFX_PIXEL_INC, 1);
	setreg(DISPC, DISPC_GFX_WINDOW_SKIP, 0);
	setreg(DISPC, DISPC_GFX_ATTRIBUTES, DISPC_GFXFORMAT_RGB16
		| DISPC_GFXBURSTSIZE_16x32 | DISPC_GFXENABLE);

	// Tell hardware to update, and wait for it
	modreg(DISPC, DISPC_CONTROL, DISPC_GOLCD, DISPC_GOLCD);

	while ((readreg(DISPC, DISPC_CONTROL) & DISPC_GOLCD))
		;
}


status_t
ArchFBArmOmap3::Init()
{
	gKernelArgs.frame_buffer.enabled = true;

	setreg(DISPC_BASE, DISPC_IRQENABLE, 0x00000);
	setreg(DISPC_BASE, DISPC_IRQSTATUS, 0x1ffff);

	omap_beagle_init();
	omap_clock_init();
	omap_dss_init();
	omap_dispc_init();

	return B_OK;
}


status_t
ArchFBArmOmap3::Probe()
{

#if 0
	// TODO: More dynamic framebuffer base?
	if (!fBase) {
		int err = platform_allocate_region(&gFrameBufferBase,
			gKernelArgs.frame_buffer.physical_buffer.size, 0, false);
		if (err < B_OK) return err;
		gKernelArgs.frame_buffer.physical_buffer.start
			= (addr_t)gFrameBufferBase;
		dprintf("video framebuffer: %p\n", gFrameBufferBase);
	}
#else
	gKernelArgs.frame_buffer.physical_buffer.start = fBase;
#endif

	gKernelArgs.frame_buffer.depth = 16;
	gKernelArgs.frame_buffer.width = 1024;
	gKernelArgs.frame_buffer.height = 768;
	gKernelArgs.frame_buffer.bytes_per_row = gKernelArgs.frame_buffer.width * 2;
	gKernelArgs.frame_buffer.physical_buffer.size
		= gKernelArgs.frame_buffer.width
		* gKernelArgs.frame_buffer.height
		* gKernelArgs.frame_buffer.depth / 8;

	TRACE("video mode: %ux%ux%u\n", gKernelArgs.frame_buffer.width,
		gKernelArgs.frame_buffer.height, gKernelArgs.frame_buffer.depth);

	return B_OK;
}


status_t
ArchFBArmOmap3::SetVideoMode(int width, int height, int depth)
{
	TRACE("%s: %dx%d@%d\n", __func__, width, height, depth);

	omap_set_lcd_mode(width, height);
	omap_attach_framebuffer(fBase, width, height, depth);

	return B_OK;
}


status_t
ArchFBArmOmap3::SetDefaultMode()
{
	CALLED();

	return SetVideoMode(gKernelArgs.frame_buffer.width,
		gKernelArgs.frame_buffer.height,
		gKernelArgs.frame_buffer.depth);
}
