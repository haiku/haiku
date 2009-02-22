/*
 * Copyright 2009, Ithamar Adema, <ithamar.adema@team-embedded.nl>.
 * Distributed under the terms of the MIT License.
 */

#ifndef _UVC_CAM_DEVICE_H
#define _UVC_CAM_DEVICE_H

#include "CamDevice.h"

class UVCCamDevice : public CamDevice
{
public:
				UVCCamDevice(CamDeviceAddon &_addon, BUSBDevice* _device);
				~UVCCamDevice();

	virtual bool		SupportsIsochronous();
	virtual status_t	StartTransfer();
	virtual status_t	StopTransfer();
private:
	void ParseVideoControl(const uint8* buf, size_t len);
	void ParseVideoStreaming(const uint8* buf, size_t len);
};

class UVCCamDeviceAddon : public CamDeviceAddon
{
	public:
				UVCCamDeviceAddon(WebCamMediaAddOn* webcam);
	virtual 		~UVCCamDeviceAddon();
						
	virtual UVCCamDevice	*Instantiate(CamRoster &roster, BUSBDevice *from);
};

#endif /* _UVC_CAM_DEVICE_H */
