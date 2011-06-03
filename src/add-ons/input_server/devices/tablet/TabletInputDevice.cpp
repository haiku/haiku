/*****************************************************************************/
// Tablet input server device addon
// Adapted by Jerome Duval and written by Stefano Ceccherini
//
// TabletInputDevice.cpp
//
// Copyright (c) 2005 Haiku Project
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

// TODO: Use strlcpy instead of strcpy

#include "TabletInputDevice.h"
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

#if DEBUG
        inline void LOG(const char *fmt, ...) { char buf[1024]; va_list ap; va_start(ap, fmt); vsprintf(buf, fmt, ap); va_end(ap); \
                fputs(buf, TabletInputDevice::sLogFile); fflush(TabletInputDevice::sLogFile); }
        #define LOG_ERR(text...) LOG(text)
FILE *TabletInputDevice::sLogFile = NULL;
#else
        #define LOG(text...)
        #define LOG_ERR(text...) fprintf(stderr, text)
#endif

#define CALLED() LOG("%s\n", __PRETTY_FUNCTION__)

static TabletInputDevice *sSingletonTabletDevice = NULL;

const static uint32 kTabletThreadPriority = B_FIRST_REAL_TIME_PRIORITY + 4;
const static char *kTabletDevicesDirectory = "/dev/input/tablet";

// "/dev/" is automatically prepended by StartMonitoringDevice()
const static char *kTabletDevicesDirectoryUSB = "input/tablet/usb";

struct tablet_device {
	tablet_device(const char *path);
	~tablet_device();
	
	input_device_ref device_ref;
	char path[B_PATH_NAME_LENGTH];
	int fd;
	thread_id device_watcher;
	mouse_settings settings;
	bool active;
};


// forward declarations
static char *get_short_name(const char *longName);


extern "C"
BInputServerDevice *
instantiate_input_device()
{
	return new TabletInputDevice();
}


TabletInputDevice::TabletInputDevice()
{
	ASSERT(sSingletonTabletDevice == NULL);
	sSingletonTabletDevice = this;
			
#if DEBUG
	sLogFile = fopen("/var/log/tablet_device_log.log", "a");
#endif
	CALLED();
	
	StartMonitoringDevice(kTabletDevicesDirectoryUSB);
}


TabletInputDevice::~TabletInputDevice()
{
	CALLED();
	StopMonitoringDevice(kTabletDevicesDirectoryUSB);

	for (int32 i = 0; i < fDevices.CountItems(); i++)
		delete (tablet_device *)fDevices.ItemAt(i);
	
#if DEBUG
	fclose(sLogFile);
#endif
}


status_t
TabletInputDevice::InitFromSettings(void *cookie, uint32 opcode)
{
	CALLED();
	tablet_device *device = (tablet_device *)cookie;
	
	// retrieve current values

	if (get_mouse_map(&device->settings.map) != B_OK)
		LOG_ERR("error when get_mouse_map\n");
	else
		ioctl(device->fd, MS_SET_MAP, &device->settings.map);
		
	if (get_click_speed(&device->settings.click_speed) != B_OK)
		LOG_ERR("error when get_click_speed\n");
	else
		ioctl(device->fd, MS_SET_CLICKSPEED, &device->settings.click_speed);
		
	if (get_mouse_speed(&device->settings.accel.speed) != B_OK)
		LOG_ERR("error when get_mouse_speed\n");
	else {
		if (get_mouse_acceleration(&device->settings.accel.accel_factor) != B_OK)
			LOG_ERR("error when get_mouse_acceleration\n");
		else {
			mouse_accel accel;
			ioctl(device->fd, MS_GET_ACCEL, &accel);
			accel.speed = device->settings.accel.speed;
			accel.accel_factor = device->settings.accel.accel_factor;
			ioctl(device->fd, MS_SET_ACCEL, &device->settings.accel);
		}
	}
	
	if (get_mouse_type(&device->settings.type) != B_OK)
		LOG_ERR("error when get_mouse_type\n");
	else
		ioctl(device->fd, MS_SET_TYPE, &device->settings.type);

	return B_OK;

}


status_t
TabletInputDevice::InitCheck()
{
	CALLED();
	RecursiveScan(kTabletDevicesDirectory);
	
	return B_OK;
}


