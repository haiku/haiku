/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _DISK_DEVICE_JOB_QUEUE_H
#define _DISK_DEVICE_JOB_QUEUE_H

#include <DiskDeviceDefs.h>
#include <ObjectList.h>


namespace BPrivate {


class DiskDeviceJob;


class DiskDeviceJobQueue {
public:
								DiskDeviceJobQueue();
								~DiskDeviceJobQueue();

			status_t			AddJob(DiskDeviceJob* job);

			status_t			Execute();

private:
	typedef	BObjectList<DiskDeviceJob> JobList;

			JobList				fJobs;
};


}	// namespace BPrivate

using BPrivate::DiskDeviceJobQueue;

#endif	// _DISK_DEVICE_JOB_QUEUE_H
