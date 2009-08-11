/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler, haiku@Clemens-Zeidler.de
 */


#include "DriverInterface.h"

#include <Autolock.h>
#include <Messenger.h>


Monitor::~Monitor()
{
}


status_t
Monitor::StartWatching(BHandler* target)
{
	if (fWatcherList.HasItem(target))
		return B_ERROR;

	fWatcherList.AddItem(target);
	return B_OK;
}


status_t
Monitor::StopWatching(BHandler* target)
{
	return fWatcherList.RemoveItem(target);
}


void
Monitor::Broadcast(uint32 message)
{
	for (int i = 0; i < fWatcherList.CountItems(); i++) {
		BMessenger messenger(fWatcherList.ItemAt(i));
		messenger.SendMessage(message);
	}
}


PowerStatusDriverInterface::PowerStatusDriverInterface()
	:
	fIsWatching(0),
	fThread(-1)
{

}


PowerStatusDriverInterface::~PowerStatusDriverInterface()
{

}


status_t
PowerStatusDriverInterface::StartWatching(BHandler* target)
{
	BAutolock autolock(fListLocker);
	status_t status = Monitor::StartWatching(target);

	if (status != B_OK)
		return status;

	if (fThread > 0)
		return B_OK;

	fThread = spawn_thread(&_ThreadWatchPowerFunction, "PowerStatusThread",
		B_LOW_PRIORITY, this);
	if (fThread >= 0) {
		atomic_set(&fIsWatching, 1);
		status = resume_thread(fThread);
	} else
		return fThread;

	if (status != B_OK && fWatcherList.CountItems() == 0)
		atomic_set(&fIsWatching, 0);

	return status;
}


status_t
PowerStatusDriverInterface::StopWatching(BHandler* target)
{
	BAutolock autolock(fListLocker);
	if (fThread < 0)
		return B_BAD_VALUE;

	if (fWatcherList.CountItems() == 1) {
		atomic_set(&fIsWatching, 0);
		wait_for_thread(fThread, NULL);
		fThread = -1;
	}

	return Monitor::StopWatching(target);
}


void
PowerStatusDriverInterface::Broadcast(uint32 message)
{
	BAutolock autolock(fListLocker);
	Monitor::Broadcast(message);
}


void
PowerStatusDriverInterface::Disconnect()
{
	atomic_set(&fIsWatching, 0);
	wait_for_thread(fThread, NULL);
	fThread = -1;
}


int32
PowerStatusDriverInterface::_ThreadWatchPowerFunction(void* data)
{
	PowerStatusDriverInterface* that = (PowerStatusDriverInterface*)data;
	that->_WatchPowerStatus();
	return 0;
}

