/*
 * Copyright 2003-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		Niels S. Reedijk
 */


#include <util/kernel_cpp.h>
#include "usb_private.h"
#include <USB_rle.h>

#define USB_MODULE_NAME "module"

Stack *gUSBStack = NULL;


/*!	The function is an evil hack to allow <tt> <kdebug>usb_keyboard </tt> to
	execute transfers.
	When invoked the first time, a new transfer is started, each time the
	function is called afterwards, it is checked whether the transfer is already
	completed. If called with argv[1] == "cancel" the function cancels a
	possibly pending transfer.
*/
static status_t
debug_run_transfer(Pipe *pipe, uint8 *data, size_t dataLength,
	usb_request_data *requestData, bool cancel)
{
	static uint8 transferBuffer[sizeof(Transfer)]
		__attribute__((aligned(16)));
	static Transfer *transfer = NULL;

	BusManager *bus = pipe->GetBusManager();

	if (cancel) {
		if (transfer != NULL) {
			bus->CancelDebugTransfer(transfer);
			transfer = NULL;
		}

		return B_OK;
	}

	if (transfer != NULL) {
		status_t error = bus->CheckDebugTransfer(transfer);
		if (error != B_DEV_PENDING)
			transfer = NULL;

		return error;
	}

	transfer = new(transferBuffer) Transfer(pipe);
	transfer->SetData(data, dataLength);
	transfer->SetRequestData(requestData);

	status_t error = bus->StartDebugTransfer(transfer);
	if (error != B_OK) {
		transfer = NULL;
		return error;
	}

	return B_DEV_PENDING;
}


static int
debug_get_pipe_for_id(int argc, char **argv)
{
	if (gUSBStack == NULL)
		return 1;

	if (!is_debug_variable_defined("_usbPipeID"))
		return 2;

	uint64 id = get_debug_variable("_usbPipeID", 0);
	Object *object = gUSBStack->GetObjectNoLock((usb_id)id);
	if (!object || (object->Type() & USB_OBJECT_PIPE) == 0)
		return 3;

	set_debug_variable("_usbPipe", (uint64)object);
	return 0;
}


static int
debug_process_transfer(int argc, char **argv)
{
	Pipe *pipe = (Pipe *)get_debug_variable("_usbPipe", 0);
	if (pipe == NULL)
		return B_BAD_VALUE;

	uint8 *data = (uint8 *)get_debug_variable("_usbTransferData", 0);
	size_t length = (size_t)get_debug_variable("_usbTransferLength", 0);
	usb_request_data *requestData
		= (usb_request_data *)get_debug_variable("_usbRequestData", 0);

	return debug_run_transfer(pipe, data, length, requestData,
		argc > 1 && strcmp(argv[1], "cancel") == 0);
}


static int
debug_clear_stall(int argc, char *argv[])
{
	Pipe *pipe = (Pipe *)get_debug_variable("_usbPipe", 0);
	if (pipe == NULL)
		return B_BAD_VALUE;

	static usb_request_data requestData;

	requestData.RequestType = USB_REQTYPE_STANDARD | USB_REQTYPE_ENDPOINT_OUT;
	requestData.Request = USB_REQUEST_CLEAR_FEATURE;
	requestData.Value = USB_FEATURE_ENDPOINT_HALT;
	requestData.Index = pipe->EndpointAddress()
		| (pipe->Direction() == Pipe::In ? USB_ENDPOINT_ADDR_DIR_IN
			: USB_ENDPOINT_ADDR_DIR_OUT);
	requestData.Length = 0;

	Pipe *parentPipe = ((Device *)pipe->Parent())->DefaultPipe();
	for (int tries = 0; tries < 100; tries++) {
		status_t result
			= debug_run_transfer(parentPipe, NULL, 0, &requestData, false);

		if (result == B_DEV_PENDING)
			continue;

		if (result == B_OK) {
			// clearing a stalled condition resets the data toggle
			pipe->SetDataToggle(false);
			return B_OK;
		}

		return result;
	}

	return B_TIMED_OUT;
}


