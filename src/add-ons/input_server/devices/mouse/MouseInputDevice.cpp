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

// TODO: Use strlcpy instead of strcpy

#include "MouseInputDevice.h"
#include "kb_mouse_settings.h"
#include "kb_mouse_driver.h"

#include <stdlib.h>
#include <unistd.h>

#include <Directory.h>
#include <Entry.h>
#include <List.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <String.h>

#if DEBUG
	#define LOG(text) fputs(text, sLogFile); fflush(sLogFile)
#else
	#define LOG(text)
#endif

#include <Debug.h>

FILE *MouseInputDevice::sLogFile = NULL;

static MouseInputDevice *sSingletonMouseDevice = NULL;

// TODO: These are "stolen" from the kb_mouse driver on bebits, which uses
// the same protocol as BeOS one. They're just here to test this add-on with
// the BeOS mouse driver.
const static uint32 kGetMouseMovements = 10099;
const static uint32 kGetMouseAccel = 10101;
const static uint32 kSetMouseAccel = 10102;
const static uint32 kSetMouseType = 10104;
const static uint32 kSetMouseMap = 10106;
const static uint32 kSetClickSpeed = 10108;

const static uint32 kMouseThreadPriority = B_FIRST_REAL_TIME_PRIORITY + 4;
const static char *kMouseDevicesDirectory = "/dev/input/mouse";

// "/dev/" is automatically prepended by StartMonitoringDevice()
const static char *kMouseDevicesDirectoryUSB = "input/mouse/usb";

struct mouse_device {
	mouse_device(const char *path);
	~mouse_device();
	
	input_device_ref device_ref;
	char driver_path[B_PATH_NAME_LENGTH];
	int driver_fd;
	thread_id device_watcher;
	uint32 buttons_state;
	mouse_settings settings;
	bool active;
};


// forward declarations
static void scan_recursively(const char *directory, BList *list);
static char *get_short_name(const char *longName);


extern "C"
BInputServerDevice *
instantiate_input_device()
{
	return new MouseInputDevice();
}


MouseInputDevice::MouseInputDevice()
	:	fDevices(NULL)
		
{
	ASSERT(sSingletonMouseDevice == NULL);
	sSingletonMouseDevice = this;
			
#if DEBUG
	if (sLogFile == NULL)
		sLogFile = fopen("/var/log/mouse_device_log.log", "a");
#endif
	
	StartMonitoringDevice(kMouseDevicesDirectoryUSB);
}


MouseInputDevice::~MouseInputDevice()
{
	StopMonitoringDevice(kMouseDevicesDirectoryUSB);

	for (int32 i = 0; i < fDevices->CountItems(); i++)
		delete (mouse_device *)fDevices->ItemAt(i);
		
	delete fDevices;
	
#if DEBUG
	fclose(sLogFile);
#endif
}


status_t
MouseInputDevice::InitFromSettings(void *cookie, uint32 opcode)
{
	mouse_device *device = (mouse_device *)cookie;
	
	// retrieve current values

	if (get_mouse_map(&device->settings.map) != B_OK)
		fprintf(stderr, "error when get_mouse_map\n");
	else
		ioctl(device->driver_fd, kSetMouseMap, &device->settings.map);
		
	if (get_click_speed(&device->settings.click_speed) != B_OK)
		fprintf(stderr, "error when get_click_speed\n");
	else
		ioctl(device->driver_fd, kSetClickSpeed, &device->settings.click_speed);
		
	if (get_mouse_speed(&device->settings.accel.speed) != B_OK)
		fprintf(stderr, "error when get_mouse_speed\n");
	else {
		if (get_mouse_acceleration(&device->settings.accel.accel_factor) != B_OK)
			fprintf(stderr, "error when get_mouse_acceleration\n");
		else {
			mouse_accel accel;
			ioctl(device->driver_fd, kGetMouseAccel, &accel);
			accel.speed = device->settings.accel.speed;
			accel.accel_factor = device->settings.accel.accel_factor;
			ioctl(device->driver_fd, kSetMouseAccel, &device->settings.accel);
		}
	}
	
	if (get_mouse_type(&device->settings.type) != B_OK)
		fprintf(stderr, "error when get_mouse_type\n");
	else
		ioctl(device->driver_fd, kSetMouseType, &device->settings.type);

	return B_OK;

}


