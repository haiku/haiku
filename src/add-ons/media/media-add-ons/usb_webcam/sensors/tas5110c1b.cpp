/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */

#include <ParameterWeb.h>

#include "CamSensor.h"
#include "CamDebug.h"
#include "addons/sonix/SonixCamDevice.h"

//#define ENABLE_GAIN 1


class TAS5110C1BSensor : public CamSensor {
public:
	TAS5110C1BSensor(CamDevice *_camera);
	~TAS5110C1BSensor();
	virtual status_t	Probe();
	virtual status_t	Setup();
	const char *Name();
	virtual bool		Use400kHz() const { return false; };
	virtual bool		UseRealIIC() const { return true /*false*/; };
	virtual uint8		IICReadAddress() const { return 0x00; };
	virtual uint8		IICWriteAddress() const { return 0x61; /*0x11;*/ /*0xff;*/ };

	virtual int			MaxWidth() const { return 352; };
	virtual int			MaxHeight() const { return 288; };

	virtual status_t	AcceptVideoFrame(uint32 &width, uint32 &height);
	virtual status_t	SetVideoFrame(BRect rect);
	virtual void		AddParameters(BParameterGroup *group, int32 &firstID);
	virtual status_t	GetParameterValue(int32 id, bigtime_t *last_change, void *value, size_t *size);
	virtual status_t	SetParameterValue(int32 id, bigtime_t when, const void *value, size_t size);

private:
	bool	fIsSonix;
	float	fGain;
};


TAS5110C1BSensor::TAS5110C1BSensor(CamDevice *_camera)
: CamSensor(_camera)
{
	fIsSonix = (dynamic_cast<SonixCamDevice *>(_camera) != NULL);
	if (fIsSonix) {
		fInitStatus = B_OK;
	} else {
		PRINT((CH ": unknown camera device!" CT));
		fInitStatus = ENODEV;
	}
	fGain = (float)0x40; // default
}


TAS5110C1BSensor::~TAS5110C1BSensor()
{
}


status_t
TAS5110C1BSensor::Probe()
{
	PRINT((CH "()" CT));
	
	return B_OK;
}


status_t
TAS5110C1BSensor::Setup()
{
	PRINT((CH "()" CT));
	if (InitCheck())
		return InitCheck();
	Device()->PowerOnSensor(false);
	Device()->PowerOnSensor(true);
	if (fIsSonix) {
#if 1
		Device()->WriteReg8(SN9C102_CHIP_CTRL, 0x01);	/* power down the sensor */
		Device()->WriteReg8(SN9C102_CHIP_CTRL, 0x44);	/* power up the sensor, enable tx, sysclk@24MHz */
		Device()->WriteReg8(SN9C102_CHIP_CTRL, 0x04);	/* power up the sensor, enable tx, sysclk@24MHz */
		Device()->WriteReg8(SN9C102_R_B_GAIN, 0x00);	/* red, blue gain = 1+0/8 = 1 */
		Device()->WriteReg8(SN9C102_G_GAIN, 0x00);	/* green gain = 1+0/8 = 1 */
		Device()->WriteReg8(SN9C102_OFFSET, 0x0a);	/* 10 pix offset */
		Device()->WriteReg8(SN9C102_CLOCK_SEL, 0x60);	/* enable sensor clk, and invert it */
		Device()->WriteReg8(SN9C102_CLOCK_SEL, 0x60);	/* enable sensor clk, and invert it */
		Device()->WriteReg8(SN9C102_SYNC_N_SCALE, 0x06);	/* no compression, normal curve, 
												 * no scaling, vsync active low,
												 * v/hsync change at rising edge,
												 * falling edge of sensor pck */
		Device()->WriteReg8(SN9C102_PIX_CLK, 0xfb);	/* pixclk = 2 * masterclk, sensor is slave mode */
		
		// some IIC stuff for the sensor
		// though it seems more data is sent than told the controller
		// this is what the XP driver sends to the ICECAM...
		uint8 tmp_7[] = { 0xb0, 0x61, 0x1c, 0xf8, 0x10, 0x00, 0x00, 0x16 };
		Device()->WriteReg(SN9C102_I2C_SETUP, tmp_7, 8);

		Device()->WriteReg8(SN9C102_PIX_CLK, 0x4b);

		uint8 tmp_8[] = { 0xa0, 0x61, 0x1c, 0x0f, 0x10, 0x00, 0x00, 0x16 };
		Device()->WriteReg(SN9C102_I2C_SETUP, tmp_8, 8);

#endif
	}
	
	
	if (fIsSonix) {
		//sonix_i2c_write_multi(dev, dev->sensor->i2c_wid, 2, 0xc0, 0x80, 0, 0, 0); /* AEC = 0x203 ??? */
		//Device()->WriteIIC8(0xc0, 0x80); /* AEC = 0x203 ??? */
		//Device()->WriteIIC8(0x1c, 0x80); /* AEC = 0x203 ??? */

		// set crop
		Device()->WriteReg8(SN9C102_H_SIZE, 69);
		Device()->WriteReg8(SN9C102_V_SIZE, 9);
		SetVideoFrame(BRect(0, 0, 352-1, 288-1));

	}

	//Device()->SetScale(1);
	
	return B_OK;
}
					

