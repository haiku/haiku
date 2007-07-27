// KResizeJob.h

#ifndef _K_DISK_DEVICE_RESIZE_JOB_H
#define _K_DISK_DEVICE_RESIZE_JOB_H

#include "KDiskDeviceJob.h"

namespace BPrivate {
namespace DiskDevice {

/**
 * Resizes a partition.
 */
class KResizeJob : public KDiskDeviceJob {
public:
	/**
	 * Creates the job.
	 * 
	 * \param parentID the device whose child should be resized
	 * \param partitionID the partition which should be resized
	 * \param size new size for the partition 
	 */
	KResizeJob(partition_id parentID, partition_id partitionID, off_t size);
	virtual ~KResizeJob();

	virtual status_t Do();

private:
	off_t	fSize;
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KResizeJob;

#endif	// _DISK_DEVICE_RESIZE_JOB_H
