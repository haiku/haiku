/*
 * Copyright 2009-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "IOSchedulerRoster.h"

#include <util/AutoLock.h>


/*static*/ IOSchedulerRoster IOSchedulerRoster::sDefaultInstance;


/*static*/ void
IOSchedulerRoster::Init()
{
	new(&sDefaultInstance) IOSchedulerRoster;
}


void
IOSchedulerRoster::AddScheduler(IOScheduler* scheduler)
{
	AutoLocker<IOSchedulerRoster> locker(this);
	fSchedulers.Add(scheduler);
	locker.Unlock();

	Notify(IO_SCHEDULER_ADDED, scheduler);
}


void
IOSchedulerRoster::RemoveScheduler(IOScheduler* scheduler)
{
	AutoLocker<IOSchedulerRoster> locker(this);
	fSchedulers.Remove(scheduler);
	locker.Unlock();

	Notify(IO_SCHEDULER_REMOVED, scheduler);
}


void
IOSchedulerRoster::Notify(uint32 eventCode, const IOScheduler* scheduler,
	IORequest* request, IOOperation* operation)
{
	AutoLocker<DefaultNotificationService> locker(fNotificationService);

	if (!fNotificationService.HasListeners())
		return;

	KMessage event;
	event.SetTo(fEventBuffer, sizeof(fEventBuffer), IO_SCHEDULER_MONITOR);
	event.AddInt32("event", eventCode);
	event.AddPointer("scheduler", scheduler);
	if (request != NULL) {
		event.AddPointer("request", request);
		if (operation != NULL)
			event.AddPointer("operation", operation);
	}

	fNotificationService.NotifyLocked(event, eventCode);
}


int32
IOSchedulerRoster::NextID()
{
	AutoLocker<IOSchedulerRoster> locker(this);
	return fNextID++;
}


IOSchedulerRoster::IOSchedulerRoster()
	:
	fNextID(1),
	fNotificationService("I/O")
{
	mutex_init(&fLock, "IOSchedulerRoster");
}


IOSchedulerRoster::~IOSchedulerRoster()
{
	mutex_destroy(&fLock);
}
