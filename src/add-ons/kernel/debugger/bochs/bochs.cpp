/*
 * Copyright 2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol, revol@free.fr
 */


#include <debug.h>
#include <driver_settings.h>
#include <ISA.h>

static isa_module_info *sISAModule;


static int
debugger_puts(const char *s, int32 length)
{
	int i;
	for (i = 0; i < length; i++)
		sISAModule->write_io_8(0xe9, s[i]);
	return i;
}


static status_t
std_ops(int32 op, ...)
{
	void *handle;
	bool load = false;

	switch (op) {
	case B_MODULE_INIT:
		handle = load_driver_settings("kernel");
		if (handle) {
			load = get_driver_boolean_parameter(handle,
				"bochs_debug_output", load, true);
			unload_driver_settings(handle);
		}
		if (load) {
			if (get_module(B_ISA_MODULE_NAME, (module_info **)&sISAModule) < B_OK)
				return B_ERROR;
		}
		return load ? B_OK : B_ERROR;
	case B_MODULE_UNINIT:
		put_module(B_ISA_MODULE_NAME);
		return B_OK;
	}
	return B_BAD_VALUE;
}


static struct debugger_module_info sModuleInfo = {
	{
		"debugger/bochs/v1",
		0,
		&std_ops
	},
	NULL,
	NULL,
	debugger_puts,
	NULL
};

module_info *modules[] = { 
	(module_info *)&sModuleInfo,
	NULL
};