static int32
bus_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT: {
			TRACE_MODULE("init\n");
			if (gUSBStack)
				return B_OK;

#ifdef TRACE_USB
			set_dprintf_enabled(true);
#endif
			Stack *stack = new(std::nothrow) Stack();
			TRACE_MODULE("usb_module: stack created %p\n", stack);
			if (!stack)
				return B_NO_MEMORY;

			if (stack->InitCheck() != B_OK) {
				delete stack;
				return ENODEV;
			}

			gUSBStack = stack;

			add_debugger_command("get_usb_pipe_for_id",
				&debug_get_pipe_for_id,
				"Sets _usbPipe by resolving _usbPipeID");
			add_debugger_command("usb_process_transfer",
				&debug_process_transfer,
				"Transfers _usbTransferData with _usbTransferLength"
				" (and/or _usbRequestData) to pipe _usbPipe");
			add_debugger_command("usb_clear_stall",
				&debug_clear_stall,
				"Tries to issue a clear feature request for the endpoint halt"
				" feature on pipe _usbPipe");
			break;
		}

		case B_MODULE_UNINIT:
			TRACE_MODULE("uninit\n");
			delete gUSBStack;
			gUSBStack = NULL;

			remove_debugger_command("get_usb_pipe_for_id",
				&debug_get_pipe_for_id);
			break;

		default:
			return EINVAL;
	}

	return B_OK;
}


// #pragma mark - ObjectBusyReleaser


class ObjectBusyReleaser {
public:
	ObjectBusyReleaser(Object* object) : fObject(object) {}

	~ObjectBusyReleaser()
	{
		Release();
	}

	void Release()
	{
		if (fObject != NULL) {
			fObject->SetBusy(false);
			fObject = NULL;
		}
	}

	inline bool IsSet() const
	{
		return fObject != NULL;
	}

	inline Object *Get() const
	{
		return fObject;
	}

	inline Object *operator->() const
	{
		return fObject;
	}

private:
	Object *fObject;
};


// #pragma mark - public methods


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
get_device_descriptor(usb_device dev)
{
	TRACE_MODULE("get_device_descriptor(%" B_PRId32 ")\n", dev);
	ObjectBusyReleaser object(gUSBStack->GetObject(dev));
	if (!object.IsSet() || (object->Type() & USB_OBJECT_DEVICE) == 0)
		return NULL;
	Device *device = (Device *)object.Get();

	return device->DeviceDescriptor();
}


const usb_configuration_info *
get_nth_configuration(usb_device dev, uint32 index)
{
	TRACE_MODULE("get_nth_configuration(%" B_PRId32 ", %" B_PRIu32 ")\n",
		dev, index);
	ObjectBusyReleaser object(gUSBStack->GetObject(dev));
	if (!object.IsSet() || (object->Type() & USB_OBJECT_DEVICE) == 0)
		return NULL;
	Device *device = (Device *)object.Get();

	return device->ConfigurationAt((int32)index);
}


const usb_configuration_info *
get_configuration(usb_device dev)
{
	TRACE_MODULE("get_configuration(%" B_PRId32 ")\n", dev);
	ObjectBusyReleaser object(gUSBStack->GetObject(dev));
	if (!object.IsSet() || (object->Type() & USB_OBJECT_DEVICE) == 0)
		return NULL;
	Device *device = (Device *)object.Get();

	return device->Configuration();
}


status_t
set_configuration(usb_device dev,
	const usb_configuration_info *configuration)
{
	TRACE_MODULE("set_configuration(%" B_PRId32 ", %p)\n", dev,
		configuration);
	ObjectBusyReleaser object(gUSBStack->GetObject(dev));
	if (!object.IsSet() || (object->Type() & USB_OBJECT_DEVICE) == 0)
		return B_DEV_INVALID_PIPE;
	Device *device = (Device *)object.Get();

	return device->SetConfiguration(configuration);
}


status_t
set_alt_interface(usb_device dev, const usb_interface_info *interface)
{
	TRACE_MODULE("set_alt_interface(%" B_PRId32 ", %p)\n", dev, interface);
	ObjectBusyReleaser object(gUSBStack->GetObject(dev));
	if (!object.IsSet() || (object->Type() & USB_OBJECT_DEVICE) == 0)
		return B_DEV_INVALID_PIPE;
	Device *device = (Device *)object.Get();

	return device->SetAltInterface(interface);
}


