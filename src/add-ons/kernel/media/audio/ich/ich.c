/*
 * BeOS AC97 host interface driver for Intel Host Controllers (ICH)
 *
 * Implemented using the audio_module API
 *
 * Copyright (c) 2003, Marcus Overhagen <marcus@overhagen.de>
 *
 * All rights reserved.
 * Redistribution only allowed under the terms of the MIT license.
 *
 */

#include <OS.h>
#include <KernelExport.h>
#include "audio_module.h"


void print_hello_world(void)
{
	dprintf("print_hello_world\n");
}


// std_ops
static
status_t
std_ops(int32 op, ...)
{
	dprintf("ich: std_ops(0x%lx)\n", op);
	switch(op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;
	}
	return B_ERROR;
}

static
void
print_hello(void)
{
	dprintf("print_hello\n");
}

static audio_module_info audio_driver_module = 
{
	// module_info
	{
		"media/audio/ich",
		0,
		std_ops
	},
	print_hello
};

_EXPORT audio_module_info *modules[] =
{
	&audio_driver_module,
	NULL
};

