//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <string.h>

#include <Message.h>
#include <NodeMonitor.h>
#include <Volume.h>

#include "RVolumeList.h"
#include "Debug.h"

// CompareIDPredicate
struct CompareIDPredicate : public UnaryPredicate<RVolume> {
	CompareIDPredicate(dev_t id) : fID(id) {}
	CompareIDPredicate(const RVolume *volume) : fID(volume->ID()) {}

	virtual int operator()(const RVolume *volume) const
	{
		dev_t id = volume->ID();
		if (fID < id)
			return -1;
		if (fID > id)
			return 1;
		return 0;
	}
	
private:
	dev_t	fID;
};

// CompareDevicePathPredicate
struct CompareDevicePathPredicate : public UnaryPredicate<RVolume> {
	CompareDevicePathPredicate(const char *path) : fPath(path) {}

	virtual int operator()(const RVolume *volume) const
	{
		return strcmp(fPath, volume->DevicePath());
	}
	
private:
	const char	*fPath;
};


// RVolume

// constructor
RVolume::RVolume()
	: fID(-1),
	  fRootNode(-1)
{
}

// SetTo
status_t
RVolume::SetTo(dev_t id)
{
	status_t error = B_OK;
	fs_info info;
	error = fs_stat_dev(id, &info);
	if (error == B_OK) {
		fID = id;
		fRootNode = info.root;
		strcpy(fDevicePath, info.device_name);
	} else
		Unset();
	return error;
}

// Unset
void
RVolume::Unset()
{
	fID = -1;
	fRootNode = -1;
}


// RVolumeList

// constructor
RVolumeList::RVolumeList()
	: BHandler(),
	  fVolumes(20, true),
	  fRoster()
{
}

// destructor
RVolumeList::~RVolumeList()
{
}

// MessageReceived
void
RVolumeList::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_NODE_MONITOR:
		{
			int32 opcode = 0;
			if (message->FindInt32("opcode", &opcode) == B_OK) {
				switch (opcode) {
					case B_DEVICE_MOUNTED:
						_DeviceMounted(message);
						break;
					case B_DEVICE_UNMOUNTED:
						_DeviceUnmounted(message);
						break;
				}
			}
			break;
		}
		default:
			BHandler::MessageReceived(message);
			break;
	}
}

// SetNextHandler
void
RVolumeList::SetNextHandler(BHandler *handler)
{
	// stop watching volumes, if removed from looper
	if (!handler) {
// TODO: Remove work-around. At this time R5 libbe's watching functions don't
// work for us. We have an own implementation in `NodeMonitoring.cpp'.
//		fRoster.StopWatching();
		stop_watching(BMessenger(this));
	}
	BHandler::SetNextHandler(handler);
	// start watching volumes and rescan list, if added to looper
	if (handler) {
// TODO: Remove work-around. At this time R5 libbe's watching functions don't
// work for us. We have an own implementation in `NodeMonitoring.cpp'.
//		fRoster.StartWatching(BMessenger(this));
		watch_node(NULL, B_WATCH_MOUNT, BMessenger(this));
		Rescan();
	}
}

// Rescan
status_t
RVolumeList::Rescan()
{
	fVolumes.MakeEmpty();
	fRoster.Rewind();
	BVolume bVolume;
	while (fRoster.GetNextVolume(&bVolume) == B_OK)
		_AddVolume(bVolume.Device());
	return B_ERROR;
}

// VolumeForDevicePath
const RVolume *
RVolumeList::VolumeForDevicePath(const char *devicePath) const
{
// TODO: ...
	return NULL;
}

// Lock
bool
RVolumeList::Lock()
{
// TODO: ...
	return false;
}

// Unlock
void
RVolumeList::Unlock()
{
// TODO: ...
}

// Dump
void
RVolumeList::Dump() const
{
	for (int32 i = 0; RVolume *volume = fVolumes.ItemAt(i); i++) {
		printf("volume %ld:\n", volume->ID());
		printf("  root node: %lld", volume->RootNode());
		printf("  device:    `%s'\n", volume->DevicePath());
	}
}

// _AddVolume
void
RVolumeList::_AddVolume(dev_t id)
{
	if (RVolume *volume = new RVolume) {
		status_t error = volume->SetTo(id);
		if (error == B_OK) {
			fVolumes.BinaryInsert(volume,
				CompareDevicePathPredicate(volume->DevicePath()));
		} else {
			delete volume;
			PRINT(("RVolumeList::_AddVolume(): initializing RVolume failed: "
				   "%s\n", strerror(error)));
		}
	} else
		PRINT(("RVolumeList::_AddVolume(): no memory!\n"));
}

// _DeviceMounted
void
RVolumeList::_DeviceMounted(BMessage *message)
{
	dev_t device;
	if (message->FindInt32("new device", &device) == B_OK) {
		PRINT(("RVolumeList::_DeviceMounted(%ld)\n", device));
		_AddVolume(device);
	}
}

// _DeviceUnmounted
void
RVolumeList::_DeviceUnmounted(BMessage *message)
{
	dev_t device;
	if (message->FindInt32("device", &device) == B_OK) {
		PRINT(("RVolumeList::_DeviceUnmounted(%ld)\n", device));
		if (RVolume *volume = fVolumes.FindIf(CompareIDPredicate(volume)))
			fVolumes.RemoveItem(volume);
	}
}

