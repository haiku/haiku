#ifndef RESET_LOCAL_DEVICE_ADDON_H
#define RESET_LOCAL_DEVICE_ADDON_H

#include <bluetooth/LocalDeviceAddOn.h>

class ResetLocalDeviceAddOn : public LocalDeviceAddOn {

public:
	ResetLocalDeviceAddOn();

	const char* GetName();
	status_t	InitCheck(LocalDevice* lDevice);

	const char* GetActionDescription();
	status_t	TakeAction(LocalDevice* lDevice);

	const char* GetActionOnRemoteDescription();
	status_t 	TakeActionOnRemote(LocalDevice* lDevice, RemoteDevice* rDevice);

	const char* GetOverridenPropertiesDescription();
	BMessage*	OverridenProperties(LocalDevice* lDevice, const char* property);
private:
	status_t fCheck;
};

#endif // RESET_LOCAL_DEVICE_ADDON_H
