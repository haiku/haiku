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
	Unload();
	return error;
}

// IsFileSystem
bool
KPartitioningSystem::IsFileSystem() const
{
	return false;
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
	// to be implemented
	if (whileMounted)
		*whileMounted = false;
	return false;
}

// SupportsResizing
bool
KPartitioningSystem::SupportsResizing(KPartition *partition,
									  bool *whileMounted)
{
	// to be implemented
	if (whileMounted)
		*whileMounted = false;
	return false;
}

// SupportsResizingChild
bool
KPartitioningSystem::SupportsResizingChild(KPartition *child)
{
	// to be implemented
	return false;
}

// SupportsMoving
bool
KPartitioningSystem::SupportsMoving(KPartition *partition, bool *whileMounted)
{
	// to be implemented
	return false;
}

// SupportsMovingChild
bool
KPartitioningSystem::SupportsMovingChild(KPartition *child)
{
	// to be implemented
	return false;
}

// SupportsSettingName
bool
KPartitioningSystem::SupportsSettingName(KPartition *partition)
{
	// to be implemented
	return false;
}

// SupportsSettingContentName
bool
KPartitioningSystem::SupportsSettingContentName(KPartition *partition,
												bool *whileMounted)
{
	// to be implemented
	return false;
}

// SupportsSettingType
bool
KPartitioningSystem::SupportsSettingType(KPartition *partition)
{
	// to be implemented
	return false;
}

// SupportsCreatingChild
bool
KPartitioningSystem::SupportsCreatingChild(KPartition *partition)
{
	// to be implemented
	return false;
}

// SupportsDeletingChild
bool
KPartitioningSystem::SupportsDeletingChild(KPartition *child)
{
	// to be implemented
	return false;
}

// SupportsInitializing
bool
KPartitioningSystem::SupportsInitializing(KPartition *partition)
{
	// to be implemented
	return false;
}

// SupportsInitializingChild
bool
KPartitioningSystem::SupportsInitializingChild(KPartition *child,
											   const char *diskSystem)
{
	// to be implemented
	return false;
}

// IsSubSystemFor
bool
KPartitioningSystem::IsSubSystemFor(KPartition *partition)
{
	// to be implemented
	return false;
}

// ValidateResize
bool
KPartitioningSystem::ValidateResize(KPartition *partition, off_t *size)
{
	// to be implemented
	return false;
}

// ValidateResizeChild
bool
KPartitioningSystem::ValidateResizeChild(KPartition *partition, off_t *size)
{
	// to be implemented
	return false;
}

// ValidateMove
bool
KPartitioningSystem::ValidateMove(KPartition *partition, off_t *start)
{
	// to be implemented
	return false;
}

// ValidateMoveChild
bool
KPartitioningSystem::ValidateMoveChild(KPartition *partition, off_t *start)
{
	// to be implemented
	return false;
}

// ValidateSetName
bool
KPartitioningSystem::ValidateSetName(KPartition *partition, char *name)
{
	// to be implemented
	return false;
}

// ValidateSetContentName
bool
KPartitioningSystem::ValidateSetContentName(KPartition *partition, char *name)
{
	// to be implemented
	return false;
}

// ValidateSetType
bool
KPartitioningSystem::ValidateSetType(KPartition *partition, const char *type)
{
	// to be implemented
	return false;
}

// ValidateCreateChild
bool
KPartitioningSystem::ValidateCreateChild(KPartition *partition, off_t *start,
										 off_t *size, const char *type,
										 const char *parameters)
{
	// to be implemented
	return false;
}

// ValidateInitialize
bool
KPartitioningSystem::ValidateInitialize(KPartition *partition,
										const char *parameters)
{
	// to be implemented
	return false;
}

// ValidateSetParameters
bool
KPartitioningSystem::ValidateSetParameters(KPartition *partition,
										   const char *parameters)
{
	// to be implemented
	return false;
}

// ValidateSetContentParameters
bool
KPartitioningSystem::ValidateSetContentParameters(KPartition *child,
												  const char *parameters)
{
	// to be implemented
	return false;
}

// CountPartitionableSpaces
int32
KPartitioningSystem::CountPartitionableSpaces(KPartition *partition)
{
	// to be implemented
	return 0;
}

// GetPartitionableSpaces
bool
KPartitioningSystem::GetPartitionableSpaces(KPartition *partition,
											partitionable_space_data *spaces,
											int32 count, int32 *actualCount)
{
	// to be implemented
	return false;
}

// GetNextSupportedType
status_t
KPartitioningSystem::GetNextSupportedType(KPartition *partition, int32 *cookie,
										  char *type)
{
	// to be implemented
	return B_ERROR;
}

// GetTypeForContentType
status_t
KPartitioningSystem::GetTypeForContentType(const char *contentType, char *type)
{
	// to be implemented
	return B_ERROR;
}

// Repair
status_t
KPartitioningSystem::Repair(KPartition *partition, bool checkOnly,
							KDiskDeviceJob *job)
{
	// to be implemented
	return B_ERROR;
}

// Resize
status_t
KPartitioningSystem::Resize(KPartition *partition, off_t size,
							KDiskDeviceJob *job)
{
	// to be implemented
	return B_ERROR;
}

// ResizeChild
status_t
KPartitioningSystem::ResizeChild(KPartition *child, off_t size,
								 KDiskDeviceJob *job)
{
	// to be implemented
	return B_ERROR;
}

// Move
status_t
KPartitioningSystem::Move(KPartition *partition, off_t offset,
						  KDiskDeviceJob *job)
{
	// to be implemented
	return B_ERROR;
}

// MoveChild
status_t
KPartitioningSystem::MoveChild(KPartition *child, off_t offset,
							   KDiskDeviceJob *job)
{
	// to be implemented
	return B_ERROR;
}

// SetName
status_t
KPartitioningSystem::SetName(KPartition *partition, char *name,
							 KDiskDeviceJob *job)
{
	// to be implemented
	return B_ERROR;
}

// SetContentName
status_t
KPartitioningSystem::SetContentName(KPartition *partition, char *name,
									KDiskDeviceJob *job)
{
	// to be implemented
	return B_ERROR;
}

// SetType
status_t
KPartitioningSystem::SetType(KPartition *partition, char *type,
							 KDiskDeviceJob *job)
{
	// to be implemented
	return B_ERROR;
}

// CreateChild
status_t
KPartitioningSystem::CreateChild(KPartition *partition, off_t offset,
								 off_t size, const char *type,
								 const char *parameters, KDiskDeviceJob *job,
								 KPartition **child, partition_id childID)
{
	// to be implemented
	return B_ERROR;
}

// DeleteChild
status_t
KPartitioningSystem::DeleteChild(KPartition *child, KDiskDeviceJob *job)
{
	// to be implemented
	return B_ERROR;
}

// Initialize
status_t
KPartitioningSystem::Initialize(KPartition *partition, const char *parameters,
								KDiskDeviceJob *job)
{
	// to be implemented
	return B_ERROR;
}

// SetParameters
status_t
KPartitioningSystem::SetParameters(KPartition *partition,
								   const char *parameters, KDiskDeviceJob *job)
{
	// to be implemented
	return B_ERROR;
}

// SetContentParameters
status_t
KPartitioningSystem::SetContentParameters(KPartition *partition,
										  const char *parameters,
										  KDiskDeviceJob *job)
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

