/*****************************************************************************/
// Mouse input server device addon
// Written by Stefano Ceccherini
//
// KeyboardInputDevice.cpp
//
// Copyright (c) 2004 Haiku Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/
#include "KeyboardInputDevice.h"

#include <stdlib.h>
#include <unistd.h>

const static uint32 kSetLeds = 0x2711;
const static uint32 kSetRepeatingKey = 0x2712;
const static uint32 kSetNonRepeatingKey = 0x2713;
const static uint32 kSetKeyRepeatRate = 0x2714;
const static uint32 kSetKeyRepeatDelay = 0x2716;
const static uint32 kGetNextKey = 0x270f;


extern "C"
BInputServerDevice *
instantiate_input_device()
{
	return new KeyboardInputDevice();
}


KeyboardInputDevice::KeyboardInputDevice()
	: 	fThread(-1),
		fQuit(false)
{
	fFd = open("dev/input/keyboard/ps2/0", O_RDWR);
	if (fFd >= 0)
		fThread = spawn_thread(DeviceWatcher, "keyboard watcher thread",
			B_NORMAL_PRIORITY, this);
	
	fLogFile = fopen("/boot/home/device_log.log", "w");
}


KeyboardInputDevice::~KeyboardInputDevice()
{
	if (fThread >= 0) {
		status_t dummy;
		wait_for_thread(fThread, &dummy);
	}
	
	if (fFd >= 0)
		close(fFd);
	
	fclose(fLogFile);
}


status_t
KeyboardInputDevice::InitFromSettings(uint32 opcode)
{
	// retrieve current values

	return B_OK;
}


status_t
KeyboardInputDevice::InitCheck()
{
	InitFromSettings();
	
	input_device_ref keyboard1 = { "Keyboard 1", B_KEYBOARD_DEVICE, (void *)this };
		
	input_device_ref *devices[2] = { &keyboard1, NULL };
		
	if (fFd >= 0 && fThread >= 0) {	
		RegisterDevices(devices);
		return BInputServerDevice::InitCheck();
	}
	return B_ERROR;
}


status_t
KeyboardInputDevice::Start(const char *name, void *cookie)
{
	fputs("Start(", fLogFile);
	fputs(name, fLogFile);
	fputs(")\n", fLogFile);
	resume_thread(fThread);
	
	return B_OK;
}


status_t
KeyboardInputDevice::Stop(const char *device, void *cookie)
{
	fputs("Stop()\n", fLogFile);
	
	suspend_thread(fThread);
	
	return B_OK;
}


status_t
KeyboardInputDevice::Control(const char *name, void *cookie,
						  uint32 command, BMessage *message)
{
	fputs("Control()\n", fLogFile);

	if (command == B_NODE_MONITOR)
		HandleMonitor(message);
	else if (command >= B_KEY_MAP_CHANGED 
		&& command <= B_KEY_REPEAT_RATE_CHANGED) {
		InitFromSettings(command);
	}
	return B_OK;
}


status_t 
KeyboardInputDevice::HandleMonitor(BMessage *message)
{
	return B_OK;
}


int32
KeyboardInputDevice::DeviceWatcher(void *arg)
{
	KeyboardInputDevice *dev = (KeyboardInputDevice *)arg;
	
	return 0;
}


