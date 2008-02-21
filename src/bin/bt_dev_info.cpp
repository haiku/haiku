/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */
 

#include <stdio.h>
#include <stdlib.h>

#include <bluetooth/LocalDevice.h>


void
DumpInfo(LocalDevice* device)
{



}

static status_t
LocalDeviceError(status_t status)
{
	// switch (status) {}
    fprintf(stderr,"No Device/s found");
    
    return status;
}


int
main(int argc, char *argv[])
{
	LocalDevice* ld = NULL;

    if(argc == 2) {
	    // device specified
        ld = LocalDevice::GetLocalDevice(atoi(argv[0]));
        if (ld == NULL)
           return LocalDeviceError(ENODEV);

        DumpInfo(ld);

    } else if (argc == 1) {
        // show all devices
        LocalDevice* firstLocalDevice = LocalDevice::GetLocalDevice();

        if (firstLocalDevice == NULL)
            return LocalDeviceError(ENODEV);

        printf("Listing %ld Bluetooth devices ...\n", LocalDevice::GetLocalDeviceCount());

        ld = firstLocalDevice;
        do {
           DumpInfo(ld);
           ld = LocalDevice::GetLocalDevice();

        } while (ld != firstLocalDevice);

    	return B_OK;

    } else {
        fprintf(stderr,"Usage: bt_dev_info [device]\n");
        return B_ERROR;
    }
}
