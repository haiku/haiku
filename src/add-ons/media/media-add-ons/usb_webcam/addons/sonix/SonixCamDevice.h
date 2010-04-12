/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SONIX_CAM_DEVICE_H
#define _SONIX_CAM_DEVICE_H

#include "CamDevice.h"

#define SN9C102_REG_COUNT	0x20
/* SN9c102 registers */
#define SN9C102_ASIC_ID		0x00
#define SN9C102_CHIP_CTRL	0x01
#define SN9C102_GPIO			0x02
#define SN9C103_G_GAIN		0x04	/* chip version dependant! */
#define SN9C102_R_GAIN		0x05
#define SN9C102_B_GAIN		0x06
#define SN9C102_I2C_SETUP	0x08
#define SN9C102_I2C_SLAVE_ID	0x09
#define SN9C102_I2C_DATA0	0x0a
#define SN9C102_I2C_DATA1	0x0b
#define SN9C102_I2C_DATA2	0x0c
#define SN9C102_I2C_DATA3	0x0d
#define SN9C102_I2C_DATA4	0x0e
#define SN9C102_CONTROL_STAT	0x0f /*I2C ??*/
#define SN9C102_R_B_GAIN		0x10	/* datasheet says so but it's WRONG */
#define SN9C102_G_GAIN		0x11 /* Green channel gain control. -> Gain = (1+G_GAIN/8)
							    Note: It is sync with VSYNC */
#define SN9C102_H_START		0x12 /* Start active pixel number after H­sync of sensor
							    Note:
							    The 1st line sequence of image data is BGBGBG
							    The 2nd line sequence of image data is GRGRGR */
#define SN9C102_V_START		0x13 /* Start active line number after V­sync of sensor */
#define SN9C102_OFFSET		0x14 /* Offset adjustment for sensor image data. */
#define SN9C102_H_SIZE		0x15 /* Horizontal pixel number for sensor. */
#define SN9C102_V_SIZE		0x16 /* Vertical pixel number for sensor. */
#define SN9C102_CLOCK_SEL	0x17
#define SN9C102_SYNC_N_SCALE	0x18
#define SN9C102_PIX_CLK		0x19
#define SN9C102_HO_SIZE		0x1a /* /32 */
#define SN9C102_VO_SIZE		0x1b /* /32 */
#define SN9C102_AE_STRX		0x1c
#define SN9C102_AE_STRY		0x1d
#define SN9C102_AE_ENDX		0x1e
#define SN9C102_AE_ENDY		0x1f

// extra regs ? maybe 103 or later only ? used by gspcav1
#define SN9C10x_CONTRAST	0x84
#define SN9C10x_BRIGHTNESS	0x96
#undef SN9C102_REG_COUNT
#define SN9C102_REG_COUNT	0x100


#define SN9C102_RGB_GAIN_MAX	0x7f

// This class represents each webcam
class SonixCamDevice : public CamDevice {
	public:
						SonixCamDevice(CamDeviceAddon &_addon, BUSBDevice* _device);
						~SonixCamDevice();
	virtual bool		SupportsBulk();
	virtual bool		SupportsIsochronous();
	virtual status_t	StartTransfer();
	virtual status_t	StopTransfer();

	// sensor chip handling
	virtual status_t	PowerOnSensor(bool on);

	// generic register-like access
	virtual ssize_t		WriteReg(uint16 address, uint8 *data, size_t count=1);
	virtual ssize_t		ReadReg(uint16 address, uint8 *data, size_t count=1, bool cached=false);

	// I2C-like access
	virtual status_t	GetStatusIIC();
	virtual status_t	WaitReadyIIC();
	virtual ssize_t		WriteIIC(uint8 address, uint8 *data, size_t count=1);
	virtual ssize_t		ReadIIC(uint8 address, uint8 *data);

	virtual status_t	SetVideoFrame(BRect rect);
	virtual status_t	SetScale(float scale);
	virtual status_t	SetVideoParams(float brightness, float contrast, float hue, float red, float green, float blue);

	virtual void		AddParameters(BParameterGroup *group, int32 &index);
	virtual status_t	GetParameterValue(int32 id, bigtime_t *last_change, void *value, size_t *size);
	virtual status_t	SetParameterValue(int32 id, bigtime_t when, const void *value, size_t size);


	// for use by deframer
	virtual size_t		MinRawFrameSize();
	virtual size_t		MaxRawFrameSize();
	virtual bool		ValidateStartOfFrameTag(const uint8 *tag, size_t taglen);
	virtual bool		ValidateEndOfFrameTag(const uint8 *tag, size_t taglen, size_t datalen);

	virtual status_t	GetFrameBitmap(BBitmap **bm, bigtime_t *stamp=NULL);
	virtual status_t	FillFrameBuffer(BBuffer *buffer, bigtime_t *stamp=NULL);


	void				DumpRegs();

	private:
//	status_t			SendCommand(uint8 dir, uint8 request, uint16 value,
//									uint16 index, uint16 length, void* data);
	uint8				fCachedRegs[SN9C102_REG_COUNT];
	int					fChipVersion;

	int					fFrameTagState;

	uint8				fRGain;
	uint8				fGGain;
	uint8				fBGain;
	float				fContrast;
	float				fBrightness;
};

// the addon itself, that instanciate

class SonixCamDeviceAddon : public CamDeviceAddon {
	public:
						SonixCamDeviceAddon(WebCamMediaAddOn* webcam);
	virtual 			~SonixCamDeviceAddon();

	virtual const char	*BrandName();
	virtual SonixCamDevice	*Instantiate(CamRoster &roster, BUSBDevice *from);

};



#endif /* _SONIX_CAM_DEVICE_H */
