// KResizeJob.h

#ifndef _K_DISK_DEVICE_RESIZE_JOB_H
#define _K_DISK_DEVICE_RESIZE_JOB_H

#include "KDiskDeviceJob.h"

namespace BPrivate {
namespace DiskDevice {

class KResizeJob : public KDiskDeviceJob {
public:
	KResizeJob(partition_id parentID, partition_id partitionID, off_t size,
			   bool resizeContents);
	virtual ~KResizeJob();

	virtual status_t Do();

private:
	bool	fResizeContents;
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KResizeJob;

#endif	// _DISK_DEVICE_RESIZE_JOB_H
