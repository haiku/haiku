//
// C++ Interface: KSetTypeJob
//
// Description: 
//
//
// Author:  <lubos@radical.ed>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
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
	/**
	 * Creates the job.
	 * 
	 * \param partitionID the partition whose type should be set
	 * \param type the new type for the partition
	 */
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
