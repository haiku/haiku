/*
 * Copyright 2004-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stefano Ceccherini
 */


#include "MouseInputDevice.h"
#include "kb_mouse_settings.h"
#include "kb_mouse_driver.h"

#include <Debug.h>
#include <Directory.h>
#include <Entry.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <String.h>
#include <View.h>

#include <errno.h>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#if DEBUG
FILE *MouseInputDevice::sLogFile = NULL;
#	define LOG_ERR(text...) LOG(text)
#else
#	define LOG(text...)
#	define LOG_ERR(text...) fprintf(stderr, text)
#endif

#define CALLED() LOG("%s\n", __PRETTY_FUNCTION__)

const static uint32 kMouseThreadPriority = B_FIRST_REAL_TIME_PRIORITY + 4;
const static char *kMouseDevicesDirectory = "/dev/input/mouse";

// "/dev/" is automatically prepended by StartMonitoringDevice()
const static char *kMouseDevicesDirectoryPS2 = "input/mouse/ps2";
const static char *kMouseDevicesDirectoryUSB = "input/mouse/usb";

class MouseDevice {
	public:
		MouseDevice(BInputServerDevice& target, const char* path);
		~MouseDevice();

		status_t Start();
		void Stop();

		status_t UpdateSettings();

		const char* Path() const { return fPath.String(); }
		input_device_ref* DeviceRef() { return &fDeviceRef; }

	private:
		void _Run();
		static status_t _ThreadFunction(void *arg);

		BMessage* _BuildMouseMessage(uint32 what, uint64 when, uint32 buttons,
					int32 deltaX, int32 deltaY) const;
		void _ComputeAcceleration(const mouse_movement& movements,
					int32& deltaX, int32& deltaY) const;
		uint32 _RemapButtons(uint32 buttons) const;

		char* _BuildShortName() const;

	private:
		BInputServerDevice& fTarget;
		BString	fPath;
		int fDevice;

		input_device_ref fDeviceRef;
		mouse_settings fSettings;
		bool fDeviceRemapsButtons;

		thread_id fThread;
		volatile bool fActive;
};


#if DEBUG
inline void
LOG(const char *fmt, ...)
{
	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(buf, fmt, ap); va_end(ap);
    fputs(buf, MouseInputDevice::sLogFile); fflush(MouseInputDevice::sLogFile);
}
#endif


extern "C" BInputServerDevice *
instantiate_input_device()
{
	return new MouseInputDevice();
}


//	#pragma mark -


MouseDevice::MouseDevice(BInputServerDevice& target, const char *driverPath)
	:
	fTarget(target),
	fDevice(-1),
	fThread(-1),
	fActive(false)
{
	fPath = driverPath;

	fDeviceRef.name = _BuildShortName();
	fDeviceRef.type = B_POINTING_DEVICE;
	fDeviceRef.cookie = this;

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	fSettings.map.button[0] = B_PRIMARY_MOUSE_BUTTON;
	fSettings.map.button[1] = B_SECONDARY_MOUSE_BUTTON;
	fSettings.map.button[2] = B_TERTIARY_MOUSE_BUTTON;
#endif

	fDeviceRemapsButtons = false;
};


MouseDevice::~MouseDevice()
{
	if (fActive)
		Stop();

	free(fDeviceRef.name);
}
	

status_t
MouseDevice::Start()
{
	fDevice = open(fPath.String(), O_RDWR);
	if (fDevice < 0)
		return errno;

	UpdateSettings();

	char threadName[B_OS_NAME_LENGTH];
	snprintf(threadName, B_OS_NAME_LENGTH, "%s watcher", fDeviceRef.name);
	
	fThread = spawn_thread(_ThreadFunction, threadName,
		kMouseThreadPriority, (void *)this);

	status_t status;
	if (fThread < B_OK)
		status = fThread;
	else {
		fActive = true;
		status = resume_thread(fThread);
	}

	if (status < B_OK) {
		LOG_ERR("%s: can't spawn/resume watching thread: %s\n",
			fDeviceRef.name, strerror(status));
		close(fDevice);
		return status;
	}

	return B_OK;
}


