//------------------------------------------------------------------------------
//	Copyright (c) 2003-2004, Niels S. Reedijk,
//                           Rudolf Cornelissen
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.


#include "agp.h"

//void function for debug messages if logging disabled
void silent(const char *, ... ) {}


/* ++++++++++
Loading/unloading the module
++++++++++ */

static int32
bus_std_ops(int32 op, ...)
{
	switch(op) {
		case B_MODULE_INIT:
			TRACE(("agp_man: bus module: init\n"));
			if (init() != B_OK)
			{
				return ENODEV;
			}
			break;
		case B_MODULE_UNINIT:
			TRACE(("agp_man: bus module: uninit\n"));
			uninit();
			break;
		default:
			return EINVAL;
	}
	return B_OK;
}


/* ++++++++++
This module exports the AGP API 
++++++++++ */

struct agp_module_info m_module_info =
{
	// First the bus_manager_info:
	{
		//module_info
		{
			B_AGP_MODULE_NAME ,
			B_KEEP_LOADED ,				// Keep loaded, even if no driver requires it
			bus_std_ops
		} ,
		NULL 							// the rescan function
	} ,
	//my functions
	&get_nth_agp_info,
	&enable_agp,
};

module_info *modules[] = {
	(module_info *)&m_module_info ,
	NULL
};
