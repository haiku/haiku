/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de
 * Distributed under the terms of the MIT License.
 */


#include <debug.h>


static void
enter_debugger(void)
{
	evaluate_debug_command("sc");
}


static status_t
std_ops(int32 op, ...)
{
	if (op == B_MODULE_INIT || op == B_MODULE_UNINIT)
		return B_OK;

	return B_BAD_VALUE;
}


static struct debugger_module_info sModuleInfo = {
	{
		"debugger/auto_stack_trace/v1",
		0,
		&std_ops
	},
	enter_debugger,
	NULL,
	NULL,
	NULL
};

module_info *modules[] = { 
	(module_info *)&sModuleInfo,
	NULL
};