void
MouseDevice::Stop()
{
	fActive = false;
		// this will stop the thread as soon as it reads the next packet

	if (fThread >= B_OK) {
		// unblock the thread, which might wait on a semaphore.
		suspend_thread(fThread);
		resume_thread(fThread);

		status_t dummy;
		wait_for_thread(fThread, &dummy);
	}

	close(fDevice);
}


status_t
MouseDevice::UpdateSettings()
{
	CALLED();

	// retrieve current values

	if (get_mouse_map(&fSettings.map) != B_OK)
		LOG_ERR("error when get_mouse_map\n");
	else
		fDeviceRemapsButtons = ioctl(fDevice, MS_SET_MAP, &fSettings.map) == B_OK;

	if (get_click_speed(&fSettings.click_speed) != B_OK)
		LOG_ERR("error when get_click_speed\n");
	else
		ioctl(fDevice, MS_SET_CLICKSPEED, &fSettings.click_speed);

	if (get_mouse_speed(&fSettings.accel.speed) != B_OK)
		LOG_ERR("error when get_mouse_speed\n");
	else {
		if (get_mouse_acceleration(&fSettings.accel.accel_factor) != B_OK)
			LOG_ERR("error when get_mouse_acceleration\n");
		else {
			mouse_accel accel;
			ioctl(fDevice, MS_GET_ACCEL, &accel);
			accel.speed = fSettings.accel.speed;
			accel.accel_factor = fSettings.accel.accel_factor;
			ioctl(fDevice, MS_SET_ACCEL, &fSettings.accel);
		}
	}

	if (get_mouse_type(&fSettings.type) != B_OK)
		LOG_ERR("error when get_mouse_type\n");
	else
		ioctl(fDevice, MS_SET_TYPE, &fSettings.type);

	return B_OK;

}


void
MouseDevice::_Run()
{
	uint32 lastButtons = 0;

	while (fActive) {
		mouse_movement movements;
		memset(&movements, 0, sizeof(movements));

		if (ioctl(fDevice, MS_READ, &movements) != B_OK)
			return;

		uint32 buttons = lastButtons ^ movements.buttons;

		uint32 remappedButtons = _RemapButtons(movements.buttons);
		int32 deltaX, deltaY;
		_ComputeAcceleration(movements, deltaX, deltaY);

		LOG("%s: buttons: 0x%lx, x: %ld, y: %ld, clicks:%ld, wheel_x:%ld, wheel_y:%ld\n",
			device->device_ref.name, movements.buttons, movements.xdelta, movements.ydelta,
			movements.clicks, movements.wheel_xdelta, movements.wheel_ydelta);
		LOG("%s: x: %ld, y: %ld\n", device->device_ref.name, deltaX, deltaY);

		BMessage *message = NULL;

		// Send single messages for each event

		if (movements.xdelta != 0 || movements.ydelta != 0) {
			BMessage* message = _BuildMouseMessage(B_MOUSE_MOVED, movements.timestamp,
				remappedButtons, deltaX, deltaY);
			if (message != NULL)
				fTarget.EnqueueMessage(message);
		}

		if (buttons != 0) {
			bool pressedButton = (buttons & movements.buttons) > 0;
			BMessage* message = _BuildMouseMessage(
				pressedButton ? B_MOUSE_DOWN : B_MOUSE_UP,
				movements.timestamp, remappedButtons, deltaX, deltaY);
			if (message != NULL) {
				if (pressedButton) {
					message->AddInt32("clicks", movements.clicks);
					LOG("B_MOUSE_DOWN\n");	
				} else
					LOG("B_MOUSE_UP\n");

				fTarget.EnqueueMessage(message);
				lastButtons = movements.buttons;
			}
		}

		if ((movements.wheel_ydelta != 0) || (movements.wheel_xdelta != 0)) {
			message = new BMessage(B_MOUSE_WHEEL_CHANGED);
			if (message == NULL)
				continue;

			if (message->AddInt64("when", movements.timestamp) == B_OK
				&& message->AddFloat("be:wheel_delta_x", movements.wheel_xdelta) == B_OK
				&& message->AddFloat("be:wheel_delta_y", movements.wheel_ydelta) == B_OK)
				fTarget.EnqueueMessage(message);
			else
				delete message;
		}
	}
}


