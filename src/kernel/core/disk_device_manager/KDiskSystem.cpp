// KDiskSystem.cpp

#include <stdlib.h>

#include "KDiskDeviceManager.h"
#include "KDiskDeviceUtils.h"
#include "KDiskSystem.h"

// constructor
KDiskSystem::KDiskSystem(const char *name)
	: fID(-1),
	  fName(NULL),
	  fPrettyName(NULL),
	  fLoadCounter(0)
{
}

// destructor
KDiskSystem::~KDiskSystem()
{
	free(fName);
}

// Init
status_t
KDiskSystem::Init()
{
	// to be implemented by derived classes
	return B_OK;
}

// SetID
void
KDiskSystem::SetID(disk_system_id id)
{
	fID = id;
}

// ID
disk_system_id
KDiskSystem::ID() const
{
	return fID;
}

// Name
const char *
KDiskSystem::Name() const
{
	return fName;
}

// PrettyName
const char *
KDiskSystem::PrettyName()
{
	return fPrettyName;
}

// IsFileSystem
bool
KDiskSystem::IsFileSystem() const
{
	// to be implemented by derived classes
	return false;
}

// IsPartitioningSystem
bool
KDiskSystem::IsPartitioningSystem() const
{
	return !IsFileSystem();
}

// Load
status_t
KDiskSystem::Load()
{
	ManagerLocker locker(KDiskDeviceManager::Default());
	status_t error = B_OK;
	if (fLoadCounter == 0)
		error = LoadModule();
	if (error == B_OK)
		fLoadCounter++;
	return error;
}

// Unload
void
KDiskSystem::Unload()
{
	ManagerLocker locker(KDiskDeviceManager::Default());
	if (fLoadCounter > 0 && --fLoadCounter == 0)
		UnloadModule();
}

// IsLoaded
bool
KDiskSystem::IsLoaded() const
{
	ManagerLocker locker(KDiskDeviceManager::Default());
	return (fLoadCounter > 0);
}

// Identify
float
KDiskSystem::Identify(KPartition *partition, void **cookie)
{
	// to be implemented by derived classes
	return -1;
}

// Scan
status_t
KDiskSystem::Scan(KPartition *partition, void *cookie)
{
	// to be implemented by derived classes
	return B_ERROR;
}

// FreeIdentifyCookie
void
KDiskSystem::FreeIdentifyCookie(KPartition *partition, void *cookie)
{
	// to be implemented by derived classes
}

// FreeCookie
void
KDiskSystem::FreeCookie(KPartition *partition)
{
	// to be implemented by derived classes
}

// FreeContentCookie
void
KDiskSystem::FreeContentCookie(KPartition *partition)
{
	// to be implemented by derived classes
}

// SupportsDefragmenting
bool
KDiskSystem::SupportsDefragmenting(KPartition *partition, bool *whileMounted)
{
	// to be implemented by derived classes
	if (whileMounted)
		*whileMounted = false;
	return false;
}

// SupportsRepairing
bool
KDiskSystem::SupportsRepairing(KPartition *partition, bool checkOnly,
							   bool *whileMounted)
{
	// to be implemented by derived classes
	if (whileMounted)
		*whileMounted = false;
	return false;
}

// SupportsResizing
bool
KDiskSystem::SupportsResizing(KPartition *partition, bool *whileMounted)
{
	// to be implemented by derived classes
	if (whileMounted)
		*whileMounted = false;
	return false;
}

// SupportsResizingChild
bool
KDiskSystem::SupportsResizingChild(KPartition *child)
{
	// to be implemented by derived classes
	return false;
}

// SupportsMoving
bool
KDiskSystem::SupportsMoving(KPartition *partition, bool *whileMounted)
{
	// to be implemented by derived classes
	return false;
}

// SupportsMovingChild
bool
KDiskSystem::SupportsMovingChild(KPartition *child)
{
	// to be implemented by derived classes
	return false;
}

// SupportsParentSystem
bool
KDiskSystem::SupportsParentSystem(KDiskSystem *system)
{
	// to be implemented by derived classes
	return false;
}

// SupportsChildSystem
bool
KDiskSystem::SupportsChildSystem(KDiskSystem *system)
{
	// to be implemented by derived classes
	return false;
}

// ValidateResize
bool
KDiskSystem::ValidateResize(KPartition *partition, off_t *size)
{
	// to be implemented by derived classes
	return false;
}

