//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <RegistrarDefs.h>

#include "DiskDeviceManager.h"

// constructor
DiskDeviceManager::DiskDeviceManager()
	: BLooper("disk device manager"),
	  fDeviceList(),
	  fVolumeList()
{
	AddHandler(&fVolumeList);
	fVolumeList.Dump();
	fDeviceList.Rescan();
	fDeviceList.Dump();
}

// destructor
DiskDeviceManager::~DiskDeviceManager()
{
	Lock();
	RemoveHandler(&fVolumeList);
	Unlock();
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

