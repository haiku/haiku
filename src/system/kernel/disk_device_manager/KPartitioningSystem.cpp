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
#include <KDiskDeviceJob.h>
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


// GetSupportedOperations
uint32
KPartitioningSystem::GetSupportedOperations(KPartition* partition, uint32 mask)
{
	ASSERT(partition != NULL);

	// Note, that for initialization, the partition's disk system does not
	// need to be this disk system.

	if (!fModule)
		return 0;

	if (!fModule->get_supported_operations)
		return (Flags() & mask);

	return fModule->get_supported_operations(partition->PartitionData(), mask)
		& mask;
}


// GetSupportedChildOperations
uint32
KPartitioningSystem::GetSupportedChildOperations(KPartition* child, uint32 mask)
{
	ASSERT(child != NULL);
	ASSERT(child->Parent() != NULL);
	ASSERT(child->Parent()->DiskSystem() == this);

	if (!fModule)
		return 0;

	if (!fModule->get_supported_child_operations)
		return (Flags() & mask);

	return fModule->get_supported_child_operations(
			child->Parent()->PartitionData(), child->PartitionData(), mask)
		& mask;
}


// SupportsInitializingChild
/*! Check whether the child partition managed by this partitioning system can
	be initialized with the given disk system.
*/
bool
KPartitioningSystem::SupportsInitializingChild(KPartition *child,
	const char *diskSystem)
{
	if (!child || child->ParentDiskSystem() != this || !diskSystem
		|| !fModule)
		return false;

	// If the hook is not implemented, the parent disk system doesn't want a
	// veto.
	if (!fModule->supports_initializing_child)
		return true;

	return fModule->supports_initializing_child(child->PartitionData(),
		diskSystem);
}


// IsSubSystemFor
//! Check whether the add-on is a subsystem for a given partition.
bool
KPartitioningSystem::IsSubSystemFor(KPartition *partition)
{
	return (partition && fModule && fModule->is_sub_system_for
		&& fModule->is_sub_system_for(partition->PartitionData()));
}


// ValidateResize
//! Validates parameters for resizing a partition
bool
KPartitioningSystem::ValidateResize(KPartition *partition, off_t *size)
{
	return (partition && size && partition->DiskSystem() == this && fModule
		&& fModule->validate_resize
		&& fModule->validate_resize(partition->PartitionData(), size));
}


// ValidateResizeChild
//! Validates parameters for resizing a child partition
bool
KPartitioningSystem::ValidateResizeChild(KPartition *child, off_t *size)
{
	return (child && size && child->Parent()
		&& child->ParentDiskSystem() == this && fModule
		&& fModule->validate_resize_child
		&& fModule->validate_resize_child(child->Parent()->PartitionData(),
				child->PartitionData(), size));
}


// ValidateMove
//! Validates parameters for moving a partition
bool
KPartitioningSystem::ValidateMove(KPartition *partition, off_t *start)
{
	return (partition && start && partition->DiskSystem() == this && fModule
		&& fModule->validate_move
		&& fModule->validate_move(partition->PartitionData(), start));
}


// ValidateMoveChild
//! Validates parameters for moving a child partition
bool
KPartitioningSystem::ValidateMoveChild(KPartition *child, off_t *start)
{
	return (child && start && child->Parent()
		&& child->ParentDiskSystem() == this && fModule
		&& fModule->validate_move_child
		&& fModule->validate_move_child(child->Parent()->PartitionData(),
				child->PartitionData(), start));
}


// ValidateSetName
//! Validates parameters for setting name of a partition
bool
KPartitioningSystem::ValidateSetName(KPartition *partition, char *name)
{
	return (partition && name && partition->Parent()
		&& partition->ParentDiskSystem() == this && fModule
		&& fModule->validate_set_name
		&& fModule->validate_set_name(partition->PartitionData(), name));
}


// ValidateSetContentName
//! Validates parameters for setting name of the content of a partition
bool
KPartitioningSystem::ValidateSetContentName(KPartition *partition, char *name)
{
	return (partition && name && partition->DiskSystem() == this
		&& fModule && fModule->validate_set_content_name
		&& fModule->validate_set_content_name(partition->PartitionData(),
				name));
}


// ValidateSetType
//! Validates parameters for setting type of a partition
bool
KPartitioningSystem::ValidateSetType(KPartition *partition, const char *type)
{
	return (partition && type && partition->ParentDiskSystem() == this
		&& fModule && fModule->validate_set_type
		&& fModule->validate_set_type(partition->PartitionData(), type));
}


