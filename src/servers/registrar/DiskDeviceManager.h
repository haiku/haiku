//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef DISK_DEVICE_MANAGER_H
#define DISK_DEVICE_MANAGER_H

#include <Looper.h>

#include "RDiskDeviceList.h"

class DiskDeviceManager : public BLooper {
public:
	DiskDeviceManager();
	virtual ~DiskDeviceManager();

	virtual void MessageReceived(BMessage *message);

private:
	RDiskDeviceList	fDeviceList;
};

#endif	// DISK_DEVICE_MANAGER_H
