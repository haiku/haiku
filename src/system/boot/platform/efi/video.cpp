/*
 * Copyright 2016, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <boot/platform.h>
#include <boot/platform/generic/video.h>

#include "efi_platform.h"


extern "C" status_t
platform_init_video(void)
{
	return B_ERROR;
}


extern "C" void
platform_switch_to_logo(void)
{
}


extern "C" void
platform_blit4(addr_t frameBuffer, const uint8 *data,
	uint16 width, uint16 height, uint16 imageWidth,
	uint16 left, uint16 top)
{
	panic("platform_blit4 unsupported");
	return;
}


extern "C" void
platform_set_palette(const uint8 *palette)
{
	panic("platform_set_palette unsupported");
	return;
}
