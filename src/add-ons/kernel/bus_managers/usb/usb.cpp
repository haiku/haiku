/*
 * Copyright 2003-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		Niels S. Reedijk
 */

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
			if (gUSBStack)
				return B_OK;

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
get_device_descriptor(usb_device device)
{
	TRACE(("usb_module: get_device_descriptor(0x%08x)\n", device));
	Object *object = gUSBStack->GetObject(device);
	if (!object || (object->Type() & USB_OBJECT_DEVICE) == 0)
		return NULL;

	return ((Device *)object)->DeviceDescriptor();
}


const usb_configuration_info *
get_nth_configuration(usb_device device, uint index)
{
	TRACE(("usb_module: get_nth_configuration(0x%08x, %d)\n", device, index));
	Object *object = gUSBStack->GetObject(device);
	if (!object || (object->Type() & USB_OBJECT_DEVICE) == 0)
		return NULL;

	return ((Device *)object)->ConfigurationAt((int32)index);
}


const usb_configuration_info *
get_configuration(usb_device device)
{
	TRACE(("usb_module: get_configuration(0x%08x)\n", device));
	Object *object = gUSBStack->GetObject(device);
	if (!object || (object->Type() & USB_OBJECT_DEVICE) == 0)
		return NULL;

	return ((Device *)object)->Configuration();
}


status_t
set_configuration(usb_device device,
	const usb_configuration_info *configuration)
{
	TRACE(("usb_module: set_configuration(0x%08x, 0x%08x)\n", device, configuration));
	Object *object = gUSBStack->GetObject(device);
	if (!object || (object->Type() & USB_OBJECT_DEVICE) == 0)
		return B_BAD_VALUE;

	return ((Device *)object)->SetConfiguration(configuration);
}


status_t
set_alt_interface(usb_device device, const usb_interface_info *interface)
{
	TRACE(("usb_module: set_alt_interface(0x%08x, 0x%08x)\n", device, interface));
	Object *object = gUSBStack->GetObject(device);
	if (!object || (object->Type() & USB_OBJECT_DEVICE) == 0)
		return B_BAD_VALUE;

	return B_ERROR;
}


status_t
set_feature(usb_id handle, uint16 selector)
{
	TRACE(("usb_module: set_feature(0x%08x, %d)\n", handle, selector));
	Object *object = gUSBStack->GetObject(handle);
	if (!object || (object->Type() & USB_OBJECT_PIPE) == 0)
		return B_BAD_VALUE;

	return ((Pipe *)object)->SetFeature(selector);
}


status_t
clear_feature(usb_id handle, uint16 selector)
{
	TRACE(("usb_module: clear_feature(0x%08x, %d)\n", handle, selector));
	Object *object = gUSBStack->GetObject(handle);
	if (!object || (object->Type() & USB_OBJECT_PIPE) == 0)
		return B_BAD_VALUE;

	return ((Pipe *)object)->ClearFeature(selector);
}


status_t
get_status(usb_id handle, uint16 *status)
{
	TRACE(("usb_module: get_status(0x%08x, 0x%08x)\n", handle, status));
	if (!status)
		return B_BAD_VALUE;

	Object *object = gUSBStack->GetObject(handle);
	if (!object || (object->Type() & USB_OBJECT_PIPE) == 0)
		return B_BAD_VALUE;

	return ((Pipe *)object)->GetStatus(status);
}


status_t
get_descriptor(usb_device device, uint8 type, uint8 index, uint16 languageID,
	void *data, size_t dataLength, size_t *actualLength)
{
	TRACE(("usb_module: get_descriptor(0x%08x, 0x%02x, 0x%02x, 0x%04x, 0x%08x, %d, 0x%08x)\n", device, type, index, languageID, data, dataLength, actualLength));
	Object *object = gUSBStack->GetObject(device);
	if (!object || (object->Type() & USB_OBJECT_DEVICE) == 0)
		return B_BAD_VALUE;

	return ((Device *)object)->GetDescriptor(type, index, languageID,
		data, dataLength, actualLength);
}


