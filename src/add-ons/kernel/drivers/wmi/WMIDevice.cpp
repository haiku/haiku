/*
 * Copyright 2020, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include "WMIPrivate.h"


WMIDevice::WMIDevice(device_node *node)
	:
	fNode(node),
	fBus(NULL),
	fBusCookie(UINT32_MAX),
	fInitStatus(B_OK)
{
	CALLED();

	{
		device_node *parent = gDeviceManager->get_parent_node(node);
		gDeviceManager->get_driver(parent, NULL, (void **)&fBus);
		gDeviceManager->put_node(parent);
	}

	fInitStatus = gDeviceManager->get_attr_uint32(node, WMI_BUS_COOKIE,
		&fBusCookie, false);
}


WMIDevice::~WMIDevice()
{
}


status_t
WMIDevice::InitCheck()
{
	return fInitStatus;
}


status_t
WMIDevice::EvaluateMethod(uint8 instance, uint32 methodId, const acpi_data* in,
	acpi_data* out)
{
	CALLED();
	return fBus->EvaluateMethod(fBusCookie, instance, methodId, in, out);
}


status_t
WMIDevice::InstallEventHandler(const char* guidString,
	acpi_notify_handler handler, void* context)
{
	CALLED();
	if (guidString == NULL || handler == NULL)
		return B_BAD_VALUE;

	return fBus->InstallEventHandler(guidString, handler, context);
}


status_t
WMIDevice::RemoveEventHandler(const char* guidString)
{
	CALLED();
	if (guidString == NULL)
		return B_BAD_VALUE;

	return fBus->RemoveEventHandler(guidString);
}


status_t
WMIDevice::GetEventData(uint32 notify, acpi_data* out)
{
	CALLED();
	return fBus->GetEventData(notify, out);
}


const char*
WMIDevice::GetUid()
{
	CALLED();
	return fBus->GetUid(fBusCookie);
}


//	#pragma mark - driver module API


static status_t
wmi_init_device(device_node *node, void **_device)
{
	CALLED();
	WMIDevice *device = new(std::nothrow) WMIDevice(node);
	if (device == NULL)
		return B_NO_MEMORY;

	status_t result = device->InitCheck();
	if (result != B_OK) {
		ERROR("failed to set up wmi device object\n");
		return result;
	}

	*_device = device;

	return B_OK;
}


static void
wmi_uninit_device(void *_device)
{
	CALLED();
	WMIDevice *device = (WMIDevice *)_device;
	delete device;
}


static status_t
wmi_evaluate_method(wmi_device _device, uint8 instance, uint32 methodId,
	const acpi_data* in, acpi_data* out)
{
	WMIDevice *device = (WMIDevice *)_device;
	return device->EvaluateMethod(instance, methodId, in, out);
}


static status_t
wmi_install_event_handler(wmi_device _device, const char* guidString,
	acpi_notify_handler handler, void* context)
{
	WMIDevice *device = (WMIDevice *)_device;
	return device->InstallEventHandler(guidString, handler, context);
}


static status_t
wmi_remove_event_handler(wmi_device _device, const char* guidString)
{
	WMIDevice *device = (WMIDevice *)_device;
	return device->RemoveEventHandler(guidString);
}


static status_t
wmi_get_event_data(wmi_device _device, uint32 notify, acpi_data* out)
{
	WMIDevice *device = (WMIDevice *)_device;
	return device->GetEventData(notify, out);
}


static const char*
wmi_get_uid(wmi_device _device)
{
	WMIDevice *device = (WMIDevice *)_device;
	return device->GetUid();
}


static status_t
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;

		default:
			return B_ERROR;
	}
}


wmi_device_interface gWMIDeviceModule = {
	{
		{
			WMI_DEVICE_MODULE_NAME,
			0,
			std_ops
		},

		NULL,	// supported devices
		NULL,	// register node
		wmi_init_device,
		wmi_uninit_device,
		NULL,	// register child devices
		NULL,	// rescan
		NULL, 	// device_removed
	},

	wmi_evaluate_method,
	wmi_install_event_handler,
	wmi_remove_event_handler,
	wmi_get_event_data,
	wmi_get_uid,
};
