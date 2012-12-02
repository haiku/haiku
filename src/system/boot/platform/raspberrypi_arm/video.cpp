/*
 * Copyright 2009 Jonas Sundström, jonas@kirilla.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <SupportDefs.h>

#include <boot/kernel_args.h>
#include <boot/stage2.h>

#include "arch_framebuffer.h"

#include "blue_screen.h"
#include "frame_buffer_console.h"

ArchFramebuffer *gFramebuffer = NULL;
static bool sOnScreenDebugOutputAvailable = false;


extern "C" void
platform_video_puts(const char* string)
{
	if (sOnScreenDebugOutputAvailable)
		blue_screen_puts(string);
}


extern "C" void
platform_set_palette(const uint8* palette)
{
#warning IMPLEMENT platform_set_palette
}


extern "C" void
platform_switch_to_logo(void)
{
#warning IMPLEMENT platform_switch_to_logo
}


extern "C" void
platform_switch_to_text_mode(void)
{
#warning IMPLEMENT platform_switch_to_text_mode
}


extern "C" status_t
platform_init_video(void)
{
	extern ArchFramebuffer* arch_get_framebuffer_arm_bcm2708();
	gFramebuffer = arch_get_framebuffer_arm_bcm2708();

	if (gFramebuffer == NULL)
		return B_ERROR;

	status_t result = gFramebuffer->Probe();
	if (result != B_OK)
		return result;

	result = gFramebuffer->Init();
	if (result != B_OK)
		return result;

	result = gFramebuffer->SetDefaultMode();
	if (result != B_OK)
		return result;

	result = frame_buffer_update(gFramebuffer->Base(), gFramebuffer->Width(),
		gFramebuffer->Height(), gFramebuffer->Depth(),
		gFramebuffer->BytesPerRow());
	if (result != B_OK)
		return result;

	result = blue_screen_init();
	if (result != B_OK)
		return result;

	result = blue_screen_enter(false);
	if (result != B_OK)
		return result;

	blue_screen_puts("Welcome to early on-screen debug output on rPi!\n");
	sOnScreenDebugOutputAvailable = true;

	gKernelArgs.frame_buffer.physical_buffer.start
		= gFramebuffer->PhysicalBase();
	gKernelArgs.frame_buffer.physical_buffer.size = gFramebuffer->Size();
	gKernelArgs.frame_buffer.bytes_per_row = gFramebuffer->BytesPerRow();
	gKernelArgs.frame_buffer.width = gFramebuffer->Width();
	gKernelArgs.frame_buffer.height = gFramebuffer->Height();
	gKernelArgs.frame_buffer.depth = gFramebuffer->Depth();
	gKernelArgs.frame_buffer.enabled = true;
	return B_OK;
}

