//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <NodeMonitor.h>
#include <RegistrarDefs.h>

#include "DiskDeviceManager.h"
#include "RDiskDevice.h"
#include "RDiskDeviceList.h"
#include "RPartition.h"
#include "RSession.h"
#include "Debug.h"

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

// _NextDiskDeviceRequest
void
DiskDeviceManager::_NextDiskDeviceRequest(BMessage *message)
{
	FUNCTION_START();
	status_t error = B_OK;
	int32 cookie = 0;
	BMessage reply(B_REG_RESULT);
	if (message->FindInt32("cookie", &cookie) == B_OK) {
		// get next device
		if (RDiskDevice *device = fDeviceList.DeviceWithID(cookie, false)) {
			// archive device
			BMessage deviceArchive;
			SET_ERROR(error, device->Archive(&deviceArchive));
			// add archived device and next cookie to reply message
			if (error == B_OK)
				SET_ERROR(error, reply.AddMessage("device", &deviceArchive));
			if (error == B_OK)
				SET_ERROR(error, reply.AddInt32("cookie", device->ID() + 1));
		} else
			error = B_ENTRY_NOT_FOUND;
	} else
		SET_ERROR(error, B_BAD_VALUE);
	// add result and send reply
	reply.AddInt32("result", error);
	message->SendReply(&reply);
}

// _GetDiskDeviceRequest
void
DiskDeviceManager::_GetDiskDeviceRequest(BMessage *message)
{
	status_t error = B_ENTRY_NOT_FOUND;
	// get the device
	RDiskDevice *device = NULL;
	int32 id = 0;
	if (message->FindInt32("device_id", &id) == B_OK)
		device = fDeviceList.DeviceWithID(id);
	else if (message->FindInt32("session_id", &id) == B_OK) {
		if (RSession *session = fDeviceList.SessionWithID(id))
			device = session->Device();
	} else if (message->FindInt32("partition_id", &id) == B_OK) {
		if (RPartition *partition = fDeviceList.PartitionWithID(id))
			device = partition->Device();
	} else
		error = B_BAD_VALUE;
	// archive device and add it to the reply message
	BMessage reply(B_REG_RESULT);
	if (device) {
		BMessage deviceArchive;
		error = device->Archive(&deviceArchive);
		if (error == B_OK)
			error = reply.AddMessage("device", &deviceArchive);
	}
	// add result and send reply
	reply.AddInt32("result", error);
	message->SendReply(&reply);
}

// _UpdateDiskDeviceRequest
void
DiskDeviceManager::_UpdateDiskDeviceRequest(BMessage *message)
{
	status_t error = B_ENTRY_NOT_FOUND;
	// get the device and check the object is up to date
	bool upToDate = false;
	RDiskDevice *device = NULL;
	int32 id = 0;
	int32 changeCounter = 0;
	uint32 policy = 0;
	if (message->FindInt32("change_counter", &changeCounter) != B_OK
		|| message->FindInt32("update_policy", (int32*)&policy) != B_OK) {
		// bad request message
		SET_ERROR(error, B_BAD_VALUE);
	} else if (message->FindInt32("device_id", &id) == B_OK) {
		device = fDeviceList.DeviceWithID(id);
		if (device)
			upToDate = (device->ChangeCounter() == changeCounter);
	} else if (message->FindInt32("session_id", &id) == B_OK) {
		if (RSession *session = fDeviceList.SessionWithID(id)) {
			device = session->Device();
			upToDate = (session->ChangeCounter() == changeCounter);
		}
	} else if (message->FindInt32("partition_id", &id) == B_OK) {
		if (RPartition *partition = fDeviceList.PartitionWithID(id)) {
			device = partition->Device();
			upToDate = (partition->ChangeCounter() == changeCounter);
		}
	} else	// bad request message
		SET_ERROR(error, B_BAD_VALUE);
	// archive device and add it to the reply message
	BMessage reply(B_REG_RESULT);
	if (device) {
		if (policy == B_REG_DEVICE_UPDATE_DEVICE_CHANGED)
			upToDate = (device->ChangeCounter() == changeCounter);
		if (!upToDate && policy != B_REG_DEVICE_UPDATE_CHECK) {
			BMessage deviceArchive;
			error = device->Archive(&deviceArchive);
			if (error == B_OK)
				error = reply.AddMessage("device", &deviceArchive);
		}
	}
	// add result and send reply
	if (error == B_OK)
		error = reply.AddBool("up_to_date", upToDate);
	reply.AddInt32("result", error);
	message->SendReply(&reply);
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
					_NextDiskDeviceRequest(message);
					break;
				case B_REG_GET_DISK_DEVICE:
					_GetDiskDeviceRequest(message);
					break;
				case B_REG_UPDATE_DISK_DEVICE:
					_UpdateDiskDeviceRequest(message);
					break;
				case B_REG_DEVICE_START_WATCHING:
				case B_REG_DEVICE_STOP_WATCHING:
					break;
				case B_NODE_MONITOR:
				{
					fVolumeList.HandleMessage(message);
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

