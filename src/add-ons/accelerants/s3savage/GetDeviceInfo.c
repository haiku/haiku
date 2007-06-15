/*
	Copyright 2007 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac 2007
*/

#include "AccelerantPrototypes.h"
#include "GlobalData.h"
#include <string.h>


// Get info about the device.

status_t 
GET_ACCELERANT_DEVICE_INFO(accelerant_device_info *adi)
{
	adi->version = 1;
	strcpy(adi->name, "S3 Savage chipset");
	strcpy(adi->chipset, si->chipsetName);
	strcpy(adi->serial_no, "unknown");
	adi->memory = si->videoMemSize;
	adi->dac_speed = 250;

	return B_OK;
}

