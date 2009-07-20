/*
 * Copyright 2005-2009 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * PS/2 hid device driver
 *
 * Authors (in chronological order):
 *		Marcus Overhagen (marcus@overhagen.de)
 */


#include <KernelExport.h>
#include <Drivers.h>

#include "PS2.h"

#define TRACE(x) dprintf x


int32 api_version = B_CUR_DRIVER_API_VERSION;

ps2_module_info *gPs2 = NULL;


status_t
init_hardware(void)
{
	TRACE(("ps2_hid: init_hardware\n"));
	return B_OK;
}


const char **
publish_devices(void)
{
	TRACE(("ps2_hid: publish_devices\n"));
	return NULL;
}


device_hooks *
find_device(const char *name)
{
	TRACE(("ps2_hid: find_device\n"));
	return NULL;
}


status_t
init_driver(void)
{
	TRACE(("ps2_hid: init_driver\n"));
	return get_module(B_PS2_MODULE_NAME, (module_info **)&gPs2);
}


void
uninit_driver(void)
{
	TRACE(("ps2_hid: uninit_driver\n"));
	put_module(B_PS2_MODULE_NAME);
}
