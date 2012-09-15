/*
 * Copyright 2009-2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol, revol@free.fr
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */


#include "arch_framebuffer.h"

#include <arch/cpu.h>
#include <boot/stage2.h>
#include <boot/platform.h>
#include <boot/menu.h>
#include <boot/kernel_args.h>
#include <boot/platform/generic/video.h>
#include <drivers/driver_settings.h>


class ArchFBArm920 : public ArchFramebuffer {
public:
							ArchFBArm920(addr_t base);
							~ArchFBArm920();
			status_t		Init();
			status_t		Probe();
			status_t		SetDefaultMode();
			status_t		SetVideoMode(int width, int height, int depth);
};

ArchFBArm920 *arch_get_fb_arm_920(addr_t base);


status_t
ArchFBArm920::Init()
{
	gKernelArgs.frame_buffer.enabled = true;

#warning TODO: ARM920 init
	return B_OK;
}


status_t
ArchFBArm920::Probe()
{
#if 0
	// TODO: More dynamic framebuffer base?
	if (!fBase) {
		int err = platform_allocate_region(&gFrameBufferBase,
			gKernelArgs.frame_buffer.physical_buffer.size, 0, false);
		if (err < B_OK)
			return err;
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

	dprintf("video mode: %ux%ux%u\n", gKernelArgs.frame_buffer.width,
		gKernelArgs.frame_buffer.height, gKernelArgs.frame_buffer.depth);

	return B_OK;
}


status_t
ArchFBArm920::SetDefaultMode()
{
	return SetVideoMode(gKernelArgs.frame_buffer.width,
		gKernelArgs.frame_buffer.height,
		gKernelArgs.frame_buffer.depth);
}


status_t
ArchFBArm920::SetVideoMode(int width, int height, int depth)
{
	#warning TODO: ArchFBArm920 SetVideoMode
	return B_OK;
}
