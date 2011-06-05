/*
 * Copyright 2004-2011, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stefano Ceccherini (stefano.ceccherini@gmail.com)
 *		Jérôme Duval
 *		Axel Dörfler, axeld@pinc-software.de
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 *		Stephan Aßmus, superstippi@gmx.de
 *		Michael Lotz, mmlr@mlotz.ch
 */


#include "TabletInputDevice.h"

#include <errno.h>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Autolock.h>
#include <Debug.h>
#include <Directory.h>
#include <Entry.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <String.h>

#include <kb_mouse_settings.h>
#include <keyboard_mouse_driver.h>


#undef TRACE
//#define TRACE_TABLET_DEVICE
#ifdef TRACE_TABLET_DEVICE

	class FunctionTracer {
	public:
		FunctionTracer(const void* pointer, const char* className,
				const char* functionName,
				int32& depth)
			: fFunctionName(),
			  fPrepend(),
			  fFunctionDepth(depth),
			  fPointer(pointer)
		{
			fFunctionDepth++;
			fPrepend.Append(' ', fFunctionDepth * 2);
			fFunctionName << className << "::" << functionName << "()";

			debug_printf("%p -> %s%s {\n", fPointer, fPrepend.String(),
				fFunctionName.String());
		}

		 ~FunctionTracer()
		{
			debug_printf("%p -> %s}\n", fPointer, fPrepend.String());
			fFunctionDepth--;
		}

	private:
		BString	fFunctionName;
		BString	fPrepend;
		int32&	fFunctionDepth;
		const void* fPointer;
	};


	static int32 sFunctionDepth = -1;
#	define TD_CALLED(x...)	FunctionTracer _ft(this, "TabletDevice", \
								__FUNCTION__, sFunctionDepth)
#	define TID_CALLED(x...)	FunctionTracer _ft(this, "TabletInputDevice", \
								__FUNCTION__, sFunctionDepth)
#	define TRACE(x...)	do { BString _to; \
							_to.Append(' ', (sFunctionDepth + 1) * 2); \
							debug_printf("%p -> %s", this, _to.String()); \
							debug_printf(x); } while (0)
#	define LOG_EVENT(text...) do {} while (0)
#	define LOG_ERR(text...) TRACE(text)
#else
#	define TRACE(x...) do {} while (0)
#	define TD_CALLED(x...) TRACE(x)
#	define TID_CALLED(x...) TRACE(x)
#	define LOG_ERR(x...) debug_printf(x)
#	define LOG_EVENT(x...) TRACE(x)
#endif


const static uint32 kTabletThreadPriority = B_FIRST_REAL_TIME_PRIORITY + 4;
const static char* kTabletDevicesDirectory = "/dev/input/tablet";


class TabletDevice {
public:
								TabletDevice(TabletInputDevice& target,
									const char* path);
								~TabletDevice();

			status_t			Start();
			void				Stop();

			status_t			UpdateSettings();

			const char*			Path() const { return fPath.String(); }
			input_device_ref*	DeviceRef() { return &fDeviceRef; }

private:
			char*				_BuildShortName() const;

	static	status_t			_ControlThreadEntry(void* arg);
			void				_ControlThread();
			void				_ControlThreadCleanup();
			void				_UpdateSettings();

			BMessage*			_BuildMouseMessage(uint32 what,
									uint64 when, uint32 buttons,
									float xPosition, float yPosition) const;

private:
			TabletInputDevice&	fTarget;
			BString				fPath;
			int					fDevice;

			input_device_ref	fDeviceRef;
			mouse_settings		fSettings;

			thread_id			fThread;
	volatile bool				fActive;
	volatile bool				fUpdateSettings;
};


extern "C" BInputServerDevice*
instantiate_input_device()
{
	return new(std::nothrow) TabletInputDevice();
}


//	#pragma mark -


TabletDevice::TabletDevice(TabletInputDevice& target, const char* driverPath)
	:
	fTarget(target),
	fPath(driverPath),
	fDevice(-1),
	fThread(-1),
	fActive(false),
	fUpdateSettings(false)
{
	TD_CALLED();

	fDeviceRef.name = _BuildShortName();
	fDeviceRef.type = B_POINTING_DEVICE;
	fDeviceRef.cookie = this;
};


