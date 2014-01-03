/*
 * Copyright 2014 Freeman Lou <freemanlou2430@yahoo.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef VKID_H
#define VKID_H


#include <InputServerDevice.h>
#include <Message.h>

#include "VirtualKeyboardWindow.h"


class VirtualKeyboardInputDevice : public BInputServerDevice {
public:
										VirtualKeyboardInputDevice();
										~VirtualKeyboardInputDevice();
	virtual status_t					InitCheck();
	virtual	status_t					Start(const char* name, void* cookie);
	virtual	status_t					Stop(const char* name, void* cookie);
	virtual status_t					Control(const char* name, void* cookie,
											uint32 command, BMessage* message);					
private:
			VirtualKeyboardWindow*		fKeyboardWindow;
			
};

extern "C" BInputServerDevice* instantiate_input_device();

#endif // VKID_H
