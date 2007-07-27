/*
 * Copyright 2003-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 *		Lubos Kulic <lubos@radical.ed>
 */
#ifndef _K_DISK_DEVICE_INITIALIZE_JOB_H
#define _K_DISK_DEVICE_INITIALIZE_JOB_H

#include "KDiskDeviceJob.h"

namespace BPrivate {
namespace DiskDevice {

/**
 * Initializes the device with given disk system (i.e. partitioning or file system)
 */
class KInitializeJob : public KDiskDeviceJob {
public:
	KInitializeJob(partition_id partition, disk_system_id diskSystemID,
				   const char *name, const char *parameters);
	virtual ~KInitializeJob();

	virtual status_t Do();
	
private:
	disk_system_id fDiskSystemID;
	char * fName;
	char * fParameters;
		
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KInitializeJob;

#endif	// _DISK_DEVICE_INITIALIZE_JOB_H
