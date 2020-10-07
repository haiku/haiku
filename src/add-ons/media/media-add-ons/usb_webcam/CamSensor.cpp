/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */

#include "CamSensor.h"
#include "CamDebug.h"


CamSensor::CamSensor(CamDevice *_camera)
	: fInitStatus(B_NO_INIT),
	  fIsBigEndian(false),
	  fTransferEnabled(false),
	  fVideoFrame(),
	  fLastParameterChanges(0),
	  fCamDevice(_camera)
{

}


CamSensor::~CamSensor()
{

}


status_t
CamSensor::Probe()
{
	// default is to match by USB IDs
	return B_OK;
}


status_t
CamSensor::InitCheck()
{
	return fInitStatus;
}


status_t
CamSensor::Setup()
{
	return fInitStatus;
}


const char *
CamSensor::Name()
{
	return "<unknown>";
}


status_t
CamSensor::StartTransfer()
{
	fTransferEnabled = true;
	return B_OK;
}


status_t
CamSensor::StopTransfer()
{
	fTransferEnabled = false;
	return B_OK;
}


status_t
CamSensor::AcceptVideoFrame(uint32 &width, uint32 &height)
{
	// minimum sanity
	if (width < 1)
		width = MaxWidth();
	if (height < 1)
		height = MaxHeight();
	if (width > MaxWidth())
		width = MaxWidth();
	if (height > MaxHeight())
		height = MaxHeight();
	return B_OK;
}


status_t
CamSensor::SetVideoFrame(BRect rect)
{
	return ENOSYS;
}


status_t
CamSensor::SetVideoParams(float brightness, float contrast, float hue, float red, float green, float blue)
{
	return ENOSYS;
}


void
CamSensor::AddParameters(BParameterGroup *group, int32 &index)
{
	fFirstParameterID = index;
}

status_t
CamSensor::GetParameterValue(int32 id, bigtime_t *last_change, void *value, size_t *size)
{
	return B_BAD_VALUE;
}

status_t
CamSensor::SetParameterValue(int32 id, bigtime_t when, const void *value, size_t size)
{
	return B_BAD_VALUE;
}


CamDevice *
CamSensor::Device()
{
	return fCamDevice;
}


status_t
CamSensor::ProbeByIICSignature(const uint8 *regList, const uint8 *matchList,
	size_t count)
{
	for (int i = 0; i < count; i++) {
		uint8 value = 0;
		ssize_t len;
		len = Device()->ReadIIC8(regList[i], &value);
		PRINT((CH ": ReadIIC8 = %" B_PRIdSSIZE " val = %d" CT, len, value));
		if (len < 1)
			return ENODEV;
		if (value != matchList[i])
			return ENODEV;
	}
	return B_OK;
}
