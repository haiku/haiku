/*
	Copyright 2007-2008 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac 2007-2008
*/

#include "accel.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>



AccelerantInfo gInfo;		// global data used by various source files of accelerant.



static status_t 
InitCommon(int fileDesc)
{
	// Initialization function used by primary and cloned accelerants.

	gInfo.deviceFileDesc = fileDesc;

	// Get pointer to registers and shared data from driver.

	S3GetPrivateData gpd;
	gpd.magic = S3_PRIVATE_DATA_MAGIC;

	status_t result = ioctl(gInfo.deviceFileDesc, S3_GET_PRIVATE_DATA, &gpd, sizeof(gpd));
	if (result != B_OK)
		return result;

	gInfo.sharedInfoArea = clone_area("S3 shared info", (void**)&(gInfo.sharedInfo),
		B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, gpd.sharedInfoArea);
	if (gInfo.sharedInfoArea < 0)
		return gInfo.sharedInfoArea;	// sharedInfoArea has error code

	gInfo.regsArea = clone_area("S3 regs area", (void**)&(gInfo.regs),
		B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, gInfo.sharedInfo->regsArea);
	if (gInfo.regsArea < 0) {
		delete_area(gInfo.sharedInfoArea);
		return gInfo.regsArea;		// regsArea has error code
	}

	// Set pointers to various device specific functions.

	if (S3_SAVAGE_FAMILY(gInfo.sharedInfo->chipType))
		Savage_SetFunctionPointers();
	else if (S3_TRIO64_FAMILY(gInfo.sharedInfo->chipType))
		Trio64_SetFunctionPointers();
	else if (S3_VIRGE_FAMILY(gInfo.sharedInfo->chipType))
		Virge_SetFunctionPointers();
	else
		return B_ERROR;		// undefined chip type code

	return B_OK;
}


static void 
UninitCommon(void)
{
	// This function is used by both primary and cloned accelerants.

	delete_area(gInfo.regsArea);
	gInfo.regs = 0;

	delete_area(gInfo.sharedInfoArea);
	gInfo.sharedInfo = 0;
}


status_t 
InitAccelerant(int fileDesc)
{
	// Initialize the accelerant.	fileDesc is the file handle of the device
	// (in /dev/graphics) that has been opened by the app_server.

	TRACE("Enter InitAccelerant()\n");

	gInfo.bAccelerantIsClone = false;		// indicate this is primary accelerant

	status_t result = InitCommon(fileDesc);
	if (result == B_OK) {
		SharedInfo& si = *gInfo.sharedInfo;

		TRACE("Vendor ID: 0x%X,  Device ID: 0x%X\n", si.vendorID, si.deviceID);

		// Ensure that InitAccelerant is executed just once (copies should be clones)

		if (si.bAccelerantInUse) {
			result = B_NOT_ALLOWED;
		} else {
			result = gInfo.ChipInit();	// perform init related to current chip
			if (result == B_OK) {
				result = si.engineLock.Init("ATI engine lock");
				if (result == B_OK) {
					if (gInfo.ShowCursor != NULL)
						gInfo.ShowCursor(false);

					// ensure that this function won't be executed again
					// (copies should be clones)
					si.bAccelerantInUse = true;
				}
			}
		}

		if (result != B_OK)
			UninitCommon();
	}

	TRACE("Leave InitAccelerant(), result: 0x%X\n", result);
	return result;
}


ssize_t 
AccelerantCloneInfoSize(void)
{
	// Return the number of bytes required to hold the information required
	// to clone the device.  The information is merely the name of the device;
	// thus, return the size of the name buffer.

	return B_OS_NAME_LENGTH;
}


void 
GetAccelerantCloneInfo(void* data)
{
	// Return the info required to clone the device.  Argument data points to
	// a buffer which is the size returned by AccelerantCloneInfoSize().

	ioctl(gInfo.deviceFileDesc, S3_DEVICE_NAME, data, B_OS_NAME_LENGTH);
}


status_t 
CloneAccelerant(void* data)
{
	// Initialize a copy of the accelerant as a clone.  Argument data points to
	// a copy of the data which was returned by GetAccelerantCloneInfo().

	TRACE("Enter CloneAccelerant()\n");

	char path[MAXPATHLEN] = "/dev/";
	strcat(path, (const char*)data);

	gInfo.deviceFileDesc = open(path, B_READ_WRITE);	// open the device
	if (gInfo.deviceFileDesc < 0)
		return errno;

	gInfo.bAccelerantIsClone = true;

	status_t result = InitCommon(gInfo.deviceFileDesc);
	if (result != B_OK) {
		close(gInfo.deviceFileDesc);
		return result;
	}

	result = gInfo.modeListArea = clone_area("S3 cloned display_modes",
		(void**) &gInfo.modeList, B_ANY_ADDRESS, B_READ_AREA,
		gInfo.sharedInfo->modeArea);
	if (result < 0) {
		UninitCommon();
		close(gInfo.deviceFileDesc);
		return result;
	}

	TRACE("Leave CloneAccelerant()\n");
	return B_OK;
}


void 
UninitAccelerant(void)
{
	delete_area(gInfo.modeListArea);
	gInfo.modeList = NULL;

	UninitCommon();

	if (gInfo.bAccelerantIsClone)
		close(gInfo.deviceFileDesc);
}


sem_id 
AccelerantRetraceSemaphore(void)
{
	// Return the semaphore id that will be used to signal that a vertical
	// retrace occured.

	return B_ERROR;
}


status_t 
GetAccelerantDeviceInfo(accelerant_device_info* adi)
{
	// Get info about the device.

	SharedInfo& si = *gInfo.sharedInfo;

	adi->version = 1;
	strcpy(adi->name, "S3 chipset");
	strcpy(adi->chipset, si.chipName);
	strcpy(adi->serial_no, "unknown");
	adi->memory = si.maxFrameBufferSize;
	adi->dac_speed = 270;

	return B_OK;
}
