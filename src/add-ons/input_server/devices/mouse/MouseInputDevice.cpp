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
 */


#include "MouseInputDevice.h"

#include <errno.h>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Autolock.h>
#include <Debug.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <String.h>
#include <View.h>

#include <kb_mouse_settings.h>
#include <keyboard_mouse_driver.h>
#include <touchpad_settings.h>


#undef TRACE
//#define TRACE_MOUSE_DEVICE
#ifdef TRACE_MOUSE_DEVICE

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
#	define MD_CALLED(x...)	FunctionTracer _ft(this, "MouseDevice", \
								__FUNCTION__, sFunctionDepth)
#	define MID_CALLED(x...)	FunctionTracer _ft(this, "MouseInputDevice", \
								__FUNCTION__, sFunctionDepth)
#	define TRACE(x...)	do { BString _to; \
							_to.Append(' ', (sFunctionDepth + 1) * 2); \
							debug_printf("%p -> %s", this, _to.String()); \
							debug_printf(x); } while (0)
#	define LOG_EVENT(text...) do {} while (0)
#	define LOG_ERR(text...) TRACE(text)
#else
#	define TRACE(x...) do {} while (0)
#	define MD_CALLED(x...) TRACE(x)
#	define MID_CALLED(x...) TRACE(x)
#	define LOG_ERR(x...) debug_printf(x)
#	define LOG_EVENT(x...) TRACE(x)
#endif


const static uint32 kMouseThreadPriority = B_FIRST_REAL_TIME_PRIORITY + 4;
const static char* kMouseDevicesDirectory = "/dev/input/mouse";
const static char* kTouchpadDevicesDirectory = "/dev/input/touchpad";


class MouseDevice {
public:
								MouseDevice(MouseInputDevice& target,
									const char* path);
								~MouseDevice();

			status_t			Start();
			void				Stop();

			status_t			UpdateSettings();
			status_t			UpdateTouchpadSettings(const BMessage* message);

			const char*			Path() const { return fPath.String(); }
			input_device_ref*	DeviceRef() { return &fDeviceRef; }

private:
			char*				_BuildShortName() const;

	static	status_t			_ControlThreadEntry(void* arg);
			void				_ControlThread();
			void				_ControlThreadCleanup();
			void				_UpdateSettings();

			status_t			_GetTouchpadSettingsPath(BPath& path);
			status_t			_ReadTouchpadSettingsMsg(BMessage* message);
			status_t			_UpdateTouchpadSettings();

			BMessage*			_BuildMouseMessage(uint32 what,
									uint64 when, uint32 buttons,
									int32 deltaX, int32 deltaY) const;
			void				_ComputeAcceleration(
									const mouse_movement& movements,
									int32& deltaX, int32& deltaY,
									float& historyDeltaX,
									float& historyDeltaY) const;
			uint32				_RemapButtons(uint32 buttons) const;

private:
			MouseInputDevice&	fTarget;
			BString				fPath;
			int					fDevice;

			input_device_ref	fDeviceRef;
			mouse_settings		fSettings;
			bool				fDeviceRemapsButtons;

			thread_id			fThread;
	volatile bool				fActive;
	volatile bool				fUpdateSettings;

			bool				fIsTouchpad;
			touchpad_settings	fTouchpadSettings;
			BMessage*			fTouchpadSettingsMessage;
			BLocker				fTouchpadSettingsLock;
};


extern "C" BInputServerDevice*
instantiate_input_device()
{
	return new(std::nothrow) MouseInputDevice();
}


//	#pragma mark -


MouseDevice::MouseDevice(MouseInputDevice& target, const char* driverPath)
	:
	fTarget(target),
	fPath(driverPath),
	fDevice(-1),
	fDeviceRemapsButtons(false),
	fThread(-1),
	fActive(false),
	fUpdateSettings(false),
	fIsTouchpad(false),
	fTouchpadSettingsMessage(NULL),
	fTouchpadSettingsLock("Touchpad settings lock")
{
	MD_CALLED();

	fDeviceRef.name = _BuildShortName();
	fDeviceRef.type = B_POINTING_DEVICE;
	fDeviceRef.cookie = this;

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	fSettings.map.button[0] = B_PRIMARY_MOUSE_BUTTON;
	fSettings.map.button[1] = B_SECONDARY_MOUSE_BUTTON;
	fSettings.map.button[2] = B_TERTIARY_MOUSE_BUTTON;
#endif
};


