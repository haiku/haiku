// KMoveJob.h

#ifndef _K_DISK_DEVICE_MOVE_JOB_H
#define _K_DISK_DEVICE_MOVE_JOB_H

#include "KDiskDeviceJob.h" 

namespace BPrivate {
namespace DiskDevice {

/**
 * Moves given child of a partition/device
 */
class KMoveJob : public KDiskDeviceJob {
public:
	/**
	 * Creates the job.
	 * 
	 * \param parentID the device whose child should be moved
	 * \param partitionID the child to move
	 * \param offset where to move
	 */
	KMoveJob(partition_id parentID, partition_id partitionID, off_t offset);
	virtual ~KMoveJob();

	virtual status_t Do();
	
	
private:
	off_t fNewOffset;
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KMoveJob;

#endif	// _DISK_DEVICE_MOVE_JOB_H
