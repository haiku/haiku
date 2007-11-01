/*
 * Copyright 2003-2007, Haiku, Inc. All Rights Reserved.
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


#if 0

// Repair
//! Repairs a partition
status_t
KPartitioningSystem::Repair(KPartition *partition, bool checkOnly,
							KDiskDeviceJob *job)
{
	// to be implemented
	return B_ERROR;
}


// Resize
//! Resizes a partition
status_t
KPartitioningSystem::Resize(KPartition *partition, off_t size,
	KDiskDeviceJob *job)
{
	// check parameters
	if (!partition || !job || size < 0)
		return B_BAD_VALUE;
	if (!fModule->resize)
		return B_ENTRY_NOT_FOUND;

	// lock partition and open partition device
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	KPartition *_partition = manager->WriteLockPartition(partition->ID());
	if (!_partition)
		return B_ERROR;
	int fd = -1;
	{
		PartitionRegistrar registrar(_partition, true);
		PartitionRegistrar deviceRegistrar(_partition->Device(), true);
		DeviceWriteLocker locker(_partition->Device(), true);
		if (partition != _partition)
			return B_ERROR;
		status_t result = partition->Open(O_RDWR, &fd);
		if (result != B_OK)
			return result;
	}

	// let the module do its job
	status_t result = fModule->resize(fd, partition->ID(), size, job->ID());

	// cleanup and return
	close(fd);
	return result;
}


// ResizeChild
//! Resizes child of a partition
status_t
KPartitioningSystem::ResizeChild(KPartition *child, off_t size,
	KDiskDeviceJob *job)
{
	// check parameters
	if (!child || !job || !child->Parent() || size < 0)
		return B_BAD_VALUE;
	if (!fModule->resize_child)
		return B_ENTRY_NOT_FOUND;

	// lock partition and open (parent) partition device
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	KPartition *_partition = manager->WriteLockPartition(child->ID());
	KPartition *_parent = manager->WriteLockPartition(child->Parent()->ID());
	if (!_partition)
		return B_ERROR;
	int fd = -1;
	{
		PartitionRegistrar registrar(_partition, true);
		PartitionRegistrar deviceRegistrar(_partition->Device(), true);
		DeviceWriteLocker locker(_partition->Device(), true);
		if (child != _partition)
			return B_ERROR;
		status_t result = child->Parent()->Open(O_RDWR, &fd);
		if (result != B_OK)
			return result;
	}

	// let the module do its job
	status_t result = fModule->resize_child(fd, child->ID(), size, job->ID());

	// cleanup and return
	close(fd);
	return result;
}


// Move
//! Moves a partition
status_t
KPartitioningSystem::Move(KPartition *partition, off_t offset,
	KDiskDeviceJob *job)
{
	// check parameters
	if (!partition || !job)
		return B_BAD_VALUE;
	if (!fModule->move)
		return B_ENTRY_NOT_FOUND;

	// lock partition and open partition device
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	KPartition *_partition = manager->WriteLockPartition(partition->ID());
	if (!_partition)
		return B_ERROR;
	int fd = -1;
	{
		PartitionRegistrar registrar(_partition, true);
		PartitionRegistrar deviceRegistrar(_partition->Device(), true);
		DeviceWriteLocker locker(_partition->Device(), true);
		if (partition != _partition)
			return B_ERROR;
		status_t result = partition->Open(O_RDWR, &fd);
		if (result != B_OK)
			return result;
	}

	// let the module do its job
	status_t result = fModule->move(fd, partition->ID(), offset, job->ID());

	// cleanup and return
	close(fd);
	return result;
}


// MoveChild
//! Moves child of a partition
status_t
KPartitioningSystem::MoveChild(KPartition *child, off_t offset,
	KDiskDeviceJob *job)
{
	// check parameters
	if (!child || !job || !child->Parent())
		return B_BAD_VALUE;
	if (!fModule->move_child)
		return B_ENTRY_NOT_FOUND;

	// lock partition and open (parent) partition device
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	KPartition *_partition = manager->WriteLockPartition(child->ID());
	KPartition *_parent = manager->WriteLockPartition(child->Parent()->ID());
	if (!_partition)
		return B_ERROR;
	int fd = -1;
	{
		PartitionRegistrar registrar(_partition, true);
		PartitionRegistrar deviceRegistrar(_partition->Device(), true);
		DeviceWriteLocker locker(_partition->Device(), true);
		if (child != _partition)
			return B_ERROR;
		status_t result = child->Parent()->Open(O_RDWR, &fd);
		if (result != B_OK)
			return result;
	}

	// let the module do its job
	status_t result = fModule->move_child(fd, child->Parent()->ID(),
		child->ID(), offset, job->ID());

	// cleanup and return
	close(fd);
	return result;
}


// SetName
//! Sets name of a partition
status_t
KPartitioningSystem::SetName(KPartition *partition, char *name,
	KDiskDeviceJob *job)
{
	// check parameters
	if (!partition || !job || !name)
		return B_BAD_VALUE;
	if (!fModule->set_name)
		return B_ENTRY_NOT_FOUND;

	// lock partition and open partition device
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	KPartition *_partition = manager->WriteLockPartition(partition->ID());
	if (!_partition)
		return B_ERROR;
	int fd = -1;
	{
		PartitionRegistrar registrar(_partition, true);
		PartitionRegistrar deviceRegistrar(_partition->Device(), true);
		DeviceWriteLocker locker(_partition->Device(), true);
		if (partition != _partition)
			return B_ERROR;
		status_t result = partition->Open(O_RDWR, &fd);
		if (result != B_OK)
			return result;
	}

	// let the module do its job
	status_t result = fModule->set_name(fd, partition->ID(), name, job->ID());

	// cleanup and return
	close(fd);
	return result;
}


// SetContentName
//! Sets name of the content of a partition
status_t
KPartitioningSystem::SetContentName(KPartition *partition, char *name,
	KDiskDeviceJob *job)
{
	// check parameters
	if (!partition || !job || !name)
		return B_BAD_VALUE;
	if (!fModule->set_content_name)
		return B_ENTRY_NOT_FOUND;

	// lock partition and open partition device
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	KPartition *_partition = manager->WriteLockPartition(partition->ID());
	if (!_partition)
		return B_ERROR;
	int fd = -1;
	{
		PartitionRegistrar registrar(_partition, true);
		PartitionRegistrar deviceRegistrar(_partition->Device(), true);
		DeviceWriteLocker locker(_partition->Device(), true);
		if (partition != _partition)
			return B_ERROR;
		status_t result = partition->Open(O_RDWR, &fd);
		if (result != B_OK)
			return result;
	}

	// let the module do its job
	status_t result = fModule->set_content_name(fd, partition->ID(), name,
		job->ID());

	// cleanup and return
	close(fd);
	return result;
}


// SetType
//! Sets type of a partition
status_t
KPartitioningSystem::SetType(KPartition *partition, char *type,
	KDiskDeviceJob *job)
{
	// check parameters
	if (!partition || !job || !type)
		return B_BAD_VALUE;
	if (!fModule->set_type)
		return B_ENTRY_NOT_FOUND;

	// lock partition and open partition device
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	KPartition *_partition = manager->WriteLockPartition(partition->ID());
	if (!_partition)
		return B_ERROR;
	int fd = -1;
	{
		PartitionRegistrar registrar(_partition, true);
		PartitionRegistrar deviceRegistrar(_partition->Device(), true);
		DeviceWriteLocker locker(_partition->Device(), true);
		if (partition != _partition)
			return B_ERROR;
		status_t result = partition->Open(O_RDWR, &fd);
		if (result != B_OK)
			return result;
	}

	// let the module do its job
	status_t result = fModule->set_type(fd, partition->ID(), type, job->ID());

	// cleanup and return
	close(fd);
	return result;
}


// SetParameters
//! Sets parameters of a partition
status_t
KPartitioningSystem::SetParameters(KPartition *partition,
	const char *parameters, KDiskDeviceJob *job)
{
	// check parameters
	if (!partition || !job || !parameters)
		return B_BAD_VALUE;
	if (!fModule->set_parameters)
		return B_ENTRY_NOT_FOUND;

	// lock partition and open partition device
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	KPartition *_partition = manager->WriteLockPartition(partition->ID());
	if (!_partition)
		return B_ERROR;
	int fd = -1;
	{
		PartitionRegistrar registrar(_partition, true);
		PartitionRegistrar deviceRegistrar(_partition->Device(), true);
		DeviceWriteLocker locker(_partition->Device(), true);
		if (partition != _partition)
			return B_ERROR;
		status_t result = partition->Open(O_RDWR, &fd);
		if (result != B_OK)
			return result;
	}

	// let the module do its job
	status_t result = fModule->set_parameters(fd, partition->ID(), parameters,
		job->ID());

	// cleanup and return
	close(fd);
	return result;
}


// SetContentParameters
//! Sets parameters of the content of a partition
status_t
KPartitioningSystem::SetContentParameters(KPartition *partition,
	const char *parameters, KDiskDeviceJob *job)
{
	// check parameters
	if (!partition || !job || !parameters)
		return B_BAD_VALUE;
	if (!fModule->set_content_parameters)
		return B_ENTRY_NOT_FOUND;

	// lock partition and open partition device
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	KPartition *_partition = manager->WriteLockPartition(partition->ID());
	if (!_partition)
		return B_ERROR;
	int fd = -1;
	{
		PartitionRegistrar registrar(_partition, true);
		PartitionRegistrar deviceRegistrar(_partition->Device(), true);
		DeviceWriteLocker locker(_partition->Device(), true);
		if (partition != _partition)
			return B_ERROR;
		status_t result = partition->Open(O_RDWR, &fd);
		if (result != B_OK)
			return result;
	}

	// let the module do its job
	status_t result = fModule->set_content_parameters(fd, partition->ID(),
		parameters, job->ID());

	// cleanup and return
	close(fd);
	return result;
}


// Initialize
//! Initializes a partition with this partitioning system
status_t
KPartitioningSystem::Initialize(KPartition *partition, const char *name,
	const char *parameters, KDiskDeviceJob *job)
{
	// check parameters
	if (!partition || !job /*|| !parameters*/)
		return B_BAD_VALUE;
	if (!fModule->initialize)
		return B_ENTRY_NOT_FOUND;

	// lock partition and open partition device
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
// TODO: This looks overly complicated.
	KPartition *_partition = manager->WriteLockPartition(partition->ID());
	if (!_partition)
		return B_ERROR;
	int fd = -1;
	off_t partitionSize;
	{
		PartitionRegistrar registrar(_partition, true);
		PartitionRegistrar deviceRegistrar(_partition->Device(), true);
		DeviceWriteLocker locker(_partition->Device(), true);
		if (partition != _partition)
			return B_ERROR;
		status_t result = partition->Open(O_RDWR, &fd);
		if (result != B_OK)
			return result;

		partitionSize = partition->Size();
	}

	// let the module do its job
	status_t result = fModule->initialize(fd, partition->ID(), name, parameters,
		partitionSize, job->ID());

	// cleanup and return
	close(fd);
	return result;
}