status_t
set_feature(usb_id handle, uint16 selector)
{
	TRACE_MODULE("set_feature(%" B_PRId32 ", %d)\n", handle, selector);
	ObjectBusyReleaser object(gUSBStack->GetObject(handle));
	if (!object.IsSet())
		return B_DEV_INVALID_PIPE;

	return object->SetFeature(selector);
}


status_t
clear_feature(usb_id handle, uint16 selector)
{
	TRACE_MODULE("clear_feature(%" B_PRId32 ", %d)\n", handle, selector);
	ObjectBusyReleaser object(gUSBStack->GetObject(handle));
	if (!object.IsSet())
		return B_DEV_INVALID_PIPE;

	return object->ClearFeature(selector);
}


status_t
get_status(usb_id handle, uint16 *status)
{
	TRACE_MODULE("get_status(%" B_PRId32 ", %p)\n", handle, status);
	if (!status)
		return B_BAD_VALUE;

	ObjectBusyReleaser object(gUSBStack->GetObject(handle));
	if (!object.IsSet())
		return B_DEV_INVALID_PIPE;

	return object->GetStatus(status);
}


status_t
get_descriptor(usb_device dev, uint8 type, uint8 index, uint16 languageID,
	void *data, size_t dataLength, size_t *actualLength)
{
	TRACE_MODULE("get_descriptor(%" B_PRId32 ", 0x%02x, 0x%02x, 0x%04x, %p, "
		"%" B_PRIuSIZE ", %p)\n",
		dev, type, index, languageID, data, dataLength, actualLength);
	ObjectBusyReleaser object(gUSBStack->GetObject(dev));
	if (!object.IsSet() || (object->Type() & USB_OBJECT_DEVICE) == 0)
		return B_DEV_INVALID_PIPE;
	Device *device = (Device *)object.Get();

	return device->GetDescriptor(type, index, languageID,
		data, dataLength, actualLength);
}


status_t
send_request(usb_device dev, uint8 requestType, uint8 request,
	uint16 value, uint16 index, uint16 length, void *data, size_t *actualLength)
{
	TRACE_MODULE("send_request(%" B_PRId32 ", 0x%02x, 0x%02x, 0x%04x, 0x%04x, "
		"%d, %p, %p)\n", dev, requestType, request, value, index, length,
		data, actualLength);
	ObjectBusyReleaser object(gUSBStack->GetObject(dev));
	if (!object.IsSet() || (object->Type() & USB_OBJECT_DEVICE) == 0)
		return B_DEV_INVALID_PIPE;
	Device *device = (Device *)object.Get();

	return device->DefaultPipe()->SendRequest(requestType, request,
		value, index, length, data, length, actualLength);
}


status_t
queue_request(usb_device dev, uint8 requestType, uint8 request,
	uint16 value, uint16 index, uint16 length, void *data,
	usb_callback_func callback, void *callbackCookie)
{
	TRACE_MODULE("queue_request(%" B_PRId32 ", 0x%02x, 0x%02x, 0x%04x, 0x%04x,"
		" %u, %p, %p, %p)\n", dev, requestType, request, value, index,
		length, data, callback,	callbackCookie);
	ObjectBusyReleaser object(gUSBStack->GetObject(dev));
	if (!object.IsSet() || (object->Type() & USB_OBJECT_DEVICE) == 0)
		return B_DEV_INVALID_PIPE;
	Device *device = (Device *)object.Get();

	return device->DefaultPipe()->QueueRequest(requestType,
		request, value, index, length, data, length, callback, callbackCookie);
}


status_t
queue_interrupt(usb_pipe pipe, void *data, size_t dataLength,
	usb_callback_func callback, void *callbackCookie)
{
	TRACE_MODULE("queue_interrupt(%" B_PRId32 ", %p, %ld, %p, %p)\n",
		pipe, data, dataLength, callback, callbackCookie);
	ObjectBusyReleaser object(gUSBStack->GetObject(pipe));
	if (!object.IsSet() || (object->Type() & USB_OBJECT_INTERRUPT_PIPE) == 0)
		return B_DEV_INVALID_PIPE;

	return ((InterruptPipe *)object.Get())->QueueInterrupt(data, dataLength,
		callback, callbackCookie);
}


