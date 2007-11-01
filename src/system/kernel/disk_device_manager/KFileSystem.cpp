// KFileSystem.cpp

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include <fs_interface.h>

#include "ddm_modules.h"
//#include "KDiskDeviceJob.h"
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


#if 0

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
	// check parameters
	if (!partition || !job || !fModule)
		return B_BAD_VALUE;
	if (!fModule->initialize)
		return B_NOT_SUPPORTED;

	// open partition device (we need a temporary read-lock)
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	if (!manager->ReadLockPartition(partition->ID()))
		return B_ERROR;
	DeviceReadLocker locker(partition->Device(), true);

	int fd = -1;
	status_t result = partition->Open(O_RDWR, &fd);
	if (result != B_OK)
		return result;

	off_t partitionSize = partition->Size();

	locker.Unlock();

	// call the module hook
	result = fModule->initialize(fd, partition->ID(), name, parameters,
		partitionSize, job->ID());

	close(fd);
	return result;
}

#endif	// 0


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

