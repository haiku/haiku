/*
 * Copyright 2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include <bluetooth/LocalDevice.h>
#include <bluetooth/bdaddrUtils.h>

#include <bluetooth/DiscoveryAgent.h>
#include <bluetooth/DiscoveryListener.h>

class simpleDiscoveryListener : public DiscoveryListener {

public:

simpleDiscoveryListener(LocalDevice *ld) : DiscoveryListener()
{
	SetLocalDeviceOwner(ld);
}


void
DeviceDiscovered(RemoteDevice* btDevice, DeviceClass cod)
{
	printf("Device Discovered!!\n");
}


void
InquiryCompleted(int discType)
{
	printf("%s: Inquiry process has finished ...\n",__FUNCTION__);
}


void
InquiryStarted(status_t status)
{
	printf("%s: Inquiry process has started ...\n",__FUNCTION__);
}


};


void
DumpInfo(LocalDevice* device)
{
	DiscoveryAgent* da = device->GetDiscoveryAgent();

	if (da == NULL) {
		printf("DiscoveryAgent could not be located\n");
		return;
	}
	
/*	printf("Discovering for  [LocalDevice] %s\t%s\n", 
         (device->GetFriendlyName()).String(),
          bdaddrUtils::ToString(device->GetBluetoothAddress()));
*/	
	simpleDiscoveryListener* sdl = new simpleDiscoveryListener(device);

	da->StartInquiry(BT_GIAC, sdl);

	getchar();
	
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
        if(argc == 2) {
            // device specified
            LocalDevice* ld = LocalDevice::GetLocalDevice(atoi(argv[0]));
            if (ld == NULL)
               return LocalDeviceError(ENODEV);

            DumpInfo(ld);

        } else if (argc == 1) {
            // show all devices
            LocalDevice* ld = NULL;

            printf("Performing discovery for %ld Bluetooth Local Devices ...\n", LocalDevice::GetLocalDeviceCount());

            for (uint32 index = 0 ; index < LocalDevice::GetLocalDeviceCount() ; index++) {

               ld = LocalDevice::GetLocalDevice();
               if (ld == NULL) {
                    LocalDeviceError(ENODEV);
                    continue;
               }
               DumpInfo(ld);

            }

            return B_OK;

        } else {
                fprintf(stderr,"Usage: bt_dev_info [device]\n");
                return B_ERROR;
        }
}
