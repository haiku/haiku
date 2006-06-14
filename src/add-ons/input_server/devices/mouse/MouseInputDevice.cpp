/*
 * Copyright 2004-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stefano Ceccherini
 */

// TODO: Use strlcpy instead of strcpy

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
#include <View.h>	// for default buttons

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

#ifndef B_FIRST_REAL_TIME_PRIORITY
#define B_FIRST_REAL_TIME_PRIORITY B_REAL_TIME_DISPLAY_PRIORITY
#endif
const static uint32 kMouseThreadPriority = B_FIRST_REAL_TIME_PRIORITY + 4;
const static char *kMouseDevicesDirectory = "/dev/input/mouse";

// "/dev/" is automatically prepended by StartMonitoringDevice()
const static char *kMouseDevicesDirectoryPS2 = "input/mouse/ps2";
const static char *kMouseDevicesDirectoryUSB = "input/mouse/usb";

struct mouse_device {
	mouse_device(const char *path);
	~mouse_device();
	
	input_device_ref device_ref;
	char path[B_PATH_NAME_LENGTH];
	int fd;
	thread_id device_watcher;
	mouse_settings settings;
	volatile bool active;
	bool remap;		// device remaps buttons itself
};


struct watcher_params {
	MouseInputDevice *object;
	mouse_device *device;
};


// forward declarations
static char *get_short_name(const char *longName);


extern "C"
BInputServerDevice *
instantiate_input_device()
{
	return new MouseInputDevice();
}


MouseInputDevice::MouseInputDevice()
{		
#if DEBUG
	sLogFile = fopen("/var/log/mouse_device_log.log", "a");
#endif
	CALLED();
	
	StartMonitoringDevice(kMouseDevicesDirectoryPS2);
	StartMonitoringDevice(kMouseDevicesDirectoryUSB);
}


MouseInputDevice::~MouseInputDevice()
{
	CALLED();
	StopMonitoringDevice(kMouseDevicesDirectoryUSB);
	StopMonitoringDevice(kMouseDevicesDirectoryPS2);
	
	int count = fDevices.CountItems();
	while (count-- > 0)
		delete (mouse_device *)fDevices.RemoveItem((int32)0);
	
#if DEBUG
	fclose(sLogFile);
#endif
}


status_t
MouseInputDevice::InitFromSettings(void *cookie, uint32 opcode)
{
	CALLED();
	mouse_device *device = (mouse_device *)cookie;
	
	// retrieve current values

	if (get_mouse_map(&device->settings.map) != B_OK)
		LOG_ERR("error when get_mouse_map\n");
	else
		device->remap = B_OK == ioctl(device->fd, MS_SET_MAP, &device->settings.map);
		
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
MouseInputDevice::InitCheck()
{
	CALLED();
	RecursiveScan(kMouseDevicesDirectory);
	
	return B_OK;
}


status_t
MouseInputDevice::Start(const char *name, void *cookie)
{
	mouse_device *device = (mouse_device *)cookie;
	
	LOG("%s(%s)\n", __PRETTY_FUNCTION__, name);

	device->fd = open(device->path, O_RDWR);
	if (device->fd < 0)
		return B_ERROR;

	status_t status = InitFromSettings(device);
	if (status < B_OK) {
		LOG_ERR("%s: can't initialize from settings: %s\n",
			name, strerror(status));
		close(device->fd);
		return status;
	}
	
	char threadName[B_OS_NAME_LENGTH];
	snprintf(threadName, B_OS_NAME_LENGTH, "%s watcher", name);
	

	const watcher_params params = { this, device };
	device->active = true;
	device->device_watcher = spawn_thread(ThreadFunction, threadName,
		kMouseThreadPriority, (void *)&params);
	
	if (device->device_watcher < B_OK) {
		LOG_ERR("%s: can't spawn watching thread: %s\n",
			name, strerror(device->device_watcher));
		close(device->fd);
		return device->device_watcher;
	}
		
	status = resume_thread(device->device_watcher);
	if (status < B_OK) {
		LOG_ERR("%s: can't resume watching thread: %s\n",
			name, strerror(status));
		kill_thread(device->device_watcher);
		close(device->fd);
		return status;
	}
	
	return B_OK;
}


status_t
MouseInputDevice::Stop(const char *name, void *cookie)
{
	mouse_device *device = (mouse_device *)cookie;
	
	LOG("%s(%s)\n", __PRETTY_FUNCTION__, name);
	
	device->active = false;
	if (device->device_watcher >= 0) {
		// unblock the thread, which is waiting on a semaphore.
		suspend_thread(device->device_watcher);
		resume_thread(device->device_watcher);
		status_t dummy;
		wait_for_thread(device->device_watcher, &dummy);
	}

	close(device->fd);
	
	return B_OK;
}


status_t
MouseInputDevice::Control(const char *name, void *cookie,
						  uint32 command, BMessage *message)
{
	LOG("%s(%s, code: %lu)\n", __PRETTY_FUNCTION__, name, command);
	status_t status = B_BAD_VALUE;
	
	if (command == B_NODE_MONITOR)
		status = HandleMonitor(message);
	else if (command >= B_MOUSE_TYPE_CHANGED 
		&& command <= B_MOUSE_ACCELERATION_CHANGED) {
		status = InitFromSettings(cookie, command);
	}
	return status;
}


// TODO: Test this. USB doesn't work on my machine
status_t 
MouseInputDevice::HandleMonitor(BMessage *message)
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
		status = AddDevice(path.Path());
	else
		status = RemoveDevice(path.Path());
	
	return status;	
}


