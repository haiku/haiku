/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */

#include <ParameterWeb.h>

#include "CamSensor.h"
#include "CamDebug.h"
#include "addons/sonix/SonixCamDevice.h"

//XXX: unfinished!

#define ENABLE_GAIN 1


class TAS5130D1BSensor : public CamSensor {
public:
	TAS5130D1BSensor(CamDevice *_camera);
	~TAS5130D1BSensor();
	virtual status_t	Probe();
	virtual status_t	Setup();
	const char *Name();
	virtual bool		Use400kHz() const { return false; };
	virtual bool		UseRealIIC() const { return false; };
	virtual uint8		IICReadAddress() const { return 0x00; };
	virtual uint8		IICWriteAddress() const { return 0x11; /*0xff;*/ };

	virtual int			MaxWidth() const { return 640; };
	virtual int			MaxHeight() const { return 480; };

	virtual status_t	AcceptVideoFrame(uint32 &width, uint32 &height);
	virtual status_t	SetVideoFrame(BRect rect);
	virtual void		AddParameters(BParameterGroup *group, int32 &firstID);
	virtual status_t	GetParameterValue(int32 id, bigtime_t *last_change, void *value, size_t *size);
	virtual status_t	SetParameterValue(int32 id, bigtime_t when, const void *value, size_t size);

private:
	bool	fIsSonix;
	float	fGain;
};


TAS5130D1BSensor::TAS5130D1BSensor(CamDevice *_camera)
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


TAS5130D1BSensor::~TAS5130D1BSensor()
{
}


status_t
TAS5130D1BSensor::Probe()
{
	PRINT((CH "()" CT));
	
	return B_OK;
}


status_t
TAS5130D1BSensor::Setup()
{
	PRINT((CH "()" CT));
	if (InitCheck())
		return InitCheck();
	/*
	Device()->PowerOnSensor(false);
	Device()->PowerOnSensor(true);
	*/
	if (fIsSonix) {
		// linux driver seems to do this, but doesn't say why
		Device()->WriteReg8(SN9C102_CHIP_CTRL, 0x01);	/* power down the sensor */
		Device()->WriteReg8(SN9C102_CLOCK_SEL, 0x20);	/* enable sensor clk */
		Device()->WriteReg8(SN9C102_CHIP_CTRL, 0x04);	/* power up the sensor, enable tx, sysclk@12MHz */
		Device()->WriteReg8(SN9C102_R_B_GAIN, 0x01);	/* red gain = 1+0/8 = 1, red gain = 1+1/8 !!? */
		Device()->WriteReg8(SN9C102_G_GAIN, 0x00);	/* green gain = 1+0/8 = 1 */
		Device()->WriteReg8(SN9C102_OFFSET, 0x0a);	/* 0 pix offset */
		Device()->WriteReg8(SN9C102_CLOCK_SEL, 0x60);	/* enable sensor clk, and invert it */
		Device()->WriteReg8(SN9C102_SYNC_N_SCALE, 0x06);	/* no compression, normal curve, 
												 * no scaling, vsync active low,
												 * v/hsync change at rising edge,
												 * raising edge of sensor pck */
		Device()->WriteReg8(SN9C102_PIX_CLK, 0xf3);	/* pixclk = masterclk, sensor is slave mode */
	}
	
	
	if (fIsSonix) {
		//sonix_i2c_write_multi(dev, dev->sensor->i2c_wid, 2, 0xc0, 0x80, 0, 0, 0); /* AEC = 0x203 ??? */
#if 0
		Device()->WriteIIC8(0xc0, 0x80); /* AEC = 0x203 ??? */
#endif

#if 1
		Device()->WriteIIC8(0x20, 0xf6 - 0xd0); /* GAIN */
		Device()->WriteIIC8(0x40, 0x47 - 0x40); /* Exposure */
		// set crop
		Device()->WriteReg8(SN9C102_H_SIZE, 104);
		Device()->WriteReg8(SN9C102_V_SIZE, 12);
		SetVideoFrame(BRect(0, 0, 320-1, 240-1));
#endif

#if 0
//XXX
		Device()->WriteIIC8(0xc0, 0x80); /* AEC = 0x203 ??? */
		Device()->WriteReg8(SN9C102_H_SIZE, 69);
		Device()->WriteReg8(SN9C102_V_SIZE, 9);
		SetVideoFrame(BRect(0, 0, 352-1, 288-1));
#endif

	}
	
	//Device()->SetScale(1);
	
	return B_OK;
}
					

const char *
TAS5130D1BSensor::Name()
{
	return "TASC tas5130d1b";
}


status_t
TAS5130D1BSensor::AcceptVideoFrame(uint32 &width, uint32 &height)
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
TAS5130D1BSensor::SetVideoFrame(BRect rect)
{
	if (fIsSonix) {
		// set crop
		Device()->WriteReg8(SN9C102_H_START, /*rect.left + */104);
		Device()->WriteReg8(SN9C102_V_START, /*rect.top + */12);
		Device()->WriteReg8(SN9C102_PIX_CLK, 0xfb);
		Device()->WriteReg8(SN9C102_HO_SIZE, 0x14);
		Device()->WriteReg8(SN9C102_VO_SIZE, 0x0a);
#if 0
		Device()->WriteReg8(SN9C102_H_START, /*rect.left + */104);
		Device()->WriteReg8(SN9C102_V_START, /*rect.top + */12);
		Device()->WriteReg8(SN9C102_PIX_CLK, 0xf3);
		Device()->WriteReg8(SN9C102_HO_SIZE, 0x1f);
		Device()->WriteReg8(SN9C102_VO_SIZE, 0x1a);
#endif
		fVideoFrame = rect;
		/* HACK: TEST IMAGE */
		//Device()->WriteReg8(SN9C102_CLOCK_SEL, 0x70);	/* enable sensor clk, and invert it, test img */

	}
	
	return B_OK;
}


void
TAS5130D1BSensor::AddParameters(BParameterGroup *group, int32 &index)
{
	BContinuousParameter *p;
	CamSensor::AddParameters(group, index);

#ifdef ENABLE_GAIN
	// NON-FUNCTIONAL
	BParameterGroup *g = group->MakeGroup("global gain");
	p = g->MakeContinuousParameter(index++, 
		B_MEDIA_RAW_VIDEO, "global gain", 
		B_GAIN, "", (float)0x00, (float)0xf6, (float)1);
#endif
}


status_t
TAS5130D1BSensor::GetParameterValue(int32 id, bigtime_t *last_change, void *value, size_t *size)
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
TAS5130D1BSensor::SetParameterValue(int32 id, bigtime_t when, const void *value, size_t size)
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


B_WEBCAM_DECLARE_SENSOR(TAS5130D1BSensor, tas5130d1b)

