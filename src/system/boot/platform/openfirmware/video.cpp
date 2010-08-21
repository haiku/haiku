/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2010 Andreas Färber <andreas.faerber@web.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <boot/platform.h>
#include <boot/stage2.h>


extern "C" void
platform_switch_to_logo(void)
{
	// in debug mode, we'll never show the logo
	if ((platform_boot_options() & BOOT_OPTION_DEBUG_OUTPUT) != 0)
		return;

	// ToDo: implement me
}


extern "C" void
platform_switch_to_text_mode(void)
{
	// nothing to do if we're in text mode
	if (!gKernelArgs.frame_buffer.enabled)
		return;

	// ToDo: implement me

	gKernelArgs.frame_buffer.enabled = false;
}


extern "C" status_t
platform_init_video(void)
{
	gKernelArgs.frame_buffer.enabled = false;

	// ToDo: implement me
	return B_OK;
}

