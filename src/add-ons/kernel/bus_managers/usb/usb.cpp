//------------------------------------------------------------------------------
//	Copyright (c) 2003-2004, Niels S. Reedijk
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

#include <USB.h>
#include <util/kernel_cpp.h>

#include "usb_p.h"

#define USB_DEBUG
#ifdef USB_DEBUG
#define TRACE dprintf
#else
#define TRACE silent
void silent( const char * , ... ) {}
#endif

/* ++++++++++
Loading/unloading the module
++++++++++ */

static int32
bus_std_ops(int32 op, ...)
{
	switch(op) {
		case B_MODULE_INIT:
			TRACE(("usb_nielx: bus module: init\n"));
			data = new Stack();
			if( data->InitCheck() != B_OK )
			{
				delete data;
				return ENODEV;
			}
			break;
		case B_MODULE_UNINIT:
			TRACE(("usb_nielx: bus module: uninit\n"));
			delete data;
			break;
		default:
			return EINVAL;
	}
	return B_OK;
}


/* ++++++++++
This module exports the USB API 
++++++++++ */

struct usb_module_info m_module_info =
{
	// First the bus_manager_info:
	{
		//module_info
		{
			"bus_managers/usb/nielx" ,
			B_KEEP_LOADED ,				// Keep loaded, even if no driver requires it
			bus_std_ops
		} ,
		NULL 							// the rescan function
	} ,
	NULL ,								// register_driver
	NULL ,								// install_notify
	NULL ,								// uninstall_notify
	NULL ,								// get_device_descriptor
	NULL ,								// get_nth_configuration_info
	NULL ,								// get_configuration
	NULL ,								// set_configuration
	NULL ,								// set_alt_interface
	NULL , 								// set_feature
	NULL , 								// clear_feature
	NULL , 								// get_status
	NULL ,								// get_descriptor
	NULL ,								// send_request
	NULL ,								// queue_interrupt
	NULL ,								// queue_bulk
	NULL ,								// queue_isochronous
	NULL ,								// queue_request
	NULL ,								// set_pipe_policy
	NULL ,								// cancel_queued_transfers
	NULL 								// usb_ioctl
};

module_info *modules[] = {
	(module_info *)&m_module_info ,
	NULL
};
