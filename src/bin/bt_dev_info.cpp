/*
 * Copyright 2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <stdio.h>
#include <stdlib.h>

#include <bluetooth/LocalDevice.h>
#include <bluetooth/bdaddrUtils.h>


static void
DumpInfo(LocalDevice* device)
{
	printf("[LocalDevice] %s\t%s\n", (device->GetFriendlyName()).String(),
		bdaddrUtils::ToString(device->GetBluetoothAddress()));

	BString classString;
	DeviceClass cod = device->GetDeviceClass();

	cod.GetServiceClass(classString);
	classString << " |";
	cod.GetMajorDeviceClass(classString);
	classString << " |";
	cod.GetMinorDeviceClass(classString);
	printf("\t\t%s: \n", classString.String());

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
	if (argc == 2) {
		// device specified
		LocalDevice* ld = LocalDevice::GetLocalDevice(atoi(argv[0]));
		if (ld == NULL)
			return LocalDeviceError(ENODEV);

		DumpInfo(ld);

	} else if (argc == 1) {
		// show all devices
		LocalDevice* ld = NULL;

		printf("Listing %ld Bluetooth Local Devices ...\n",
			LocalDevice::GetLocalDeviceCount());
		for (uint32 index = 0 ; index < LocalDevice::GetLocalDeviceCount() ; index++) {

			ld = LocalDevice::GetLocalDevice();
			if (ld == NULL)
				return LocalDeviceError(ENODEV);
			DumpInfo(ld);
		}

		return B_OK;
	
	} else {
		fprintf(stderr,"Usage: bt_dev_info [device]\n");
		return B_ERROR;
	}
}
