// KDeleteChildJob.h

#ifndef _K_DISK_DEVICE_DELETE_CHILD_JOB_H
#define _K_DISK_DEVICE_DELETE_CHILD_JOB_H

#include "KDiskDeviceJob.h"

namespace BPrivate {
namespace DiskDevice {

/**
 * Deletes specific child of a partition/device
 */
class KDeleteChildJob : public KDiskDeviceJob {
public:
	/**
	 * Creates the job
	 * 
	 * \param parent device of the deleted partition
	 * \param partition partition supposed to be removed
	 */
	KDeleteChildJob(partition_id parent, partition_id partition);
	virtual ~KDeleteChildJob();

	virtual status_t Do();
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KDeleteChildJob;

#endif	// _DISK_DEVICE_DELETE_CHILD_JOB_H
