/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */

#include "CamSensor.h"
#include "CamDebug.h"

#define PB_ADDR_QC	0xba

#define PB_IDENT	0x00


class PB0100Sensor : public CamSensor {
public:
	PB0100Sensor(CamDevice *_camera);
	~PB0100Sensor();
	virtual status_t	Probe();

	virtual uint8		IICReadAddress() const { return PB_ADDR_QC; };
	virtual uint8		IICWriteAddress() const { return PB_ADDR_QC; };
};


PB0100Sensor::PB0100Sensor(CamDevice *_camera)
: CamSensor(_camera)
{
		Device()->SetIICBitsMode(16);

}


PB0100Sensor::~PB0100Sensor()
{
}


status_t
PB0100Sensor::Probe()
{
	status_t err;
	uint16 data;
	PRINT((CH "()" CT));
	Device()->SetIICBitsMode(16);
	// QuickCam only ?
	err = Device()->ReadIIC16(PB_IDENT, &data);
	if (err < B_OK)
		return ENODEV;
	if (data == 0x64) {
		PRINT((CH ": found %s sensor!" CT, Name()));
		return B_OK;
	}
	return ENODEV;
}
					

B_WEBCAM_DECLARE_SENSOR(PB0100Sensor, pb0100)

