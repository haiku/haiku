/*
 * Copyright 2020, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT license.
 */


#define DRIVER_NAME "wmi_asus"
#include "WMIPrivate.h"


#define ACPI_ASUS_WMI_MGMT_GUID		"97845ED0-4E6D-11DE-8A39-0800200C9A66"
#define ACPI_ASUS_WMI_EVENT_GUID	"0B3CBB35-E3C2-45ED-91C2-4C5A6D195D1C"
#define ACPI_ASUS_UID_ASUSWMI		"ASUSWMI"

#define ASUS_WMI_METHODID_INIT		0x54494E49
#define ASUS_WMI_METHODID_SPEC		0x43455053
#define ASUS_WMI_METHODID_SFUN		0x4E554653
#define ASUS_WMI_METHODID_DCTS		0x53544344
#define ASUS_WMI_METHODID_DSTS		0x53545344
#define ASUS_WMI_METHODID_DEVS		0x53564544


#define ASUS_WMI_DEVID_ALS_ENABLE		0x00050001
#define ASUS_WMI_DEVID_BRIGHTNESS		0x00050012
#define ASUS_WMI_DEVID_KBD_BACKLIGHT	0x00050021


class WMIAsus {
public:
								WMIAsus(device_node *node);
								~WMIAsus();

private:
			status_t 			_EvaluateMethod(uint32 methodId, uint32 arg0,
									uint32 arg1, uint32 *returnValue);
			status_t			_GetDevState(uint32 devId, uint32* value);
			status_t			_SetDevState(uint32 devId, uint32 arg,
									uint32* value);
	static	void				_NotifyHandler(acpi_handle handle,
									uint32 notify, void *context);
			void				_Notify(acpi_handle handle, uint32 notify);
			status_t			_EnableAls(uint32 enable);
private:
			device_node*		fNode;
			wmi_device_interface* wmi;
			wmi_device			wmi_cookie;
			uint32				fDstsID;
			const char*			fEventGuidString;
			bool				fEnableALS;
};



WMIAsus::WMIAsus(device_node *node)
	:
	fNode(node),
	fDstsID(ASUS_WMI_METHODID_DSTS),
	fEventGuidString(NULL),
	fEnableALS(false)
{
	CALLED();

	device_node *parent;
	parent = gDeviceManager->get_parent_node(node);
	gDeviceManager->get_driver(parent, (driver_module_info **)&wmi,
		(void **)&wmi_cookie);

	gDeviceManager->put_node(parent);

	const char* uid = wmi->get_uid(wmi_cookie);
	if (uid != NULL && strcmp(uid, ACPI_ASUS_UID_ASUSWMI) == 0)
		fDstsID = ASUS_WMI_METHODID_DCTS;
	TRACE("WMIAsus using _UID %s\n", uid);
	uint32 value;
	if (_EvaluateMethod(ASUS_WMI_METHODID_INIT, 0, 0, &value) == B_OK) {
		TRACE("_INIT: %x\n", value);
	}
	if (_EvaluateMethod(ASUS_WMI_METHODID_SPEC, 0, 9, &value) == B_OK) {
		TRACE("_SPEC: %x\n", value);
	}
	if (_EvaluateMethod(ASUS_WMI_METHODID_SFUN, 0, 0, &value) == B_OK) {
		TRACE("_SFUN: %x\n", value);
	}

	// some ASUS laptops need to be ALS forced
	fEnableALS =
		gSMBios->match_vendor_product("ASUSTeK COMPUTER INC.", "UX430UAR");
	if (fEnableALS && _EnableAls(1) == B_OK) {
		TRACE("ALSC enabled\n");
	}

	// install event handler
	if (wmi->install_event_handler(wmi_cookie, ACPI_ASUS_WMI_EVENT_GUID,
		_NotifyHandler, this) == B_OK) {
		fEventGuidString = ACPI_ASUS_WMI_EVENT_GUID;
	}
}


WMIAsus::~WMIAsus()
{
	// for ALS
	if (fEnableALS && _EnableAls(0) == B_OK) {
		TRACE("ALSC disabled\n");
	}

	if (fEventGuidString != NULL)
		wmi->remove_event_handler(wmi_cookie, fEventGuidString);
}