status_t
send_request(usb_device device, uint8 requestType, uint8 request,
	uint16 value, uint16 index, uint16 length, void *data, size_t *actualLength)
{
	TRACE(("usb_module: send_request(0x%08x, 0x%02x, 0x%02x, 0x%04x, 0x%04x, %d, 0x%08x, 0x%08x)\n", device, requestType, request, value, index, length, data, actualLength));
	Object *object = gUSBStack->GetObject(device);
	if (!object || (object->Type() & USB_OBJECT_DEVICE) == 0)
		return B_BAD_VALUE;

	return ((Device *)object)->SendRequest(requestType, request, value, index,
		length, data, length, actualLength);
}


status_t
queue_request(usb_device device, uint8 requestType, uint8 request,
	uint16 value, uint16 index, uint16 length, void *data,
	usb_callback_func callback, void *callbackCookie)
{
	TRACE(("usb_module: queue_request(0x%08x, 0x%02x, 0x%02x, 0x%04x, 0x%04x, %d, 0x%08x, 0x%08x, 0x%08x)\n", device, requestType, request, value, index, length, data, callback, callbackCookie));
	Object *object = gUSBStack->GetObject(device);
	if (!object || (object->Type() & USB_OBJECT_DEVICE) == 0)
		return B_BAD_VALUE;

	return ((Device *)object)->QueueRequest(requestType, request, value, index,
		length, data, length, callback, callbackCookie);
}


status_t
queue_interrupt(usb_pipe pipe, void *data, size_t dataLength,
	usb_callback_func callback, void *callbackCookie)
{
	TRACE(("usb_module: queue_interrupt(0x%08x, 0x%08x, %d, 0x%08x, 0x%08x)\n", pipe, data, dataLength, callback, callbackCookie));
	Object *object = gUSBStack->GetObject(pipe);
	if (!object || (object->Type() & USB_OBJECT_INTERRUPT_PIPE) == 0)
		return B_BAD_VALUE;

	return ((InterruptPipe *)object)->QueueInterrupt(data, dataLength, callback,
		callbackCookie);
}


status_t
queue_bulk(usb_pipe pipe, void *data, size_t dataLength,
	usb_callback_func callback, void *callbackCookie)
{
	TRACE(("usb_module: queue_bulk(0x%08x, 0x%08x, %d, 0x%08x, 0x%08x)\n", pipe, data, dataLength, callback, callbackCookie));
	Object *object = gUSBStack->GetObject(pipe);
	if (!object || (object->Type() & USB_OBJECT_BULK_PIPE) == 0)
		return B_BAD_VALUE;

	return ((BulkPipe *)object)->QueueBulk(data, dataLength, callback,
		callbackCookie);
}


status_t
queue_bulk_v(usb_pipe pipe, iovec *vector, size_t vectorCount,
	usb_callback_func callback, void *callbackCookie)
{
	TRACE(("usb_module: queue_bulk(0x%08x, 0x%08x, %d, 0x%08x, 0x%08x)\n", pipe, vector, vectorCount, callback, callbackCookie));
	Object *object = gUSBStack->GetObject(pipe);
	if (!object || (object->Type() & USB_OBJECT_BULK_PIPE) == 0)
		return B_BAD_VALUE;

	return B_ERROR;
}


status_t
queue_isochronous(usb_pipe pipe, void *data, size_t dataLength,
	rlea *rleArray, uint16 bufferDurationMS, usb_callback_func callback,
	void *callbackCookie)
{
	TRACE(("usb_module: queue_isochronous(0x%08x, 0x%08x, %d, 0x%08x, %d, 0x%08x, 0x%08x)\n", pipe, data, dataLength, rleArray, bufferDurationMS, callback, callbackCookie));
	Object *object = gUSBStack->GetObject(pipe);
	if (!object || (object->Type() & USB_OBJECT_ISO_PIPE) == 0)
		return B_BAD_VALUE;

	return ((IsochronousPipe *)object)->QueueIsochronous(data, dataLength,
		rleArray, bufferDurationMS, callback, callbackCookie);
}