// CreateChild
//! Creates a child partition
status_t
KPartitioningSystem::CreateChild(KPartition *partition, off_t offset,
	off_t size, const char *type, const char *parameters, KDiskDeviceJob *job,
	KPartition **child, partition_id childID)
{
	// check parameters
	if (!partition || !job || !type /*|| !parameters*/ || !child)
		return B_BAD_VALUE;
	if (!fModule->create_child)
		return B_ENTRY_NOT_FOUND;

	// lock partition and open partition device
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	KPartition *_partition = manager->WriteLockPartition(partition->ID());
	if (!_partition)
		return B_ERROR;
	int fd = -1;
	{
		PartitionRegistrar registrar(_partition, true);
		PartitionRegistrar deviceRegistrar(_partition->Device(), true);
		DeviceWriteLocker locker(_partition->Device(), true);
		if (partition != _partition)
			return B_ERROR;
		status_t result = partition->Open(O_RDWR, &fd);
		if (result != B_OK)
			return result;
	}

	// let the module do its job
	status_t result = fModule->create_child(fd, partition->ID(), offset, size,
		type, parameters, job->ID(), &childID);

	// find and return the child
	*child = manager->FindPartition(childID, false);

	// cleanup and return
	close(fd);
	return result;
}


// DeleteChild
//! Deletes a child partition
status_t
KPartitioningSystem::DeleteChild(KPartition *child, KDiskDeviceJob *job)
{
	// to be implemented
	return B_ERROR;
}

#endif	// 0


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