status_t
MouseInputDevice::AddDevice(const char *path)
{
	CALLED();

	mouse_device *device = new mouse_device(path);
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
MouseInputDevice::RemoveDevice(const char *path)
{
	CALLED();
	mouse_device *device;
	for (int i = 0; (device = (mouse_device *)fDevices.ItemAt(i)) != NULL; i++) {
		if (!strcmp(device->path, path)) {
			fDevices.RemoveItem(device);

			input_device_ref *devices[2];
			devices[0] = &device->device_ref;
			devices[1] = NULL;
			UnregisterDevices(devices);

			delete device;
			return B_OK;
		}	
	}
	
	return B_ENTRY_NOT_FOUND;
}


int32
MouseInputDevice::ThreadFunction(void  *arg)
{
	watcher_params *params = (watcher_params *)arg;
	return params->object->DeviceWatcher(params->device);
}


int32
MouseInputDevice::DeviceWatcher(mouse_device *dev)
{
	mouse_movement movements;
	uint32 buttons_state = 0;
	BMessage *message = NULL;
	while (dev->active) {
		memset(&movements, 0, sizeof(movements));
		if (ioctl(dev->fd, MS_READ, &movements) != B_OK) {
			return 0;
		}
		
		uint32 buttons = buttons_state ^ movements.buttons;	
	
		LOG("%s: buttons: 0x%lx, x: %ld, y: %ld, clicks:%ld, wheel_x:%ld, wheel_y:%ld\n", dev->device_ref.name, movements.buttons, 
			movements.xdelta, movements.ydelta, movements.clicks, movements.wheel_xdelta, movements.wheel_ydelta);
		
		// TODO: add acceleration computing
		int32 xdelta = movements.xdelta * dev->settings.accel.speed >> 15;
		int32 ydelta = movements.ydelta * dev->settings.accel.speed >> 15;

		LOG("%s: x: %ld, y: %ld, \n", dev->device_ref.name, xdelta, ydelta);
		
		if (movements.xdelta != 0 || movements.ydelta != 0) {
			message = new BMessage(B_MOUSE_MOVED);
			if (message) {
				message->AddInt64("when", movements.timestamp);
				message->AddInt32("buttons", Remap(dev, movements.buttons));
				message->AddInt32("x", xdelta);
				message->AddInt32("y", ydelta);
					
				EnqueueMessage(message);
			}
		}
		
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
			message->AddInt32("buttons", Remap(dev, movements.buttons));						
			message->AddInt32("x", xdelta);
			message->AddInt32("y", ydelta);
			EnqueueMessage(message);
			buttons_state = movements.buttons;
		}
				
		if ((movements.wheel_ydelta != 0) || (movements.wheel_xdelta != 0)) {
			message = new BMessage(B_MOUSE_WHEEL_CHANGED);
			if (message) {
				message->AddInt64("when", movements.timestamp);
				message->AddFloat("be:wheel_delta_x", movements.wheel_xdelta);
				message->AddFloat("be:wheel_delta_y", movements.wheel_ydelta);
				
				EnqueueMessage(message);
			}	
		}
		
	}
	
	return 0;
}


void
MouseInputDevice::RecursiveScan(const char *directory)
{
	CALLED();

	BEntry entry;
	BDirectory dir(directory);
	while (dir.GetNextEntry(&entry) == B_OK) {
		BPath path;
		entry.GetPath(&path);

		if (0 == strcmp(path.Leaf(), "serial"))
			continue; // skip serial

		if (entry.IsDirectory())
			RecursiveScan(path.Path());
		else
			AddDevice(path.Path());
	}
}


uint32
MouseInputDevice::Remap(mouse_device* device, uint32 buttons)
{
	if (device->remap)
		return buttons;
		
	uint32 newbuttons = 0;
	for (int32 i=0; buttons; i++) {
		if (buttons & 0x1) {
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
			newbuttons |= device->settings.map.button[i];
#else
			if (i==0)
				newbuttons |= device->settings.map.left;
			if (i==1)
				newbuttons |= device->settings.map.right;
			if (i==2)
				newbuttons |= device->settings.map.middle;
#endif
		}
		buttons >>= 1;
	}
	
	return newbuttons;
}


// mouse_device
mouse_device::mouse_device(const char *driver_path)
	:
	fd(-1),
	device_watcher(-1),
	active(false)
{
	strcpy(path, driver_path);
	device_ref.name = get_short_name(path);
	device_ref.type = B_POINTING_DEVICE;
	device_ref.cookie = this;
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	settings.map.button[0] = B_PRIMARY_MOUSE_BUTTON;
	settings.map.button[1] = B_SECONDARY_MOUSE_BUTTON;
	settings.map.button[2] = B_TERTIARY_MOUSE_BUTTON;
#endif
	remap = false;
};


mouse_device::~mouse_device()
{
	free(device_ref.name);
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

	if (name == "ps2")
		name = "PS/2";
	else
		name.Capitalize();

	name << " Mouse " << index;

	return strdup(name.String());
}