status_t
set_pipe_policy(usb_pipe pipe, uint8 maxQueuedPackets,
	uint16 maxBufferDurationMS, uint16 sampleSize)
{
	TRACE(("usb_module: set_pipe_policy(0x%08x, %d, %d, %d)\n", pipe, maxQueuedPackets, maxBufferDurationMS, sampleSize));
	Object *object = gUSBStack->GetObject(pipe);
	if (!object || (object->Type() & USB_OBJECT_ISO_PIPE) == 0)
		return B_BAD_VALUE;

	return B_ERROR;
}


status_t
cancel_queued_transfers(usb_pipe pipe)
{
	TRACE(("usb_module: cancel_queued_transfers(0x%08x)\n", pipe));
	Object *object = gUSBStack->GetObject(pipe);
	if (!object || (object->Type() & USB_OBJECT_PIPE) == 0)
		return B_BAD_VALUE;

	return ((Pipe *)object)->CancelQueuedTransfers();
}


status_t
usb_ioctl(uint32 opcode, void *buffer, size_t bufferSize)
{
	TRACE(("usb_module: usb_ioctl(0x%08x, 0x%08x, %d)\n", opcode, buffer, bufferSize));

	switch (opcode) {
		case 'DNAM': {
			Object *object = gUSBStack->GetObject(*(usb_id *)buffer);
			if (!object || (object->Type() & USB_OBJECT_DEVICE) == 0)
				return B_BAD_VALUE;

			uint32 index = 0;
			return ((Device *)object)->BuildDeviceName((char *)buffer, &index,
				bufferSize, NULL);
		}
	}

	return B_DEV_INVALID_IOCTL;
}


