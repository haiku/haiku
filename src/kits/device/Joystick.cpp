/*
 * Copyright 2002-2008, Marcus Overhagen, Stefano Ceccherini, Fredrik Modéen.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*
joystick preference app
	JoyCalib::JoyCalib(BRect, BJoystick &, BWindow *):
__8JoyCalibG5BRectR9BJoystickP7BWindow:
*/

#include <List.h>
#include <Path.h>
#include <Directory.h>

#include <String.h>
#include <Debug.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

#include <sys/ioctl.h>

#include "Joystick.h"

#if DEBUG
inline void
LOG(const char *fmt, ...)
{
	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	va_end(ap);
	fputs(buf, BJoystick::sLogFile); fflush(BJoystick::sLogFile);
}
#	define LOG_ERR(text...) LOG(text)
FILE *BJoystick::sLogFile = NULL;
#else
#	define LOG(text...)
#	define LOG_ERR(text...) fprintf(stderr, text)
#endif

#define CALLED() LOG("%s\n", __PRETTY_FUNCTION__)

#include "JoystickTweaker.h"

BJoystick::BJoystick()
	:
	_mBeBoxMode(false),
	_fDevices(new BList),
	m_info(new _joystick_info())
{
#if DEBUG
	sLogFile = fopen("/var/log/libdevice.log", "a");
#endif
	//ScanDevices();
}


BJoystick::~BJoystick()
{
	if (ffd >= 0)
		close(ffd);

	for (int32 count = _fDevices->CountItems() - 1; count >= 0; count--) {
		free(_fDevices->RemoveItem(count));
	}

	delete _fDevices;
	delete m_info;
}


status_t
BJoystick::Open(const char *portName)
{
	CALLED();
	return Open(portName, true);
}


status_t
BJoystick::Open(const char *portName, bool enter_enhanced)
{
	CALLED();
	char buf[64];

	if(!enter_enhanced)
		_mBeBoxMode = !enter_enhanced;

	if (portName == NULL)
		return B_BAD_VALUE;

	if (portName[0] != '/')
		snprintf(buf, 64, DEVICEPATH"/%s", portName);
	else
		snprintf(buf, 64, "%s", portName);

	if (ffd >= 0)
		close(ffd);

	// TODO: BeOS don't use O_EXCL, and this seems to lead to some issues. I
	// added this flag having read some comments by Marco Nelissen on the
	// annotated BeBook. I think BeOS uses O_RDWR|O_NONBLOCK here.
	ffd = open(buf, O_RDWR | O_NONBLOCK | O_EXCL);

	if (ffd >= 0) {
		// we used open() with O_NONBLOCK flag to let it return immediately,
		// but we want read/write operations to block if needed, so we clear
		// that bit here.
		int flags = fcntl(ffd, F_GETFL);
		fcntl(ffd, F_SETFL, flags & ~O_NONBLOCK);

		//DriverControl();
	}
	// TODO: I wonder why the return type is a status_t,
	// since we (as BeOS does) return the descriptor number for the device...

	//Read the Joystick Description file for this port/joystick
	_BJoystickTweaker jt(*this);
	jt.get_info(m_info, portName);

	LOG("ioctl - %d\n", m_info->num_buttons);
	ioctl(ffd, B_JOYSTICK_SET_DEVICE_MODULE, m_info);
	ioctl(ffd, B_JOYSTICK_GET_DEVICE_MODULE, m_info);
	LOG("ioctl - %d\n", m_info->num_buttons);
	if (ffd >= 0) {
		return ffd;
	} else
		return errno;
}


void
BJoystick::Close(void)
{
	CALLED();
	if (ffd >= 0)
		close(ffd);
	ffd = -1;
}


void
BJoystick::ScanDevices(bool use_disabled)
{
	CALLED();
	if (use_disabled) {
		_BJoystickTweaker temp(*this);
		temp.scan_including_disabled();
	}
}


int32
BJoystick::CountDevices()
{
	CALLED();
	int32 count = 0;

	// Refresh devices list
	ScanDevices(true);

	if (_fDevices != NULL)
		count = _fDevices->CountItems();

	LOG("Count = %d\n", count);
	return count;
}


