// KSetParametersJob.h

#ifndef _K_DISK_DEVICE_SET_PARAMETERS_JOB_H
#define _K_DISK_DEVICE_SET_PARAMETERS_JOB_H

namespace BPrivate {
namespace DiskDevice {

#include "KDiskDeviceJob.h"

class KSetParametersJob : public KDiskDeviceJob {
public:
	KSetParametersJob(partition_id parent, partition_id partition,
					  const char *parameters, const char *contentParameters);
	virtual ~KSetParametersJob();

	virtual status_t Do();
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KSetParametersJob;

#endif	// _DISK_DEVICE_SET_PARAMETERS_JOB_H