status_t
MouseDevice::_ThreadFunction(void* arg)
{
	MouseDevice* device = (MouseDevice *)arg;
	device->_Run();
	return B_OK;
}


BMessage*
MouseDevice::_BuildMouseMessage(uint32 what, uint64 when, uint32 buttons,
	int32 deltaX, int32 deltaY) const
{
	BMessage* message = new BMessage(what);
	if (message == NULL)
		return NULL;

	if (message->AddInt64("when", when) < B_OK
		|| message->AddInt32("buttons", buttons) < B_OK
		|| message->AddInt32("x", deltaX) < B_OK
		|| message->AddInt32("y", deltaY) < B_OK) {
		delete message;
		return NULL;
	}

	return message;
}


void
MouseDevice::_ComputeAcceleration(const mouse_movement& movements,
	int32& deltaX, int32& deltaY) const
{
	// basic mouse speed
	deltaX = movements.xdelta * fSettings.accel.speed >> 16;
	deltaY = movements.ydelta * fSettings.accel.speed >> 16;

	// acceleration
	double acceleration = 1;
	if (fSettings.accel.accel_factor) {
		acceleration = 1 + sqrt(deltaX * deltaX + deltaY * deltaY)
			* fSettings.accel.accel_factor / 524288.0;
	}

	// make sure that we move at least one pixel (if there was a movement)
	if (deltaX > 0)
		deltaX = (int32)floor(deltaX * acceleration);
	else
		deltaX = (int32)ceil(deltaX * acceleration);

	if (deltaY > 0)
		deltaY = (int32)floor(deltaY * acceleration);
	else
		deltaY = (int32)ceil(deltaY * acceleration);
}


uint32
MouseDevice::_RemapButtons(uint32 buttons) const
{
	if (fDeviceRemapsButtons)
		return buttons;

	uint32 newButtons = 0;
	for (int32 i = 0; buttons; i++) {
		if (buttons & 0x1) {
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
			newButtons |= fSettings.map.button[i];
#else
			if (i == 0)
				newButtons |= fSettings.map.left;
			if (i == 1)
				newButtons |= fSettings.map.right;
			if (i == 2)
				newButtons |= fSettings.map.middle;
#endif
		}
		buttons >>= 1;
	}

	return newButtons;
}


