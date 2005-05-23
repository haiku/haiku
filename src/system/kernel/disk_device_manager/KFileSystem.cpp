// KFileSystem.cpp

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include <fs_interface.h>

#include "ddm_modules.h"
#include "KDiskDeviceUtils.h"
#include "KFileSystem.h"
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
	error = SetPrettyName(fModule->pretty_name);
	SetFlags(/*fModule->flags |*/ B_DISK_SYSTEM_IS_FILE_SYSTEM);
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
	fModule->free_identify_partition_cookie(partition->PartitionData(),
											cookie);
}

// FreeContentCookie
void
KFileSystem::FreeContentCookie(KPartition *partition)
{
	if (!partition || !fModule || !fModule->free_partition_content_cookie)
		return;
	fModule->free_partition_content_cookie(partition->PartitionData());
}

// SupportsDefragmenting
bool
KFileSystem::SupportsDefragmenting(KPartition *partition, bool *whileMounted)
{
	bool _whileMounted = false;
	if (!whileMounted)
		whileMounted = &_whileMounted;
	if (!partition || partition->DiskSystem() != this || !fModule
		|| !fModule->supports_defragmenting) {
		return (*whileMounted = false);
	}
	return fModule->supports_defragmenting(partition->PartitionData(),
										   whileMounted);
}

// SupportsRepairing
bool
KFileSystem::SupportsRepairing(KPartition *partition, bool checkOnly,
							   bool *whileMounted)
{
	bool _whileMounted = false;
	if (!whileMounted)
		whileMounted = &_whileMounted;
	if (!partition || partition->DiskSystem() != this || !fModule
		|| !fModule->supports_repairing) {
		return (*whileMounted = false);
	}
	return fModule->supports_repairing(partition->PartitionData(), checkOnly,
									   whileMounted);
}

// SupportsResizing
bool
KFileSystem::SupportsResizing(KPartition *partition, bool *whileMounted)
{
	bool _whileMounted = false;
	if (!whileMounted)
		whileMounted = &_whileMounted;
	if (!partition || partition->DiskSystem() != this || !fModule
		|| !fModule->supports_resizing) {
		return (*whileMounted = false);
	}
	return fModule->supports_resizing(partition->PartitionData(),
									  whileMounted);
}

// SupportsMoving
bool
KFileSystem::SupportsMoving(KPartition *partition, bool *isNoOp)
{
	bool _isNoOp = false;
	if (!isNoOp)
		isNoOp = &_isNoOp;
	if (!partition || partition->DiskSystem() != this || !fModule
		|| !fModule->supports_moving) {
		return (*isNoOp = false);
	}
	return fModule->supports_moving(partition->PartitionData(), isNoOp);
}

// SupportsSettingContentName
bool
KFileSystem::SupportsSettingContentName(KPartition *partition,
										bool *whileMounted)
{
	bool _whileMounted = false;
	if (!whileMounted)
		whileMounted = &_whileMounted;
	if (!partition || partition->DiskSystem() != this || !fModule
		|| !fModule->supports_setting_content_name) {
		return (*whileMounted = false);
	}
	return fModule->supports_setting_content_name(partition->PartitionData(),
												  whileMounted);
}

// SupportsSettingContentParameters
bool
KFileSystem::SupportsSettingContentParameters(KPartition *partition,
											  bool *whileMounted)
{
	bool _whileMounted = false;
	if (!whileMounted)
		whileMounted = &_whileMounted;
	if (!partition || partition->DiskSystem() != this || !fModule
		|| !fModule->supports_setting_content_parameters) {
		return (*whileMounted = false);
	}
	return fModule->supports_setting_content_parameters(
				partition->PartitionData(), whileMounted);
}

// SupportsInitializing
bool
KFileSystem::SupportsInitializing(KPartition *partition)
{
	return (partition && fModule && fModule->supports_initializing
			&& fModule->supports_initializing(partition->PartitionData()));
}

// ValidateResize
bool
KFileSystem::ValidateResize(KPartition *partition, off_t *size)
{
	return (partition && size && partition->DiskSystem() == this && fModule
			&& fModule->validate_resize
			&& fModule->validate_resize(partition->PartitionData(), size));
}

// ValidateMove
bool
KFileSystem::ValidateMove(KPartition *partition, off_t *start)
{
	return (partition && start && partition->DiskSystem() == this && fModule
			&& fModule->validate_move
			&& fModule->validate_move(partition->PartitionData(), start));
}

// ValidateSetContentName
bool
KFileSystem::ValidateSetContentName(KPartition *partition, char *name)
{
	return (partition && name && partition->DiskSystem() == this
			&& fModule && fModule->validate_set_content_name
			&& fModule->validate_set_content_name(partition->PartitionData(),
												  name));
}

// ValidateSetContentParameters
bool
KFileSystem::ValidateSetContentParameters(KPartition *partition,
										  const char *parameters)
{
	return (partition && partition->DiskSystem() == this && fModule
			&& fModule->validate_set_content_parameters
			&& fModule->validate_set_content_parameters(
					partition->PartitionData(), parameters));
}

// ValidateInitialize
bool
KFileSystem::ValidateInitialize(KPartition *partition, char *name,
								const char *parameters)
{
	return (partition && name && fModule && fModule->validate_initialize
			&& fModule->validate_initialize(partition->PartitionData(), name,
											parameters));
}

// ShadowPartitionChanged
status_t
KFileSystem::ShadowPartitionChanged(KPartition *partition, uint32 operation)
{
	if (!partition)
		return B_BAD_VALUE;
	if (!fModule)
		return B_ERROR;
	// If not implemented, we assume, that the file system doesn't have to
	// make any additional changes.
	if (!fModule->shadow_changed)
		return B_OK;
	return fModule->shadow_changed(partition->PartitionData(), operation);
}

// Defragment
status_t
KFileSystem::Defragment(KPartition *partition, KDiskDeviceJob *job)
{
	// to be implemented
	return B_ERROR;
}

// Repair
status_t
KFileSystem::Repair(KPartition *partition, bool checkOnly, KDiskDeviceJob *job)
{
	// to be implemented
	return B_ERROR;
}

// Resize
status_t
KFileSystem::Resize(KPartition *partition, off_t size, KDiskDeviceJob *job)
{
	// to be implemented
	return B_ERROR;
}

// Move
status_t
KFileSystem::Move(KPartition *partition, off_t offset, KDiskDeviceJob *job)
{
	// to be implemented
	return B_ERROR;
}

// SetContentName
status_t
KFileSystem::SetContentName(KPartition *partition, char *name,
							KDiskDeviceJob *job)
{
	// to be implemented
	return B_ERROR;
}


status_t
KFileSystem::SetContentParameters(KPartition *partition,
	const char *parameters, KDiskDeviceJob *job)
{
	// to be implemented
	return B_ERROR;
}


status_t
KFileSystem::Initialize(KPartition *partition, const char *name,
	const char *parameters, KDiskDeviceJob *job)
{
	// to be implemented
	return B_ERROR;
}


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