status_t
MouseInputDevice::InitCheck()
{
	BList list;
	scan_recursively(kMouseDevicesDirectory, &list);
	
	char path[B_PATH_NAME_LENGTH];
	if (list.CountItems() > 0) {
		fDevices = new BList;
		for (int32 i = 0; i < list.CountItems(); i++) {
			strcpy(path, kMouseDevicesDirectory);
			strcat(path, "/");
			strcat(path, (char *)list.ItemAt(i));
			AddDevice(path);
			free(list.ItemAt(i));
		}
	}	
	
	if (fDevices && fDevices->CountItems() > 0)
		return BInputServerDevice::InitCheck();
		
	return B_ERROR;
}


status_t
MouseInputDevice::Start(const char *name, void *cookie)
{
	mouse_device *device = (mouse_device *)cookie;
	
	char log[128];
	snprintf(log, 128, "Start(%s)\n", name);
	
	LOG(log);
	
	char threadName[B_OS_NAME_LENGTH];
	snprintf(threadName, B_OS_NAME_LENGTH, "%s watcher", name);
	
	device->active = true;
	device->device_watcher = spawn_thread(DeviceWatcher, threadName,
									kMouseThreadPriority, device);
	
	resume_thread(device->device_watcher);
	
	
	return B_OK;
}


status_t
MouseInputDevice::Stop(const char *name, void *cookie)
{
	mouse_device *device = (mouse_device *)cookie;
	
	char log[128];
	snprintf(log, 128, "Stop(%s)\n", name);

	LOG(log);
	
	device->active = false;
	if (device->device_watcher >= 0) {
		status_t dummy;
		wait_for_thread(device->device_watcher, &dummy);
	}
	
	return B_OK;
}


