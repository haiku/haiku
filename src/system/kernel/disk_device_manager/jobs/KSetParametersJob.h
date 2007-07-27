// KSetParametersJob.h

#ifndef _K_DISK_DEVICE_SET_PARAMETERS_JOB_H
#define _K_DISK_DEVICE_SET_PARAMETERS_JOB_H

#include "KDiskDeviceJob.h"

namespace BPrivate {
namespace DiskDevice {


/**
 * Sets the parameters for partition and/or for its content
 * 
 * - can set both partition parameters and parameters for its content - 
 * 	but it's possible to create it with only one of these parameters
 */
class KSetParametersJob : public KDiskDeviceJob {
public:
	/**
	 * Creates the job.
	 * 
	 * \param partition the partition whose params (or content params) should be set
	 * \param parameters the new parameters for the partition
	 * \param contentParameters the new parameters for partition's content
	 */
	KSetParametersJob(partition_id parent, partition_id partition,
					  const char *parameters, const char *contentParameters);
	virtual ~KSetParametersJob();

	virtual status_t Do();
	
	
private:
		char * fParams, * fContentParams;
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KSetParametersJob;

#endif	// _DISK_DEVICE_SET_PARAMETERS_JOB_H
