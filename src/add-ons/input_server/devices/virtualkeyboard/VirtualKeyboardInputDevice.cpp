/*
 * Copyright 2014 Freeman Lou <freemanlou2430@yahoo.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "VirtualKeyboardInputDevice.h"

extern "C" BInputServerDevice*
instantiate_input_device()
{
	return new(std::nothrow) VirtualKeyboardInputDevice();
}


VirtualKeyboardInputDevice::VirtualKeyboardInputDevice()
	:
	BInputServerDevice()
{
	fKeyboardWindow = new VirtualKeyboardWindow();
}


status_t
VirtualKeyboardInputDevice::SystemShuttingDown()
{
	if (fKeyboardWindow)
		fKeyboardWindow->PostMessage(SYSTEM_SHUTTING_DOWN);
	return B_OK;	
}


status_t
VirtualKeyboardInputDevice::InitCheck()
{
	return BInputServerDevice::InitCheck();
}


status_t
VirtualKeyboardInputDevice::Start(const char* name, void* cookie)
{
	fKeyboardWindow->Show();
	
}
