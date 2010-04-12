/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */

#include "CamSensor.h"

static const uint8 sProbeAddressList[] = {
	0x00,0x01,0x02,0x11,0x13,0x14,0x15,0x16,0x17
};
static const uint8 sProbeMatchList[] = {
	0x02,0x09,0x01,0x02,0x02,0x01,0xe2,0x02,0x82
};

class HV7131E1Sensor : public CamSensor {
public:
	HV7131E1Sensor(CamDevice *_camera);
	~HV7131E1Sensor();
	status_t	Probe();
	const char*	Name();

//XXX not sure
	virtual bool		Use400kHz() const { return false; };
	virtual bool		UseRealIIC() const { return false; };

};


HV7131E1Sensor::HV7131E1Sensor(CamDevice *_camera)
	: CamSensor(_camera)
{

}


HV7131E1Sensor::~HV7131E1Sensor()
{
}


status_t
HV7131E1Sensor::Probe()
{
	return ProbeByIICSignature(sProbeAddressList, sProbeMatchList,
		sizeof(sProbeMatchList));
}


const char *
HV7131E1Sensor::Name()
{
	return "Hynix hv7131e1";
}


B_WEBCAM_DECLARE_SENSOR(HV7131E1Sensor, hv7131e1)

