/*
 * Copyright 2002-2008, Marcus Overhagen, Stefano Ceccherini, Fredrik Modéen.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Joystick.h>
#include <JoystickTweaker.h>

#include <new>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

#include <sys/ioctl.h>

#include <Debug.h>
#include <Directory.h>
#include <List.h>
#include <Path.h>
#include <String.h>


#if DEBUG
static FILE *sLogFile = NULL;

inline void
LOG(const char *fmt, ...)
{
	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	va_end(ap);
	fputs(buf, sLogFile); fflush(sLogFile);
}

#	define LOG_ERR(text...) LOG(text)

#else
#	define LOG(text...)
#	define LOG_ERR(text...) fprintf(stderr, text)
#endif

#define CALLED() LOG("%s\n", __PRETTY_FUNCTION__)


BJoystick::BJoystick()
	:
	// legacy members for standard mode
	timestamp(0),
	horizontal(0),
	vertical(0),
	button1(0),
	button2(0),

	fBeBoxMode(false),
	fFD(-1),
	fDevices(new(std::nothrow) BList),
	fJoystickInfo(new(std::nothrow) joystick_info),
	fExtendedJoystick(new(std::nothrow) BList)
{
#if DEBUG
	sLogFile = fopen("/var/log/joystick.log", "a");
#endif

	if (fJoystickInfo != NULL)
		memset(fJoystickInfo, 0, sizeof(joystick_info));

	RescanDevices();
}


BJoystick::~BJoystick()
{
	if (fFD >= 0)
		close(fFD);

	if (fDevices != NULL) {
		for (int32 i = 0; i < fDevices->CountItems(); i++)
			delete (BString *)fDevices->ItemAt(i);

		delete fDevices;
	}

	delete fJoystickInfo;

	if (fExtendedJoystick != NULL) {
		for (int32 i = 0; i < fExtendedJoystick->CountItems(); i++)
			delete (extended_joystick *)fExtendedJoystick->ItemAt(i);

		delete fExtendedJoystick;
	}
}


status_t
BJoystick::Open(const char *portName)
{
	CALLED();
	return Open(portName, true);
}


status_t
BJoystick::Open(const char *portName, bool enhanced)
{
	CALLED();

	if (portName == NULL)
		return B_BAD_VALUE;

	if (fJoystickInfo == NULL || fExtendedJoystick == NULL)
		return B_NO_INIT;

	fBeBoxMode = !enhanced;

	char nameBuffer[64];
	if (portName[0] != '/') {
		snprintf(nameBuffer, sizeof(nameBuffer), DEVICE_BASE_PATH"/%s",
			portName);
	} else
		snprintf(nameBuffer, sizeof(nameBuffer), "%s", portName);

	if (fFD >= 0)
		close(fFD);

	// TODO: BeOS don't use O_EXCL, and this seems to lead to some issues. I
	// added this flag having read some comments by Marco Nelissen on the
	// annotated BeBook. I think BeOS uses O_RDWR|O_NONBLOCK here.
	fFD = open(nameBuffer, O_RDWR | O_NONBLOCK | O_EXCL);

	if (fFD >= 0) {
		// we used open() with O_NONBLOCK flag to let it return immediately,
		// but we want read/write operations to block if needed, so we clear
		// that bit here.
		int flags = fcntl(fFD, F_GETFL);
		fcntl(fFD, F_SETFL, flags & ~O_NONBLOCK);

		// read the Joystick Description file for this port/joystick
		_BJoystickTweaker joystickTweaker(*this);
		joystickTweaker.GetInfo(fJoystickInfo, portName);

		LOG("ioctl - %d\n", fJoystickInfo->module_info.num_buttons);
		ioctl(fFD, B_JOYSTICK_SET_DEVICE_MODULE, &fJoystickInfo->module_info,
			sizeof(joystick_module_info));
		ioctl(fFD, B_JOYSTICK_GET_DEVICE_MODULE, &fJoystickInfo->module_info,
			sizeof(joystick_module_info));
		LOG("ioctl - %d\n", fJoystickInfo->module_info.num_buttons);

		// Allocate the extended_joystick structures to hold the info for each
		// "stick". Note that the whole num_sticks thing seems a bit bogus, as
		// all sticks would be required to have exactly the same attributes,
		// i.e. axis, hat and button counts, since there is only one global
		// joystick_info for the whole device. What's implemented here is a
		// "best guess", using the read position in Update() to select the
		// stick for which an extended_joystick structure shall be returned.
		for (uint16 i = 0; i < fJoystickInfo->module_info.num_sticks; i++) {
			extended_joystick *extendedJoystick
				= new(std::nothrow) extended_joystick;
			if (extendedJoystick == NULL)
				return B_NO_MEMORY;

			if (!fExtendedJoystick->AddItem(extendedJoystick)) {
				delete extendedJoystick;
				return B_NO_MEMORY;
			}
		}

		return fFD;
	} else
		return errno;
}


void
BJoystick::Close(void)
{
	CALLED();
	if (fFD >= 0) {
		close(fFD);
		fFD = -1;
	}
}


void
BJoystick::ScanDevices(bool useDisabled)
{
	CALLED();
	if (useDisabled) {
		_BJoystickTweaker joystickTweaker(*this);
		joystickTweaker.scan_including_disabled();
	}
}


int32
BJoystick::CountDevices()
{
	CALLED();

	if (fDevices == NULL)
		return 0;

	int32 count = fDevices->CountItems();

	LOG("Count = %d\n", count);
	return count;
}


status_t
BJoystick::GetDeviceName(int32 index, char *name, size_t bufSize)
{
	CALLED();
	if (fDevices == NULL)
		return B_NO_INIT;

	if (index >= fDevices->CountItems())
		return B_BAD_INDEX;

	if (name == NULL)
		return B_BAD_VALUE;

	BString *deviceName = (BString *)fDevices->ItemAt(index);
	if (deviceName->Length() > (int32)bufSize)
		return B_NAME_TOO_LONG;

	strlcpy(name, deviceName->String(), bufSize);
	LOG("Device Name = %s\n", name);
	return B_OK;
}


status_t
BJoystick::RescanDevices()
{
	CALLED();

	if (fDevices == NULL)
		return B_NO_INIT;

	ScanDevices(true);
	return B_OK;
}


bool
BJoystick::EnterEnhancedMode(const entry_ref *ref)
{
	CALLED();
	fBeBoxMode = false;
	return !fBeBoxMode;
}


int32
BJoystick::CountSticks()
{
	CALLED();
	if (fJoystickInfo == NULL)
		return 0;

	return fJoystickInfo->module_info.num_sticks;
}


int32
BJoystick::CountAxes()
{
	CALLED();
	if (fJoystickInfo == NULL)
		return 0;

	return fJoystickInfo->module_info.num_axes;
}


int32
BJoystick::CountHats()
{
	CALLED();
	if (fJoystickInfo == NULL)
		return 0;

	return fJoystickInfo->module_info.num_hats;
}


int32
BJoystick::CountButtons()
{
	CALLED();
	if (fJoystickInfo == NULL)
		return 0;

	return fJoystickInfo->module_info.num_buttons;
}


status_t
BJoystick::GetControllerModule(BString *outName)
{
	CALLED();
	if (fJoystickInfo == NULL || fFD < 0)
		return B_NO_INIT;

	if (outName == NULL)
		return B_BAD_VALUE;

	outName->SetTo(fJoystickInfo->module_info.module_name);
	return B_OK;
}


status_t
BJoystick::GetControllerName(BString *outName)
{
	CALLED();
	if (fJoystickInfo == NULL || fFD < 0)
		return B_NO_INIT;

	if (outName == NULL)
		return B_BAD_VALUE;

	outName->SetTo(fJoystickInfo->module_info.device_name);
	return B_OK;
}


bool
BJoystick::IsCalibrationEnabled()
{
	CALLED();
	if (fJoystickInfo == NULL)
		return false;

	return fJoystickInfo->calibration_enable;
}


status_t
BJoystick::EnableCalibration(bool calibrates)
{
	CALLED();
	if (fJoystickInfo == NULL || fFD < 0)
		return B_NO_INIT;

	status_t result = ioctl(fFD, B_JOYSTICK_SET_RAW_MODE, &calibrates,
		sizeof(calibrates));
	if (result == B_OK)
		fJoystickInfo->calibration_enable = calibrates;

	return result;
}


status_t
BJoystick::SetMaxLatency(bigtime_t maxLatency)
{
	CALLED();
	if (fJoystickInfo == NULL || fFD < 0)
		return B_NO_INIT;

	status_t result = ioctl(fFD, B_JOYSTICK_SET_MAX_LATENCY, &maxLatency,
		sizeof(maxLatency));
	if (result == B_OK)
		fJoystickInfo->max_latency = maxLatency;

	return result;
}


status_t
BJoystick::GetAxisNameAt(int32 index, BString *outName)
{
	CALLED();

	if (index >= CountAxes())
		return B_BAD_INDEX;

	if (outName == NULL)
		return B_BAD_VALUE;

	// TODO: actually retrieve the name from the driver (via a new ioctl)
	*outName = "Axis ";
	*outName << index;
	return B_OK;
}


status_t
BJoystick::GetHatNameAt(int32 index, BString *outName)
{
	CALLED();

	if (index >= CountHats())
		return B_BAD_INDEX;

	if (outName == NULL)
		return B_BAD_VALUE;

	// TODO: actually retrieve the name from the driver (via a new ioctl)
	*outName = "Hat ";
	*outName << index;
	return B_OK;
}


status_t
BJoystick::GetButtonNameAt(int32 index, BString *outName)
{
	CALLED();

	if (index >= CountButtons())
		return B_BAD_INDEX;

	if (outName == NULL)
		return B_BAD_VALUE;

	// TODO: actually retrieve the name from the driver (via a new ioctl)
	*outName = "Button ";
	*outName << index;
	return B_OK;
}


status_t
BJoystick::GetAxisValues(int16 *outValues, int32 forStick)
{
	CALLED();

	if (fJoystickInfo == NULL || fExtendedJoystick == NULL)
		return B_NO_INIT;

	if (forStick < 0
		|| forStick >= (int32)fJoystickInfo->module_info.num_sticks)
		return B_BAD_INDEX;

	extended_joystick *extendedJoystick
		= (extended_joystick *)fExtendedJoystick->ItemAt(forStick);
	if (extendedJoystick == NULL)
		return B_NO_INIT;

	memcpy(outValues, extendedJoystick->axes,
		fJoystickInfo->module_info.num_axes * sizeof(uint16));
	return B_OK;
}


status_t
BJoystick::GetHatValues(uint8 *outHats, int32 forStick)
{
	CALLED();

	if (fJoystickInfo == NULL || fExtendedJoystick == NULL)
		return B_NO_INIT;

	if (forStick < 0
		|| forStick >= (int32)fJoystickInfo->module_info.num_sticks)
		return B_BAD_INDEX;

	extended_joystick *extendedJoystick
		= (extended_joystick *)fExtendedJoystick->ItemAt(forStick);
	if (extendedJoystick == NULL)
		return B_NO_INIT;

	memcpy(outHats, extendedJoystick->hats,
		fJoystickInfo->module_info.num_hats);
	return B_OK;
}


uint32
BJoystick::ButtonValues(int32 forStick)
{
	CALLED();

	if (fJoystickInfo == NULL || fExtendedJoystick == NULL)
		return 0;

	if (forStick < 0
		|| forStick >= (int32)fJoystickInfo->module_info.num_sticks)
		return 0;

	extended_joystick *extendedJoystick
		= (extended_joystick *)fExtendedJoystick->ItemAt(forStick);
	if (extendedJoystick == NULL)
		return 0;

	return extendedJoystick->buttons;
}


status_t
BJoystick::Update()
{
	CALLED();
	if (fJoystickInfo == NULL || fExtendedJoystick == NULL || fFD < 0)
		return B_NO_INIT;

	for (uint16 i = 0; i < fJoystickInfo->module_info.num_sticks; i++) {
		extended_joystick *extendedJoystick
			= (extended_joystick *)fExtendedJoystick->ItemAt(i);
		if (extendedJoystick == NULL)
			return B_NO_INIT;

		ssize_t result = read_pos(fFD, i, extendedJoystick,
			sizeof(extended_joystick));
		if (result < 0)
			return result;

		if (result != sizeof(extended_joystick))
			return B_ERROR;

		if (i > 0)
			continue;

		// fill in the legacy values for the first stick
		timestamp = extendedJoystick->timestamp;
		horizontal = extendedJoystick->axes[0];
		vertical = extendedJoystick->axes[1];
		button1 = (extendedJoystick->buttons & 1) == 0;
		button2 = (extendedJoystick->buttons & 2) == 0;
	}

	return B_OK;
}


void
BJoystick::Calibrate(struct _extended_joystick *reading)
{
	CALLED();
}


status_t
BJoystick::GatherEnhanced_info(const entry_ref *ref)
{
	CALLED();
	return B_ERROR;
}


status_t
BJoystick::SaveConfig(const entry_ref *ref)
{
	CALLED();
	return B_ERROR;
}


//	#pragma mark - FBC protection


void BJoystick::_ReservedJoystick1() {}
void BJoystick::_ReservedJoystick2() {}
void BJoystick::_ReservedJoystick3() {}
status_t BJoystick::_Reserved_Joystick_4(void*, ...) { return B_ERROR; }
status_t BJoystick::_Reserved_Joystick_5(void*, ...) { return B_ERROR; }
status_t BJoystick::_Reserved_Joystick_6(void*, ...) { return B_ERROR; }
