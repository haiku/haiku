/*
*/

#include "CamSensor.h"

class HV7131E1Sensor : public CamSensor {
public:
	HV7131E1Sensor(CamDevice *_camera);
	~HV7131E1Sensor();
	const char*	Name();

};

// -----------------------------------------------------------------------------
HV7131E1Sensor::HV7131E1Sensor(CamDevice *_camera)
: CamSensor(_camera)
{
	
}

// -----------------------------------------------------------------------------
HV7131E1Sensor::~HV7131E1Sensor()
{
}

// -----------------------------------------------------------------------------
const char *
HV7131E1Sensor::Name()
{
	return "Hynix hv7131e1";
}

// -----------------------------------------------------------------------------
B_WEBCAM_DECLARE_SENSOR(HV7131E1Sensor, hv7131e1)

