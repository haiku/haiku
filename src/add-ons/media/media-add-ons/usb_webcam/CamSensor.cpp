#include "CamSensor.h"
#include "CamDebug.h"

// -----------------------------------------------------------------------------
CamSensor::CamSensor(CamDevice *_camera)
	: fInitStatus(B_NO_INIT),
	  fTransferEnabled(false),
	  fVideoFrame(),
	  fCamDevice(_camera)
{
	
}

// -----------------------------------------------------------------------------
CamSensor::~CamSensor()
{
	
}
					
// -----------------------------------------------------------------------------
status_t
CamSensor::InitCheck()
{
	return fInitStatus;
}
					
// -----------------------------------------------------------------------------
status_t
CamSensor::Setup()
{
	return fInitStatus;
}
					
// -----------------------------------------------------------------------------
const char *
CamSensor::Name()
{
	return "<unknown>";
}

// -----------------------------------------------------------------------------
status_t
CamSensor::StartTransfer()
{
	fTransferEnabled = true;
	return B_OK;
}

// -----------------------------------------------------------------------------
status_t
CamSensor::StopTransfer()
{
	fTransferEnabled = false;
	return B_OK;
}

// -----------------------------------------------------------------------------
status_t
CamSensor::SetVideoFrame(BRect rect)
{
	return ENOSYS;
}

// -----------------------------------------------------------------------------
status_t
CamSensor::SetVideoParams(float brightness, float contrast, float hue, float red, float green, float blue)
{
	return ENOSYS;
}

// -----------------------------------------------------------------------------
CamDevice *
CamSensor::Device()
{
	return fCamDevice;
}