/*
	This module exports the USB API v3
*/
struct usb_module_info gModuleInfoV3 = {
	// First the bus_manager_info:
	{
		{
			"bus_managers/usb/v3",
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
	queue_bulk_v,						// queue_bulk_v
	queue_isochronous,					// queue_isochronous
	queue_request,						// queue_request
	set_pipe_policy,					// set_pipe_policy
	cancel_queued_transfers,			// cancel_queued_transfers
	usb_ioctl							// usb_ioctl
};


//
// #pragma mark -
//


const usb_device_descriptor *
get_device_descriptor_v2(const void *device)
{
	return get_device_descriptor((usb_id)device);
}


const usb_configuration_info *
get_nth_configuration_v2(const void *device, uint index)
{
	return get_nth_configuration((usb_id)device, index);
}


const usb_configuration_info *
get_configuration_v2(const void *device)
{
	return get_configuration((usb_id)device);
}


status_t
set_configuration_v2(const void *device,
	const usb_configuration_info *configuration)
{
	return set_configuration((usb_id)device, configuration);
}


status_t
set_alt_interface_v2(const void *device, const usb_interface_info *interface)
{
	return set_alt_interface((usb_id)device, interface);
}


status_t
set_feature_v2(const void *object, uint16 selector)
{
	return set_feature((usb_id)object, selector);
}


status_t
clear_feature_v2(const void *object, uint16 selector)
{
	return clear_feature((usb_id)object, selector);
}


status_t
get_status_v2(const void *object, uint16 *status)
{
	return get_status((usb_id)object, status);
}


status_t
get_descriptor_v2(const void *device, uint8 type, uint8 index,
	uint16 languageID, void *data, size_t dataLength, size_t *actualLength)
{
	return get_descriptor((usb_id)device, type, index, languageID, data,
		dataLength, actualLength);
}


status_t
send_request_v2(const void *device, uint8 requestType, uint8 request,
	uint16 value, uint16 index, uint16 length, void *data,
	size_t /*dataLength*/, size_t *actualLength)
{
	return send_request((usb_id)device, requestType, request, value, index,
		length, data, actualLength);
}


status_t
queue_request_v2(const void *device, uint8 requestType, uint8 request,
	uint16 value, uint16 index, uint16 length, void *data,
	size_t /*dataLength*/, usb_callback_func callback, void *callbackCookie)
{
	return queue_request((usb_id)device, requestType, request, value, index,
		length, data, callback, callbackCookie);
}


status_t
queue_interrupt_v2(const void *pipe, void *data, size_t dataLength,
	usb_callback_func callback, void *callbackCookie)
{
	return queue_interrupt((usb_id)pipe, data, dataLength, callback,
		callbackCookie);
}


status_t
queue_bulk_v2(const void *pipe, void *data, size_t dataLength,
	usb_callback_func callback, void *callbackCookie)
{
	return queue_bulk((usb_id)pipe, data, dataLength, callback,
		callbackCookie);
}


status_t
queue_isochronous_v2(const void *pipe, void *data, size_t dataLength,
	rlea *rleArray, uint16 bufferDurationMS, usb_callback_func callback,
	void *callbackCookie)
{
	return queue_isochronous((usb_id)pipe, data, dataLength, rleArray,
		bufferDurationMS, callback, callbackCookie);
}


status_t
set_pipe_policy_v2(const void *pipe, uint8 maxQueuedPackets,
	uint16 maxBufferDurationMS, uint16 sampleSize)
{
	return set_pipe_policy((usb_id)pipe, maxQueuedPackets, maxBufferDurationMS,
		sampleSize);
}


status_t
cancel_queued_transfers_v2(const void *pipe)
{
	return cancel_queued_transfers((usb_id)pipe);
}


struct usb_module_info_v2 {
	bus_manager_info				binfo;
	status_t						(*register_driver)(const char *, const usb_support_descriptor *, size_t, const char *);
	status_t						(*install_notify)(const char *, const usb_notify_hooks *);
	status_t						(*uninstall_notify)(const char *);
	const usb_device_descriptor		*(*get_device_descriptor)(const void *);
	const usb_configuration_info	*(*get_nth_configuration)(const void *, uint);
	const usb_configuration_info	*(*get_configuration)(const void *);
	status_t						(*set_configuration)(const void *, const usb_configuration_info *);
	status_t						(*set_alt_interface)(const void *, const usb_interface_info *);
	status_t						(*set_feature)(const void *, uint16);
	status_t						(*clear_feature)(const void *, uint16);
	status_t						(*get_status)(const void *, uint16 *);
	status_t						(*get_descriptor)(const void *, uint8, uint8, uint16, void *, size_t, size_t *);
	status_t						(*send_request)(const void *, uint8, uint8, uint16, uint16, uint16, void *, size_t, size_t *);
	status_t						(*queue_interrupt)(const void *, void *, size_t, usb_callback_func, void *);
	status_t						(*queue_bulk)(const void *, void *, size_t, usb_callback_func, void *);
	status_t						(*queue_isochronous)(const void *, void *, size_t, rlea *, uint16, usb_callback_func, void *);
	status_t						(*queue_request)(const void *, uint8, uint8, uint16, uint16, uint16, void *, size_t, usb_callback_func, void *);
	status_t						(*set_pipe_policy)(const void *, uint8, uint16, uint16);
	status_t						(*cancel_queued_transfers)(const void *);
	status_t						(*usb_ioctl)(uint32 opcode, void *,size_t);
};


/*
	This module exports the USB API v2
*/
struct usb_module_info_v2 gModuleInfoV2 = {
	// First the bus_manager_info:
	{
		{
			"bus_managers/usb/v2",
			B_KEEP_LOADED,				// Keep loaded, even if no driver requires it
			bus_std_ops
		},
		NULL							// the rescan function
	},

	register_driver,					// register_driver
	install_notify,						// install_notify
	uninstall_notify,					// uninstall_notify
	get_device_descriptor_v2,			// get_device_descriptor
	get_nth_configuration_v2,			// get_nth_configuration
	get_configuration_v2,				// get_configuration
	set_configuration_v2,				// set_configuration
	set_alt_interface_v2,				// set_alt_interface
	set_feature_v2,						// set_feature
	clear_feature_v2,					// clear_feature
	get_status_v2, 						// get_status
	get_descriptor_v2,					// get_descriptor
	send_request_v2,					// send_request
	queue_interrupt_v2,					// queue_interrupt
	queue_bulk_v2,						// queue_bulk
	queue_isochronous_v2,				// queue_isochronous
	queue_request_v2,					// queue_request
	set_pipe_policy_v2,					// set_pipe_policy
	cancel_queued_transfers_v2,			// cancel_queued_transfers
	usb_ioctl							// usb_ioctl
};


//
// #pragma mark -
//


module_info *modules[] = {
	(module_info *)&gModuleInfoV2,
	(module_info *)&gModuleInfoV3,
	NULL
};
