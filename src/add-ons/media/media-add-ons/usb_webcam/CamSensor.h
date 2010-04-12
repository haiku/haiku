/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CAM_SENSOR_H
#define _CAM_SENSOR_H

#include "CamDevice.h"
#include <Rect.h>

// This class represents the camera's (cmos or whatever) sensor chip
class CamSensor {
	public:
						CamSensor(CamDevice *_camera);
	virtual				~CamSensor();

	virtual status_t	Probe(); // returns B_OK if found.

	virtual status_t	InitCheck();

	virtual status_t	Setup();

	virtual const char*	Name();

	virtual status_t	StartTransfer();
	virtual status_t	StopTransfer();
	virtual bool		TransferEnabled() const { return fTransferEnabled; };

	virtual bool		IsBigEndian() const { return fIsBigEndian; };
	virtual bool		Use400kHz() const { return false; };
	virtual bool		UseRealIIC() const { return true; };
	virtual uint8		IICReadAddress() const { return 0; };
	virtual uint8		IICWriteAddress() const { return 0; };

	virtual int			MaxWidth() const { return -1; };
	virtual int			MaxHeight() const { return -1; };

	virtual status_t	AcceptVideoFrame(uint32 &width, uint32 &height);
	virtual status_t	SetVideoFrame(BRect rect);
	virtual BRect		VideoFrame() const { return fVideoFrame; };
	virtual status_t	SetVideoParams(float brightness, float contrast, float hue, float red, float green, float blue);

	virtual void		AddParameters(BParameterGroup *group, int32 &index);
	virtual status_t	GetParameterValue(int32 id, bigtime_t *last_change, void *value, size_t *size);
	virtual status_t	SetParameterValue(int32 id, bigtime_t when, const void *value, size_t size);

	CamDevice			*Device();

#if 0
	// generic register-like access
	virtual status_t	WriteReg(uint16 address, uint8 *data, size_t count=1);
	virtual status_t	WriteReg8(uint16 address, uint8 data);
	virtual status_t	WriteReg16(uint16 address, uint16 data);
	virtual status_t	ReadReg(uint16 address, uint8 *data, size_t count=1, bool cached=false);

	// I2C-like access
	virtual status_t	WriteIIC(uint8 address, uint8 *data, size_t count=1);
	virtual status_t	ReadIIC(uint8 address, uint8 *data);
#endif
	protected:

	status_t			ProbeByIICSignature(const uint8 *regList,
											const uint8 *matchList,
											size_t count);

		status_t		fInitStatus;
		bool			fIsBigEndian;
		bool			fTransferEnabled;
		BRect			fVideoFrame;
		int32			fFirstParameterID;
		bigtime_t		fLastParameterChanges;
	private:
		CamDevice		*fCamDevice;
};

// internal modules
#define B_WEBCAM_DECLARE_SENSOR(sensorclass,sensorname) \
extern "C" CamSensor *Instantiate##sensorclass(CamDevice *cam); \
CamSensor *Instantiate##sensorclass(CamDevice *cam) \
{ return new sensorclass(cam); };


#endif /* _CAM_SENSOR_H */