TabletDevice::~TabletDevice()
{
	TD_CALLED();
	TRACE("delete\n");

	if (fActive)
		Stop();

	free(fDeviceRef.name);
}


status_t
TabletDevice::Start()
{
	TD_CALLED();

	fDevice = open(fPath.String(), O_RDWR);
		// let the control thread handle any error on opening the device

	char threadName[B_OS_NAME_LENGTH];
	snprintf(threadName, B_OS_NAME_LENGTH, "%s watcher", fDeviceRef.name);

	fThread = spawn_thread(_ControlThreadEntry, threadName,
		kTabletThreadPriority, (void*)this);

	status_t status;
	if (fThread < 0)
		status = fThread;
	else {
		fActive = true;
		status = resume_thread(fThread);
	}

	if (status < B_OK) {
		LOG_ERR("%s: can't spawn/resume watching thread: %s\n",
			fDeviceRef.name, strerror(status));
		if (fDevice >= 0)
			close(fDevice);

		return status;
	}

	return fDevice >= 0 ? B_OK : B_ERROR;
}


void
TabletDevice::Stop()
{
	TD_CALLED();

	fActive = false;
		// this will stop the thread as soon as it reads the next packet

	close(fDevice);
	fDevice = -1;

	if (fThread >= 0) {
		// unblock the thread, which might wait on a semaphore.
		suspend_thread(fThread);
		resume_thread(fThread);

		status_t dummy;
		wait_for_thread(fThread, &dummy);
	}
}


status_t
TabletDevice::UpdateSettings()
{
	TD_CALLED();

	if (fThread < 0)
		return B_ERROR;

	// trigger updating the settings in the control thread
	fUpdateSettings = true;

	return B_OK;
}


char*
TabletDevice::_BuildShortName() const
{
	BString string(fPath);
	BString name;

	int32 slash = string.FindLast("/");
	string.CopyInto(name, slash + 1, string.Length() - slash);
	int32 index = atoi(name.String()) + 1;

	int32 previousSlash = string.FindLast("/", slash);
	string.CopyInto(name, previousSlash + 1, slash - previousSlash - 1);

	if (name.Length() < 4)
		name.ToUpper();
	else
		name.Capitalize();

	name << " Tablet " << index;
	return strdup(name.String());
}


// #pragma mark - control thread


status_t
TabletDevice::_ControlThreadEntry(void* arg)
{
	TabletDevice* device = (TabletDevice*)arg;
	device->_ControlThread();
	return B_OK;
}