status_t
WMIAsus::_EvaluateMethod(uint32 methodId, uint32 arg0, uint32 arg1,
	uint32 *returnValue)
{
	CALLED();
	uint32 params[] = { arg0, arg1 };
	acpi_data inBuffer = { sizeof(params), params };
	acpi_data outBuffer = { ACPI_ALLOCATE_BUFFER, NULL };
	status_t status = wmi->evaluate_method(wmi_cookie, 0, methodId, &inBuffer,
		&outBuffer);
	if (status != B_OK)
		return status;

	acpi_object_type* object = (acpi_object_type*)outBuffer.pointer;
	uint32 result = 0;
	if (object != NULL) {
		if (object->object_type == ACPI_TYPE_INTEGER)
			result = object->integer.integer;
		free(object);
	}
	if (returnValue != NULL)
		*returnValue = result;

	return B_OK;
}


status_t
WMIAsus::_EnableAls(uint32 enable)
{
	CALLED();
	return _SetDevState(ASUS_WMI_DEVID_ALS_ENABLE, enable, NULL);
}


status_t
WMIAsus::_GetDevState(uint32 devId, uint32 *value)
{
	return _EvaluateMethod(fDstsID, devId, 0, value);
}


status_t
WMIAsus::_SetDevState(uint32 devId, uint32 arg, uint32 *value)
{
	return _EvaluateMethod(ASUS_WMI_METHODID_DEVS, devId, arg, value);
}


void
WMIAsus::_NotifyHandler(acpi_handle handle, uint32 notify, void *context)
{
	WMIAsus* device = (WMIAsus*)context;
	device->_Notify(handle, notify);
}


void
WMIAsus::_Notify(acpi_handle handle, uint32 notify)
{
	CALLED();

	acpi_data response = { ACPI_ALLOCATE_BUFFER, NULL };
	wmi->get_event_data(wmi_cookie, notify, &response);

	acpi_object_type* object = (acpi_object_type*)response.pointer;
	uint32 result = 0;
	if (object != NULL) {
		if (object->object_type == ACPI_TYPE_INTEGER)
			result = object->integer.integer;
		free(object);
	}
	if (result != 0) {
		if (result == 0xc4 || result == 0xc5) {
			TRACE("WMIAsus::_Notify() keyboard backlight key\n");
			uint32 value;
			if (_GetDevState(ASUS_WMI_DEVID_KBD_BACKLIGHT, &value) == B_OK) {
				TRACE("WMIAsus::_Notify() getkeyboard backlight key %"
					B_PRIx32 "\n", value);
				value &= 0x7;
				if (result == 0xc4) {
					if (value < 3)
						value++;
				} else if (value > 0)
					value--;
				TRACE("WMIAsus::_Notify() set keyboard backlight key %"
					B_PRIx32 "\n", value);
				_SetDevState(ASUS_WMI_DEVID_KBD_BACKLIGHT, value | 0x80, NULL);
			}
		} else if (result == 0x6b) {
			TRACE("WMIAsus::_Notify() touchpad control\n");
		} else {
			TRACE("WMIAsus::_Notify() key 0x%" B_PRIx32 "\n", result);
		}
	}
}


//	#pragma mark - driver module API


static float
wmi_asus_support(device_node *parent)
{
	// make sure parent is really a wmi device
	const char *bus;
	if (gDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false))
		return -1;

	if (strcmp(bus, "wmi"))
		return 0.0;

	// check whether it's an asus wmi device
	const char *guid;
	if (gDeviceManager->get_attr_string(parent, WMI_GUID_STRING_ITEM, &guid,
		false) != B_OK || strcmp(guid, ACPI_ASUS_WMI_MGMT_GUID) != 0) {
		return 0.0;
	}

	TRACE("found an asus wmi device\n");

	return 0.6;
}


static status_t
wmi_asus_register_device(device_node *node)
{
	CALLED();
	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, { string: "WMI ASUS" }},
		{ NULL }
	};

	return gDeviceManager->register_node(node, WMI_ASUS_DRIVER_NAME, attrs,
		NULL, NULL);
}


static status_t
wmi_asus_init_driver(device_node *node, void **driverCookie)
{
	CALLED();

	WMIAsus* device = new(std::nothrow) WMIAsus(node);
	if (device == NULL)
		return B_NO_MEMORY;
	*driverCookie = device;

	return B_OK;
}


static void
wmi_asus_uninit_driver(void *driverCookie)
{
	CALLED();
	WMIAsus* device = (WMIAsus*)driverCookie;
	delete device;
}


static status_t
wmi_asus_register_child_devices(void *cookie)
{
	CALLED();
	return B_OK;
}


driver_module_info gWMIAsusDriverModule = {
	{
		WMI_ASUS_DRIVER_NAME,
		0,
		NULL
	},

	wmi_asus_support,
	wmi_asus_register_device,
	wmi_asus_init_driver,
	wmi_asus_uninit_driver,
	wmi_asus_register_child_devices,
	NULL,	// rescan
	NULL,	// removed
};

