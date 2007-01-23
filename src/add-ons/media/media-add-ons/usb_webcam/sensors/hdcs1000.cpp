/*
*/

#include "CamSensor.h"

class HDCS1000Sensor : public CamSensor {
public:
	HDCS1000Sensor(CamDevice *_camera);
	~HDCS1000Sensor();
};

// -----------------------------------------------------------------------------
HDCS1000Sensor::HDCS1000Sensor(CamDevice *_camera)
: CamSensor(_camera)
{
	
}

// -----------------------------------------------------------------------------
HDCS1000Sensor::~HDCS1000Sensor()
{
}

// -----------------------------------------------------------------------------
B_WEBCAM_DECLARE_SENSOR(HDCS1000Sensor, hdcs1000)

