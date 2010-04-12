/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */

#include "CamSensor.h"
#include "CamDebug.h"

#define HDCS_ADDR_QC	0xaa

#define HDCS_IDENT	0x00

class HDCS1000Sensor : public CamSensor {
public:
	HDCS1000Sensor(CamDevice *_camera);
	~HDCS1000Sensor();

	virtual status_t	Probe();

	virtual uint8		IICReadAddress() const { return HDCS_ADDR_QC; };
	virtual uint8		IICWriteAddress() const { return HDCS_ADDR_QC; };
};


HDCS1000Sensor::HDCS1000Sensor(CamDevice *_camera)
	: CamSensor(_camera)
{
}


HDCS1000Sensor::~HDCS1000Sensor()
{
}


status_t
HDCS1000Sensor::Probe()
{
	status_t err;
	uint8 data;
	PRINT((CH "()" CT));
	Device()->SetIICBitsMode(8);
	// QuickCam only ?
	err = Device()->ReadIIC8(HDCS_IDENT+1, &data);
	if (err < B_OK)
		return ENODEV;
	if (data == 8) {
		PRINT((CH ": found %s sensor!" CT, Name()));
		return B_OK;
	}
	return ENODEV;
}


B_WEBCAM_DECLARE_SENSOR(HDCS1000Sensor, hdcs1000)