// ValidateSetParameters
//! Validates parameters for setting parameters of a partition
bool
KPartitioningSystem::ValidateSetParameters(KPartition *partition,
	const char *parameters)
{
	return (partition && partition->ParentDiskSystem() == this 
		&& fModule && fModule->validate_set_parameters
		&& fModule->validate_set_parameters(partition->PartitionData(),
				parameters));
}


// ValidateSetContentParameters
//! Validates parameters for setting parameters of the content of a partition
bool
KPartitioningSystem::ValidateSetContentParameters(KPartition *partition,
	const char *parameters)
{
	return (partition && partition->DiskSystem() == this && fModule
			&& fModule->validate_set_content_parameters
			&& fModule->validate_set_content_parameters(
					partition->PartitionData(), parameters));
}


// ValidateInitialize
//! Validates parameters for initializing a partition
bool
KPartitioningSystem::ValidateInitialize(KPartition *partition, char *name,
	const char *parameters)
{
	return (partition && name && fModule && fModule->validate_initialize
		&& fModule->validate_initialize(partition->PartitionData(), name,
				parameters));
}


// ValidateCreateChild
//! Validates parameters for creating child of a partition
bool
KPartitioningSystem::ValidateCreateChild(KPartition *partition, off_t *start,
	off_t *size, const char *type, const char *parameters, int32 *index)
{
	int32 _index = 0;
	if (!index)
		index = &_index;
	return (partition && start && size && type
		&& partition->DiskSystem() == this && fModule
		&& fModule->validate_create_child
		&& fModule->validate_create_child(partition->PartitionData(), start,
				size, type, parameters, index));
}


// CountPartitionableSpaces
//! Counts partitionable spaces on a partition
int32
KPartitioningSystem::CountPartitionableSpaces(KPartition *partition)
{
	if (!partition || partition->DiskSystem() != this || !fModule)
		return 0;
	if (!fModule->get_partitionable_spaces) {
		// TODO: Fallback algorithm.
		return 0;
	}
	int32 count = 0;
	status_t error = fModule->get_partitionable_spaces(
		partition->PartitionData(), NULL, 0, &count);
	return (error == B_OK || error == B_BUFFER_OVERFLOW ? count : 0);
}


// GetPartitionableSpaces
//! Retrieves a list of partitionable spaces on a partition
status_t
KPartitioningSystem::GetPartitionableSpaces(KPartition *partition,
	partitionable_space_data *buffer, int32 count, int32 *actualCount)
{
	if (!partition || partition->DiskSystem() != this || count > 0 && !buffer
		|| !actualCount || !fModule) {
		return B_BAD_VALUE;
	}
	if (!fModule->get_partitionable_spaces) {
		// TODO: Fallback algorithm.
		return B_ENTRY_NOT_FOUND;
	}
	return fModule->get_partitionable_spaces(partition->PartitionData(),
		buffer, count, actualCount);
}


// GetNextSupportedType
//! Iterates through supported partition types
status_t
KPartitioningSystem::GetNextSupportedType(KPartition *partition, int32 *cookie,
	char *type)
{
	if (!partition || partition->DiskSystem() != this || !cookie || !type
		|| !fModule) {
		return B_BAD_VALUE;
	}
	if (!fModule->get_next_supported_type)
		return B_ENTRY_NOT_FOUND;
	return fModule->get_next_supported_type(partition->PartitionData(), cookie,
											type);
}


// GetTypeForContentType
//! Translates the "pretty" content type to an internal type
status_t
KPartitioningSystem::GetTypeForContentType(const char *contentType, char *type)
{
	if (!contentType || !type || !fModule)
		return B_BAD_VALUE;
	if (!fModule->get_type_for_content_type)
		return B_ENTRY_NOT_FOUND;
	return fModule->get_type_for_content_type(contentType, type);
}


// ShadowPartitionChanged
//! Calls for additional modifications when shadow partition is changed
status_t
KPartitioningSystem::ShadowPartitionChanged(KPartition *partition,
	uint32 operation)
{
	if (!partition)
		return B_BAD_VALUE;
	if (!fModule)
		return B_ERROR;
	// If not implemented, we assume, that the partitioning system doesn't
	// have to make any additional changes.
	if (!fModule->shadow_changed)
		return B_OK;
	return fModule->shadow_changed(partition->PartitionData(), operation);
}


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
	if (!partition || !job || !name /*|| !parameters*/)
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