status_t
TabletInputDevice::Start(const char *name, void *cookie)
{
	tablet_device *device = (tablet_device *)cookie;
	
	LOG("%s(%s)\n", __PRETTY_FUNCTION__, name);

	device->fd = open(device->path, O_RDWR);
	if (device->fd<0)
		return B_ERROR;

	InitFromSettings(device);
	
	char threadName[B_OS_NAME_LENGTH];
	snprintf(threadName, B_OS_NAME_LENGTH, "%s watcher", name);
	
	device->active = true;
	device->device_watcher = spawn_thread(DeviceWatcher, threadName,
		kTabletThreadPriority, device);
	
	resume_thread(device->device_watcher);
	
	return B_OK;
}


status_t
TabletInputDevice::Stop(const char *name, void *cookie)
{
	tablet_device *device = (tablet_device *)cookie;
	
	LOG("%s(%s)\n", __PRETTY_FUNCTION__, name);

	close(device->fd);
	
	device->active = false;
	if (device->device_watcher >= 0) {
		suspend_thread(device->device_watcher);
		resume_thread(device->device_watcher);
		status_t dummy;
		wait_for_thread(device->device_watcher, &dummy);
	}
	
	return B_OK;
}


status_t
TabletInputDevice::Control(const char *name, void *cookie,
						  uint32 command, BMessage *message)
{
	LOG("%s(%s, code: %lu)\n", __PRETTY_FUNCTION__, name, command);

	if (command == B_NODE_MONITOR)
		HandleMonitor(message);
	else if (command >= B_MOUSE_TYPE_CHANGED 
		&& command <= B_MOUSE_ACCELERATION_CHANGED) {
		InitFromSettings(cookie, command);
	}
	return B_OK;
}


// TODO: Test this. USB doesn't work on my machine
status_t 
TabletInputDevice::HandleMonitor(BMessage *message)
{
	CALLED();
	int32 opcode = 0;
	status_t status;
	if ((status = message->FindInt32("opcode", &opcode)) < B_OK)
	        return status;
	
	if ((opcode != B_ENTRY_CREATED)
	        && (opcode != B_ENTRY_REMOVED))
	        return B_OK;
	
	
	BEntry entry;
	BPath path;
	dev_t device;
	ino_t directory;
	const char *name = NULL;
	
	message->FindInt32("device", &device);
	message->FindInt64("directory", &directory);
	message->FindString("name", &name);
	
	entry_ref ref(device, directory, name);
	
	if ((status = entry.SetTo(&ref)) != B_OK)
	        return status;
	if ((status = entry.GetPath(&path)) != B_OK)
	        return status;
	if ((status = path.InitCheck()) != B_OK)
	        return status;
	
	if (opcode == B_ENTRY_CREATED)
	        AddDevice(path.Path());
	else
	        RemoveDevice(path.Path());
	
	return status;	
}


status_t
TabletInputDevice::AddDevice(const char *path)
{
	CALLED();

	tablet_device *device = new tablet_device(path);
	if (!device) {
		LOG("No memory\n");
		return B_NO_MEMORY;
	}

	input_device_ref *devices[2];
	devices[0] = &device->device_ref;
	devices[1] = NULL;
	
	fDevices.AddItem(device);
	
	return RegisterDevices(devices);
}


status_t
TabletInputDevice::RemoveDevice(const char *path)
{
	CALLED();
	int32 i = 0;
	tablet_device *device = NULL;
	while ((device = (tablet_device *)fDevices.ItemAt(i)) != NULL) {
		if (!strcmp(device->path, path)) {
			fDevices.RemoveItem(device);
			delete device;
			return B_OK;
		}	
	}
	
	return B_ENTRY_NOT_FOUND;
}


