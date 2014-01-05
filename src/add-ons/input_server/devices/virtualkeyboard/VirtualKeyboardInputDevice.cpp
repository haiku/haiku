/*
 * Copyright 2014 Freeman Lou <freemanlou2430@yahoo.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "VirtualKeyboardInputDevice.h"

#include <InterfaceDefs.h>

extern "C" BInputServerDevice*
instantiate_input_device()
{
	return new(std::nothrow) VirtualKeyboardInputDevice();
}


VirtualKeyboardInputDevice::VirtualKeyboardInputDevice()
{
}


VirtualKeyboardInputDevice::~VirtualKeyboardInputDevice()
{
}


status_t
VirtualKeyboardInputDevice::InitCheck()
{
	static input_device_ref keyboard = {"VirtualKeyboard",
		B_KEYBOARD_DEVICE, (void*) this};
	static input_device_ref* devices[2] = {&keyboard, NULL};

	RegisterDevices(devices);
	return B_OK;
}


status_t
VirtualKeyboardInputDevice::Start(const char* name, void* cookie)
{
	fKeyboardWindow = new VirtualKeyboardWindow(this);
	fKeyboardWindow->Show();
	return B_OK;
}


status_t
VirtualKeyboardInputDevice::Stop(const char* name, void* cookie)
{
	if (fKeyboardWindow->Lock()) {
		fKeyboardWindow->Quit();
		fKeyboardWindow = NULL;
	}
	return B_OK;
}


status_t
VirtualKeyboardInputDevice::Control(const char* name, void* cookie,
	uint32 command, BMessage* message)
{
	return B_OK;
}
