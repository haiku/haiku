#ifndef LOCAL_DEVICE_ADDON_H
#define LOCAL_DEVICE_ADDON_H

#include <bluetooth/LocalDevice.h>
#include <bluetooth/RemoteDevice.h>

class LocalDeviceAddOn {

public:

	virtual const char* GetName()=0;

	virtual status_t	InitCheck(LocalDevice* lDevice)=0;

	virtual const char* GetActionDescription()=0;
	virtual status_t	TakeAction(LocalDevice* lDevice)=0;

	virtual const char* GetActionOnRemoteDescription()=0;
	virtual status_t 	TakeActionOnRemote(LocalDevice* lDevice, RemoteDevice* rDevice)=0;

	virtual const char* GetOverridenPropertiesDescription()=0;
	virtual BMessage*	OverridenProperties(LocalDevice* lDevice, const char* property)=0;

};

#define INSTANTIATE_LOCAL_DEVICE_ADDON(addon) LocalDeviceAddOn* InstantiateLocalDeviceAddOn(){return new addon();}
#define EXPORT_LOCAL_DEVICE_ADDON extern "C" __declspec(dllexport) LocalDeviceAddOn* InstantiateLocalDeviceAddOn();



#endif
