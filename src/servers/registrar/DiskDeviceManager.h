//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef DISK_DEVICE_MANAGER_H
#define DISK_DEVICE_MANAGER_H

#include <Locker.h>
#include <Looper.h>
#include <OS.h>

#include "PriorityMessageQueue.h"
#include "RDiskDeviceList.h"
#include "RVolumeList.h"
#include "WatchingService.h"

class EventQueue;
class MessageEvent;

class DiskDeviceManager : public BLooper {
public:
	DiskDeviceManager(EventQueue *eventQueue);
	virtual ~DiskDeviceManager();

	virtual void MessageReceived(BMessage *message);

private:
	// requests
	void _NextDiskDeviceRequest(BMessage *request);
	void _GetDiskDeviceRequest(BMessage *request);
	void _UpdateDiskDeviceRequest(BMessage *request);
	void _StartWatchingRequest(BMessage *request);
	void _StopWatchingRequest(BMessage *request);

	bool _PushMessage(BMessage *message, int32 priority);
	BMessage *_PopMessage();

	static int32 _WorkerEntry(void *parameters);
	int32 _Worker();

private:
	class RescanEvent;

private:
	EventQueue				*fEventQueue;
	MessageEvent			*fRescanEvent;
	BLocker					fDeviceListLock;
	WatchingService			fWatchingService;
	RVolumeList				fVolumeList;
	RDiskDeviceList			fDeviceList;
	PriorityMessageQueue	fMessageQueue;
	thread_id				fWorker;
	sem_id					fMessageCounter;
	bool					fTerminating;
};

#endif	// DISK_DEVICE_MANAGER_H