status_t
queue_bulk(usb_pipe pipe, void *data, size_t dataLength,
	usb_callback_func callback, void *callbackCookie)
{
	TRACE_MODULE("queue_bulk(%" B_PRId32 ", %p, %" B_PRIuSIZE ", %p, %p)\n",
		pipe, data, dataLength, callback, callbackCookie);
	ObjectBusyReleaser object(gUSBStack->GetObject(pipe));
	if (!object.IsSet() || (object->Type() & USB_OBJECT_BULK_PIPE) == 0)
		return B_DEV_INVALID_PIPE;

	return ((BulkPipe *)object.Get())->QueueBulk(data, dataLength, callback,
		callbackCookie);
}


status_t
queue_bulk_v(usb_pipe pipe, iovec *vector, size_t vectorCount,
	usb_callback_func callback, void *callbackCookie)
{
	TRACE_MODULE("queue_bulk_v(%" B_PRId32 ", %p, %" B_PRIuSIZE " %p, %p)\n",
		pipe, vector, vectorCount, callback, callbackCookie);
	ObjectBusyReleaser object(gUSBStack->GetObject(pipe));
	if (!object.IsSet() || (object->Type() & USB_OBJECT_BULK_PIPE) == 0)
		return B_DEV_INVALID_PIPE;

	return ((BulkPipe *)object.Get())->QueueBulkV(vector, vectorCount,
		callback, callbackCookie, false);
}


status_t
queue_bulk_v_physical(usb_pipe pipe, iovec *vector, size_t vectorCount,
	usb_callback_func callback, void *callbackCookie)
{
	TRACE_MODULE("queue_bulk_v_physical(%" B_PRId32 ", %p, %" B_PRIuSIZE
		", %p, %p)\n", pipe, vector, vectorCount, callback, callbackCookie);
	ObjectBusyReleaser object(gUSBStack->GetObject(pipe));
	if (!object.IsSet() || (object->Type() & USB_OBJECT_BULK_PIPE) == 0)
		return B_DEV_INVALID_PIPE;

	return ((BulkPipe *)object.Get())->QueueBulkV(vector, vectorCount,
		callback, callbackCookie, true);
}


status_t
queue_isochronous(usb_pipe pipe, void *data, size_t dataLength,
	usb_iso_packet_descriptor *packetDesc, uint32 packetCount,
	uint32 *startingFrameNumber, uint32 flags, usb_callback_func callback,
	void *callbackCookie)
{
	TRACE_MODULE("queue_isochronous(%" B_PRId32 ", %p, %" B_PRIuSIZE ", %p, "
		"%" B_PRId32 ", %p, 0x%08" B_PRIx32 ", %p, %p)\n",
		pipe, data, dataLength, packetDesc, packetCount, startingFrameNumber,
		flags, callback, callbackCookie);
	ObjectBusyReleaser object(gUSBStack->GetObject(pipe));
	if (!object.IsSet() || (object->Type() & USB_OBJECT_ISO_PIPE) == 0)
		return B_DEV_INVALID_PIPE;

	return ((IsochronousPipe *)object.Get())->QueueIsochronous(data, dataLength,
		packetDesc, packetCount, startingFrameNumber, flags, callback,
		callbackCookie);
}


status_t
set_pipe_policy(usb_pipe pipe, uint8 maxQueuedPackets,
	uint16 maxBufferDurationMS, uint16 sampleSize)
{
	TRACE_MODULE("set_pipe_policy(%" B_PRId32 ", %d, %d, %d)\n", pipe,
		maxQueuedPackets, maxBufferDurationMS, sampleSize);
	ObjectBusyReleaser object(gUSBStack->GetObject(pipe));
	if (!object.IsSet() || (object->Type() & USB_OBJECT_ISO_PIPE) == 0)
		return B_DEV_INVALID_PIPE;

	return ((IsochronousPipe *)object.IsSet())->SetPipePolicy(maxQueuedPackets,
		maxBufferDurationMS, sampleSize);
}


