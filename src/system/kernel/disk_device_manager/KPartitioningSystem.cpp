/*
 * Copyright 2003-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 *		Lubos Kulic <lubos@radical.ed>
 */

/**	\file KPartitioningSystem.cpp
 *
 * 	\brief Implementation of \ref KPartitioningSystem class
 */

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include <ddm_modules.h>
#include <KDiskDevice.h>
//#include <KDiskDeviceJob.h>
#include <KDiskDeviceManager.h>
#include <KDiskDeviceUtils.h>
#include <KPartition.h>
#include <KPartitioningSystem.h>


// constructor
KPartitioningSystem::KPartitioningSystem(const char *name)
	: KDiskSystem(name),
	  fModule(NULL)
{
}


// destructor
KPartitioningSystem::~KPartitioningSystem()
{
}


// Init
status_t
KPartitioningSystem::Init()
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

	SetFlags(fModule->flags & ~(uint32)B_DISK_SYSTEM_IS_FILE_SYSTEM);
	Unload();
	return error;
}


// Identify
//! Try to identify a given partition
float
KPartitioningSystem::Identify(KPartition *partition, void **cookie)
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
//! Scan the partition
status_t
KPartitioningSystem::Scan(KPartition *partition, void *cookie)
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
KPartitioningSystem::FreeIdentifyCookie(KPartition *partition, void *cookie)
{
	if (!partition || !fModule || !fModule->free_identify_partition_cookie)
		return;
	fModule->free_identify_partition_cookie(partition->PartitionData(),
		cookie);
}


// FreeCookie
void
KPartitioningSystem::FreeCookie(KPartition *partition)
{
	if (!partition || !fModule || !fModule->free_partition_cookie
		|| partition->ParentDiskSystem() != this) {
		return;
	}
	fModule->free_partition_cookie(partition->PartitionData());
	partition->SetCookie(NULL);
}


// FreeContentCookie
void
KPartitioningSystem::FreeContentCookie(KPartition *partition)
{
	if (!partition || !fModule || !fModule->free_partition_content_cookie
		|| partition->DiskSystem() != this) {
		return;
	}
	fModule->free_partition_content_cookie(partition->PartitionData());
	partition->SetContentCookie(NULL);
}


// Repair
//! Repairs a partition
status_t
KPartitioningSystem::Repair(KPartition* partition, bool checkOnly,
	disk_job_id job)
{
	// to be implemented
	return B_ERROR;
}


// Resize
//! Resizes a partition
status_t
KPartitioningSystem::Resize(KPartition* partition, off_t size, disk_job_id job)
{
	// check parameters
	if (!partition || size < 0 || !fModule)
		return B_BAD_VALUE;
	if (!fModule->resize)
		return B_NOT_SUPPORTED;

	// open partition device
	int fd = -1;
	status_t result = partition->Open(O_RDWR, &fd);
	if (result != B_OK)
		return result;

	// let the module do its job
	result = fModule->resize(fd, partition->ID(), size, job);

	// cleanup and return
	close(fd);
	return result;
}


// ResizeChild
//! Resizes child of a partition
status_t
KPartitioningSystem::ResizeChild(KPartition* child, off_t size, disk_job_id job)
{
	// check parameters
	if (!child || !child->Parent() || size < 0 || !fModule)
		return B_BAD_VALUE;
	if (!fModule->resize_child)
		return B_NOT_SUPPORTED;

	// open partition device
	int fd = -1;
	status_t result = child->Parent()->Open(O_RDWR, &fd);
	if (result != B_OK)
		return result;

	// let the module do its job
	result = fModule->resize_child(fd, child->ID(), size, job);

	// cleanup and return
	close(fd);
	return result;
}


// Move
//! Moves a partition
status_t
KPartitioningSystem::Move(KPartition* partition, off_t offset, disk_job_id job)
{
	// check parameters
	if (!partition)
		return B_BAD_VALUE;
	if (!fModule->move)
		return B_NOT_SUPPORTED;

	// open partition device
	int fd = -1;
	status_t result = partition->Open(O_RDWR, &fd);
	if (result != B_OK)
		return result;

	// let the module do its job
	result = fModule->move(fd, partition->ID(), offset, job);

	// cleanup and return
	close(fd);
	return result;
}


// MoveChild
//! Moves child of a partition
status_t
KPartitioningSystem::MoveChild(KPartition* child, off_t offset, disk_job_id job)
{
	// check parameters
	if (!child || !child->Parent() || !fModule)
		return B_BAD_VALUE;
	if (!fModule->move_child)
		return B_NOT_SUPPORTED;

	// open partition device
	int fd = -1;
	status_t result = child->Parent()->Open(O_RDWR, &fd);
	if (result != B_OK)
		return result;

	// let the module do its job
	result = fModule->move_child(fd, child->Parent()->ID(), child->ID(), offset,
		job);

	// cleanup and return
	close(fd);
	return result;
}


