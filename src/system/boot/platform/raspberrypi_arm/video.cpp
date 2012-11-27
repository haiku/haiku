/*
 * Copyright 2009 Jonas Sundström, jonas@kirilla.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <SupportDefs.h>

#include "arch_framebuffer.h"

ArchFramebuffer *gFramebuffer = NULL;


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

	return gFramebuffer->SetDefaultMode();
}

