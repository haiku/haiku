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


Stack *gUSBStack = NULL;


static int32
bus_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT: {
#ifdef TRACE_USB
			set_dprintf_enabled(true);
			load_driver_symbols("usb");
#endif
			TRACE(("usb_module: init\n"));

			Stack *stack = new(std::nothrow) Stack();
			if (!stack)
				return B_NO_MEMORY;

			if (stack->InitCheck() != B_OK) {
				delete stack;
				return ENODEV;
			}

			gUSBStack = stack;
			break;
		}

		case B_MODULE_UNINIT:
			TRACE(("usb_module: bus module: uninit\n"));
			delete gUSBStack;
			gUSBStack = NULL;
			break;

		default:
			return EINVAL;
	}

	return B_OK;
}


status_t
register_driver(const char *driverName,
	const usb_support_descriptor *descriptors,
	size_t count, const char *optionalRepublishDriverName)
{
	return gUSBStack->RegisterDriver(driverName, descriptors, count,
		optionalRepublishDriverName);
}


status_t
install_notify(const char *driverName, const usb_notify_hooks *hooks)
{
	return gUSBStack->InstallNotify(driverName, hooks);
}


status_t
uninstall_notify(const char *driverName)
{
	return gUSBStack->UninstallNotify(driverName);
}


const usb_device_descriptor *
get_device_descriptor(const usb_device *device)
{
	TRACE(("usb_module: get_device_descriptor(0x%08x)\n", device));
	if (!device)
		return NULL;

	return ((const Device *)device)->DeviceDescriptor();
}


const usb_configuration_info *
get_nth_configuration(const usb_device *device, uint index)
{
	TRACE(("usb_module: get_nth_configuration(0x%08x, %d)\n", device, index));
	if (!device)
		return NULL;

	return ((const Device *)device)->ConfigurationAt((int32)index);
}



const usb_configuration_info *
get_configuration(const usb_device *device)
{
	TRACE(("usb_module: get_configuration(0x%08x)\n", device));
	if (!device)
		return NULL;

	return ((const Device *)device)->Configuration();
}


status_t
set_configuration(const usb_device *device,
	const usb_configuration_info *configuration)
{
	TRACE(("usb_module: set_configuration(0x%08x, 0x%08x)\n", device, configuration));
	if (!device)
		return B_BAD_VALUE;

	return ((Device *)device)->SetConfiguration(configuration);
}


status_t
set_alt_interface(const usb_device *device,
	const usb_interface_info *interface)
{
	TRACE(("usb_module: set_alt_interface(0x%08x, 0x%08x)\n", device, interface));
	return B_ERROR;
}


status_t
set_feature(const void *object, uint16 selector)
{
	TRACE(("usb_module: set_feature(0x%08x, %d)\n", object, selector));
	if (!object)
		return B_BAD_VALUE;

	return ((Pipe *)object)->SetFeature(selector);
}


status_t
clear_feature(const void *object, uint16 selector)
{
	TRACE(("usb_module: clear_feature(0x%08x, %d)\n", object, selector));
	if (!object)
		return B_BAD_VALUE;

	return ((Pipe *)object)->ClearFeature(selector);
}


status_t
get_status(const void *object, uint16 *status)
{
	TRACE(("usb_module: get_status(0x%08x, 0x%08x)\n", object, status));
	if (!object || !status)
		return B_BAD_VALUE;

	return ((Pipe *)object)->GetStatus(status);
}


status_t
get_descriptor(const usb_device *device, uint8 type, uint8 index,
	uint16 languageID, void *data, size_t dataLength, size_t *actualLength)
{
	TRACE(("usb_module: get_descriptor(0x%08x, 0x%02x, 0x%02x, 0x%04x, 0x%08x, %d, 0x%08x)\n", device, type, index, languageID, data, dataLength, actualLength));
	if (!device || !data)
		return B_BAD_VALUE;

	return ((Device *)device)->GetDescriptor(type, index, languageID,
		data, dataLength, actualLength);
}


status_t
send_request(const usb_device *device, uint8 requestType, uint8 request,
	uint16 value, uint16 index, uint16 length, void *data, size_t dataLength,
	size_t *actualLength)
{
	TRACE(("usb_module: send_request(0x%08x, 0x%02x, 0x%02x, 0x%04x, 0x%04x, %d, 0x%08x, %d, 0x%08x)\n", device, requestType, request, value, index, length, data, dataLength, actualLength));
	if (!device)
		return B_BAD_VALUE;

	return ((Device *)device)->SendRequest(requestType, request, value, index,
		length, data, dataLength, actualLength);
}