char *
MouseDevice::_BuildShortName() const
{
	BString string(fPath);
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


//	#pragma mark -


MouseInputDevice::MouseInputDevice()
{		
#if DEBUG
	sLogFile = fopen("/var/log/mouse_device_log.log", "a");
#endif
	CALLED();
	
	StartMonitoringDevice(kMouseDevicesDirectoryPS2);
	StartMonitoringDevice(kMouseDevicesDirectoryUSB);

	_RecursiveScan(kMouseDevicesDirectory);
}


MouseInputDevice::~MouseInputDevice()
{
	CALLED();
	StopMonitoringDevice(kMouseDevicesDirectoryUSB);
	StopMonitoringDevice(kMouseDevicesDirectoryPS2);

	int32 count = fDevices.CountItems();
	while (count-- > 0) {
		delete (MouseDevice *)fDevices.RemoveItem(count);
	}

#if DEBUG
	fclose(sLogFile);
#endif
}


status_t
MouseInputDevice::InitCheck()
{
	CALLED();
	return B_OK;
}


status_t
MouseInputDevice::Start(const char *name, void *cookie)
{
	LOG("%s(%s)\n", __PRETTY_FUNCTION__, name);
	MouseDevice* device = (MouseDevice*)cookie;

	return device->Start();
}


status_t
MouseInputDevice::Stop(const char *name, void *cookie)
{
	LOG("%s(%s)\n", __PRETTY_FUNCTION__, name);
	MouseDevice* device = (MouseDevice*)cookie;

	device->Stop();
	return B_OK;
}


status_t
MouseInputDevice::Control(const char* name, void* cookie,
	uint32 command, BMessage* message)
{
	LOG("%s(%s, code: %lu)\n", __PRETTY_FUNCTION__, name, command);
	MouseDevice* device = (MouseDevice*)cookie;
	
	if (command == B_NODE_MONITOR)
		return _HandleMonitor(message);

	if (command >= B_MOUSE_TYPE_CHANGED 
		&& command <= B_MOUSE_ACCELERATION_CHANGED)
		return device->UpdateSettings();

	return B_BAD_VALUE;
}


// TODO: Test this. USB doesn't work on my machine
status_t 
MouseInputDevice::_HandleMonitor(BMessage* message)
{
	CALLED();

	int32 opcode;
	if (message->FindInt32("opcode", &opcode) < B_OK)
		return B_BAD_VALUE;

	if (opcode != B_ENTRY_CREATED && opcode != B_ENTRY_REMOVED)
		return B_OK;

	BEntry entry;
	BPath path;
	dev_t device;
	ino_t directory;
	const char *name;

	if (message->FindInt32("device", &device) < B_OK
		|| message->FindInt64("directory", &directory) < B_OK
		|| message->FindString("name", &name) < B_OK)
		return B_BAD_VALUE;

	entry_ref ref(device, directory, name);
	status_t status;

	if ((status = entry.SetTo(&ref)) != B_OK)
		return status;
	if ((status = entry.GetPath(&path)) != B_OK)
		return status;
	if ((status = path.InitCheck()) != B_OK)
		return status;

	if (opcode == B_ENTRY_CREATED)
		status = _AddDevice(path.Path());
	else
		status = _RemoveDevice(path.Path());

	return status;	
}


MouseDevice*
MouseInputDevice::_FindDevice(const char *path)
{
	CALLED();

	for (int32 i = fDevices.CountItems(); i-- > 0;) {
		MouseDevice* device = (MouseDevice*)fDevices.ItemAt(i);
		if (!strcmp(device->Path(), path)) 
			return device;
	}

	return NULL;
}


status_t
MouseInputDevice::_AddDevice(const char *path)
{
	CALLED();

	MouseDevice* device = new (std::nothrow) MouseDevice(*this, path);
	if (!device) {
		LOG("No memory\n");
		return B_NO_MEMORY;
	}

	if (!fDevices.AddItem(device)) {
		delete device;
		return B_NO_MEMORY;
	}

	input_device_ref *devices[2];
	devices[0] = device->DeviceRef();
	devices[1] = NULL;

	return RegisterDevices(devices);
}


status_t
MouseInputDevice::_RemoveDevice(const char *path)
{
	CALLED();

	MouseDevice* device = _FindDevice(path);
	if (device == NULL)
		return B_ENTRY_NOT_FOUND;

	fDevices.RemoveItem(device);

	input_device_ref *devices[2];
	devices[0] = device->DeviceRef();
	devices[1] = NULL;

	UnregisterDevices(devices);

	delete device;
	return B_OK;
}


void
MouseInputDevice::_RecursiveScan(const char* directory)
{
	CALLED();

	BEntry entry;
	BDirectory dir(directory);
	while (dir.GetNextEntry(&entry) == B_OK) {
		BPath path;
		entry.GetPath(&path);

		if (!strcmp(path.Leaf(), "serial")) {
			// skip serial
			continue;
		}

		if (entry.IsDirectory())
			_RecursiveScan(path.Path());
		else
			_AddDevice(path.Path());
	}
}

