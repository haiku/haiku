// KFileSystem.cpp

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

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
	Unload();
	return error;
}

// IsFileSystem
bool
KFileSystem::IsFileSystem() const
{
	return true;
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
	// to be implemented
	if (whileMounted)
		*whileMounted = false;
	return false;
}

// SupportsRepairing
bool
KFileSystem::SupportsRepairing(KPartition *partition, bool checkOnly,
							   bool *whileMounted)
{
	// to be implemented
	if (whileMounted)
		*whileMounted = false;
	return false;
}

// SupportsResizing
bool
KFileSystem::SupportsResizing(KPartition *partition, bool *whileMounted)
{
	// to be implemented
	if (whileMounted)
		*whileMounted = false;
	return false;
}

// SupportsMoving
bool
KFileSystem::SupportsMoving(KPartition *partition, bool *whileMounted)
{
	// to be implemented
	if (whileMounted)
		*whileMounted = false;
	return false;
}

// SupportsSettingContentName
bool
KFileSystem::SupportsSettingContentName(KPartition *partition,
										bool *whileMounted)
{
	// to be implemented
	return false;
}

// SupportsInitializing
bool
KFileSystem::SupportsInitializing(KPartition *partition)
{
	// to be implemented
	return false;
}

// ValidateResize
bool
KFileSystem::ValidateResize(KPartition *partition, off_t *size)
{
	// to be implemented
	return false;
}

// ValidateMove
bool
KFileSystem::ValidateMove(KPartition *partition, off_t *start)
{
	// to be implemented
	return false;
}

// ValidateSetContentName
bool
KFileSystem::ValidateSetContentName(KPartition *partition, char *name)
{
	// to be implemented
	return false;
}

// ValidateInitialize
bool
KFileSystem::ValidateInitialize(KPartition *partition, const char *parameters)
{
	// to be implemented
	return false;
}

// ValidateSetContentParameters
bool
KFileSystem::ValidateSetContentParameters(KPartition *child,
										  const char *parameters)
{
	// to be implemented
	return false;
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

// Initialize
status_t
KFileSystem::Initialize(KPartition *partition, const char *parameters,
						KDiskDeviceJob *job)
{
	// to be implemented
	return B_ERROR;
}

// SetContentParameters
status_t
KFileSystem::SetContentParameters(KPartition *partition,
								  const char *parameters, KDiskDeviceJob *job)
{
	// to be implemented
	return B_ERROR;
}

// LoadModule
status_t
KFileSystem::LoadModule()
{
	if (fModule)		// shouldn't happen
		return B_OK;
	return get_module(Name(), (module_info**)&fModule);
}

// UnloadModule
void
KFileSystem::UnloadModule()
{
	if (fModule) {
		put_module(fModule->module.name);
		fModule = NULL;
	}
}