void
TabletDevice::_ControlThread()
{
	TD_CALLED();

	if (fDevice < 0) {
		_ControlThreadCleanup();
		return;
	}

	_UpdateSettings();

	static const bigtime_t kTransferDelay = 1000000 / 125;
		// 125 transfers per second should be more than enough
	bigtime_t nextTransferTime = system_time() + kTransferDelay;
	uint32 lastButtons = 0;
	float lastXPosition = 0;
	float lastYPosition = 0;

	while (fActive) {
		tablet_movement movements;

		snooze_until(nextTransferTime, B_SYSTEM_TIMEBASE);
		nextTransferTime += kTransferDelay;

		if (ioctl(fDevice, MS_READ, &movements, sizeof(movements)) != B_OK) {
			LOG_ERR("Tablet device exiting, %s\n", strerror(errno));
			_ControlThreadCleanup();
			return;
		}

		// take care of updating the settings first, if necessary
		if (fUpdateSettings) {
			fUpdateSettings = false;
			_UpdateSettings();
		}

		LOG_EVENT("%s: buttons: 0x%lx, x: %f, y: %f, clicks: %ld, contact: %c, "
			"pressure: %f, wheel_x: %ld, wheel_y: %ld, eraser: %c, "
			"tilt: %f/%f\n", fDeviceRef.name, movements.buttons, movements.xpos,
			movements.ypos, movements.clicks, movements.has_contact ? 'y' : 'n',
			movements.pressure, movements.wheel_xdelta, movements.wheel_ydelta,
			movements.eraser ? 'y' : 'n', movements.tilt_x, movements.tilt_y);

		// Send single messages for each event

		uint32 buttons = lastButtons ^ movements.buttons;
		if (buttons != 0) {
			bool pressedButton = (buttons & movements.buttons) > 0;
			BMessage* message = _BuildMouseMessage(
				pressedButton ? B_MOUSE_DOWN : B_MOUSE_UP,
				movements.timestamp, movements.buttons, movements.xpos,
				movements.ypos);
			if (message != NULL) {
				if (pressedButton) {
					message->AddInt32("clicks", movements.clicks);
					LOG_EVENT("B_MOUSE_DOWN\n");
				} else
					LOG_EVENT("B_MOUSE_UP\n");

				fTarget.EnqueueMessage(message);
				lastButtons = movements.buttons;
			}
		}

		if (movements.xpos != lastXPosition
			|| movements.ypos != lastYPosition) {
			BMessage* message = _BuildMouseMessage(B_MOUSE_MOVED,
				movements.timestamp, movements.buttons, movements.xpos,
				movements.ypos);
			if (message != NULL) {
				message->AddFloat("be:tablet_x", movements.xpos);
				message->AddFloat("be:tablet_y", movements.ypos);
				message->AddFloat("be:tablet_pressure", movements.pressure);
				message->AddInt32("be:tablet_eraser", movements.eraser);
				if (movements.tilt_x != 0.0 || movements.tilt_y != 0.0) {
					message->AddFloat("be:tablet_tilt_x", movements.tilt_x);
					message->AddFloat("be:tablet_tilt_y", movements.tilt_y);
				}

				fTarget.EnqueueMessage(message);
				lastXPosition = movements.xpos;
				lastYPosition = movements.ypos;
			}
		}

		if (movements.wheel_ydelta != 0 || movements.wheel_xdelta != 0) {
			BMessage* message = new BMessage(B_MOUSE_WHEEL_CHANGED);
			if (message == NULL)
				continue;

			if (message->AddInt64("when", movements.timestamp) == B_OK
				&& message->AddFloat("be:wheel_delta_x",
					movements.wheel_xdelta) == B_OK
				&& message->AddFloat("be:wheel_delta_y",
					movements.wheel_ydelta) == B_OK)
				fTarget.EnqueueMessage(message);
			else
				delete message;
		}
	}
}


void
TabletDevice::_ControlThreadCleanup()
{
	// NOTE: Only executed when the control thread detected an error
	// and from within the control thread!

	if (fActive) {
		fThread = -1;
		fTarget._RemoveDevice(fPath.String());
	} else {
		// In case active is already false, another thread
		// waits for this thread to quit, and may already hold
		// locks that _RemoveDevice() wants to acquire. In other
		// words, the device is already being removed, so we simply
		// quit here.
	}
}


void
TabletDevice::_UpdateSettings()
{
	TD_CALLED();

	if (get_click_speed(&fSettings.click_speed) != B_OK)
		LOG_ERR("error when get_click_speed\n");
	else
		ioctl(fDevice, MS_SET_CLICKSPEED, &fSettings.click_speed);
}


BMessage*
TabletDevice::_BuildMouseMessage(uint32 what, uint64 when, uint32 buttons,
	float xPosition, float yPosition) const
{
	BMessage* message = new BMessage(what);
	if (message == NULL)
		return NULL;

	if (message->AddInt64("when", when) < B_OK
		|| message->AddInt32("buttons", buttons) < B_OK
		|| message->AddFloat("x", xPosition) < B_OK
		|| message->AddFloat("y", yPosition) < B_OK) {
		delete message;
		return NULL;
	}

	return message;
}


//	#pragma mark -


TabletInputDevice::TabletInputDevice()
	:
	fDevices(2, true),
	fDeviceListLock("TabletInputDevice list")
{
	TID_CALLED();

	StartMonitoringDevice(kTabletDevicesDirectory);
	_RecursiveScan(kTabletDevicesDirectory);
}


