/*
 * Copyright 2007-2012 Haiku, Inc.  All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Gerald Zajac
 */


#include "accelerant.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>


AccelerantInfo gInfo;	// global data used by source files of accelerant

static uint32 videoValue;


static int32
SuppressArtifacts(void* dataPtr)
{
	// The Intel 810 & 815 video chips create annoying artifacts which are
	// most noticeable when the cursor is moved by itself or the user goes up
	// and down through a menu.  However, if a large number of video memory
	// locations are accessed frequently like when the GLTeapot demo is
	// running, the artifacts are greatly suppressed.  Thus, that is the reason
	// why this function accesses a large number of video memory locations
	// frequently.  Note that the accessed memory locations are at the end of
	// the video memory.  This is because some artifacts still occur at the
	// top of the screen if the accessed memory is at the beginning of the
	// video memory.

	// Note that this function will reduce the general performance of a
	// computer somewhat, but it is much less of a hit than if double
	// buffering was used for the video.  Base on the frame rate of the
	// the GLTeapot demo, it is less than a 10% reduction.

	SharedInfo& si = *((SharedInfo*)dataPtr);

	while (true) {
		uint32* src = ((uint32*)(si.videoMemAddr)) + si.videoMemSize / 4 - 1;
		uint32 count = 65000;

		while (count-- > 0)
			videoValue = *src--;

		snooze(30000);		// sleep for 30 msec
	}

	return 0;
}


static status_t
InitCommon(int fileDesc)
{
	// Initialization function used by primary and cloned accelerants.

	gInfo.deviceFileDesc = fileDesc;

	// Get area ID of shared data from driver.

	area_id sharedArea;
	status_t result = ioctl(gInfo.deviceFileDesc, INTEL_GET_SHARED_DATA,
		&sharedArea, sizeof(sharedArea));
	if (result != B_OK)
		return result;

	gInfo.sharedInfoArea = clone_area("i810 shared info",
		(void**)&(gInfo.sharedInfo), B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA,
		sharedArea);
	if (gInfo.sharedInfoArea < 0)
		return gInfo.sharedInfoArea; // sharedInfoArea has error code

	gInfo.regsArea = clone_area("i810 regs area", (void**)&(gInfo.regs),
		B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, gInfo.sharedInfo->regsArea);
	if (gInfo.regsArea < 0) {
		delete_area(gInfo.sharedInfoArea);
		return gInfo.regsArea; // regsArea has error code
	}

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

	gInfo.bAccelerantIsClone = false;	// indicate this is primary accelerant

	status_t result = InitCommon(fileDesc);
	if (result == B_OK) {
		SharedInfo& si = *gInfo.sharedInfo;

		TRACE("Vendor ID: 0x%X,  Device ID: 0x%X\n", si.vendorID, si.deviceID);

		// Ensure that InitAccelerant is executed just once (copies should be
		// clones)

		if (si.bAccelerantInUse) {
			result = B_NOT_ALLOWED;
		} else {
			result = I810_Init();	// perform init related to current chip
			if (result == B_OK) {
				result = si.engineLock.Init("i810 engine lock");
				if (result == B_OK) {
					// Ensure that this function won't be executed again
					// (copies should be clones)
					si.bAccelerantInUse = true;

					thread_id threadID = spawn_thread(SuppressArtifacts,
						"SuppressArtifacts_Thread", B_DISPLAY_PRIORITY,
						gInfo.sharedInfo);
					result = resume_thread(threadID);
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

	ioctl(gInfo.deviceFileDesc, INTEL_DEVICE_NAME, data, B_OS_NAME_LENGTH);
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

	result = gInfo.modeListArea = clone_area("i810 cloned display_modes",
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


status_t
GetAccelerantDeviceInfo(accelerant_device_info* adi)
{
	// Get info about the device.

	SharedInfo& si = *gInfo.sharedInfo;

	adi->version = 1;
	strcpy(adi->name, "Intel 810/815 chipset");
	strcpy(adi->chipset, si.chipName);
	strcpy(adi->serial_no, "unknown");
	adi->memory = si.maxFrameBufferSize;
	adi->dac_speed = 270;

	return B_OK;
}
