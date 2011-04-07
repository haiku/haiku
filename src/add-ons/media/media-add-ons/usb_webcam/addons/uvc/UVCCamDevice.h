/*
 * Copyright 2011, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2009, Ithamar Adema, <ithamar.adema@team-embedded.nl>.
 * Distributed under the terms of the MIT License.
 */
#ifndef _UVC_CAM_DEVICE_H
#define _UVC_CAM_DEVICE_H


#include "CamDevice.h"
#include "USB_video.h"


class UVCCamDevice : public CamDevice {
public:
								UVCCamDevice(CamDeviceAddon &_addon,
									BUSBDevice* _device);
	virtual						~UVCCamDevice();

	virtual bool				SupportsIsochronous();
	virtual status_t			StartTransfer();
	virtual status_t			StopTransfer();
	virtual status_t			SuggestVideoFrame(uint32 &width,
									uint32 &height);
	virtual status_t			AcceptVideoFrame(uint32 &width,
									uint32 &height);

private:
			void				_ParseVideoControl(
									const usbvc_class_descriptor* descriptor,
									size_t len);
			void				_ParseVideoStreaming(
									const usbvc_class_descriptor* descriptor,
									size_t len);
			status_t			_ProbeCommitFormat();
			status_t			_SelectBestAlternate();

			usbvc_interface_header_descriptor *fHeaderDescriptor;
			
			const BUSBEndpoint*	fInterruptIn;
			uint32				fControlIndex;
			uint32				fStreamingIndex;
			uint32				fMaxVideoFrameSize;
			uint32				fMaxPayloadTransferSize;
};


class UVCCamDeviceAddon : public CamDeviceAddon {
public:
								UVCCamDeviceAddon(WebCamMediaAddOn* webcam);
	virtual 					~UVCCamDeviceAddon();

	virtual const char*			BrandName();
	virtual UVCCamDevice*		Instantiate(CamRoster &roster,
									BUSBDevice *from);
};

#endif /* _UVC_CAM_DEVICE_H */

