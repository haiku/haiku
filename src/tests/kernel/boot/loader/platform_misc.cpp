/*
 * Copyright 2003-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <boot/platform.h>


extern bool gShowMenu;


uint32
platform_boot_options(void)
{
	return gShowMenu ? BOOT_OPTION_MENU: 0;
}


status_t
platform_init_video(void)
{
	return B_OK;
}


void
platform_switch_to_logo(void)
{
}


void
platform_switch_to_text_mode(void)
{
}

