#ifndef _QUICK_CAM_DEVICE_H
#define _QUICK_CAM_DEVICE_H

#include "CamDevice.h"

// This class represents each webcam
class QuickCamDevice : public CamDevice
{
	public: 
						QuickCamDevice(CamDeviceAddon &_addon, BUSBDevice* _device);
						~QuickCamDevice();
							
	private:
};

// the addon itself, that instanciate

class QuickCamDeviceAddon : public CamDeviceAddon
{
	public:
						QuickCamDeviceAddon(WebCamMediaAddOn* webcam);
	virtual 			~QuickCamDeviceAddon();
						
	virtual const char	*BrandName();
	virtual QuickCamDevice	*Instantiate(CamRoster &roster, BUSBDevice *from);
	
};

#endif /* _QUICK_CAM_CAM_DEVICE_H */
