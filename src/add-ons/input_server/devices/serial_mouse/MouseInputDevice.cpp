/*****************************************************************************/
// Mouse input server device addon
// Written by Stefano Ceccherini
// Adapted for serial mice by Oscar Lesta
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
#include "kb_mouse_settings.h"
#include "kb_mouse_driver.h"

#include <stdlib.h>
#include <unistd.h>

#include <Debug.h>
#include <Directory.h>
#include <Entry.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <String.h>

#include "SerialMouse.h"

//------------------------------------------------------------------------------

#if DEBUG
        inline void LOG(const char *fmt, ...) { char buf[1024]; va_list ap; va_start(ap, fmt); vsprintf(buf, fmt, ap); va_end(ap); \
                fputs(buf, MouseInputDevice::sLogFile); fflush(MouseInputDevice::sLogFile); }
        #define LOG_ERR(text...) LOG(text)
FILE *MouseInputDevice::sLogFile = NULL;
#else
        #define LOG(text...)
        #define LOG_ERR(text...) fprintf(stderr, text)
#endif

#define CALLED() LOG("%s\n", __PRETTY_FUNCTION__)

static MouseInputDevice *sSingletonMouseDevice = NULL;


const static uint32 kMouseThreadPriority = B_FIRST_REAL_TIME_PRIORITY + 4;

//------------------------------------------------------------------------------

struct mouse_device {
	mouse_device();
	~mouse_device();

	status_t init_check();	// > 0 if a mouse was detected.

	input_device_ref device_ref;
	SerialMouse* sm;
	thread_id device_watcher;
	mouse_settings settings;
	bool active;
};


extern "C"
BInputServerDevice* instantiate_input_device()
{
	return new MouseInputDevice();
}

//------------------------------------------------------------------------------
// #pragma mark -

MouseInputDevice::MouseInputDevice()
{
	ASSERT(sSingletonMouseDevice == NULL);
	sSingletonMouseDevice = this;

#if DEBUG
	sLogFile = fopen("/var/log/serial_mouse.log", "w");
#endif
	CALLED();
}

MouseInputDevice::~MouseInputDevice()
{
	CALLED();

	for (int32 i = 0; i < fDevices.CountItems(); i++)
		delete (mouse_device*) fDevices.ItemAt(i);

#if DEBUG
	fclose(sLogFile);
#endif
}

// SerialMouse does not know anything about mouse_settings, I choose to let
// mouse_device hold that instead (bercause mice type, button mapping,
// speed/accel, etc., are all handled here, not in SerialMouse).

status_t
MouseInputDevice::InitFromSettings(void* cookie, uint32 opcode)
{
	CALLED();
	mouse_device* device = (mouse_device*) cookie;

	// retrieve current values.
	// TODO: shouldn't we use sane values if we get errors here?

	if (get_mouse_map(&device->settings.map) != B_OK)
		LOG_ERR("error when get_mouse_map\n");

	if (get_click_speed(&device->settings.click_speed) != B_OK)
		LOG_ERR("error when get_click_speed\n");

	if (get_mouse_speed(&device->settings.accel.speed) != B_OK)
		LOG_ERR("error when get_mouse_speed\n");
	else
	{
		if (get_mouse_acceleration(&device->settings.accel.accel_factor) != B_OK)
			LOG_ERR("error when get_mouse_acceleration\n");
	}

	if (get_mouse_type(&device->settings.type) != B_OK)
		LOG_ERR("error when get_mouse_type\n");

	return B_OK;
}

status_t
MouseInputDevice::InitCheck()
{
	CALLED();

	// TODO: We should iterate here to support more than just one mouse.
	//       (I've tried, but kept entering and endless loop or crashing
	//        the input_server :-( ).

	mouse_device* device = new mouse_device();
	if (!device)
	{
		LOG("No memory\n");
		return B_NO_MEMORY;
	}

	if (device->init_check() <= B_OK)
	{
		LOG("The mouse was eaten by a rabid cat.\n");
		delete device;
		return B_ERROR;
	}

	input_device_ref* devices[2];
	devices[0] = &device->device_ref;
	devices[1] = NULL;

	fDevices.AddItem(device);

	return RegisterDevices(devices);
}


status_t
MouseInputDevice::Start(const char* name, void* cookie)
{
	mouse_device* device = (mouse_device*) cookie;

	LOG("%s(%s)\n", __PRETTY_FUNCTION__, name);

	InitFromSettings(device);

	char threadName[B_OS_NAME_LENGTH];
	// "Microsoft watcher" sounded even more akward than this...
	snprintf(threadName, B_OS_NAME_LENGTH, "%s Mouse", name);

	device->active = true;
	device->device_watcher = spawn_thread(DeviceWatcher, threadName,
										kMouseThreadPriority, device);

	resume_thread(device->device_watcher);

	return B_OK;
}


status_t
MouseInputDevice::Stop(const char* name, void* cookie)
{
	mouse_device* device = (mouse_device*) cookie;

	LOG("%s(%s)\n", __PRETTY_FUNCTION__, name);

	device->active = false;

	if (device->device_watcher >= 0)
	{
		suspend_thread(device->device_watcher);
		resume_thread(device->device_watcher);
		status_t dummy;
		wait_for_thread(device->device_watcher, &dummy);
	}

	return B_OK;
}


