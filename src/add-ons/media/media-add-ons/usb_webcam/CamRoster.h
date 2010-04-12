/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CAM_ROSTER_H
#define _CAM_ROSTER_H

#include <image.h>
#include <List.h>
#include <Locker.h>

#include "CamDevice.h"

class WebCamMediaAddOn;
class CamDeviceAddon;

class CamRoster : public BUSBRoster {
public:
						CamRoster(WebCamMediaAddOn* _addon);
	virtual				~CamRoster();
	virtual status_t	DeviceAdded(BUSBDevice* _device);
	virtual void		DeviceRemoved(BUSBDevice* _device);

			uint32		CountCameras();
			bool		Lock();
			void		Unlock();
	// those must be called with Lock()
			CamDevice*	CameraAt(int32 index);



private:
			status_t	LoadInternalAddons();
			status_t	LoadExternalAddons();

	BLocker				fLocker;
	WebCamMediaAddOn*	fAddon;
	BList			fCamerasAddons;
	BList			fCameras;
};

#endif
