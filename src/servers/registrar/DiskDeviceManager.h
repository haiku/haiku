//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef DISK_DEVICE_MANAGER_H
#define DISK_DEVICE_MANAGER_H

#include <Looper.h>

class DiskDeviceManager : public BLooper {
public:
	DiskDeviceManager();
	virtual ~DiskDeviceManager();

	virtual void MessageReceived(BMessage *message);
};

#endif	// DISK_DEVICE_MANAGER_H
