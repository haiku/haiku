/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


//! Empty main temporary directory


#include "InitTemporaryDirectoryJob.h"

#include <FindDirectory.h>
#include <Path.h>


InitTemporaryDirectoryJob::InitTemporaryDirectoryJob()
	:
	AbstractEmptyDirectoryJob("init /tmp")
{
}


status_t
InitTemporaryDirectoryJob::Execute()
{
	// TODO: the /tmp entries could be scanned synchronously, and deleted
	// later
	BPath path;
	status_t status = find_directory(B_SYSTEM_TEMP_DIRECTORY, &path, true);
	if (status == B_OK)
		status = CreateAndEmpty(path.Path());

	chmod(path.Path(), 0777);
	return status;
}
