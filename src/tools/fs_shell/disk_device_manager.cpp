/*
 * Copyright 2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */

#include "fssh_disk_device_manager.h"

#include "fssh_errors.h"


fssh_status_t
fssh_scan_partition(fssh_partition_id partitionID)
{
	return FSSH_B_OK;
}


bool
fssh_update_disk_device_job_progress(fssh_disk_job_id jobID, float progress)
{
	return true;
}