TabletInputDevice::~TabletInputDevice()
{
	TID_CALLED();

	StopMonitoringDevice(kTabletDevicesDirectory);
	fDevices.MakeEmpty();
}


status_t
TabletInputDevice::InitCheck()
{
	TID_CALLED();

	return BInputServerDevice::InitCheck();
}


status_t
TabletInputDevice::Start(const char* name, void* cookie)
{
	TID_CALLED();

	TabletDevice* device = (TabletDevice*)cookie;

	return device->Start();
}


status_t
TabletInputDevice::Stop(const char* name, void* cookie)
{
	TRACE("%s(%s)\n", __PRETTY_FUNCTION__, name);

	TabletDevice* device = (TabletDevice*)cookie;
	device->Stop();

	return B_OK;
}


status_t
TabletInputDevice::Control(const char* name, void* cookie,
	uint32 command, BMessage* message)
{
	TRACE("%s(%s, code: %lu)\n", __PRETTY_FUNCTION__, name, command);

	TabletDevice* device = (TabletDevice*)cookie;

	if (command == B_NODE_MONITOR)
		return _HandleMonitor(message);

	if (command == B_CLICK_SPEED_CHANGED)
		return device->UpdateSettings();

	return B_BAD_VALUE;
}


status_t
TabletInputDevice::_HandleMonitor(BMessage* message)
{
	TID_CALLED();

	const char* path;
	int32 opcode;
	if (message->FindInt32("opcode", &opcode) != B_OK
		|| (opcode != B_ENTRY_CREATED && opcode != B_ENTRY_REMOVED)
		|| message->FindString("path", &path) != B_OK)
		return B_BAD_VALUE;

	if (opcode == B_ENTRY_CREATED)
		return _AddDevice(path);

	// Don't handle B_ENTRY_REMOVED, let the control thread take care of it.
	return B_OK;
}


void
TabletInputDevice::_RecursiveScan(const char* directory)
{
	TID_CALLED();

	BEntry entry;
	BDirectory dir(directory);
	while (dir.GetNextEntry(&entry) == B_OK) {
		BPath path;
		entry.GetPath(&path);

		if (entry.IsDirectory())
			_RecursiveScan(path.Path());
		else
			_AddDevice(path.Path());
	}
}


TabletDevice*
TabletInputDevice::_FindDevice(const char* path) const
{
	TID_CALLED();

	for (int32 i = fDevices.CountItems() - 1; i >= 0; i--) {
		TabletDevice* device = fDevices.ItemAt(i);
		if (strcmp(device->Path(), path) == 0)
			return device;
	}

	return NULL;
}


status_t
TabletInputDevice::_AddDevice(const char* path)
{
	TID_CALLED();

	BAutolock _(fDeviceListLock);

	_RemoveDevice(path);

	TabletDevice* device = new(std::nothrow) TabletDevice(*this, path);
	if (device == NULL) {
		TRACE("No memory\n");
		return B_NO_MEMORY;
	}

	if (!fDevices.AddItem(device)) {
		TRACE("No memory in list\n");
		delete device;
		return B_NO_MEMORY;
	}

	input_device_ref* devices[2];
	devices[0] = device->DeviceRef();
	devices[1] = NULL;

	TRACE("adding path: %s, name: %s\n", path, devices[0]->name);

	return RegisterDevices(devices);
}


status_t
TabletInputDevice::_RemoveDevice(const char* path)
{
	TID_CALLED();

	BAutolock _(fDeviceListLock);

	TabletDevice* device = _FindDevice(path);
	if (device == NULL) {
		TRACE("%s not found\n", path);
		return B_ENTRY_NOT_FOUND;
	}

	input_device_ref* devices[2];
	devices[0] = device->DeviceRef();
	devices[1] = NULL;

	TRACE("removing path: %s, name: %s\n", path, devices[0]->name);

	UnregisterDevices(devices);

	fDevices.RemoveItem(device);

	return B_OK;
}
