#ifndef _CAM_ROSTER_H
#define _CAM_ROSTER_H

#include <image.h>
#include <support/Vector.h>
#include <usb/USBRoster.h>
#include <Locker.h>

class WebCamMediaAddOn;
class CamDevice;
class CamDeviceAddon;

namespace Z {
namespace USB {
	class BUSBDevice;
}
}

using Z::USB::BUSBDevice;

class CamRoster : public BUSBRoster
{
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
	B::Support::
	BVector<CamDeviceAddon*> fCamerasAddons;
	B::Support::
	BVector<CamDevice*> fCameras;
};

#endif
