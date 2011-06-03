/*
 * Copyright 2002-2008, Marcus Overhagen, Stefano Ceccherini, Fredrik Modéen.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Joystick.h>
#include <JoystickTweaker.h>

#include <new>
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
	button1(true),
	button2(true),

	fBeBoxMode(false),
	fFD(-1),
	fDevices(new(std::nothrow) BList),
	fJoystickInfo(new(std::nothrow) joystick_info),
	fJoystickData(new(std::nothrow) BList)
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

	if (fJoystickData != NULL) {
		for (int32 i = 0; i < fJoystickData->CountItems(); i++) {
			variable_joystick *variableJoystick
				= (variable_joystick *)fJoystickData->ItemAt(i);
			if (variableJoystick == NULL)
				continue;

			free(variableJoystick->data);
			delete variableJoystick;
		}

		delete fJoystickData;
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

	if (fJoystickInfo == NULL || fJoystickData == NULL)
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
	// annotated BeBook. I think BeOS uses O_RDWR | O_NONBLOCK here.
	fFD = open(nameBuffer, O_RDWR | O_NONBLOCK | O_EXCL);
	if (fFD < 0)
		return B_ERROR;

	// read the Joystick Description file for this port/joystick
	_BJoystickTweaker joystickTweaker(*this);
	joystickTweaker.GetInfo(fJoystickInfo, portName);

	LOG("ioctl - %d\n", fJoystickInfo->module_info.num_buttons);
	ioctl(fFD, B_JOYSTICK_SET_DEVICE_MODULE, &fJoystickInfo->module_info,
		sizeof(joystick_module_info));
	ioctl(fFD, B_JOYSTICK_GET_DEVICE_MODULE, &fJoystickInfo->module_info,
		sizeof(joystick_module_info));
	LOG("ioctl - %d\n", fJoystickInfo->module_info.num_buttons);

	// Allocate the variable_joystick structures to hold the info for each
	// "stick". Note that the whole num_sticks thing seems a bit bogus, as
	// all sticks would be required to have exactly the same attributes,
	// i.e. axis, hat and button counts, since there is only one global
	// joystick_info for the whole device. What's implemented here is a
	// "best guess", using the read position in Update() to select the
	// stick for which data shall be returned.
	bool supportsVariable
		= (fJoystickInfo->module_info.flags & js_flag_variable_size_reads) != 0;
	for (uint16 i = 0; i < fJoystickInfo->module_info.num_sticks; i++) {
		variable_joystick *variableJoystick
			= new(std::nothrow) variable_joystick;
		if (variableJoystick == NULL)
			return B_NO_MEMORY;

		if (supportsVariable) {
			// The driver supports arbitrary controls.
			variableJoystick->axis_count = fJoystickInfo->module_info.num_axes;
			variableJoystick->hat_count = fJoystickInfo->module_info.num_hats;
			variableJoystick->button_blocks
				= (fJoystickInfo->module_info.num_buttons + 31) / 32;
		} else {
			// The driver doesn't support our variable requests so we construct
			// a data structure that is compatible with extended_joystick and
			// just use that in reads. This allows us to use a single data
			// format internally but be compatible with both inputs.
			variableJoystick->axis_count = MAX_AXES;
			variableJoystick->hat_count = MAX_HATS;
			variableJoystick->button_blocks = (MAX_BUTTONS + 31) / 32;

			// Also ensure that we don't read over those boundaries.
			if (fJoystickInfo->module_info.num_axes > MAX_AXES)
				fJoystickInfo->module_info.num_axes = MAX_AXES;
			if (fJoystickInfo->module_info.num_hats > MAX_HATS)
				fJoystickInfo->module_info.num_hats = MAX_HATS;
			if (fJoystickInfo->module_info.num_buttons > MAX_BUTTONS)
				fJoystickInfo->module_info.num_buttons = MAX_BUTTONS;
		}

		variableJoystick->data_size = sizeof(bigtime_t)			// timestamp
			+ variableJoystick->button_blocks * sizeof(uint32)	// bitmaps
			+ variableJoystick->axis_count * sizeof(int16)		// axis values
			+ variableJoystick->hat_count * sizeof(uint8);		// hat values

		variableJoystick->data
			= (uint8 *)malloc(variableJoystick->data_size);
		if (variableJoystick->data == NULL) {
			delete variableJoystick;
			return B_NO_MEMORY;
		}

		// fill in the convenience pointers
		variableJoystick->timestamp = (bigtime_t *)variableJoystick->data;
		variableJoystick->buttons = (uint32 *)(&variableJoystick->timestamp[1]);
		variableJoystick->axes = (int16 *)(&variableJoystick->buttons[
			variableJoystick->button_blocks]);
		variableJoystick->hats = (uint8 *)(&variableJoystick->axes[
			variableJoystick->axis_count]);

		if (!fJoystickData->AddItem(variableJoystick)) {
			free(variableJoystick->data);
			delete variableJoystick;
			return B_NO_MEMORY;
		}
	}

	return fFD;
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


status_t
BJoystick::Update()
{
	CALLED();
	if (fJoystickInfo == NULL || fJoystickData == NULL || fFD < 0)
		return B_NO_INIT;

	for (uint16 i = 0; i < fJoystickInfo->module_info.num_sticks; i++) {
		variable_joystick *values
			= (variable_joystick *)fJoystickData->ItemAt(i);
		if (values == NULL)
			return B_NO_INIT;

		ssize_t result = read_pos(fFD, i, values->data,
			values->data_size);
		if (result < 0)
			return result;

		if ((size_t)result != values->data_size)
			return B_ERROR;

		if (i > 0)
			continue;

		// fill in the legacy values for the first stick
		timestamp = *values->timestamp;

		if (values->axis_count >= 1)
			horizontal = values->axes[0];
		else
			horizontal = 0;

		if (values->axis_count >= 2)
			vertical = values->axes[1];
		else
			vertical = 0;

		if (values->button_blocks > 0) {
			button1 = (*values->buttons & 1) == 0;
			button2 = (*values->buttons & 2) == 0;
		} else {
			button1 = true;
			button2 = true;
		}
	}

	return B_OK;
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


status_t
BJoystick::GetAxisValues(int16 *outValues, int32 forStick)
{
	CALLED();

	if (fJoystickInfo == NULL || fJoystickData == NULL)
		return B_NO_INIT;

	if (forStick < 0
		|| forStick >= (int32)fJoystickInfo->module_info.num_sticks)
		return B_BAD_INDEX;

	variable_joystick *variableJoystick
		= (variable_joystick *)fJoystickData->ItemAt(forStick);
	if (variableJoystick == NULL)
		return B_NO_INIT;

	memcpy(outValues, variableJoystick->axes,
		fJoystickInfo->module_info.num_axes * sizeof(uint16));
	return B_OK;
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


int32
BJoystick::CountHats()
{
	CALLED();
	if (fJoystickInfo == NULL)
		return 0;

	return fJoystickInfo->module_info.num_hats;
}


status_t
BJoystick::GetHatValues(uint8 *outHats, int32 forStick)
{
	CALLED();

	if (fJoystickInfo == NULL || fJoystickData == NULL)
		return B_NO_INIT;

	if (forStick < 0
		|| forStick >= (int32)fJoystickInfo->module_info.num_sticks)
		return B_BAD_INDEX;

	variable_joystick *variableJoystick
		= (variable_joystick *)fJoystickData->ItemAt(forStick);
	if (variableJoystick == NULL)
		return B_NO_INIT;

	memcpy(outHats, variableJoystick->hats,
		fJoystickInfo->module_info.num_hats);
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


int32
BJoystick::CountButtons()
{
	CALLED();
	if (fJoystickInfo == NULL)
		return 0;

	return fJoystickInfo->module_info.num_buttons;
}


uint32
BJoystick::ButtonValues(int32 forStick)
{
	CALLED();

	if (fJoystickInfo == NULL || fJoystickData == NULL)
		return 0;

	if (forStick < 0
		|| forStick >= (int32)fJoystickInfo->module_info.num_sticks)
		return 0;

	variable_joystick *variableJoystick
		= (variable_joystick *)fJoystickData->ItemAt(forStick);
	if (variableJoystick == NULL || variableJoystick->button_blocks == 0)
		return 0;

	return *variableJoystick->buttons;
}


status_t
BJoystick::GetButtonValues(bool *outButtons, int32 forStick)
{
	CALLED();

	if (fJoystickInfo == NULL || fJoystickData == NULL)
		return B_NO_INIT;

	if (forStick < 0
		|| forStick >= (int32)fJoystickInfo->module_info.num_sticks)
		return B_BAD_INDEX;

	variable_joystick *variableJoystick
		= (variable_joystick *)fJoystickData->ItemAt(forStick);
	if (variableJoystick == NULL)
		return B_NO_INIT;

	int16 buttonCount = fJoystickInfo->module_info.num_buttons;
	for (int16 i = 0; i < buttonCount; i++) {
		outButtons[i]
			= (variableJoystick->buttons[i / 32] & (1 << (i % 32))) != 0;
	}

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


void
BJoystick::Calibrate(struct _extended_joystick *reading)
{
	CALLED();
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


//	#pragma mark - FBC protection


void BJoystick::_ReservedJoystick1() {}
void BJoystick::_ReservedJoystick2() {}
void BJoystick::_ReservedJoystick3() {}
status_t BJoystick::_Reserved_Joystick_4(void*, ...) { return B_ERROR; }
status_t BJoystick::_Reserved_Joystick_5(void*, ...) { return B_ERROR; }
status_t BJoystick::_Reserved_Joystick_6(void*, ...) { return B_ERROR; }
