//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//  Mad props to Axel Dörfler and his BFS implementation, from which
//  this UDF implementation draws much influence (and a little code :-P).
//----------------------------------------------------------------------
#include "Volume.h"

#include "Block.h"
#include "Debug.h"

using namespace UDF;

//----------------------------------------------------------------------
// Volume 
//----------------------------------------------------------------------

/*! \brief Creates an unmounted volume with the given id.
*/
Volume::Volume(nspace_id id)
	: fID(id)
	, fDevice(0)
	, fReadOnly(false)
{
}

status_t
Volume::Identify(int device, off_t base)
{
	return B_ERROR;	
}

/*! \brief Attempts to mount the given device.
*/
status_t
Volume::Mount(const char *deviceName, uint32 flags)
{
	if (!deviceName)
		RETURN_ERROR(B_BAD_VALUE);
		
	fReadOnly = flags & B_READ_ONLY;
	
	// Open the device, trying read only if readwrite fails
	fDevice = open(deviceName, fReadOnly ? O_RDONLY : O_RDWR);
	if (fDevice < B_OK && !fReadOnly) {
		fReadOnly = true;
		fDevice = open(deviceName, O_RDONLY);
	}		
	if (fDevice < B_OK)
		RETURN_ERROR(fDevice);
		
	// If the device is actually a normal file, try to disable the cache
	// for the file in the parent filesystem
	struct stat stat;
	status_t err = fstat(fDevice, &stat) < 0 ? B_OK : B_ERROR;
	if (!err) {
		if (stat.st_mode & S_IFREG && ioctl(fDevice, IOCTL_FILE_UNCACHED_IO, NULL) < 0) {
			// Probably should die some sort of painful death here.
		}
	}
	
	// Now identify the volume

	return B_ERROR;
}

