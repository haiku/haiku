//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <RegistrarDefs.h>

#include "DiskDeviceManager.h"

// constructor
DiskDeviceManager::DiskDeviceManager()
	: BLooper("disk device manager")
{
}

// destructor
DiskDeviceManager::~DiskDeviceManager()
{
}

// MessageReceived
void
DiskDeviceManager::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_REG_NEXT_DISK_DEVICE:
		case B_REG_GET_DISK_DEVICE:
		case B_REG_UPDATE_DISK_DEVICE:
		case B_REG_DEVICE_START_WATCHING:
		case B_REG_DEVICE_STOP_WATCHING:
			break;
		default:
			BLooper::MessageReceived(message);
			break;
	}
}

