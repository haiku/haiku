/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


//! Empty main temporary directory


#include <fs_volume.h>

#include "InitSharedMemoryDirectoryJob.h"


InitSharedMemoryDirectoryJob::InitSharedMemoryDirectoryJob()
	:
	AbstractEmptyDirectoryJob("init /var/shared_memory")
{
}


status_t
InitSharedMemoryDirectoryJob::Execute()
{
	status_t status = CreateAndEmpty("/var/shared_memory");
	if (status != B_OK)
		return status;

	status = fs_mount_volume("/var/shared_memory", NULL, "ramfs", 0, NULL);
	if (status < B_OK)
		return status;

	chmod("/var/shared_memory", 0777);
	return B_OK;
}
