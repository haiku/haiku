/*
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Lubos Kulic <lubos@radical.ed>
 */
#ifndef _K_DISK_DEVICE_SET_TYPE_JOB_H
#define _K_DISK_DEVICE_SET_TYPE_JOB_H

#include <KDiskDeviceJob.h>

namespace BPrivate {

namespace DiskDevice {

/**
 * Sets type of the device/partition
 */
class KSetTypeJob : public KDiskDeviceJob
{
public:
	KSetTypeJob(partition_id parentID, partition_id partitionID, const char *type);
    virtual ~KSetTypeJob();
	
	virtual status_t Do();
	
private:
	char * fType;
};

}
}

using BPrivate::DiskDevice::KSetTypeJob;

#endif
