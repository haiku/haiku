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
class KSetNameJob : public KDiskDeviceJob
{
public:
	/**
	 * Creates the job.
	 * 	 
	 * 
	 * \param partitionID the partition/device whose name should be set
	 * \param name the new name for \c partitionID
	 * \param contentName the new name for the content of \c partitionID
	 */
    KSetNameJob(partition_id parentID, partition_id partitionID, const char * name, 
			   const char * contentName);

    virtual ~KSetNameJob();
	
	virtual status_t Do();
	
private:
	char * fName, *fContentName; 
		

};

}

}

using BPrivate::DiskDevice::KSetNameJob;

#endif