status_t
cancel_queued_transfers(usb_pipe pipe)
{
	TRACE_MODULE("cancel_queued_transfers(%" B_PRId32 ")\n", pipe);
	ObjectBusyReleaser object(gUSBStack->GetObject(pipe));
	if (!object.IsSet() || (object->Type() & USB_OBJECT_PIPE) == 0)
		return B_DEV_INVALID_PIPE;

	return ((Pipe *)object.Get())->CancelQueuedTransfers(false);
}


status_t
usb_ioctl(uint32 opcode, void *buffer, size_t bufferSize)
{
	TRACE_MODULE("usb_ioctl(%" B_PRIu32 ", %p, %" B_PRIuSIZE ")\n", opcode,
		buffer, bufferSize);

	switch (opcode) {
		case 'DNAM': {
			ObjectBusyReleaser object(gUSBStack->GetObject(*(usb_id *)buffer));
			if (!object.IsSet() || (object->Type() & USB_OBJECT_DEVICE) == 0)
				return B_BAD_VALUE;
			Device *device = (Device *)object.Get();

			uint32 index = 0;
			return device->BuildDeviceName((char *)buffer, &index,
				bufferSize, NULL);
		}
	}

	return B_DEV_INVALID_IOCTL;
}


status_t
get_nth_roothub(uint32 index, usb_device *rootHub)
{
	if (!rootHub)
		return B_BAD_VALUE;

	BusManager *busManager = gUSBStack->BusManagerAt(index);
	if (!busManager)
		return B_ENTRY_NOT_FOUND;

	Hub *hub = busManager->GetRootHub();
	if (!hub)
		return B_NO_INIT;

	*rootHub = hub->USBID();
	return B_OK;
}


