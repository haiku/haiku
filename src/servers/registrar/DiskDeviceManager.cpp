//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <NodeMonitor.h>
#include <RegistrarDefs.h>

#include "DiskDeviceManager.h"

enum {
	REQUEST_PRIORITY		= 0,
	NODE_MONITOR_PRIORITY	= 10,
};

// constructor
DiskDeviceManager::DiskDeviceManager()
	: BLooper("disk device manager"),
	  fDeviceListLock(),
	  fVolumeList(BMessenger(this), fDeviceListLock),
	  fDeviceList(BMessenger(this), fDeviceListLock, fVolumeList),
	  fMessageQueue(),
	  fWorker(-1),
	  fMessageCounter(-1),
	  fTerminating(false)
{
	// set up the device list
	fVolumeList.Rescan();
	fVolumeList.Dump();
	fDeviceList.Rescan();
	fDeviceList.Dump();
	// start the worker thread
	fMessageCounter = create_sem(0, "disk device manager msgcounter");
	if (fMessageCounter >= 0) {
		fWorker = spawn_thread(_WorkerEntry, "disk device manager worker",
							   B_NORMAL_PRIORITY, this);
		if (fWorker >= 0)
			resume_thread(fWorker);
	}
}

// destructor
DiskDeviceManager::~DiskDeviceManager()
{
	fTerminating = true;
	delete_sem(fMessageCounter);
	int32 dummy;
	wait_for_thread(fWorker, &dummy);
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
			DetachCurrentMessage();
			if (!_PushMessage(message, REQUEST_PRIORITY))
				delete message;
			break;
		case B_NODE_MONITOR:
			DetachCurrentMessage();
			if (!_PushMessage(message, NODE_MONITOR_PRIORITY))
				delete message;
			break;
		default:
			BLooper::MessageReceived(message);
			break;
	}
}

// _PushMessage
bool
DiskDeviceManager::_PushMessage(BMessage *message, int32 priority)
{
	bool result = fMessageQueue.PushMessage(message, priority);
	if (result)
		release_sem(fMessageCounter);
	return result;
}

// _PopMessage
BMessage *
DiskDeviceManager::_PopMessage()
{
	BMessage *result = NULL;
	status_t error = acquire_sem(fMessageCounter);
	if (error == B_OK)
		result = fMessageQueue.PopMessage();
	return result;
}

// _WorkerEntry
int32
DiskDeviceManager::_WorkerEntry(void *parameters)
{
	return static_cast<DiskDeviceManager*>(parameters)->_Worker();
}

// _Worker
int32
DiskDeviceManager::_Worker()
{
	while (!fTerminating) {
		if (BMessage *message = _PopMessage()) {
			// dispatch the message
			switch (message->what) {
				case B_REG_NEXT_DISK_DEVICE:
				case B_REG_GET_DISK_DEVICE:
				case B_REG_UPDATE_DISK_DEVICE:
				case B_REG_DEVICE_START_WATCHING:
				case B_REG_DEVICE_STOP_WATCHING:
					fDeviceList.HandleMessage(message);
					break;
				case B_NODE_MONITOR:
				{
					int32 opcode = 0;
					if (message->FindInt32("opcode", &opcode) == B_OK) {
						if (opcode == B_DEVICE_MOUNTED
							|| opcode == B_DEVICE_UNMOUNTED) {
							fVolumeList.HandleMessage(message);
						} else {
							fDeviceList.HandleMessage(message);
						}
					}
					break;
				}
				default:
					// error
					break;
			}
			delete message;
		}
	}
	return 0;
}

