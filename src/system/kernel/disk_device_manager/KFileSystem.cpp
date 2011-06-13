/*
 * Copyright 2003-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 */

#include "KFileSystem.h"

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include <fs_interface.h>

#include "ddm_modules.h"
#include "KDiskDeviceUtils.h"
#include "KPartition.h"


// constructor
KFileSystem::KFileSystem(const char *name)
	: KDiskSystem(name),
	fModule(NULL)
{
}


// destructor
KFileSystem::~KFileSystem()
{
}


// Init
status_t
KFileSystem::Init()
{
	status_t error = KDiskSystem::Init();
	if (error != B_OK)
		return error;
	error = Load();
	if (error != B_OK)
		return error;
	error = SetShortName(fModule->short_name);
	if (error == B_OK)
		error = SetPrettyName(fModule->pretty_name);

	SetFlags(fModule->flags | B_DISK_SYSTEM_IS_FILE_SYSTEM);
	Unload();
	return error;
}


// Identify
float
KFileSystem::Identify(KPartition *partition, void **cookie)
{
	if (!partition || !cookie || !fModule || !fModule->identify_partition)
		return -1;
	int fd = -1;
	if (partition->Open(O_RDONLY, &fd) != B_OK)
		return -1;
	float result = fModule->identify_partition(fd, partition->PartitionData(),
		cookie);
	close(fd);
	return result;
}


// Scan
status_t
KFileSystem::Scan(KPartition *partition, void *cookie)
{
	if (!partition || !fModule || !fModule->scan_partition)
		return B_ERROR;
	int fd = -1;
	status_t result = partition->Open(O_RDONLY, &fd);
	if (result != B_OK)
		return result;
	result = fModule->scan_partition(fd, partition->PartitionData(), cookie);
	close(fd);
	return result;
}


// FreeIdentifyCookie
void
KFileSystem::FreeIdentifyCookie(KPartition *partition, void *cookie)
{
	if (!partition || !fModule || !fModule->free_identify_partition_cookie)
		return;
	fModule->free_identify_partition_cookie(partition->PartitionData(), cookie);
}


// FreeContentCookie
void
KFileSystem::FreeContentCookie(KPartition *partition)
{
	if (!partition || !fModule || !fModule->free_partition_content_cookie)
		return;
	fModule->free_partition_content_cookie(partition->PartitionData());
}


// Defragment
status_t
KFileSystem::Defragment(KPartition* partition, disk_job_id job)
{
	// to be implemented
	return B_ERROR;
}


// Repair
status_t
KFileSystem::Repair(KPartition* partition, bool checkOnly, disk_job_id job)
{
	// to be implemented
	return B_ERROR;
}


// Resize
status_t
KFileSystem::Resize(KPartition* partition, off_t size, disk_job_id job)
{
	// to be implemented
	return B_ERROR;
}


// Move
status_t
KFileSystem::Move(KPartition* partition, off_t offset, disk_job_id job)
{
	// to be implemented
	return B_ERROR;
}


// SetContentName
status_t
KFileSystem::SetContentName(KPartition* partition, const char* name,
	disk_job_id job)
{
	// check parameters
	if (!partition || !fModule)
		return B_BAD_VALUE;
	if (!fModule->set_content_name)
		return B_NOT_SUPPORTED;

	// open partition device
	int fd = -1;
	status_t result = partition->Open(O_RDWR, &fd);
	if (result != B_OK)
		return result;

	// let the module do its job
	result = fModule->set_content_name(fd, partition->ID(), name, job);

	// cleanup and return
	close(fd);
	return result;
}


// SetContentParameters
status_t
KFileSystem::SetContentParameters(KPartition* partition,
	const char* parameters, disk_job_id job)
{
	// check parameters
	if (!partition || !fModule)
		return B_BAD_VALUE;
	if (!fModule->set_content_parameters)
		return B_NOT_SUPPORTED;

	// open partition device
	int fd = -1;
	status_t result = partition->Open(O_RDWR, &fd);
	if (result != B_OK)
		return result;

	// let the module do its job
	result = fModule->set_content_parameters(fd, partition->ID(), parameters,
		job);

	// cleanup and return
	close(fd);
	return result;
}


// Initialize
status_t
KFileSystem::Initialize(KPartition* partition, const char* name,
	const char* parameters, disk_job_id job)
{
	// check parameters
	if (!partition || !fModule)
		return B_BAD_VALUE;
	if (!fModule->initialize)
		return B_NOT_SUPPORTED;

	// open partition device
	int fd = -1;
	status_t result = partition->Open(O_RDWR, &fd);
	if (result != B_OK)
		return result;

	// let the module do its job
	result = fModule->initialize(fd, partition->ID(), name, parameters,
		partition->Size(), job);

	// cleanup and return
	close(fd);
	return result;
}


status_t
KFileSystem::Uninitialize(KPartition* partition, disk_job_id job)
{
	// check parameters
	if (partition == NULL || fModule == NULL)
		return B_BAD_VALUE;
	if (fModule->uninitialize == NULL)
		return B_NOT_SUPPORTED;

	// open partition device
	int fd = -1;
	status_t result = partition->Open(O_RDWR, &fd);
	if (result != B_OK)
		return result;

	// let the module do its job
	result = fModule->uninitialize(fd, partition->ID(), partition->Size(),
		partition->BlockSize(), job);

	// cleanup and return
	close(fd);
	return result;
}


// LoadModule
status_t
KFileSystem::LoadModule()
{
	if (fModule)		// shouldn't happen
		return B_OK;

	return get_module(Name(), (module_info **)&fModule);
}


// UnloadModule
void
KFileSystem::UnloadModule()
{
	if (fModule) {
		put_module(fModule->info.name);
		fModule = NULL;
	}
}