status_t
MouseInputDevice::Control(const char* name, void* cookie,
						  uint32 command, BMessage* message)
{
	LOG("%s(%s, code: %lu)\n", __PRETTY_FUNCTION__, name, command);

	if (command >= B_MOUSE_TYPE_CHANGED &&
		command <= B_MOUSE_ACCELERATION_CHANGED)
	{
		InitFromSettings(cookie, command);
	}

	return B_OK;
}


int32
MouseInputDevice::DeviceWatcher(void* arg)
{
	mouse_device* dev = (mouse_device*) arg;

	mouse_movement movements;
	uint32 buttons_state = 0;
	uint8  clicks_count = 0;
	bigtime_t last_click_time = 0;
	BMessage* message;

	CALLED();

	while (dev->active)
	{
		memset(&movements, 0, sizeof(movements));
		if (dev->sm->GetMouseEvent(&movements) != B_OK) {
			snooze(10000); // this is a realtime thread, and something is wrong...
			continue;
		}
/*
		LOG("%s: buttons: 0x%lx, x: %ld, y: %ld, clicks:%ld, wheel_x:%ld, \
			wheel_y:%ld\n", dev->device_ref.name, movements.buttons,
			movements.xdelta, movements.ydelta, movements.clicks,
			movements.wheel_xdelta, movements.wheel_ydelta);
*/
		// TODO: Apply buttons mapping here.
		uint32 buttons = buttons_state ^ movements.buttons;

	  	if (movements.buttons) {
		
  			if ((movements.timestamp - last_click_time) <= dev->settings.click_speed)
				clicks_count = (clicks_count % 3) + 1;
  			else
  				clicks_count = 1;

			last_click_time = movements.timestamp;
  		}

		if (buttons != 0) {
			message = new BMessage(B_MOUSE_UP);

			message->AddInt64("when", movements.timestamp);
			message->AddInt32("buttons", movements.buttons);

			if ((buttons & movements.buttons) > 0) {
				message->what = B_MOUSE_DOWN;
				message->AddInt32("clicks", clicks_count);
				LOG("B_MOUSE_DOWN\n");
			} else {
				LOG("B_MOUSE_UP\n");
			}

			message->AddInt32("x", movements.xdelta);
			message->AddInt32("y", movements.ydelta);

			sSingletonMouseDevice->EnqueueMessage(message);
			buttons_state = movements.buttons;
		}

		if (movements.xdelta != 0 || movements.ydelta != 0) {
			int32 xdelta = movements.xdelta;
			int32 ydelta = movements.ydelta;

			// TODO: properly scale these values.
			// (s >> 14, a >> 8) or (s >> 15, a >> 11) feels more or less OK
			// with the default values: yields to speed = 2; accel_factor = 32
			// Maybe we should use floats and then round to nearest integer?
			uint32 speed = (dev->settings.accel.speed >> 15);
			uint32 accel_factor = (dev->settings.accel.accel_factor >> 11);

//			LOG("accel.enabled? = %s\n", (dev->settings.accel.enabled) ? "Yes" : "No");
//			LOG("speed = %ld; accel_factor = %ld\n", speed, accel_factor);

			if (speed && accel_factor) {
				xdelta *= speed;
				ydelta *= speed;

				// preserve the sign.
				bool xneg = (xdelta < 0);
				bool yneg = (ydelta < 0);

				if (movements.xdelta != 0) {
					xdelta = (xdelta * xdelta) / accel_factor;
					(xdelta) ? : (xdelta = 1);
					if (xneg) xdelta *= -1;
	     		}

				if (movements.ydelta != 0) {
					ydelta = (ydelta * ydelta) / accel_factor;
					(ydelta) ? : (ydelta = 1);
					if (yneg) ydelta *= -1;
	     		}
	    	}

//			LOG("%s: x: %ld, y: %ld\n", dev->device_ref.name, xdelta, ydelta);

			message = new BMessage(B_MOUSE_MOVED);
			if (message) {
				message->AddInt64("when", movements.timestamp);
				message->AddInt32("buttons", movements.buttons);
				message->AddInt32("x", xdelta);
				message->AddInt32("y", ydelta);

				sSingletonMouseDevice->EnqueueMessage(message);
			}
		}

		if ((movements.wheel_ydelta != 0) || (movements.wheel_xdelta != 0)) {
			message = new BMessage(B_MOUSE_WHEEL_CHANGED);
			if (message) {
				message->AddInt64("when", movements.timestamp);
				message->AddFloat("be:wheel_delta_x", movements.wheel_xdelta);
				message->AddFloat("be:wheel_delta_y", movements.wheel_ydelta);

				sSingletonMouseDevice->EnqueueMessage(message);
			}
		}
	}

	return B_OK;
}

//==============================================================================
// #pragma mark -

// mouse_device
mouse_device::mouse_device()
{
	device_watcher = -1;
	active = false;
	sm = new SerialMouse();
	device_ref.type = B_POINTING_DEVICE;
	device_ref.cookie = this;
};


status_t
mouse_device::init_check()
{
	status_t result = sm->IsMousePresent();

	if (result > 0)	// Positive values indicate a mouse present.
		device_ref.name = (char *)sm->MouseDescription(); 

	return result;
}


mouse_device::~mouse_device()
{
	delete sm;
}
