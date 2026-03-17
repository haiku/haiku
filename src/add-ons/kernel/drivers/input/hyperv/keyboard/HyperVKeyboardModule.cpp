/*
 * Copyright 2026 John Davis. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "HyperVKeyboard.h"


device_manager_info* gDeviceManager;


static status_t
hyperv_keyboard_init_device(void* driverCookie, void** _deviceCookie)
{
	CALLED();
	*_deviceCookie = driverCookie;
	return B_OK;
}


static void
hyperv_keyboard_uninit_device(void* deviceCookie)
{
	CALLED();
}


static status_t
hyperv_keyboard_open(void* deviceCookie, const char* path, int openMode, void** _cookie)
{
	CALLED();

	HyperVKeyboard* keyboard = reinterpret_cast<HyperVKeyboard*>(deviceCookie);
	status_t status = keyboard->Open();
	if (status == B_OK)
		*_cookie = keyboard;

	return status;
}


static status_t
hyperv_keyboard_close(void* cookie)
{
	CALLED();
	HyperVKeyboard* keyboard = reinterpret_cast<HyperVKeyboard*>(cookie);
	return keyboard->Close();
}


static status_t
hyperv_keyboard_free(void* cookie)
{
	CALLED();
	return B_OK;
}


static status_t
hyperv_keyboard_read(void* cookie, off_t pos, void* buffer, size_t* _length)
{
	CALLED();
	*_length = 0;
	return B_NOT_ALLOWED;
}


static status_t
hyperv_keyboard_write(void* cookie, off_t pos, const void* buffer, size_t* _length)
{
	CALLED();
	*_length = 0;
	return B_NOT_ALLOWED;
}


static status_t
hyperv_keyboard_control(void* cookie, uint32 op, void* buffer, size_t length)
{
	CALLED();
	HyperVKeyboard* keyboard = reinterpret_cast<HyperVKeyboard*>(cookie);
	return keyboard->Control(op, buffer, length);
}


static float
hyperv_keyboard_supports_device(device_node* parent)
{
	CALLED();

	// Check if parent is the Hyper-V bus manager
	const char* bus;
	if (gDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false) != B_OK)
		return -1;

	if (strcmp(bus, HYPERV_BUS_NAME) != 0)
		return 0.0f;

	// Check if parent is a Hyper-V keyboard device
	const char* type;
	if (gDeviceManager->get_attr_string(parent, HYPERV_DEVICE_TYPE_STRING_ITEM, &type, false)
			!= B_OK)
		return 0.0f;

	if (strcmp(type, VMBUS_TYPE_KEYBOARD) != 0)
		return 0.0f;

	TRACE("Hyper-V keyboard found!\n");
	return 0.8f;
}


static status_t
hyperv_keyboard_register_device(device_node* parent)
{
	CALLED();

	device_attr attributes[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE,
			{ .string = HYPERV_PRETTYNAME_KEYBOARD }},
		{ NULL }
	};

	return gDeviceManager->register_node(parent, HYPERV_KEYBOARD_DRIVER_MODULE_NAME, attributes,
		NULL, NULL);
}


static status_t
hyperv_keyboard_init_driver(device_node* node, void** _driverCookie)
{
	CALLED();

	HyperVKeyboard* keyboard = new(std::nothrow) HyperVKeyboard(node);
	if (keyboard == NULL) {
		ERROR("Unable to allocate Hyper-V Keyboard object\n");
		return B_NO_MEMORY;
	}

	status_t status = keyboard->InitCheck();
	if (status != B_OK) {
		ERROR("Failed to set up Hyper-V Keyboard object\n");
		delete keyboard;
		return status;
	}
	TRACE("Hyper-V Keyboard object created\n");

	*_driverCookie = keyboard;
	return B_OK;
}


static void
hyperv_keyboard_uninit_driver(void* driverCookie)
{
	CALLED();
	HyperVKeyboard* keyboard = reinterpret_cast<HyperVKeyboard*>(driverCookie);
	delete keyboard;
}


static status_t
hyperv_keyboard_register_child_devices(void* driverCookie)
{
	CALLED();
	HyperVKeyboard* keyboard = reinterpret_cast<HyperVKeyboard*>(driverCookie);
	return keyboard->Register();
}


device_module_info sHyperVKeyboardDeviceModule = {
	{
		HYPERV_KEYBOARD_DEVICE_MODULE_NAME,
		0,
		NULL
	},

	hyperv_keyboard_init_device,
	hyperv_keyboard_uninit_device,
	NULL,	// remove

	hyperv_keyboard_open,
	hyperv_keyboard_close,
	hyperv_keyboard_free,
	hyperv_keyboard_read,
	hyperv_keyboard_write,
	NULL,	// io
	hyperv_keyboard_control,

	NULL,	// select
	NULL	// deselect
};


driver_module_info sHyperVKeyboardDriverModule = {
	{
		HYPERV_KEYBOARD_DRIVER_MODULE_NAME,
		0,
		NULL
	},

	hyperv_keyboard_supports_device,
	hyperv_keyboard_register_device,
	hyperv_keyboard_init_driver,
	hyperv_keyboard_uninit_driver,
	hyperv_keyboard_register_child_devices,
	NULL,	// rescan
	NULL	// removed
};


module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&gDeviceManager },
	{}
};


module_info* modules[] = {
	(module_info*)&sHyperVKeyboardDriverModule,
	(module_info*)&sHyperVKeyboardDeviceModule,
	NULL
};
