// KPartitioningSystem.cpp

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "ddm_modules.h"
#include "KDiskDeviceUtils.h"
#include "KPartition.h"
#include "KPartitioningSystem.h"

// constructor
KPartitioningSystem::KPartitioningSystem(const char *name)
	: KDiskSystem(name)
{
}

// destructor
KPartitioningSystem::~KPartitioningSystem()
{
}

// IsFileSystem
bool
KPartitioningSystem::IsFileSystem() const
{
	return true;
}

// Identify
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
	if (!partition || !fModule || !fModule->free_partition_cookie)
		return;
	fModule->free_partition_cookie(partition->PartitionData());
}

// FreeContentCookie
void
KPartitioningSystem::FreeContentCookie(KPartition *partition)
{
	if (!partition || !fModule || !fModule->free_partition_content_cookie)
		return;
	fModule->free_partition_content_cookie(partition->PartitionData());
}

// SupportsRepairing
bool
KPartitioningSystem::SupportsRepairing(KPartition *partition, bool checkOnly,
									   bool *whileMounted)
{
	// to be implemented by derived classes
	if (whileMounted)
		*whileMounted = false;
	return false;
}

// SupportsResizing
bool
KPartitioningSystem::SupportsResizing(KPartition *partition,
									  bool *whileMounted)
{
	// to be implemented by derived classes
	if (whileMounted)
		*whileMounted = false;
	return false;
}

// SupportsResizingChild
bool
KPartitioningSystem::SupportsResizingChild(KPartition *child)
{
	// to be implemented by derived classes
	return false;
}

// SupportsMoving
bool
KPartitioningSystem::SupportsMoving(KPartition *partition, bool *whileMounted)
{
	// to be implemented by derived classes
	return false;
}

// SupportsMovingChild
bool
KPartitioningSystem::SupportsMovingChild(KPartition *child)
{
	// to be implemented by derived classes
	return false;
}

// SupportsParentSystem
bool
KPartitioningSystem::SupportsParentSystem(KDiskSystem *system)
{
	// to be implemented by derived classes
	return false;
}

// SupportsChildSystem
bool
KPartitioningSystem::SupportsChildSystem(KDiskSystem *system)
{
	// to be implemented by derived classes
	return false;
}

// ValidateResize
bool
KPartitioningSystem::ValidateResize(KPartition *partition, off_t *size)
{
	// to be implemented by derived classes
	return false;
}

// ValidateMove
bool
KPartitioningSystem::ValidateMove(KPartition *partition, off_t *start)
{
	// to be implemented by derived classes
	return false;
}

// ValidateResizeChild
bool
KPartitioningSystem::ValidateResizeChild(KPartition *partition, off_t *size)
{
	// to be implemented by derived classes
	return false;
}

// ValidateMoveChild
bool
KPartitioningSystem::ValidateMoveChild(KPartition *partition, off_t *start)
{
	// to be implemented by derived classes
	return false;
}

// ValidateCreateChild
bool
KPartitioningSystem::ValidateCreateChild(KPartition *partition, off_t *start,
										 off_t *size, const char *parameters)
{
	// to be implemented by derived classes
	return false;
}

// ValidateInitialize
bool
KPartitioningSystem::ValidateInitialize(KPartition *partition,
										const char *parameters)
{
	// to be implemented by derived classes
	return false;
}

// ValidateSetParameters
bool
KPartitioningSystem::ValidateSetParameters(KPartition *partition,
										   const char *parameters)
{
	// to be implemented by derived classes
	return false;
}

// ValidateSetContentParameters
bool
KPartitioningSystem::ValidateSetContentParameters(KPartition *child,
												  const char *parameters)
{
	// to be implemented by derived classes
	return false;
}

// CountPartitionableSpaces
int32
KPartitioningSystem::CountPartitionableSpaces(KPartition *partition)
{
	// to be implemented by derived classes
	return 0;
}

// GetPartitionableSpaces
bool
KPartitioningSystem::GetPartitionableSpaces(KPartition *partition,
											partitionable_space_data *spaces,
											int32 count, int32 *actualCount)
{
	// to be implemented by derived classes
	return false;
}

// Repair
status_t
KPartitioningSystem::Repair(KPartition *partition, bool checkOnly,
							KDiskDeviceJob *job)
{
	// to be implemented by derived classes
	return B_ERROR;
}

// Resize
status_t
KPartitioningSystem::Resize(KPartition *partition, off_t size,
							KDiskDeviceJob *job)
{
	// to be implemented by derived classes
	return B_ERROR;
}

// ResizeChild
status_t
KPartitioningSystem::ResizeChild(KPartition *child, off_t size,
								 KDiskDeviceJob *job)
{
	// to be implemented by derived classes
	return B_ERROR;
}

// Move
status_t
KPartitioningSystem::Move(KPartition *partition, off_t offset,
						  KDiskDeviceJob *job)
{
	// to be implemented by derived classes
	return B_ERROR;
}

// MoveChild
status_t
KPartitioningSystem::MoveChild(KPartition *child, off_t offset,
							   KDiskDeviceJob *job)
{
	// to be implemented by derived classes
	return B_ERROR;
}

// CreateChild
status_t
KPartitioningSystem::CreateChild(KPartition *partition, off_t offset,
								 off_t size, const char *parameters,
								 KDiskDeviceJob *job, KPartition **child,
								 partition_id childID)
{
	// to be implemented by derived classes
	return B_ERROR;
}

// DeleteChild
status_t
KPartitioningSystem::DeleteChild(KPartition *child, KDiskDeviceJob *job)
{
	// to be implemented by derived classes
	return B_ERROR;
}

// Initialize
status_t
KPartitioningSystem::Initialize(KPartition *partition, const char *parameters,
								KDiskDeviceJob *job)
{
	// to be implemented by derived classes
	return B_ERROR;
}

// SetParameters
status_t
KPartitioningSystem::SetParameters(KPartition *partition,
								   const char *parameters, KDiskDeviceJob *job)
{
	// to be implemented by derived classes
	return B_ERROR;
}

// SetContentParameters
status_t
KPartitioningSystem::SetContentParameters(KPartition *partition,
										  const char *parameters,
										  KDiskDeviceJob *job)
{
	// to be implemented by derived classes
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