int32
TabletInputDevice::DeviceWatcher(void *arg)
{
	tablet_device *dev = (tablet_device *)arg;
	
	tablet_movement movements;
	tablet_movement old_movements;
	uint32 buttons_state = 0;
	BMessage *message;
	while (dev->active) {
		memset(&movements, 0, sizeof(movements));
		if (ioctl(dev->fd, MS_READ, &movements, 
			sizeof(movements)) != B_OK) {
			snooze(10000); // this is a realtime thread, and something is wrong...
			continue;
		}
		
		uint32 buttons = buttons_state ^ movements.buttons;	
	
		LOG("%s: buttons: 0x%lx, x: %f, y: %f, clicks:%ld, wheel_x:%ld, wheel_y:%ld\n", dev->device_ref.name, movements.buttons, 
			movements.xpos, movements.ypos, movements.clicks, movements.wheel_xdelta, movements.wheel_ydelta);

		movements.xpos /= 10206.0;
		movements.ypos /= 7422.0;
		float x = movements.xpos;
		float y = movements.ypos;

		LOG("%s: x: %f, y: %f, \n", dev->device_ref.name, x, y);
		
		if (buttons != 0) {
			message = new BMessage(B_MOUSE_UP);				
			if ((buttons & movements.buttons) > 0) {
				message->what = B_MOUSE_DOWN;
				message->AddInt32("clicks", movements.clicks);
				LOG("B_MOUSE_DOWN\n");	
			} else {
				LOG("B_MOUSE_UP\n");
			}
			
			message->AddInt64("when", movements.timestamp);
			message->AddInt32("buttons", movements.buttons);						
			message->AddFloat("x", movements.xpos);
			message->AddFloat("y", movements.ypos);
			sSingletonTabletDevice->EnqueueMessage(message);
			buttons_state = movements.buttons;
		}
		
		if (movements.xpos != 0.0 || movements.ypos != 0.0) {
			message = new BMessage(B_MOUSE_MOVED);
			if (message) {
				message->AddInt64("when", movements.timestamp);
				message->AddInt32("buttons", movements.buttons);
				message->AddFloat("x", x);
				message->AddFloat("y", y);
				message->AddFloat("be:tablet_x", movements.xpos);
				message->AddFloat("be:tablet_y", movements.ypos);
				message->AddFloat("be:tablet_pressure", movements.pressure);
				message->AddInt32("be:tablet_eraser", movements.eraser);
				if (movements.tilt_x != 0.0 || movements.tilt_y != 0.0) {
					message->AddFloat("be:tablet_tilt_x", movements.tilt_x);
					message->AddFloat("be:tablet_tilt_y", movements.tilt_y);
				}
					
				sSingletonTabletDevice->EnqueueMessage(message);
			}
		}
		
		if ((movements.wheel_ydelta != 0) || (movements.wheel_xdelta != 0)) {
			message = new BMessage(B_MOUSE_WHEEL_CHANGED);
			if (message) {
				message->AddInt64("when", movements.timestamp);
				message->AddFloat("be:wheel_delta_x", movements.wheel_xdelta);
				message->AddFloat("be:wheel_delta_y", movements.wheel_ydelta);
				
				sSingletonTabletDevice->EnqueueMessage(message);
			}	
		}

		old_movements = movements;
		
	}
	
	return 0;
}


// tablet_device
tablet_device::tablet_device(const char *driver_path) 
{
	fd = -1;
	device_watcher = -1;
	active = false;
	strcpy(path, driver_path);
	device_ref.name = get_short_name(path);
	device_ref.type = B_POINTING_DEVICE;
	device_ref.cookie = this;
};


tablet_device::~tablet_device()
{
	free(device_ref.name);
}
	

void
TabletInputDevice::RecursiveScan(const char *directory)
{
        CALLED();
	BEntry entry;
	BDirectory dir(directory);
	while (dir.GetNextEntry(&entry) == B_OK) {
		BPath path;
		entry.GetPath(&path);

		if (entry.IsDirectory())
			RecursiveScan(path.Path());
		else
			AddDevice(path.Path());
	}
}


static char *
get_short_name(const char *longName)
{
	BString string(longName);
	BString name;
	
	int32 slash = string.FindLast("/");
	string.CopyInto(name, slash + 1, string.Length() - slash);
	int32 index = atoi(name.String()) + 1;
	
	int32 previousSlash = string.FindLast("/", slash);
	string.CopyInto(name, previousSlash + 1, slash - previousSlash - 1); 
	name << " Tablet " << index;
		
	return strdup(name.String());
}
