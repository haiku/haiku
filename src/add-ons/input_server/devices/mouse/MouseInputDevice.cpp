/*****************************************************************************/
// Mouse input server device addon
// Written by Stefano Ceccherini
//
// MouseInputDevice.cpp
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
#include "MouseInputDevice.h"

#include <stdlib.h>
#include <unistd.h>

const static uint32 kGetMouseMovements = 10099;
const static uint32 kGetMouseAccel = 10101;
const static uint32 kSetMouseAccel = 10102;
const static uint32 kSetMouseType = 10104;
const static uint32 kSetMouseMap = 10106;
const static uint32 kSetClickSpeed = 10108;

struct mouse_movement {
  int32 ser_fd_index;
  int32 buttons;
  int32 xdelta;
  int32 ydelta;
  int32 click_count;
  int32 mouse_mods;
  int64 mouse_time;
};


extern "C"
BInputServerDevice *
instantiate_input_device()
{
	return new MouseInputDevice();
}


MouseInputDevice::MouseInputDevice()
	: 	fThread(-1),
		fQuit(false)
{
	fFd = open("dev/input/mouse/ps2/0", O_RDWR);
	if (fFd >= 0)
		fThread = spawn_thread(DeviceWatcher, "mouse watcher thread",
			B_NORMAL_PRIORITY, this);
	
	fLogFile = fopen("/boot/home/device_log.log", "w");
}


MouseInputDevice::~MouseInputDevice()
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
MouseInputDevice::InitFromSettings(uint32 opcode)
{
	// retrieve current values

	if (get_mouse_map(&fSettings.map)!=B_OK)
		fprintf(stderr, "error when get_mouse_map\n");
	else
		ioctl(fFd, kSetMouseMap, &fSettings.map);
		
	if (get_click_speed(&fSettings.click_speed)!=B_OK)
		fprintf(stderr, "error when get_click_speed\n");
	else
		ioctl(fFd, kSetClickSpeed, &fSettings.click_speed);
		
	if (get_mouse_speed(&fSettings.accel.speed)!=B_OK)
		fprintf(stderr, "error when get_mouse_speed\n");
	else {
		if (get_mouse_acceleration(&fSettings.accel.accel_factor)!=B_OK)
			fprintf(stderr, "error when get_mouse_acceleration\n");
		else {
			mouse_accel accel;
			ioctl(fFd, kGetMouseAccel, &accel);
			accel.speed = fSettings.accel.speed;
			accel.accel_factor = fSettings.accel.accel_factor;
			ioctl(fFd, kSetMouseAccel, &fSettings.accel);
		}
	}
	
	if (get_mouse_type(&fSettings.type)!=B_OK)
		fprintf(stderr, "error when get_mouse_type\n");
	else
		ioctl(fFd, kSetMouseType, &fSettings.type);

	return B_OK;
}


status_t
MouseInputDevice::InitCheck()
{
	InitFromSettings();
	
	input_device_ref mouse1 = { "Mouse 1", B_POINTING_DEVICE, (void *)this };
		
	input_device_ref *devices[2] = { &mouse1, NULL };
		
	if (fFd >= 0 && fThread >= 0) {	
		RegisterDevices(devices);
		return BInputServerDevice::InitCheck();
	}
	return B_ERROR;
}


status_t
MouseInputDevice::Start(const char *name, void *cookie)
{
	fputs("Start(", fLogFile);
	fputs(name, fLogFile);
	fputs(")\n", fLogFile);
	resume_thread(fThread);
	
	return B_OK;
}


status_t
MouseInputDevice::Stop(const char *device, void *cookie)
{
	fputs("Stop()\n", fLogFile);
	
	suspend_thread(fThread);
	
	return B_OK;
}


status_t
MouseInputDevice::Control(const char *name, void *cookie,
						  uint32 command, BMessage *message)
{
	fputs("Control()\n", fLogFile);

	if (command == B_NODE_MONITOR)
		HandleMonitor(message);
	else if (command >= B_MOUSE_TYPE_CHANGED 
		&& command <= B_MOUSE_ACCELERATION_CHANGED) {
		InitFromSettings(command);
	}
	return B_OK;
}


status_t 
MouseInputDevice::HandleMonitor(BMessage *message)
{
	return B_OK;
}


int32
MouseInputDevice::DeviceWatcher(void *arg)
{
	MouseInputDevice *dev = (MouseInputDevice *)arg;
	mouse_movement movements;
	BMessage *message;
	char log[128];
	while (!dev->fQuit) {
		ioctl(dev->fFd, kGetMouseMovements, &movements);
		
		// TODO: send B_MOUSE_UP/B_MOUSE_DOWN messages
		
		message = new BMessage(B_MOUSE_MOVED);
		if (message) {
			message->AddInt32("buttons", movements.buttons);
			message->AddInt32("x", movements.xdelta);
			message->AddInt32("y", movements.ydelta);
			snprintf(log, 128, "buttons: %ld, x: %ld, y: %ld\n", 
				movements.buttons, movements.xdelta, movements.ydelta);
			
			fputs(log, dev->fLogFile);
				
			dev->EnqueueMessage(message);
		}
		snooze(1000);
	}
	
	return 0;
}