MouseDevice::~MouseDevice()
{
	MD_CALLED();
	TRACE("delete\n");

	if (fActive)
		Stop();

	free(fDeviceRef.name);
	delete fTouchpadSettingsMessage;
}


status_t
MouseDevice::Start()
{
	MD_CALLED();

	fDevice = open(fPath.String(), O_RDWR);
		// let the control thread handle any error on opening the device

	char threadName[B_OS_NAME_LENGTH];
	snprintf(threadName, B_OS_NAME_LENGTH, "%s watcher", fDeviceRef.name);

	fThread = spawn_thread(_ControlThreadEntry, threadName,
		kMouseThreadPriority, (void*)this);

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
MouseDevice::Stop()
{
	MD_CALLED();

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
MouseDevice::UpdateSettings()
{
	MD_CALLED();

	if (fThread < 0)
		return B_ERROR;

	// trigger updating the settings in the control thread
	fUpdateSettings = true;

	return B_OK;
}


status_t
MouseDevice::UpdateTouchpadSettings(const BMessage* message)
{
	if (!fIsTouchpad)
		return B_BAD_TYPE;
	if (fThread < 0)
		return B_ERROR;

	BAutolock _(fTouchpadSettingsLock);

	// trigger updating the settings in the control thread
	fUpdateSettings = true;

	delete fTouchpadSettingsMessage;
	fTouchpadSettingsMessage = new BMessage(*message);
	if (fTouchpadSettingsMessage == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


char*
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

	if (name.Length() < 4)
		name.ToUpper();
	else
		name.Capitalize();

	if (string.FindFirst("touchpad") >= 0) {
		name << " Touchpad ";
	} else {
		if (string.FindFirst("intelli") >= 0)
			name.Prepend("Extended ");

		name << " Mouse ";
	}
	name << index;

	return strdup(name.String());
}


// #pragma mark - control thread


status_t
MouseDevice::_ControlThreadEntry(void* arg)
{
	MouseDevice* device = (MouseDevice*)arg;
	device->_ControlThread();
	return B_OK;
}


void
MouseDevice::_ControlThread()
{
	MD_CALLED();

	if (fDevice < 0) {
		_ControlThreadCleanup();
		// TOAST!
		return;
	}

	// touchpad settings
	if (ioctl(fDevice, MS_IS_TOUCHPAD, NULL) == B_OK) {
		TRACE("is touchpad %s\n", fPath.String());
		fIsTouchpad = true;

		fTouchpadSettings = kDefaultTouchpadSettings;

		BPath path;
		status_t status = _GetTouchpadSettingsPath(path);
		BFile settingsFile(path.Path(), B_READ_ONLY);
		if (status == B_OK && settingsFile.InitCheck() == B_OK) {
			if (settingsFile.Read(&fTouchpadSettings, sizeof(touchpad_settings))
					!= sizeof(touchpad_settings)) {
				TRACE("failed to load settings\n");
			}
		}
		_UpdateTouchpadSettings();
	}

	_UpdateSettings();

	uint32 lastButtons = 0;
	float historyDeltaX = 0.0;
	float historyDeltaY = 0.0;

	static const bigtime_t kTransferDelay = 1000000 / 125;
		// 125 transfers per second should be more than enough
#define USE_REGULAR_INTERVAL 1
#if USE_REGULAR_INTERVAL
	bigtime_t nextTransferTime = system_time() + kTransferDelay;
#endif

	while (fActive) {
		mouse_movement movements;

#if USE_REGULAR_INTERVAL
		snooze_until(nextTransferTime, B_SYSTEM_TIMEBASE);
		nextTransferTime += kTransferDelay;
#endif

		if (ioctl(fDevice, MS_READ, &movements, sizeof(movements)) != B_OK) {
			LOG_ERR("Mouse device exiting, %s\n", strerror(errno));
			_ControlThreadCleanup();
			// TOAST!
			return;
		}

		// take care of updating the settings first, if necessary
		if (fUpdateSettings) {
			fUpdateSettings = false;
			if (fIsTouchpad) {
				BAutolock _(fTouchpadSettingsLock);
				if (fTouchpadSettingsMessage != NULL) {
					_ReadTouchpadSettingsMsg(fTouchpadSettingsMessage);
					_UpdateTouchpadSettings();
					delete fTouchpadSettingsMessage;
					fTouchpadSettingsMessage = NULL;
				} else
					_UpdateSettings();
			} else
				_UpdateSettings();
		}

		uint32 buttons = lastButtons ^ movements.buttons;

		uint32 remappedButtons = _RemapButtons(movements.buttons);
		int32 deltaX, deltaY;
		_ComputeAcceleration(movements, deltaX, deltaY, historyDeltaX,
			historyDeltaY);

		LOG_EVENT("%s: buttons: 0x%lx, x: %ld, y: %ld, clicks:%ld, "
			"wheel_x:%ld, wheel_y:%ld\n",
			fDeviceRef.name, movements.buttons,
			movements.xdelta, movements.ydelta, movements.clicks,
			movements.wheel_xdelta, movements.wheel_ydelta);
		LOG_EVENT("%s: x: %ld, y: %ld (%.4f, %.4f)\n", fDeviceRef.name,
			deltaX, deltaY, historyDeltaX, historyDeltaY);

		// Send single messages for each event

		if (buttons != 0) {
			bool pressedButton = (buttons & movements.buttons) > 0;
			BMessage* message = _BuildMouseMessage(
				pressedButton ? B_MOUSE_DOWN : B_MOUSE_UP,
				movements.timestamp, remappedButtons, deltaX, deltaY);
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

		if (movements.xdelta != 0 || movements.ydelta != 0) {
			BMessage* message = _BuildMouseMessage(B_MOUSE_MOVED,
				movements.timestamp, remappedButtons, deltaX, deltaY);
			if (message != NULL)
				fTarget.EnqueueMessage(message);
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

#if !USE_REGULAR_INTERVAL
		snooze(kTransferDelay);
#endif
	}
}


void
MouseDevice::_ControlThreadCleanup()
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
MouseDevice::_UpdateSettings()
{
	MD_CALLED();

	// retrieve current values

	if (get_mouse_map(&fSettings.map) != B_OK)
		LOG_ERR("error when get_mouse_map\n");
	else {
		fDeviceRemapsButtons
			= ioctl(fDevice, MS_SET_MAP, &fSettings.map) == B_OK;
	}

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
}


status_t
MouseDevice::_GetTouchpadSettingsPath(BPath& path)
{
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status < B_OK)
		return status;
	return path.Append(TOUCHPAD_SETTINGS_FILE);
}


status_t
MouseDevice::_ReadTouchpadSettingsMsg(BMessage* message)
{
	message->FindBool("scroll_twofinger", &fTouchpadSettings.scroll_twofinger);
	message->FindBool("scroll_twofinger_horizontal",
		&fTouchpadSettings.scroll_twofinger_horizontal);
	message->FindFloat("scroll_rightrange",
		&fTouchpadSettings.scroll_rightrange);
	message->FindFloat("scroll_bottomrange",
		&fTouchpadSettings.scroll_bottomrange);

	message->FindInt16("scroll_xstepsize",
		(int16*)&fTouchpadSettings.scroll_xstepsize);
	message->FindInt16("scroll_ystepsize",
		(int16*)&fTouchpadSettings.scroll_ystepsize);
	message->FindInt8("scroll_acceleration",
		(int8*)&fTouchpadSettings.scroll_acceleration);
	message->FindInt8("tapgesture_sensibility",
		(int8*)&fTouchpadSettings.tapgesture_sensibility);

	return B_OK;
}


status_t
MouseDevice::_UpdateTouchpadSettings()
{
	if (fIsTouchpad) {
		ioctl(fDevice, MS_SET_TOUCHPAD_SETTINGS, &fTouchpadSettings);
		return B_OK;
	}
	return B_ERROR;
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
	int32& _deltaX, int32& _deltaY, float& historyDeltaX,
	float& historyDeltaY) const
{
	// basic mouse speed
	float deltaX = (float)movements.xdelta * fSettings.accel.speed / 65536.0
		+ historyDeltaX;
	float deltaY = (float)movements.ydelta * fSettings.accel.speed / 65536.0
		+ historyDeltaY;

	// acceleration
	double acceleration = 1;
	if (fSettings.accel.accel_factor) {
		acceleration = 1 + sqrt(deltaX * deltaX + deltaY * deltaY)
			* fSettings.accel.accel_factor / 524288.0;
	}

	deltaX *= acceleration;
	deltaY *= acceleration;

	if (deltaX >= 0)
		_deltaX = (int32)floorf(deltaX);
	else
		_deltaX = (int32)ceilf(deltaX);

	if (deltaY >= 0)
		_deltaY = (int32)floorf(deltaY);
	else
		_deltaY = (int32)ceilf(deltaY);

	historyDeltaX = deltaX - _deltaX;
	historyDeltaY = deltaY - _deltaY;
}


uint32
MouseDevice::_RemapButtons(uint32 buttons) const
{
	if (fDeviceRemapsButtons)
		return buttons;

	uint32 newButtons = 0;
	for (int32 i = 0; buttons; i++) {
		if (buttons & 0x1) {
#if defined(HAIKU_TARGET_PLATFORM_HAIKU) || defined(HAIKU_TARGET_PLATFORM_DANO)
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


//	#pragma mark -


MouseInputDevice::MouseInputDevice()
	:
	fDevices(2, true),
	fDeviceListLock("MouseInputDevice list")
{
	MID_CALLED();

	StartMonitoringDevice(kMouseDevicesDirectory);
	StartMonitoringDevice(kTouchpadDevicesDirectory);
	_RecursiveScan(kMouseDevicesDirectory);
	_RecursiveScan(kTouchpadDevicesDirectory);
}


MouseInputDevice::~MouseInputDevice()
{
	MID_CALLED();

	StopMonitoringDevice(kTouchpadDevicesDirectory);
	StopMonitoringDevice(kMouseDevicesDirectory);
	fDevices.MakeEmpty();
}


status_t
MouseInputDevice::InitCheck()
{
	MID_CALLED();

	return BInputServerDevice::InitCheck();
}


status_t
MouseInputDevice::Start(const char* name, void* cookie)
{
	MID_CALLED();

	MouseDevice* device = (MouseDevice*)cookie;

	return device->Start();
}


status_t
MouseInputDevice::Stop(const char* name, void* cookie)
{
	TRACE("%s(%s)\n", __PRETTY_FUNCTION__, name);

	MouseDevice* device = (MouseDevice*)cookie;
	device->Stop();

	return B_OK;
}


status_t
MouseInputDevice::Control(const char* name, void* cookie,
	uint32 command, BMessage* message)
{
	TRACE("%s(%s, code: %lu)\n", __PRETTY_FUNCTION__, name, command);

	MouseDevice* device = (MouseDevice*)cookie;

	if (command == B_NODE_MONITOR)
		return _HandleMonitor(message);

	if (command == MS_SET_TOUCHPAD_SETTINGS)
		return device->UpdateTouchpadSettings(message);

	if (command >= B_MOUSE_TYPE_CHANGED
		&& command <= B_MOUSE_ACCELERATION_CHANGED)
		return device->UpdateSettings();

	return B_BAD_VALUE;
}


status_t
MouseInputDevice::_HandleMonitor(BMessage* message)
{
	MID_CALLED();

	const char* path;
	int32 opcode;
	if (message->FindInt32("opcode", &opcode) != B_OK
		|| (opcode != B_ENTRY_CREATED && opcode != B_ENTRY_REMOVED)
		|| message->FindString("path", &path) != B_OK)
		return B_BAD_VALUE;

	if (opcode == B_ENTRY_CREATED)
		return _AddDevice(path);

#if 0
	return _RemoveDevice(path);
#else
	// Don't handle B_ENTRY_REMOVED, let the control thread take care of it.
	return B_OK;
#endif
}


void
MouseInputDevice::_RecursiveScan(const char* directory)
{
	MID_CALLED();

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


MouseDevice*
MouseInputDevice::_FindDevice(const char* path) const
{
	MID_CALLED();

	for (int32 i = fDevices.CountItems() - 1; i >= 0; i--) {
		MouseDevice* device = fDevices.ItemAt(i);
		if (strcmp(device->Path(), path) == 0)
			return device;
	}

	return NULL;
}


status_t
MouseInputDevice::_AddDevice(const char* path)
{
	MID_CALLED();

	BAutolock _(fDeviceListLock);

	_RemoveDevice(path);

	MouseDevice* device = new(std::nothrow) MouseDevice(*this, path);
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
MouseInputDevice::_RemoveDevice(const char* path)
{
	MID_CALLED();

	BAutolock _(fDeviceListLock);

	MouseDevice* device = _FindDevice(path);
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
