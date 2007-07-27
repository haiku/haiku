/*
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Lubos Kulic <lubos@radical.ed>
 */
#ifndef _K_DISK_DEVICE_SET_NAME_JOB_H
#define _K_DISK_DEVICE_SET_NAME_JOB_H

#include <KDiskDeviceJob.h>

namespace BPrivate {

namespace DiskDevice {

/**
 * Sets the name for partition/device and/or its content
 * 
 * - can set both name and content name for the partition - but
 * 	it's possible to create it with only one of these name parameters and so 
 * 	set only one of the names
 */
class KSetNameJob : public KDiskDeviceJob {
public:
    KSetNameJob(partition_id parentID, partition_id partitionID,
		const char* name,  const char* contentName);
    virtual ~KSetNameJob();
	
	virtual status_t Do();
	
private:
	char*	fName;
	char*	fContentName; 
};

}
}

using BPrivate::DiskDevice::KSetNameJob;

#endif
