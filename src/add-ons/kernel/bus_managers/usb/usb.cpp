/*
 * Copyright 2003-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels S. Reedijk
 */

#include <USB.h>
#include <util/kernel_cpp.h>
#include "usb_p.h"


#define TRACE_USB
#ifdef TRACE_USB
#define TRACE(x)	dprintf x
#else
#define TRACE(x)	/* nothing */
#endif


static int32
bus_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT: {
#ifdef TRACE_USB
			set_dprintf_enabled(true);
			load_driver_symbols("usb");
#endif
			TRACE(("usb_nielx: bus module: init\n"));

			Stack *stack = new Stack();
			if (stack->InitCheck() != B_OK) {
				delete stack;
				return ENODEV;
			}

			break;
		}

		case B_MODULE_UNINIT:
			TRACE(("usb_nielx: bus module: uninit\n"));
			delete usb_stack;
			break;

		default:
			return EINVAL;
	}

	return B_OK;
}


/*
	This module exports the USB API 
*/
struct usb_module_info gModuleInfo = {
	// First the bus_manager_info:
	{
		{
			"bus_managers/usb/nielx",
			B_KEEP_LOADED,				// Keep loaded, even if no driver requires it
			bus_std_ops
		},
		NULL 							// the rescan function
	},

	NULL,								// register_driver
	NULL,								// install_notify
	NULL,								// uninstall_notify
	NULL,								// get_device_descriptor
	NULL,								// get_nth_configuration_info
	NULL,								// get_configuration
	NULL,								// set_configuration
	NULL,								// set_alt_interface
	NULL, 								// set_feature
	NULL, 								// clear_feature
	NULL, 								// get_status
	NULL,								// get_descriptor
	NULL,								// send_request
	NULL,								// queue_interrupt
	NULL,								// queue_bulk
	NULL,								// queue_isochronous
	NULL,								// queue_request
	NULL,								// set_pipe_policy
	NULL,								// cancel_queued_transfers
	NULL 								// usb_ioctl
};


module_info *modules[] = {
	(module_info *)&gModuleInfo,
	NULL
};