status_t
BJoystick::GetDeviceName(int32 n, char *name, size_t bufSize)
{
	CALLED();
	BString *temp = new BString();
	if (_fDevices != NULL && _fDevices->CountItems() > n)
		temp = static_cast<BString*>(_fDevices->ItemAt(n));
	else
		return B_BAD_INDEX;

	if (temp != NULL && name != NULL) {
		if(temp->Length() > bufSize)
			return B_NAME_TOO_LONG;
		strncpy(name, temp->String(), bufSize);
		name[bufSize - 1] = '\0';
		LOG("Device Name = %s\n", name);
		return B_OK;
	}
	return B_ERROR;
}


bool
BJoystick::EnterEnhancedMode(const entry_ref *ref)
{
	CALLED();
	_mBeBoxMode = false;
	return !_mBeBoxMode;
}


int32
BJoystick::CountSticks()
{
	CALLED();
	return m_info->num_sticks;
}


int32
BJoystick::CountAxes()
{
	CALLED();
	return m_info->num_axes;
}


int32
BJoystick::CountHats()
{
	CALLED();
	return m_info->num_hats;
}


int32
BJoystick::CountButtons()
{
	CALLED();
	return m_info->num_buttons;
}

status_t
BJoystick::GetControllerModule(BString *out_name)
{
	CALLED();
	if (m_info != NULL && ffd >= 0) {
		out_name->SetTo(m_info->module_name);
		return B_OK;
	} else
		return B_ERROR;

}


status_t
BJoystick::GetControllerName(BString *out_name)
{
	CALLED();
	if (m_info != NULL && ffd >= 0) {
		out_name->SetTo(m_info->controller_name);
		return B_OK;
	} else
		return B_ERROR;
}


bool
BJoystick::IsCalibrationEnabled()
{
	CALLED();
	return m_info->calibration_enable;
}


status_t
BJoystick::EnableCalibration(bool calibrates)
{
	CALLED();
	if(ffd >= 0) {
		m_info->calibration_enable = calibrates;
		return B_OK;
	} else
		return B_NO_INIT;
}


status_t
BJoystick::SetMaxLatency(bigtime_t max_latency)
{
	CALLED();
	m_info->max_latency = max_latency;
	 //else B_ERROR (when?)
	return B_OK;
}

//--------- not done -------------------

status_t
BJoystick::GetAxisNameAt(int32 index, BString *out_name)
{
	CALLED();
	return B_BAD_INDEX;
}


status_t
BJoystick::GetHatNameAt(int32 index, BString *out_name)
{
	CALLED();
	return B_BAD_INDEX;
}


status_t
BJoystick::GetButtonNameAt(int32 index, BString *out_name)
{
	CALLED();
	return B_BAD_INDEX;
}


status_t
BJoystick::GetAxisValues(int16 *out_values, int32 for_stick)
{
	CALLED();
	return B_BAD_VALUE;
}


status_t
BJoystick::GetHatValues(uint8 *out_hats, int32 for_stick)
{
	CALLED();
	return B_BAD_VALUE;
}


uint32
BJoystick::ButtonValues(int32 for_stick)
{
	CALLED();
	return 0;
}


status_t
BJoystick::Update(void)
{
	CALLED();
	if(ffd >= 0) {
		return B_OK;
	} else
		return B_ERROR;
}


void
BJoystick::Calibrate(struct _extended_joystick *reading)
{
	CALLED();
}


status_t
BJoystick::gather_enhanced_info(const entry_ref *ref)
{
	CALLED();
	return B_ERROR;
}


status_t
BJoystick::save_config(const entry_ref *ref)
{
	CALLED();
	return B_ERROR;
}


/* These functions are here to maintain Binary Compatibility */
void BJoystick::_ReservedJoystick1() {CALLED();}
void BJoystick::_ReservedJoystick2() {CALLED();}
void BJoystick::_ReservedJoystick3() {CALLED();}
status_t BJoystick::_Reserved_Joystick_4(void *, ...) {CALLED();return B_ERROR;}
status_t BJoystick::_Reserved_Joystick_5(void *, ...) {CALLED();return B_ERROR;}
status_t BJoystick::_Reserved_Joystick_6(void *, ...) {CALLED();return B_ERROR;}