status_t
get_nth_child(usb_device _hub, uint8 index, usb_device *childDevice)
{
	if (!childDevice)
		return B_BAD_VALUE;

	ObjectBusyReleaser object(gUSBStack->GetObject(_hub));
	if (!object.IsSet() || (object->Type() & USB_OBJECT_HUB) == 0)
		return B_DEV_INVALID_PIPE;

	Hub *hub = (Hub *)object.Get();
	for (uint8 i = 0; i < 8; i++) {
		if (hub->ChildAt(i) == NULL)
			continue;

		if (index-- > 0)
			continue;

		*childDevice = hub->ChildAt(i)->USBID();
		return B_OK;
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
get_device_parent(usb_device _device, usb_device *parentHub, uint8 *portIndex)
{
	if (!parentHub || !portIndex)
		return B_BAD_VALUE;

	ObjectBusyReleaser object(gUSBStack->GetObject(_device));
	if (!object.IsSet() || (object->Type() & USB_OBJECT_DEVICE) == 0)
		return B_DEV_INVALID_PIPE;

	Object *parent = object->Parent();
	if (!parent || (parent->Type() & USB_OBJECT_HUB) == 0)
		return B_ENTRY_NOT_FOUND;

	Hub *hub = (Hub *)parent;
	for (uint8 i = 0; i < 8; i++) {
		if (hub->ChildAt(i) == object.Get()) {
			*portIndex = i;
			*parentHub = hub->USBID();
			return B_OK;
		}
	}

	return B_ERROR;
}


status_t
reset_port(usb_device _hub, uint8 portIndex)
{
	ObjectBusyReleaser object(gUSBStack->GetObject(_hub));
	if (!object.IsSet() || (object->Type() & USB_OBJECT_HUB) == 0)
		return B_DEV_INVALID_PIPE;

	Hub *hub = (Hub *)object.Get();
	return hub->ResetPort(portIndex);
}


status_t
disable_port(usb_device _hub, uint8 portIndex)
{
	ObjectBusyReleaser object(gUSBStack->GetObject(_hub));
	if (!object.IsSet() || (object->Type() & USB_OBJECT_HUB) == 0)
		return B_DEV_INVALID_PIPE;

	Hub *hub = (Hub *)object.Get();
	return hub->DisablePort(portIndex);
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
	usb_ioctl,							// usb_ioctl
	get_nth_roothub,					// get_nth_roothub
	get_nth_child,						// get_nth_child
	get_device_parent,					// get_device_parent
	reset_port,							// reset_port
	disable_port						// disable_port
	//queue_bulk_v_physical				// queue_bulk_v_physical
};


//
// #pragma mark -
//


const usb_device_descriptor *
get_device_descriptor_v2(const void *device)
{
	return get_device_descriptor((usb_id)(ssize_t)device);
}


const usb_configuration_info *
get_nth_configuration_v2(const void *device, uint index)
{
	return get_nth_configuration((usb_id)(ssize_t)device, index);
}


const usb_configuration_info *
get_configuration_v2(const void *device)
{
	return get_configuration((usb_id)(ssize_t)device);
}


status_t
set_configuration_v2(const void *device,
	const usb_configuration_info *configuration)
{
	return set_configuration((usb_id)(ssize_t)device, configuration);
}


status_t
set_alt_interface_v2(const void *device, const usb_interface_info *interface)
{
	return set_alt_interface((usb_id)(ssize_t)device, interface);
}


status_t
set_feature_v2(const void *object, uint16 selector)
{
	return set_feature((usb_id)(ssize_t)object, selector);
}


status_t
clear_feature_v2(const void *object, uint16 selector)
{
	return clear_feature((usb_id)(ssize_t)object, selector);
}


status_t
get_status_v2(const void *object, uint16 *status)
{
	return get_status((usb_id)(ssize_t)object, status);
}


status_t
get_descriptor_v2(const void *device, uint8 type, uint8 index,
	uint16 languageID, void *data, size_t dataLength, size_t *actualLength)
{
	return get_descriptor((usb_id)(ssize_t)device, type, index, languageID, data,
		dataLength, actualLength);
}


status_t
send_request_v2(const void *device, uint8 requestType, uint8 request,
	uint16 value, uint16 index, uint16 length, void *data,
	size_t /*dataLength*/, size_t *actualLength)
{
	return send_request((usb_id)(ssize_t)device, requestType, request, value, index,
		length, data, actualLength);
}


status_t
queue_request_v2(const void *device, uint8 requestType, uint8 request,
	uint16 value, uint16 index, uint16 length, void *data,
	size_t /*dataLength*/, usb_callback_func callback, void *callbackCookie)
{
	return queue_request((usb_id)(ssize_t)device, requestType, request, value, index,
		length, data, callback, callbackCookie);
}


status_t
queue_interrupt_v2(const void *pipe, void *data, size_t dataLength,
	usb_callback_func callback, void *callbackCookie)
{
	return queue_interrupt((usb_id)(ssize_t)pipe, data, dataLength, callback,
		callbackCookie);
}


status_t
queue_bulk_v2(const void *pipe, void *data, size_t dataLength,
	usb_callback_func callback, void *callbackCookie)
{
	return queue_bulk((usb_id)(ssize_t)pipe, data, dataLength, callback,
		callbackCookie);
}


status_t
queue_isochronous_v2(const void *pipe, void *data, size_t dataLength,
	rlea *rleArray, uint16 bufferDurationMS, usb_callback_func callback,
	void *callbackCookie)
{
	// ToDo: convert rlea to usb_iso_packet_descriptor
	// ToDo: use a flag to indicate that the callback shall produce a rlea
	usb_iso_packet_descriptor *packetDesc = NULL;
	return queue_isochronous((usb_id)(ssize_t)pipe, data, dataLength, packetDesc, 0,
		NULL, 0, callback, callbackCookie);
}


status_t
set_pipe_policy_v2(const void *pipe, uint8 maxQueuedPackets,
	uint16 maxBufferDurationMS, uint16 sampleSize)
{
	return set_pipe_policy((usb_id)(ssize_t)pipe, maxQueuedPackets, maxBufferDurationMS,
		sampleSize);
}


status_t
cancel_queued_transfers_v2(const void *pipe)
{
	return cancel_queued_transfers((usb_id)(ssize_t)pipe);
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
