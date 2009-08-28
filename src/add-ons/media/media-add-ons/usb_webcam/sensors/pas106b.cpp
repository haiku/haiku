/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */

#include "CamDebug.h"
#include "CamSensor.h"
#include "addons/sonix/SonixCamDevice.h"

//XXX: unfinished!

static const uint8 sProbeAddressList[] = {
	0x01,0x02,0x03,0x04,0x05,0x06,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x10
};
static const uint8 sProbeMatchList[] = {
	0x70,0x02,0x12,0x05,0x05,0x06,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x06
};

class PAS106BSensor : public CamSensor {
public:
	PAS106BSensor(CamDevice *_camera);
	~PAS106BSensor();
	status_t	Probe();
	const char*	Name();

	virtual bool		Use400kHz() const { return true; }; // supports both
	virtual bool		UseRealIIC() const { return true; };

private:
	bool	fIsSonix;
};


PAS106BSensor::PAS106BSensor(CamDevice *_camera)
: CamSensor(_camera)
{
	fIsSonix = (dynamic_cast<SonixCamDevice *>(_camera) != NULL);
	if (fIsSonix) {
		fInitStatus = B_OK;
	} else {
		PRINT((CH ": unknown camera device!" CT));
		fInitStatus = ENODEV;
	}
}


PAS106BSensor::~PAS106BSensor()
{
}


status_t
PAS106BSensor::Probe()
{
	Device()->PowerOnSensor(false);
	Device()->PowerOnSensor(true);
	if (fIsSonix) {
		Device()->WriteReg8(SN9C102_CHIP_CTRL, 0x00);	/* power on the sensor, Fsys_clk=12MHz */
		Device()->WriteReg8(SN9C102_CLOCK_SEL, 0x17);	/* enable sensor, force 24MHz */
	}

	return ProbeByIICSignature(sProbeAddressList, sProbeMatchList, 
		sizeof(sProbeMatchList));
}


const char *
PAS106BSensor::Name()
{
	return "PixArt Imaging pas106b";
}


B_WEBCAM_DECLARE_SENSOR(PAS106BSensor, pas106b)