// SetName
//! Sets name of a partition
status_t
KPartitioningSystem::SetName(KPartition* child, const char* name,
	disk_job_id job)
{
	// check parameters
	if (!child || !child->Parent() || !fModule)
		return B_BAD_VALUE;
	if (!fModule->set_name)
		return B_NOT_SUPPORTED;

	// open partition device
	int fd = -1;
	status_t result = child->Parent()->Open(O_RDWR, &fd);
	if (result != B_OK)
		return result;

	// let the module do its job
	result = fModule->set_name(fd, child->ID(), name, job);
// TODO: Change hook interface!

	// cleanup and return
	close(fd);
	return result;
}


// SetContentName
//! Sets name of the content of a partition
status_t
KPartitioningSystem::SetContentName(KPartition* partition, const char* name,
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


// SetType
//! Sets type of a partition
status_t
KPartitioningSystem::SetType(KPartition* child, const char* type,
	disk_job_id job)
{
	// check parameters
	if (!child || !child->Parent() || !type || !fModule)
		return B_BAD_VALUE;
	if (!fModule->set_type)
		return B_NOT_SUPPORTED;

	// open partition device
	int fd = -1;
	status_t result = child->Parent()->Open(O_RDWR, &fd);
	if (result != B_OK)
		return result;

	// let the module do its job
	result = fModule->set_type(fd, child->Parent()->ID(), type, job);
// TODO: Change hook interface!

	// cleanup and return
	close(fd);
	return result;
}


// SetParameters
//! Sets parameters of a partition
status_t
KPartitioningSystem::SetParameters(KPartition* child, const char* parameters,
	disk_job_id job)
{
	// check parameters
	if (!child || !child->Parent() || !fModule)
		return B_BAD_VALUE;
	if (!fModule->set_parameters)
		return B_NOT_SUPPORTED;

	// open partition device
	int fd = -1;
	status_t result = child->Parent()->Open(O_RDWR, &fd);
	if (result != B_OK)
		return result;

	// let the module do its job
	result = fModule->set_parameters(fd, child->ID(), parameters, job);
// TODO: Change hook interface!

	// cleanup and return
	close(fd);
	return result;
}


// SetContentParameters
//! Sets parameters of the content of a partition
status_t
KPartitioningSystem::SetContentParameters(KPartition* partition,
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
//! Initializes a partition with this partitioning system
status_t
KPartitioningSystem::Initialize(KPartition* partition, const char* name,
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
KPartitioningSystem::Uninitialize(KPartition* partition, disk_job_id job)
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
	result = fModule->uninitialize(fd, partition->ID(), partition->Size(), job);

	// cleanup and return
	close(fd);
	return result;
}


// CreateChild
//! Creates a child partition
status_t
KPartitioningSystem::CreateChild(KPartition* partition, off_t offset,
	off_t size, const char* type, const char* name, const char* parameters,
	disk_job_id job, KPartition** child, partition_id childID)
{
	// check parameters
	if (!partition || !type || !parameters || !child || !fModule)
		return B_BAD_VALUE;
	if (!fModule->create_child)
		return B_NOT_SUPPORTED;

	// open partition device
	int fd = -1;
	status_t result = partition->Open(O_RDWR, &fd);
	if (result != B_OK)
		return result;

	// let the module do its job
	result = fModule->create_child(fd, partition->ID(), offset, size,
		type, name, parameters, job, &childID);

	// find and return the child
	*child = KDiskDeviceManager::Default()->FindPartition(childID);

	// cleanup and return
	close(fd);
	return result;
}


// DeleteChild
//! Deletes a child partition
status_t
KPartitioningSystem::DeleteChild(KPartition* child, disk_job_id job)
{
	if (!child || !child->Parent())
		return B_BAD_VALUE;
	if (!fModule->delete_child)
		return B_NOT_SUPPORTED;

	int fd = -1;
	KPartition* parent = child->Parent();
	status_t result = parent->Open(O_RDWR, &fd);
	if (result != B_OK)
		return result;

	result = fModule->delete_child(fd, parent->ID(), child->ID(), job);
	close(fd);
	return result;
}


// LoadModule
status_t
KPartitioningSystem::LoadModule()
{
	if (fModule)		// shouldn't happen
		return B_OK;
	return get_module(Name(), (module_info**)&fModule);
}


// UnloadModule
void
KPartitioningSystem::UnloadModule()
{
	if (fModule) {
		put_module(fModule->module.name);
		fModule = NULL;
	}
}
