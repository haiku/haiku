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

class DiskDeviceManager : public BLooper {
public:
	DiskDeviceManager();
	virtual ~DiskDeviceManager();

	virtual void MessageReceived(BMessage *message);

private:
	// requests
	void _NextDiskDeviceRequest(BMessage *message);
	void _GetDiskDeviceRequest(BMessage *message);
	void _UpdateDiskDeviceRequest(BMessage *message);

	bool _PushMessage(BMessage *message, int32 priority);
	BMessage *_PopMessage();

	static int32 _WorkerEntry(void *parameters);
	int32 _Worker();

private:
	BLocker					fDeviceListLock;
	RVolumeList				fVolumeList;
	RDiskDeviceList			fDeviceList;
	PriorityMessageQueue	fMessageQueue;
	thread_id				fWorker;
	sem_id					fMessageCounter;
	bool					fTerminating;
};

#endif	// DISK_DEVICE_MANAGER_H
