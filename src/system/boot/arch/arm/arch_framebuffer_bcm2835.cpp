/*
 * Copyright 2012-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz, mmlr@mlotz.ch
 *		François Revol, revol@free.fr
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */


#include "arch_framebuffer.h"

#include <arch/arm/bcm283X.h>
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

#include "arch_mailbox.h"
#include "arch_mmu.h"
#include "fdt_support.h"


//XXX
extern "C" bool
mmu_get_virtual_mapping(addr_t virtualAddress, phys_addr_t *_physicalAddress);

extern "C" ArchMailbox*
arch_get_mailbox_arm_bcm2835(addr_t base);


extern void* gFDT;


struct framebuffer_config {
	uint32	width;
	uint32	height;
	uint32	virtual_width;
	uint32	virtual_height;
	uint32	bytes_per_row;			// from GPU
	uint32	bits_per_pixel;
	uint32	x_offset;
	uint32	y_offset;
	uint32	frame_buffer_address;	// from GPU
	uint32	screen_size;			// from GPU
	uint16	color_map[256];
};


static framebuffer_config sFramebufferConfig __attribute__((aligned(16)));


class ArchFBArmBCM2835 : public ArchFramebuffer {
public:
							ArchFBArmBCM2835(addr_t base)
								: ArchFramebuffer(base) {}
							~ArchFBArmBCM2835() {}

virtual	status_t			Init();
virtual	status_t			Probe();
virtual	status_t			SetDefaultMode();
virtual	status_t			SetVideoMode(int width, int height, int depth);
private:
		ArchMailbox*		fMailbox;
};


extern "C" ArchFramebuffer*
arch_get_fb_arm_bcm2835(addr_t base)
{
    return new ArchFBArmBCM2835(base);
}


status_t
ArchFBArmBCM2835::Init()
{
	if (!gFDT) {
		dprintf("ERROR: FDT access is unavailable!");
		return B_ERROR;
	}
	phys_addr_t mboxBase = fdt_get_device_reg_byname(gFDT, "/axi/mbox");
	if (!mboxBase) {
		dprintf("ERROR: /axi/mbox is unavailable!");
		return B_ERROR;
	}
	fMailbox = arch_get_mailbox_arm_bcm2835(mboxBase);

	if (fMailbox == NULL) {
		dprintf("ERROR: Broadcom mailbox is unavailable!");
		return B_ERROR;
	}

	gKernelArgs.frame_buffer.enabled = true;
	return B_OK;
}


status_t
ArchFBArmBCM2835::Probe()
{
	return B_OK;
}


status_t
ArchFBArmBCM2835::SetDefaultMode()
{
	status_t result;
	do {
		result = SetVideoMode(1920, 1080, 16);
	} while (result != B_OK);

	return B_OK;
}


status_t
ArchFBArmBCM2835::SetVideoMode(int width, int height, int depth)
{
	//debug_assert(((uint32)&sFramebufferConfig & 0x0f) == 0);

	sFramebufferConfig.width = width;
	sFramebufferConfig.height = height;
	sFramebufferConfig.virtual_width = sFramebufferConfig.width;
	sFramebufferConfig.virtual_height = sFramebufferConfig.height;
	sFramebufferConfig.bytes_per_row = 0; // from GPU
	sFramebufferConfig.bits_per_pixel = depth;
	sFramebufferConfig.x_offset = 0;
	sFramebufferConfig.y_offset = 0;
	sFramebufferConfig.frame_buffer_address = 0; // from GPU
	sFramebufferConfig.screen_size = 0; // from GPU

	if (depth < 16) {
		const int colorMapEntries = sizeof(sFramebufferConfig.color_map)
			/ sizeof(sFramebufferConfig.color_map[0]);
		for (int i = 0; i < colorMapEntries; i++)
			sFramebufferConfig.color_map[i] = 0x1111 * i;
	}

	status_t result = fMailbox->Write(ARM_MAILBOX_CHANNEL_FRAMEBUFFER,
		(uint32)&sFramebufferConfig | BCM283X_VIDEO_CORE_L2_COHERENT);
	if (result != B_OK)
		return result;

	uint32 value;
	result = fMailbox->Read(ARM_MAILBOX_CHANNEL_FRAMEBUFFER, value);
	if (result != B_OK)
		return result;

	if (value != 0) {
		dprintf("failed to configure framebuffer: %" B_PRIx32 "\n", value);
		return B_ERROR;
	}

	if (sFramebufferConfig.frame_buffer_address == 0) {
		dprintf("didn't get the framebuffer address\n");
		return B_ERROR;
	}

	//debug_assert(sFramebufferConfig.x_offset == 0
	//	&& sFramebufferConfig.y_offset == 0
	//	&& sFramebufferConfig.width == (uint32)width
	//	&& sFramebufferConfig.height == (uint32)height
	//	&& sFramebufferConfig.virtual_width == sFramebufferConfig.width
	//	&& sFramebufferConfig.virtual_height == sFramebufferConfig.height
	//	&& sFramebufferConfig.bits_per_pixel == (uint32)depth
	//	&& sFramebufferConfig.bytes_per_row
	//		>= sFramebufferConfig.bits_per_pixel / 8
	//			* sFramebufferConfig.width
	//	&& sFramebufferConfig.screen_size >= sFramebufferConfig.bytes_per_row
	//		* sFramebufferConfig.height);

	fPhysicalBase
		= BCM283X_BUS_TO_PHYSICAL(sFramebufferConfig.frame_buffer_address);
	fSize = sFramebufferConfig.screen_size;

	fBase = (addr_t)mmu_map_physical_memory(fPhysicalBase, fSize, kDefaultPageFlags);

	dprintf("video framebuffer: va: %p pa: %p\n", (void *)fBase,
		(void *)fPhysicalBase);

	fCurrentWidth = width;
	fCurrentHeight = height;
	fCurrentDepth = depth;
	fCurrentBytesPerRow = sFramebufferConfig.bytes_per_row;

	gKernelArgs.frame_buffer.physical_buffer.start = (addr_t)fPhysicalBase;

	return B_OK;
}

