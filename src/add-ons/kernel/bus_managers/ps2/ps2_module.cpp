/*
 * Copyright 2005-2007 Marcus Overhagen
 * Distributed under the terms of the MIT License.
 *
 * PS/2 bus manager
 *
 * Authors (in chronological order):
 *		Marcus Overhagen (marcus@overhagen.de)
 */


#include "PS2.h"
#include "ps2_common.h"


static int32
function1()
{
	return 0;
}


static int32
function2()
{
	return 0;
}


static status_t
std_ops(int32 op, ...)
{
	switch(op) {
		case B_MODULE_INIT:
			return ps2_init();

		case B_MODULE_UNINIT:
			ps2_uninit();
			break;
		default:
			return B_ERROR;
	}
	return B_OK;
}


static ps2_module_info ps2_module = {
	{
		{
			B_PS2_MODULE_NAME,
			B_KEEP_LOADED,
			std_ops
		},
		NULL
	},
	&function1,
	&function2,
};


module_info *modules[] = {
	(module_info *)&ps2_module,
	NULL
};