status_t
queue_request(const usb_device *device, uint8 requestType, uint8 request,
	uint16 value, uint16 index, uint16 length, void *data, size_t dataLength,
	usb_callback_func callback, void *callbackCookie)
{
	TRACE(("usb_module: queue_request(0x%08x, 0x%02x, 0x%02x, 0x%04x, 0x%04x, %d, 0x%08x, %d, 0x%08x, 0x%08x)\n", device, requestType, request, value, index, length, data, dataLength, callback, callbackCookie));
	if (!device)
		return B_BAD_VALUE;

	return ((Device *)device)->QueueRequest(requestType, request, value, index,
		length, data, dataLength, callback, callbackCookie);
}


status_t
queue_interrupt(const usb_pipe *pipe, void *data, size_t dataLength,
	usb_callback_func callback, void *callbackCookie)
{
	TRACE(("usb_module: queue_interrupt(0x%08x, 0x%08x, %d, 0x%08x, 0x%08x)\n", pipe, data, dataLength, callback, callbackCookie));
	if (((Pipe *)pipe)->Type() != Pipe::Interrupt)
		return B_BAD_VALUE;

	return ((InterruptPipe *)pipe)->QueueInterrupt(data, dataLength, callback,
		callbackCookie);
}


status_t
queue_bulk(const usb_pipe *pipe, void *data, size_t dataLength,
	usb_callback_func callback, void *callbackCookie)
{
	TRACE(("usb_module: queue_bulk(0x%08x, 0x%08x, %d, 0x%08x, 0x%08x)\n", pipe, data, dataLength, callback, callbackCookie));
	if (((Pipe *)pipe)->Type() != Pipe::Bulk)
		return B_BAD_VALUE;

	return ((BulkPipe *)pipe)->QueueBulk(data, dataLength, callback,
		callbackCookie);
}


status_t
queue_isochronous(const usb_pipe *pipe, void *data, size_t dataLength,
	rlea *rleArray, uint16 bufferDurationMS, usb_callback_func callback,
	void *callbackCookie)
{
	TRACE(("usb_module: queue_isochronous(0x%08x, 0x%08x, %d, 0x%08x, %d, 0x%08x, 0x%08x)\n", pipe, data, dataLength, rleArray, bufferDurationMS, callback, callbackCookie));
	if (((Pipe *)pipe)->Type() != Pipe::Isochronous)
		return B_BAD_VALUE;

	return ((IsochronousPipe *)pipe)->QueueIsochronous(data, dataLength,
		rleArray, bufferDurationMS, callback, callbackCookie);
}


status_t
set_pipe_policy(const usb_pipe *pipe, uint8 maxQueuedPackets,
	uint16 maxBufferDurationMS, uint16 sampleSize)
{
	TRACE(("usb_module: set_pipe_policy(0x%08x, %d, %d, %d)\n", pipe, maxQueuedPackets, maxBufferDurationMS, sampleSize));
	return B_ERROR;
}


status_t
cancel_queued_transfers(const usb_pipe *pipe)
{
	TRACE(("usb_module: cancel_queued_transfers(0x%08x)\n", pipe));
	return ((Pipe *)pipe)->CancelQueuedTransfers();
}


status_t
usb_ioctl(uint32 opcode, void *buffer, size_t bufferSize)
{
	TRACE(("usb_module: usb_ioctl(0x%08x, 0x%08x, %d)\n", opcode, buffer, bufferSize));
	return B_ERROR;
}


/*
	This module exports the USB API 
*/
struct usb_module_info gModuleInfo = {
	// First the bus_manager_info:
	{
		{
			B_USB_MODULE_NAME,
			B_KEEP_LOADED,				// Keep loaded, even if no driver requires it
			bus_std_ops
		},
		NULL							// the rescan function
	},

	register_driver,					// register_driver
	install_notify,						// install_notify
	uninstall_notify,					// uninstall_notify
	get_device_descriptor,				// get_device_descriptor
	get_nth_configuration,				// get_nth_configuration
	get_configuration,					// get_configuration
	set_configuration,					// set_configuration
	set_alt_interface,					// set_alt_interface
	set_feature,						// set_feature
	clear_feature, 						// clear_feature
	get_status, 						// get_status
	get_descriptor,						// get_descriptor
	send_request,						// send_request
	queue_interrupt,					// queue_interrupt
	queue_bulk,							// queue_bulk
	queue_isochronous,					// queue_isochronous
	queue_request,						// queue_request
	set_pipe_policy,					// set_pipe_policy
	cancel_queued_transfers,			// cancel_queued_transfers
	usb_ioctl							// usb_ioctl
};


module_info *modules[] = {
	(module_info *)&gModuleInfo,
	NULL
};