const char *
TAS5110C1BSensor::Name()
{
	return "TASC tas5110c1b";
}


status_t
TAS5110C1BSensor::AcceptVideoFrame(uint32 &width, uint32 &height)
{
	// default sanity checks
	status_t err = CamSensor::AcceptVideoFrame(width, height);
	if (err < B_OK)
		return err;
	// must be modulo 16
	width /= 16;
	width *= 16;
	height /= 16;
	height *= 16;
	return B_OK;
}


status_t
TAS5110C1BSensor::SetVideoFrame(BRect rect)
{
	if (fIsSonix) {
		// set crop
		Device()->WriteReg8(SN9C102_H_START, /*rect.left + */69);
		Device()->WriteReg8(SN9C102_V_START, /*rect.top + */9);
		Device()->WriteReg8(SN9C102_PIX_CLK, 0xfb);
		Device()->WriteReg8(SN9C102_HO_SIZE, 0x14);
		Device()->WriteReg8(SN9C102_VO_SIZE, 0x0a);
		fVideoFrame = rect;
		/* HACK: TEST IMAGE */
		//Device()->WriteReg8(SN9C102_CLOCK_SEL, 0x70);	/* enable sensor clk, and invert it, test img */

	}
	
	return B_OK;
}


void
TAS5110C1BSensor::AddParameters(BParameterGroup *group, int32 &index)
{
	CamSensor::AddParameters(group, index);

#ifdef ENABLE_GAIN
	BContinuousParameter *p;
	// NON-FUNCTIONAL
	BParameterGroup *g = group->MakeGroup("global gain");
	p = g->MakeContinuousParameter(index++, 
		B_MEDIA_RAW_VIDEO, "global gain", 
		B_GAIN, "", (float)0x00, (float)0xf6, (float)1);
#endif
}


status_t
TAS5110C1BSensor::GetParameterValue(int32 id, bigtime_t *last_change, void *value, size_t *size)
{
#ifdef ENABLE_GAIN
	if (id == fFirstParameterID) {
		*size = sizeof(float);
		*((float *)value) = fGain;
		*last_change = fLastParameterChanges;
	}
#endif
	return B_BAD_VALUE;
}


status_t
TAS5110C1BSensor::SetParameterValue(int32 id, bigtime_t when, const void *value, size_t size)
{
#ifdef ENABLE_GAIN
	if (id == fFirstParameterID) {
		if (!value || (size != sizeof(float)))
			return B_BAD_VALUE;
		if (*(float *)value == fGain)
			return B_OK;
		fGain = *(float *)value;
		fLastParameterChanges = when;
		PRINT((CH ": gain: %f" CT, fGain));

		if (fIsSonix) {
			// some drivers do:
			//Device()->WriteIIC8(0x20, (uint8)0xf6 - (uint8)fGain);
			// but it doesn't seem to work
		
			// works, not sure why yet, XXX check datasheet for AEG/AEC
			uint8 buf[2] = { 0x20, 0x70 };
			buf[1] = (uint8)0xff - (uint8)fGain;
			Device()->WriteIIC(0x02, buf, 2);
		}

		return B_OK;
	}
#endif
	return B_BAD_VALUE;
}



B_WEBCAM_DECLARE_SENSOR(TAS5110C1BSensor, tas5110c1b)

