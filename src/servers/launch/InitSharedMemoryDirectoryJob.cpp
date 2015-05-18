/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


//! Empty main temporary directory


#include "InitSharedMemoryDirectoryJob.h"


InitSharedMemoryDirectoryJob::InitSharedMemoryDirectoryJob()
	:
	AbstractEmptyDirectoryJob("init /var/shared_memory")
{
}


status_t
InitSharedMemoryDirectoryJob::Execute()
{
	return CreateAndEmpty("/var/shared_memory");
}
