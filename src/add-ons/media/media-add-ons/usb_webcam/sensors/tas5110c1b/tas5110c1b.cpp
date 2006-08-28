/*
*/

#include "CamSensor.h"
#include "CamDebug.h"
#include "addons/sonix/SonixCamDevice.h"

class TAS5110C1BSensor : public CamSensor {
public:
	TAS5110C1BSensor(CamDevice *_camera);
	~TAS5110C1BSensor();
	virtual status_t	Setup();
	const char *Name();
	virtual bool		Use400kHz() const { return false; };
	virtual bool		UseRealIIC() const { return false; };
	virtual uint8		IICReadAddress() const { return 0x00; };
	virtual uint8		IICWriteAddress() const { return 0xff; };
	virtual int			MaxWidth() const { return 352; };
	virtual int			MaxHeight() const { return 288; };
	virtual status_t	SetVideoFrame(BRect rect);
private:
	bool	fIsSonix;
};

// -----------------------------------------------------------------------------
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
}

// -----------------------------------------------------------------------------
TAS5110C1BSensor::~TAS5110C1BSensor()
{
}

// -----------------------------------------------------------------------------
status_t
TAS5110C1BSensor::Setup()
{
	PRINT((CH "()" CT));
	if (InitCheck())
		return InitCheck();
	if (fIsSonix) {
		Device()->WriteReg8(SN9C102_CHIP_CTRL, 0x01);	/* power down the sensor */
		Device()->WriteReg8(SN9C102_CHIP_CTRL, 0x44);	/* power up the sensor, enable tx, sysclk@24MHz */
		Device()->WriteReg8(SN9C102_R_B_GAIN, 0x00);	/* red, blue gain = 1+0/8 = 1 */
		Device()->WriteReg8(SN9C102_G_GAIN, 0x00);	/* green gain = 1+0/8 = 1 */
		Device()->WriteReg8(SN9C102_OFFSET, 0x0a);	/* 10 pix offset */
		Device()->WriteReg8(SN9C102_CLOCK_SEL, 0x60);	/* enable sensor clk, and invert it */
		Device()->WriteReg8(SN9C102_SYNC_N_SCALE, 0x06);	/* no compression, normal curve, 
												 * no scaling, vsync active low,
												 * v/hsync change at rising edge,
												 * falling edge of sensor pck */
		Device()->WriteReg8(SN9C102_PIX_CLK, 0xfb);	/* pixclk = 2 * masterclk, sensor is slave mode */
	}
	
	//sonix_i2c_write_multi(dev, dev->sensor->i2c_wid, 2, 0xc0, 0x80, 0, 0, 0); /* AEC = 0x203 ??? */
	Device()->WriteIIC8(0xc0, 0x80); /* AEC = 0x203 ??? */
	
	if (fIsSonix) {
		// set crop
		Device()->WriteReg8(SN9C102_H_SIZE, 69);
		Device()->WriteReg8(SN9C102_V_SIZE, 9);
		Device()->WriteReg8(SN9C102_PIX_CLK, 0xfb);
		Device()->WriteReg8(SN9C102_HO_SIZE, 0x14);
		Device()->WriteReg8(SN9C102_VO_SIZE, 0x0a);
		fVideoFrame.Set(0, 0, 352-1, 288-1);
		/* HACK: TEST IMAGE */
		//Device()->WriteReg8(SN_CLOCK_SEL, 0x70);	/* enable sensor clk, and invert it, test img */

	}
	
	//Device()->SetScale(1);
	
	return B_OK;
}
					
// -----------------------------------------------------------------------------
const char *
TAS5110C1BSensor::Name()
{
	return "TASC tas5110c1b";
}

// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
B_WEBCAM_DECLARE_SENSOR(TAS5110C1BSensor, tas5110c1b)

