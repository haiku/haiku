/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <Joystick.h>

BJoystick::BJoystick()
{
}


BJoystick::~BJoystick()
{
}


status_t
BJoystick::Open(const char *portName)
{
	return B_ERROR;
}


status_t
BJoystick::Open(const char *portName,
				bool enter_enhanced)
{
	return B_ERROR;
}


void
BJoystick::Close(void)
{
}


status_t
BJoystick::Update(void)
{
	return B_ERROR;
}


status_t
BJoystick::SetMaxLatency(bigtime_t max_latency)
{
	return B_ERROR;
}


int32
BJoystick::CountDevices()
{
	return 0;
}


status_t
BJoystick::GetDeviceName(int32 n,
						 char *name,
						 size_t bufSize)
{
	return B_ERROR;
}


bool
BJoystick::EnterEnhancedMode(const entry_ref *ref)
{
	return false;
}


int32
BJoystick::CountSticks()
{
	return 0;
}


int32
BJoystick::CountAxes()
{
	return 0;
}


int32
BJoystick::CountHats()
{
	return 0;
}


int32
BJoystick::CountButtons()
{
	return 0;
}


status_t
BJoystick::GetAxisValues(int16 *out_values,
						 int32 for_stick)
{
	return B_ERROR;
}


status_t
BJoystick::GetHatValues(uint8 *out_hats,
						int32 for_stick)
{
	return B_ERROR;
}


uint32
BJoystick::ButtonValues(int32 for_stick)
{
	return 0;
}


status_t
BJoystick::GetAxisNameAt(int32 index,
						 BString *out_name)
{
	return B_ERROR;
}


status_t
BJoystick::GetHatNameAt(int32 index,
						BString *out_name)
{
	return B_ERROR;
}


status_t
BJoystick::GetButtonNameAt(int32 index,
						   BString *out_name)
{
	return B_ERROR;
}


status_t
BJoystick::GetControllerModule(BString *out_name)
{
	return B_ERROR;
}


status_t
BJoystick::GetControllerName(BString *out_name)
{
	return B_ERROR;
}


bool
BJoystick::IsCalibrationEnabled()
{
	return false;
}


status_t
BJoystick::EnableCalibration(bool calibrates)
{
	return false;
}


void
BJoystick::Calibrate(struct _extended_joystick *reading)
{
}


status_t
BJoystick::gather_enhanced_info(const entry_ref *ref)
{
	return B_ERROR;
}


status_t
BJoystick::save_config(const entry_ref *ref)
{
	return B_ERROR;
}


void
BJoystick::_ReservedJoystick1()
{
}


void
BJoystick::_ReservedJoystick2()
{
}


void
BJoystick::_ReservedJoystick3()
{
}


status_t
BJoystick::_Reserved_Joystick_4(void *, ...)
{
	return B_ERROR;
}


status_t
BJoystick::_Reserved_Joystick_5(void *, ...)
{
	return B_ERROR;
}


status_t
BJoystick::_Reserved_Joystick_6(void *, ...)
{
	return B_ERROR;
}


