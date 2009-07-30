/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
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
	BAutolock autolock(fListLocker);
	if (fWatcherList.HasItem(target))
		return B_ERROR;

	fWatcherList.AddItem(target);
	return B_OK;
}


status_t
Monitor::StopWatching(BHandler* target)
{
	BAutolock autolock(fListLocker);
	return fWatcherList.RemoveItem(target);
}


void
Monitor::Broadcast(uint32 message)
{
	for (int i = 0; i < fWatcherList.CountItems(); i++)
	{
		BMessenger messenger(fWatcherList.ItemAt(i));
		messenger.SendMessage(message);
	}
}


PowerStatusDriverInterface::PowerStatusDriverInterface()
	:
	fIsWatching(0),
	fThreadId(-1)
{
	
}

PowerStatusDriverInterface::~PowerStatusDriverInterface()
{
	
}

#include <stdio.h>
status_t
PowerStatusDriverInterface::StartWatching(BHandler* target)
{
	status_t status = Monitor::StartWatching(target);
	
	if (status != B_OK)
		return status;

	if (fThreadId > 0)
		return B_OK;

	printf("spawn\n");
	fThreadId = spawn_thread(&_ThreadWatchPowerFunction, "PowerStatusThread",
								B_LOW_PRIORITY, this);
	if (fThreadId >= 0) {
		atomic_set(&fIsWatching, 1);
		status = resume_thread(fThreadId);
	}
	else
		return fThreadId;

	if (status != B_OK && fWatcherList.CountItems() == 0) {
		atomic_set(&fIsWatching, 0);
	}
	return status;
	
}


status_t
PowerStatusDriverInterface::StopWatching(BHandler* target)
{
	if (fThreadId < 0)
		return B_BAD_VALUE;

	status_t status;
	if (fWatcherList.CountItems() == 1) {
		atomic_set(&fIsWatching, 0);
	
		status = wait_for_thread(fThreadId, &status);
		fThreadId = -1;
	}
	
	return Monitor::StopWatching(target);
}


void
PowerStatusDriverInterface::Disconnect()
{
	atomic_set(&fIsWatching, 0);
	status_t status;
	wait_for_thread(fThreadId, &status);
	fThreadId = -1;
}


int32
PowerStatusDriverInterface::_ThreadWatchPowerFunction(void* data)
{
	PowerStatusDriverInterface* that = (PowerStatusDriverInterface*)data;
	that->_WatchPowerStatus();
	return 0;
}

