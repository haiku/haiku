/*
 * Copyright 2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <stdio.h>
#include <stdlib.h>

#include <bluetooth/bdaddrUtils.h>
#include <bluetooth/LocalDevice.h>

#include <bluetooth/DeviceClass.h>
#include <bluetooth/DiscoveryAgent.h>
#include <bluetooth/DiscoveryListener.h>

thread_id mainThread;

class SimpleDiscoveryListener : public DiscoveryListener {

public:

SimpleDiscoveryListener()
	: DiscoveryListener()
{

}


void
DeviceDiscovered(RemoteDevice* btDevice, DeviceClass cod)
{
	BString classString;

	printf("\t%s: Device %s discovered.\n",__FUNCTION__, 
		bdaddrUtils::ToString(btDevice->GetBluetoothAddress()));

	cod.GetServiceClass(classString);
	classString << " |";
	cod.GetMajorDeviceClass(classString);
	classString << " |";
	cod.GetMinorDeviceClass(classString);

	printf("\t\t%s: \n", classString.String());
}


void
InquiryCompleted(int discType)
{
	printf("\t%s: Inquiry process has finished ...\n",__FUNCTION__);
	(void)send_data(mainThread, discType, NULL, 0);
}


void
InquiryStarted(status_t status)
{
	printf("\t%s: Inquiry process has started ...\n",__FUNCTION__);
}


};


void
DumpInfo(LocalDevice* device)
{
	DiscoveryAgent* dAgent = device->GetDiscoveryAgent();

	if (dAgent == NULL) {
		printf("DiscoveryAgent could not be located\n");
		return;
	}

	printf("Discovering for  [LocalDevice] %s\t%s\n\n",
		(device->GetFriendlyName()).String(),
		bdaddrUtils::ToString(device->GetBluetoothAddress()));

	SimpleDiscoveryListener* dListener = new SimpleDiscoveryListener();

	dAgent->StartInquiry(BT_GIAC, dListener);

	thread_id sender;
	(void)receive_data(&sender, NULL, 0);

	printf("Retrieving names ...\n");

	for (int32 index = 0 ; index < dAgent->RetrieveDevices(0).CountItems(); index++ ) {

		RemoteDevice* rDevice = dAgent->RetrieveDevices(0).ItemAt(index);
		printf("\t%s \t@ %s ...\n", rDevice->GetFriendlyName(true).String(),
			bdaddrUtils::ToString(rDevice->GetBluetoothAddress()));

    }

}


static status_t
LocalDeviceError(status_t status)
{
    fprintf(stderr,"No Device/s found");

    return status;
}


int
main(int argc, char *argv[])
{
	mainThread = find_thread(NULL);

	if (argc == 2) {
		// device specified
		LocalDevice* device = LocalDevice::GetLocalDevice(atoi(argv[0]));
		if (device == NULL)
			return LocalDeviceError(ENODEV);

		DumpInfo(device);

	} else if (argc == 1) {
		// show all devices
		LocalDevice* device = NULL;

		printf("Performing discovery for %ld Bluetooth Local Devices ...\n",
			LocalDevice::GetLocalDeviceCount());

		for (uint32 index = 0 ; index < LocalDevice::GetLocalDeviceCount() ; index++) {

			device = LocalDevice::GetLocalDevice();
			if (device == NULL) {
				LocalDeviceError(ENODEV);
				continue;
			}
			DumpInfo(device);

		}

		return B_OK;

	} else {
		fprintf(stderr,"Usage: bt_dev_info [device]\n");
		return B_ERROR;
	}
}
