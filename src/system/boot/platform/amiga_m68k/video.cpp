/*
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "rom_calls.h"
#include "video.h"
//#include "mmu.h"
//#include "images.h"

#include <arch/cpu.h>
#include <boot/stage2.h>
#include <boot/platform.h>
#include <boot/menu.h>
#include <boot/kernel_args.h>
#include <util/list.h>
#include <drivers/driver_settings.h>
#include <GraphicsDefs.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//#define TRACE_VIDEO
#ifdef TRACE_VIDEO
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif



//	#pragma mark -


bool
video_mode_hook(Menu *menu, MenuItem *item)
{
	// nothing yet
	return true;
}


Menu *
video_mode_menu()
{
	return NULL;
}


//	#pragma mark -


extern "C" void
platform_switch_to_logo(void)
{
	// TODO: implement me
}


extern "C" void
platform_switch_to_text_mode(void)
{
	// TODO: implement me
}


extern "C" status_t
platform_init_video(void)
{
	// TODO: implement me
	return B_OK;
}

