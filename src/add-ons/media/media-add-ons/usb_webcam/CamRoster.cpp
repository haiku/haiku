/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */

#include "CamRoster.h"

#include "AddOn.h"
#include "CamDevice.h"
#include "CamDebug.h"

#include <OS.h>

#undef B_WEBCAM_MKINTFUNC
#define B_WEBCAM_MKINTFUNC(modname) \
extern "C" status_t get_webcam_addon_##modname(WebCamMediaAddOn* webcam, CamDeviceAddon **addon);
#include "CamInternalAddons.h"
#undef B_WEBCAM_MKINTFUNC


CamRoster::CamRoster(WebCamMediaAddOn* _addon)
	: BUSBRoster(),
	fLocker("WebcamRosterLock"),
	fAddon(_addon)
{
	PRINT((CH "()" CT));
	LoadInternalAddons();
	LoadExternalAddons();
}


CamRoster::~CamRoster()
{
}


status_t
CamRoster::DeviceAdded(BUSBDevice* _device)
{
	PRINT((CH "()" CT));
	status_t err;
	for (int16 i = fCamerasAddons.CountItems() - 1; i >= 0; --i ) {
		CamDeviceAddon *ao = (CamDeviceAddon *)fCamerasAddons.ItemAt(i);
		PRINT((CH ": checking %s for support..." CT, ao->BrandName()));
		err = ao->Sniff(_device);
		if (err < B_OK)
			continue;
		CamDevice *cam = ao->Instantiate(*this, _device);
		PRINT((CH ": found camera %s:%s!" CT, cam->BrandName(), cam->ModelName()));
		err = cam->InitCheck();
		if (err >= B_OK) {
			fCameras.AddItem(cam);
			fAddon->CameraAdded(cam);
			return B_OK;
		}
		PRINT((CH " error 0x%08" B_PRIx32 CT, err));
	}
	return B_ERROR;
}


void
CamRoster::DeviceRemoved(BUSBDevice* _device)
{
	PRINT((CH "()" CT));
	for (int32 i = 0; i < fCameras.CountItems(); ++i) {
		CamDevice* cam = (CamDevice *)fCameras.ItemAt(i);
		if (cam->Matches(_device)) {
			PRINT((CH ": camera %s:%s removed" CT, cam->BrandName(), cam->ModelName()));
			fCameras.RemoveItem(i);
			fAddon->CameraRemoved(cam);
			// XXX: B_DONT_DO_THAT!
			//delete cam;
			cam->Unplugged();
			return;
		}
	}
}


uint32
CamRoster::CountCameras()
{
	int32 count;
	fLocker.Lock();
	PRINT((CH "(): %" B_PRId32 " cameras" CT, fCameras.CountItems()));
	count = fCameras.CountItems();
	fLocker.Unlock();
	return count;
}


bool
CamRoster::Lock()
{
	PRINT((CH "()" CT));
	return fLocker.Lock();
}


void
CamRoster::Unlock()
{
	PRINT((CH "()" CT));
	fLocker.Unlock();
	return;
}


CamDevice*
CamRoster::CameraAt(int32 index)
{
	PRINT((CH "()" CT));
	return (CamDevice *)fCameras.ItemAt(index);
}


status_t
CamRoster::LoadInternalAddons()
{
	PRINT((CH "()" CT));
	CamDeviceAddon *addon;
	status_t err;

#undef B_WEBCAM_MKINTFUNC
#define B_WEBCAM_MKINTFUNC(modname) \
	err = get_webcam_addon_##modname(fAddon, &addon); \
	if (err >= B_OK) { \
		fCamerasAddons.AddItem(addon); \
		PRINT((CH ": registered %s addon" CT, addon->BrandName())); \
	}

#include "CamInternalAddons.h"
#undef B_WEBCAM_MKINTFUNC


	return B_OK;
}


status_t
CamRoster::LoadExternalAddons()
{
	PRINT((CH "()" CT));
	// FIXME
	return B_ERROR;
	int32 index;
	int32 sclass;
	status_t err;
	CamDeviceAddon *addon;
	status_t (*get_webcam_addon_func)(WebCamMediaAddOn* webcam, CamDeviceAddon **addon);
	for (index = 0; get_nth_image_symbol(fAddon->ImageID(),
										index, NULL, NULL,
										&sclass,
										(void **)&get_webcam_addon_func) == B_OK; index++) {
		PRINT((CH ": got sym" CT));
//		if (sclass != B_SYMBOL_TYPE_TEXT)
//			continue;
		err = (*get_webcam_addon_func)(fAddon, &addon);
		PRINT((CH ": Loaded addon '%s' with error 0x%08" B_PRIx32 CT,
			(err > 0) ? NULL : addon->BrandName(),
			err));
	}
	return B_OK;
}



