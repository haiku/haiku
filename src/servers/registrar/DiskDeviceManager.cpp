//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <NodeMonitor.h>
#include <RegistrarDefs.h>

#include "DiskDeviceManager.h"
#include "Debug.h"
#include "EventMaskWatcher.h"
#include "EventQueue.h"
#include "MessageEvent.h"
#include "RDiskDevice.h"
#include "RDiskDeviceList.h"
#include "RPartition.h"
#include "RSession.h"

// priorities of the different message kinds
enum {
	REQUEST_PRIORITY			= 0,
	RESCAN_PRIORITY				= 5,
	NODE_MONITOR_PRIORITY		= 10,
	WATCHING_REQUEST_PRIORITY	= 20,
};

// time interval between device rescans
static const bigtime_t kRescanEventInterval = 5000000;

// constructor
DiskDeviceManager::DiskDeviceManager(EventQueue *eventQueue)
	: BLooper("disk device manager"),
	  fEventQueue(eventQueue),
	  fRescanEvent(NULL),
	  fDeviceListLock(),
	  fWatchingService(),
	  fVolumeList(BMessenger(this), fDeviceListLock),
	  fDeviceList(BMessenger(this), fDeviceListLock, fVolumeList,
	  			  &fWatchingService),
	  fMessageQueue(),
	  fWorker(-1),
	  fMessageCounter(-1),
	  fTerminating(false)
{
	// set up the device list
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
	// set up the rescan event
	fRescanEvent = new MessageEvent(system_time() + kRescanEventInterval,
									this, B_REG_ROSTER_DEVICE_RESCAN);
	fRescanEvent->SetAutoDelete(false);
	fEventQueue->AddEvent(fRescanEvent);
}

// destructor
DiskDeviceManager::~DiskDeviceManager()
{
	// remove the rescan event from the event queue
	fEventQueue->RemoveEvent(fRescanEvent);
	// terminate the worker thread
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
			DetachCurrentMessage();
			if (!_PushMessage(message, REQUEST_PRIORITY))
				delete message;
			break;
		case B_REG_DEVICE_START_WATCHING:
		case B_REG_DEVICE_STOP_WATCHING:
			DetachCurrentMessage();
			if (!_PushMessage(message, WATCHING_REQUEST_PRIORITY))
				delete message;
			break;
		case B_REG_ROSTER_DEVICE_RESCAN:
			DetachCurrentMessage();
			if (!_PushMessage(message, RESCAN_PRIORITY))
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
DiskDeviceManager::_NextDiskDeviceRequest(BMessage *request)
{
	FUNCTION_START();
	Lock();
	status_t error = B_OK;
	int32 cookie = 0;
	BMessage reply(B_REG_RESULT);
	if (request->FindInt32("cookie", &cookie) == B_OK) {
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
	request->SendReply(&reply);
	Unlock();
}

// _GetDiskDeviceRequest
void
DiskDeviceManager::_GetDiskDeviceRequest(BMessage *request)
{
	Lock();
	status_t error = B_OK;
	// get the device
	RDiskDevice *device = NULL;
	int32 id = 0;
	if (request->FindInt32("device_id", &id) == B_OK)
		device = fDeviceList.DeviceWithID(id);
	else if (request->FindInt32("session_id", &id) == B_OK) {
		if (RSession *session = fDeviceList.SessionWithID(id))
			device = session->Device();
	} else if (request->FindInt32("partition_id", &id) == B_OK) {
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
	} else	// requested object not found
		error = B_ENTRY_NOT_FOUND;
	// add result and send reply
	reply.AddInt32("result", error);
	request->SendReply(&reply);
	Unlock();
}

// _UpdateDiskDeviceRequest
void
DiskDeviceManager::_UpdateDiskDeviceRequest(BMessage *request)
{
	Lock();
	status_t error = B_OK;
	// get the device and check the object is up to date
	bool upToDate = false;
	RDiskDevice *device = NULL;
	int32 id = 0;
	int32 changeCounter = 0;
	uint32 policy = 0;
	if (request->FindInt32("change_counter", &changeCounter) != B_OK
		|| request->FindInt32("update_policy", (int32*)&policy) != B_OK) {
		// bad request request
		SET_ERROR(error, B_BAD_VALUE);
	} else if (request->FindInt32("device_id", &id) == B_OK) {
		device = fDeviceList.DeviceWithID(id);
		if (device)
			upToDate = (device->ChangeCounter() == changeCounter);
PRINT(("DiskDeviceManager::_UpdateDiskDeviceRequest(): device id: %ld, "
"device: %p\n", id, device));
	} else if (request->FindInt32("session_id", &id) == B_OK) {
		if (RSession *session = fDeviceList.SessionWithID(id)) {
			device = session->Device();
			upToDate = (session->ChangeCounter() == changeCounter);
		}
	} else if (request->FindInt32("partition_id", &id) == B_OK) {
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
	} else	// requested object not found
		error = B_ENTRY_NOT_FOUND;
	// add result and send reply
	if (error == B_OK)
		error = reply.AddBool("up_to_date", upToDate);
	reply.AddInt32("result", error);
	request->SendReply(&reply);
	Unlock();
}

// _StartWatchingRequest
void
DiskDeviceManager::_StartWatchingRequest(BMessage *request)
{
	FUNCTION_START();
	Lock();
	status_t error = B_OK;
	// get the parameters
	BMessenger target;
	uint32 events;
	if (error == B_OK
		&& (request->FindMessenger("target", &target) != B_OK
			|| request->FindInt32("events", (int32*)&events) != B_OK)) {
		SET_ERROR(error, B_BAD_VALUE);
	}
	// add the new watcher
	if (error == B_OK) {
		Watcher *watcher = new(nothrow) EventMaskWatcher(target, events);
		if (watcher) {
			if (!fWatchingService.AddWatcher(watcher)) {
				SET_ERROR(error, B_NO_MEMORY);
				delete watcher;
			}
		} else
			SET_ERROR(error, B_NO_MEMORY);
	}
	// add result and send reply
	BMessage reply(B_REG_RESULT);
	reply.AddInt32("result", error);
	request->SendReply(&reply);
	Unlock();
};

// _StopWatchingRequest
void
DiskDeviceManager::_StopWatchingRequest(BMessage *request)
{
	FUNCTION_START();
	Lock();
	status_t error = B_OK;
	// get the parameters
	BMessenger target;
	if (error == B_OK && request->FindMessenger("target", &target) != B_OK)
		error = B_BAD_VALUE;
	// remove the watcher
	if (error == B_OK) {
		if (!fWatchingService.RemoveWatcher(target))
			error = B_BAD_VALUE;
	}
	// add result and send reply
	BMessage reply(B_REG_RESULT);
	reply.AddInt32("result", error);
	request->SendReply(&reply);
	Unlock();
};

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
					_StartWatchingRequest(message);
					break;
				case B_REG_DEVICE_STOP_WATCHING:
					_StopWatchingRequest(message);
					break;
				case B_REG_ROSTER_DEVICE_RESCAN:
					fDeviceList.Rescan();
					fRescanEvent->SetTime(system_time()
										  + kRescanEventInterval);
					fEventQueue->AddEvent(fRescanEvent);
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

