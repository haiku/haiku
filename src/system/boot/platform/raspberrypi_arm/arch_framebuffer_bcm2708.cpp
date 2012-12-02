/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz, mmlr@mlotz.ch
 */

#include <string.h>

#include <arm_mmu.h>

#include "arch_mmu.h"

#include "arch_framebuffer.h"

#include "bcm2708.h"
#include "mailbox.h"
#include "platform_debug.h"


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


class ArchFramebufferBCM2708 : public ArchFramebuffer {
public:
							ArchFramebufferBCM2708();
virtual						~ArchFramebufferBCM2708();

virtual	status_t			Init();
virtual	status_t			Probe();
virtual	status_t			SetDefaultMode();
virtual	status_t			SetVideoMode(int width, int height, int depth);
};


static ArchFramebufferBCM2708 sArchFramebuffer;


extern "C" ArchFramebuffer*
arch_get_framebuffer_arm_bcm2708()
{
	return &sArchFramebuffer;
}


ArchFramebufferBCM2708::ArchFramebufferBCM2708()
	:	ArchFramebuffer(0)
{
}


ArchFramebufferBCM2708::~ArchFramebufferBCM2708()
{
}


status_t
ArchFramebufferBCM2708::Init()
{
	return B_OK;
}


status_t
ArchFramebufferBCM2708::Probe()
{
	return B_OK;
}


status_t
ArchFramebufferBCM2708::SetDefaultMode()
{
	status_t result;
	do {
		result = SetVideoMode(1920, 1080, 16);
	} while (result != B_OK);

	return B_OK;
}


status_t
ArchFramebufferBCM2708::SetVideoMode(int width, int height, int depth)
{
	debug_assert(((uint32)&sFramebufferConfig & 0x0f) == 0);

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

	status_t result = write_mailbox(ARM_MAILBOX_CHANNEL_FRAMEBUFFER,
		(uint32)&sFramebufferConfig | BCM2708_VIDEO_CORE_L2_COHERENT);
	if (result != B_OK)
		return result;

	uint32 value;
	result = read_mailbox(ARM_MAILBOX_CHANNEL_FRAMEBUFFER, value);
	if (result != B_OK)
		return result;

	if (value != 0) {
		dprintf("failed to configure framebuffer: %" B_PRIx32 "\n", value);
		debug_toggle_led(5, DEBUG_DELAY_SHORT);
		return B_ERROR;
	}

	if (sFramebufferConfig.frame_buffer_address == 0) {
		dprintf("didn't get the framebuffer address\n");
		debug_toggle_led(10, DEBUG_DELAY_SHORT);
		return B_ERROR;
	}

	debug_assert(sFramebufferConfig.x_offset == 0
		&& sFramebufferConfig.y_offset == 0
		&& sFramebufferConfig.width == (uint32)width
		&& sFramebufferConfig.height == (uint32)height
		&& sFramebufferConfig.virtual_width == sFramebufferConfig.width
		&& sFramebufferConfig.virtual_height == sFramebufferConfig.height
		&& sFramebufferConfig.bits_per_pixel == (uint32)depth
		&& sFramebufferConfig.bytes_per_row
			>= sFramebufferConfig.bits_per_pixel / 8
				* sFramebufferConfig.width
		&& sFramebufferConfig.screen_size >= sFramebufferConfig.bytes_per_row
			* sFramebufferConfig.height);

	fPhysicalBase
		= BCM2708_BUS_TO_PHYSICAL(sFramebufferConfig.frame_buffer_address);
	fSize = sFramebufferConfig.screen_size;

	fBase = (addr_t)mmu_map_physical_memory(fPhysicalBase, fSize,
		kDefaultPageFlags);

	fCurrentWidth = width;
	fCurrentHeight = height;
	fCurrentDepth = depth;
	fCurrentBytesPerRow = sFramebufferConfig.bytes_per_row;
	return B_OK;
}