// ValidateMove
bool
KDiskSystem::ValidateMove(KPartition *partition, off_t *start)
{
	// to be implemented by derived classes
	return false;
}

// ValidateResizeChild
bool
KDiskSystem::ValidateResizeChild(KPartition *partition, off_t *size)
{
	// to be implemented by derived classes
	return false;
}

// ValidateMoveChild
bool
KDiskSystem::ValidateMoveChild(KPartition *partition, off_t *start)
{
	// to be implemented by derived classes
	return false;
}

// ValidateCreateChild
bool
KDiskSystem::ValidateCreateChild(KPartition *partition, off_t *start,
								 off_t *size, const char *parameters)
{
	// to be implemented by derived classes
	return false;
}

// ValidateInitialize
bool
KDiskSystem::ValidateInitialize(KPartition *partition, const char *parameters)
{
	// to be implemented by derived classes
	return false;
}

// ValidateSetParameters
bool
KDiskSystem::ValidateSetParameters(KPartition *partition,
								   const char *parameters)
{
	// to be implemented by derived classes
	return false;
}

// ValidateSetContentParameters
bool
KDiskSystem::ValidateSetContentParameters(KPartition *child,
										  const char *parameters)
{
	// to be implemented by derived classes
	return false;
}

// CountPartitionableSpaces
int32
KDiskSystem::CountPartitionableSpaces(KPartition *partition)
{
	// to be implemented by derived classes
	return 0;
}

// GetPartitionableSpaces
bool
KDiskSystem::GetPartitionableSpaces(KPartition *partition,
									partitionable_space_data *spaces,
									int32 count, int32 *actualCount)
{
	// to be implemented by derived classes
	return false;
}

// Defragment
status_t
KDiskSystem::Defragment(KPartition *partition, KDiskDeviceJob *job)
{
	// to be implemented by derived classes
	return B_ERROR;
}

// Repair
status_t
KDiskSystem::Repair(KPartition *partition, bool checkOnly,
					KDiskDeviceJob *job)
{
	// to be implemented by derived classes
	return B_ERROR;
}

// Resize
status_t
KDiskSystem::Resize(KPartition *partition, off_t size, KDiskDeviceJob *job)
{
	// to be implemented by derived classes
	return B_ERROR;
}

// ResizeChild
status_t
KDiskSystem::ResizeChild(KPartition *child, off_t size, KDiskDeviceJob *job)
{
	// to be implemented by derived classes
	return B_ERROR;
}

// Move
status_t
KDiskSystem::Move(KPartition *partition, off_t offset, KDiskDeviceJob *job)
{
	// to be implemented by derived classes
	return B_ERROR;
}

// MoveChild
status_t
KDiskSystem::MoveChild(KPartition *child, off_t offset, KDiskDeviceJob *job)
{
	// to be implemented by derived classes
	return B_ERROR;
}

// CreateChild
status_t
KDiskSystem::CreateChild(KPartition *partition, off_t offset, off_t size,
						 const char *parameters, KDiskDeviceJob *job,
						 KPartition **child, partition_id childID)
{
	// to be implemented by derived classes
	return B_ERROR;
}

// DeleteChild
status_t
KDiskSystem::DeleteChild(KPartition *child, KDiskDeviceJob *job)
{
	// to be implemented by derived classes
	return B_ERROR;
}

// Initialize
status_t
KDiskSystem::Initialize(KPartition *partition, const char *parameters,
						KDiskDeviceJob *job)
{
	// to be implemented by derived classes
	return B_ERROR;
}

// SetParameters
status_t
KDiskSystem::SetParameters(KPartition *partition, const char *parameters,
						   KDiskDeviceJob *job)
{
	// to be implemented by derived classes
	return B_ERROR;
}

// SetContentParameters
status_t
KDiskSystem::SetContentParameters(KPartition *partition,
								  const char *parameters, KDiskDeviceJob *job)
{
	// to be implemented by derived classes
	return B_ERROR;
}

// LoadModule
status_t
KDiskSystem::LoadModule()
{
	// to be implemented by derived classes
	return B_ERROR;
}

// UnloadModule
void
KDiskSystem::UnloadModule()
{
	// to be implemented by derived classes
}

// SetPrettyName
status_t
KDiskSystem::SetPrettyName(const char *name)
{
	return set_string(fPrettyName, name);
}