status_t
MouseInputDevice::Control(const char *name, void *cookie,
						  uint32 command, BMessage *message)
{
	char log[128];
	snprintf(log, 128, "Control(%s, code: %lu)\n", name, command);

	LOG(log);

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
MouseInputDevice::HandleMonitor(BMessage *message)
{
	int32 opcode = 0;
	status_t status;
	status = message->FindInt32("opcode", &opcode);
	if (status < B_OK)
		return status;
	
	BEntry entry;
	BPath path;
	dev_t device;
	ino_t directory;
	ino_t node;
	const char *name = NULL;
		
	switch (opcode) {
		case B_ENTRY_CREATED:
		{
			message->FindInt32("device", &device);
			message->FindInt64("directory", &directory);
			message->FindInt64("node", &node);
			message->FindString("name", &name);
			
			entry_ref ref(device, directory, name);
			
			status = entry.SetTo(&ref);
			if (status == B_OK);
				status = entry.GetPath(&path);
			if (status == B_OK)
				status = path.InitCheck();
			if (status == B_OK)
				status = AddDevice(path.Path());
			
			break;
		}
		case B_ENTRY_REMOVED:
		{
			message->FindInt32("device", &device);
			message->FindInt64("directory", &directory);
			message->FindInt64("node", &node);
			message->FindString("name", &name);
			
			entry_ref ref(device, directory, name);
			
			status = entry.SetTo(&ref);
			if (status == B_OK);
				status = entry.GetPath(&path);
			if (status == B_OK)
				status = path.InitCheck();
			if (status == B_OK)
				status = RemoveDevice(path.Path());
			
			break;
		}
		default:
			status = B_BAD_VALUE;
			break;
	}
	
	return status;
}


status_t
MouseInputDevice::AddDevice(const char *path)
{
	mouse_device *device = new mouse_device(path);
	if (!device) {
		LOG("No memory\n");
		return B_NO_MEMORY;
	}
		
	input_device_ref *devices[2];
	devices[0] = &device->device_ref;
	devices[1] = NULL;
	
	fDevices->AddItem(device);
	
	InitFromSettings(device);
	
	return RegisterDevices(devices);
}


status_t
MouseInputDevice::RemoveDevice(const char *path)
{
	if (fDevices) {
		int32 i = 0;
		mouse_device *device = NULL;
		while ((device = (mouse_device *)fDevices->ItemAt(i)) != NULL) {
			if (!strcmp(device->driver_path, path)) {
				fDevices->RemoveItem(device);
				delete device;
				return B_OK;
			}	
		}
	}
	
	return B_ENTRY_NOT_FOUND;
}


int32
MouseInputDevice::DeviceWatcher(void *arg)
{
	mouse_device *dev = (mouse_device *)arg;
	
	mouse_movement movements;
	BMessage *message;
	char log[128];
	while (dev->active) {
		memset(&movements, 0, sizeof(movements));
		if (ioctl(dev->driver_fd, kGetMouseMovements, &movements) < B_OK)
			continue;
		
		uint32 buttons = dev->buttons_state ^ movements.buttons;	
	
		snprintf(log, 128, "%s: buttons: 0x%lx, x: %ld, y: %ld, clicks:%ld\n",
				dev->device_ref.name, movements.buttons, movements.xdelta,
				movements.ydelta, movements.clicks);
			
		LOG(log);
		
		// TODO: add acceleration computing
		int32 xdelta = movements.xdelta * dev->settings.accel.speed >> 15;
		int32 ydelta = movements.ydelta * dev->settings.accel.speed >> 15;

		snprintf(log, 128, "%s: x: %ld, y: %ld, \n",
                                dev->device_ref.name, xdelta, ydelta);

                LOG(log);


		// TODO: B_MOUSE_DOWN and B_MOUSE_UP messages don't seem
		// to be generated correctly.	
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
			message->AddInt32("x", xdelta);
			message->AddInt32("y", ydelta);
			sSingletonMouseDevice->EnqueueMessage(message);
			dev->buttons_state = movements.buttons;
		}
		
		if (movements.xdelta != 0 || movements.ydelta != 0) {
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
	
	return 0;
}


// mouse_device
mouse_device::mouse_device(const char *path) 
{
	driver_fd = -1;
	device_watcher = -1;
	buttons_state = 0;
	active = false;
	strcpy(driver_path, path);
	device_ref.name = get_short_name(path);
	device_ref.type = B_POINTING_DEVICE;
	device_ref.cookie = this;
	
	// TODO: Add a function which checks if the object
	// has initialized correctly. Specifically, this is one
	// of the operations which could fail
	driver_fd = open(driver_path, O_RDWR);
};


mouse_device::~mouse_device()
{
	free(device_ref.name);
	if (driver_fd >= 0)
		close(driver_fd);
}
	

// static functions

// On exit, "list" will contain a string for every file in
// the tree, starting from the given "directory". Note that
// the file names will also contain the name of the parent folder.  
static void
scan_recursively(const char *directory, BList *list)
{
	// TODO: See if it's simpler to use POSIX functions
	BEntry entry;
	BDirectory dir(directory);
	char buf[B_OS_NAME_LENGTH];
	while (dir.GetNextEntry(&entry) == B_OK) {
		entry.GetName(buf);
		BPath child(&dir, buf);
		
		if (entry.IsDirectory())
			scan_recursively(child.Path(), list);
		else {
			BPath parent(directory);
			BString deviceName = parent.Leaf();
			deviceName << "/" << buf;
			list->AddItem(strdup(deviceName.String()));
		}
	}
}


static char *
get_short_name(const char *longName)
{
	BString string(longName);
	BString name;
	
	int32 slash = string.FindLast("/");
	int32 previousSlash = string.FindLast("/", slash) + 1;
	string.CopyInto(name, slash, string.Length() - slash);
	
	int32 deviceIndex = atoi(name.String()) + 1;
	string.CopyInto(name, previousSlash, slash - previousSlash); 
	name << " mouse " << deviceIndex;
		
	return strdup(name.String());
}
