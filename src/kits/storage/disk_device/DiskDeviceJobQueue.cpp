/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "DiskDeviceJobQueue.h"

#include <stdio.h>
#include <string.h>

#include <typeinfo>

#include "DiskDeviceJob.h"


#undef TRACE
//#define TRACE(x...)
#define TRACE(x...)	printf(x)


// constructor
DiskDeviceJobQueue::DiskDeviceJobQueue()
	: fJobs(20)
{
}


// destructor
DiskDeviceJobQueue::~DiskDeviceJobQueue()
{
}


// AddJob
status_t
DiskDeviceJobQueue::AddJob(DiskDeviceJob* job)
{
	if (!job)
		return B_BAD_VALUE;

	return fJobs.AddItem(job) ? B_OK : B_NO_MEMORY;
}


// Execute
status_t
DiskDeviceJobQueue::Execute()
{
	int32 count = fJobs.CountItems();
	for (int32 i = 0; i < count; i++) {
		DiskDeviceJob* job = fJobs.ItemAt(i);

		TRACE("DiskDeviceJobQueue::Execute(): executing job: %s\n",
			typeid(*job).name());

		status_t error = job->Do();
		if (error != B_OK) {
			TRACE("DiskDeviceJobQueue::Execute(): executing job failed: %s\n",
				strerror(error));
			return error;
		}
	}

	return B_OK;
}
