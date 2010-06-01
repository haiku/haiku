/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "IOScheduler.h"

#include <stdlib.h>
#include <string.h>

#include "IOSchedulerRoster.h"


IOScheduler::IOScheduler(DMAResource* resource)
	:
	fDMAResource(resource),
	fName(NULL),
	fID(IOSchedulerRoster::Default()->NextID()),
	fIOCallback(NULL),
	fIOCallbackData(NULL),
	fSchedulerRegistered(false)
{
}


IOScheduler::~IOScheduler()
{
	if (fSchedulerRegistered)
		IOSchedulerRoster::Default()->RemoveScheduler(this);

	free(fName);
}


status_t
IOScheduler::Init(const char* name)
{
	fName = strdup(name);
	if (fName == NULL)
		return B_NO_MEMORY;

	IOSchedulerRoster::Default()->AddScheduler(this);
	fSchedulerRegistered = true;

	return B_OK;
}


void
IOScheduler::SetCallback(IOCallback& callback)
{
	SetCallback(&IOCallback::WrapperFunction, &callback);
}


void
IOScheduler::SetCallback(io_callback callback, void* data)
{
	fIOCallback = callback;
	fIOCallbackData = data;
}


void
IOScheduler::SetDeviceCapacity(off_t deviceCapacity)
{
}


void
IOScheduler::MediaChanged()
{
}
