// KDiskSystem.cpp

#include <stdio.h>
#include <stdlib.h>

#include <KernelExport.h>
#include <util/kernel_cpp.h>

#include "ddm_userland_interface.h"
#include "KDiskDeviceManager.h"
#include "KDiskDeviceUtils.h"
#include "KDiskSystem.h"

// debugging
//#define DBG(x)
#define DBG(x) x
#define OUT dprintf

// constructor
KDiskSystem::KDiskSystem(const char *name)
	: fID(_NextID()),
	  fName(NULL),
	  fPrettyName(NULL),
	  fLoadCounter(0)
{
	set_string(fName, name);
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
	return (fName ? B_OK : B_NO_MEMORY);
}

// SetID
/*void
KDiskSystem::SetID(disk_system_id id)
{
	fID = id;
}*/

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

// Flags
uint32
KDiskSystem::Flags() const
{
	return fFlags;
}

// IsFileSystem
bool
KDiskSystem::IsFileSystem() const
{
	return (fFlags & B_DISK_SYSTEM_IS_FILE_SYSTEM);
}

// IsPartitioningSystem
bool
KDiskSystem::IsPartitioningSystem() const
{
	return !(fFlags & B_DISK_SYSTEM_IS_FILE_SYSTEM);
}

// GetInfo
void
KDiskSystem::GetInfo(user_disk_system_info *info)
{
	if (!info)
		return;
	info->id = ID();
	strcpy(info->name, Name());
	strcpy(info->pretty_name, PrettyName());
	info->flags = Flags();
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
KDiskSystem::SupportsMoving(KPartition *partition, bool *isNoOp)
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

// SupportsSettingName
bool
KDiskSystem::SupportsSettingName(KPartition *partition)
{
	// to be implemented by derived classes
	return false;
}

// SupportsSettingContentName
bool
KDiskSystem::SupportsSettingContentName(KPartition *partition,
										bool *whileMounted)
{
	// to be implemented by derived classes
	return false;
}

// SupportsSettingType
bool
KDiskSystem::SupportsSettingType(KPartition *partition)
{
	// to be implemented by derived classes
	return false;
}

// SupportsSettingParameters
bool
KDiskSystem::SupportsSettingParameters(KPartition *partition)
{
	// to be implemented by derived classes
	return false;
}

// SupportsSettingContentParameters
bool
KDiskSystem::SupportsSettingContentParameters(KPartition *partition,
											  bool *whileMounted)
{
	// to be implemented by derived classes
	return false;
}

// SupportsInitializing
bool
KDiskSystem::SupportsInitializing(KPartition *partition)
{
	// to be implemented by derived classes
	return false;
}

// SupportsInitializingChild
bool
KDiskSystem::SupportsInitializingChild(KPartition *child,
									   const char *diskSystem)
{
	// to be implemented by derived classes
	return false;
}

// SupportsCreatingChild
bool
KDiskSystem::SupportsCreatingChild(KPartition *parent)
{
	// to be implemented by derived classes
	return false;
}

// SupportsDeletingChild
bool
KDiskSystem::SupportsDeletingChild(KPartition *child)
{
	// to be implemented by derived classes
	return false;
}

// IsSubSystemFor
bool
KDiskSystem::IsSubSystemFor(KPartition *partition)
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

// ValidateResizeChild
bool
KDiskSystem::ValidateResizeChild(KPartition *child, off_t *size)
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

// ValidateMoveChild
bool
KDiskSystem::ValidateMoveChild(KPartition *child, off_t *start)
{
	// to be implemented by derived classes
	return false;
}

// ValidateSetName
bool
KDiskSystem::ValidateSetName(KPartition *partition, char *name)
{
	// to be implemented by derived classes
	return false;
}

// ValidateSetContentName
bool
KDiskSystem::ValidateSetContentName(KPartition *partition, char *name)
{
	// to be implemented by derived classes
	return false;
}

// ValidateSetType
bool
KDiskSystem::ValidateSetType(KPartition *partition, const char *type)
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
KDiskSystem::ValidateSetContentParameters(KPartition *partition,
										  const char *parameters)
{
	// to be implemented by derived classes
	return false;
}

// ValidateInitialize
bool
KDiskSystem::ValidateInitialize(KPartition *partition, char *name,
								const char *parameters)
{
	// to be implemented by derived classes
	return false;
}

// ValidateCreateChild
bool
KDiskSystem::ValidateCreateChild(KPartition *partition, off_t *start,
								 off_t *size, const char *type,
								 const char *parameters, int32 *index)
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
status_t
KDiskSystem::GetPartitionableSpaces(KPartition *partition,
									partitionable_space_data *buffer,
									int32 count, int32 *actualCount)
{
	// to be implemented by derived classes
	return B_ERROR;
}

// GetNextSupportedType
status_t
KDiskSystem::GetNextSupportedType(KPartition *partition, int32 *cookie,
								  char *type)
{
	// to be implemented by derived classes
	return B_ENTRY_NOT_FOUND;
}

// ShadowPartitionChanged
status_t
KDiskSystem::ShadowPartitionChanged(KPartition *partition, uint32 operation)
{
	// to be implemented by derived classes
	return B_ENTRY_NOT_FOUND;
}

// GetTypeForContentType
status_t
KDiskSystem::GetTypeForContentType(const char *contentType, char *type)
{
	// to be implemented by derived classes
	return B_ENTRY_NOT_FOUND;
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

// SetName
status_t
KDiskSystem::SetName(KPartition *partition, char *name, KDiskDeviceJob *job)
{
	// to be implemented by derived classes
	return B_ERROR;
}

// SetContentName
status_t
KDiskSystem::SetContentName(KPartition *partition, char *name,
							KDiskDeviceJob *job)
{
	// to be implemented by derived classes
	return B_ERROR;
}

// SetType
status_t
KDiskSystem::SetType(KPartition *partition, char *type, KDiskDeviceJob *job)
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

// Initialize
status_t
KDiskSystem::Initialize(KPartition *partition, const char *name,
						const char *parameters, KDiskDeviceJob *job)
{
	// to be implemented by derived classes
	return B_ERROR;
}

// CreateChild
status_t
KDiskSystem::CreateChild(KPartition *partition, off_t offset, off_t size,
						 const char *type, const char *parameters,
						 KDiskDeviceJob *job, KPartition **child,
						 partition_id childID)
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

// SetFlags
void
KDiskSystem::SetFlags(uint32 flags)
{
	fFlags = flags;
}

// _NextID
int32
KDiskSystem::_NextID()
{
	return atomic_add(&fNextID, 1);
}


// fNextID
int32 KDiskSystem::fNextID = 0;

