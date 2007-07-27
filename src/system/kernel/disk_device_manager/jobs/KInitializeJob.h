// KInitializeJob.h

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
	/**
	 * Creates the job.
	 * 
	 * \param partition the partition to initialize
	 * \param diskSystemID which disk system the partition should be initialized with
	 * \param parameters additional parameters for the operation
	 */
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
