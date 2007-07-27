// KCreateChildJob.h

#ifndef _K_DISK_DEVICE_CREATE_CHILD_JOB_H
#define _K_DISK_DEVICE_CREATE_CHILD_JOB_H

#include "KDiskDeviceJob.h"

namespace BPrivate {
namespace DiskDevice {

/**
 * Create new child of given partition
 */
class KCreateChildJob : public KDiskDeviceJob {
public:
	/**
	 * Creates the job.
	 * 
	 * \param partition whose children should we create
 	 * \param child new child ID
	 * \param offset where the child should start
	 * \param size size of the new child
	 * \param parameters additional parameters for the operation
	 */
	KCreateChildJob(partition_id partition, partition_id child, off_t offset,
					off_t size, const char *type, const char *parameters);
	virtual ~KCreateChildJob();

	virtual status_t Do();
	
private:
	partition_id fChildID;
	off_t fOffset, fSize;
	char * fType;
	char * fParameters;
	
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KCreateChildJob;

#endif	// _DISK_DEVICE_CREATE_CHILD_JOB_H
