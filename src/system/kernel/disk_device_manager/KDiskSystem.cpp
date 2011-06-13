/*
 * Copyright 2003-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <stdlib.h>

#include <KernelExport.h>
#include <util/kernel_cpp.h>

#include "ddm_userland_interface.h"
#include "KDiskDeviceManager.h"
#include "KDiskDeviceUtils.h"
#include "KDiskSystem.h"


// debugging
//#define TRACE_KDISK_SYSTEM
#ifdef TRACE_KDISK_SYSTEM
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...)	do { } while (false)
#endif


// constructor
KDiskSystem::KDiskSystem(const char *name)
	: fID(_NextID()),
	  fName(NULL),
	  fShortName(NULL),
	  fPrettyName(NULL),
	  fLoadCounter(0)
{
	set_string(fName, name);
}


// destructor
KDiskSystem::~KDiskSystem()
{
	free(fName);
	free(fShortName);
	free(fPrettyName);
}


// Init
status_t
KDiskSystem::Init()
{
	return fName ? B_OK : B_NO_MEMORY;
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


// ShortName
const char *
KDiskSystem::ShortName() const
{
	return fShortName;
}


// PrettyName
const char *
KDiskSystem::PrettyName() const
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
	strlcpy(info->name, Name(), sizeof(info->name));
	strlcpy(info->short_name, ShortName(), sizeof(info->short_name));
	strlcpy(info->pretty_name, PrettyName(), sizeof(info->pretty_name));
	info->flags = Flags();
}


// Load
status_t
KDiskSystem::Load()
{
	ManagerLocker locker(KDiskDeviceManager::Default());
TRACE("KDiskSystem::Load(): %s -> %ld\n", Name(), fLoadCounter + 1);
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
TRACE("KDiskSystem::Unload(): %s -> %ld\n", Name(), fLoadCounter - 1);
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


// Defragment
status_t
KDiskSystem::Defragment(KPartition* partition, disk_job_id job)
{
	// to be implemented by derived classes
	return B_ERROR;
}


// Repair
status_t
KDiskSystem::Repair(KPartition* partition, bool checkOnly, disk_job_id job)
{
	// to be implemented by derived classes
	return B_ERROR;
}


// Resize
status_t
KDiskSystem::Resize(KPartition* partition, off_t size, disk_job_id job)
{
	// to be implemented by derived classes
	return B_ERROR;
}


// ResizeChild
status_t
KDiskSystem::ResizeChild(KPartition* child, off_t size, disk_job_id job)
{
	// to be implemented by derived classes
	return B_ERROR;
}


// Move
status_t
KDiskSystem::Move(KPartition* partition, off_t offset, disk_job_id job)
{
	// to be implemented by derived classes
	return B_ERROR;
}


// MoveChild
status_t
KDiskSystem::MoveChild(KPartition* child, off_t offset, disk_job_id job)
{
	// to be implemented by derived classes
	return B_ERROR;
}


// SetName
status_t
KDiskSystem::SetName(KPartition* partition, const char* name, disk_job_id job)
{
	// to be implemented by derived classes
	return B_ERROR;
}


// SetContentName
status_t
KDiskSystem::SetContentName(KPartition* partition, const char* name,
	disk_job_id job)
{
	// to be implemented by derived classes
	return B_ERROR;
}


// SetType
status_t
KDiskSystem::SetType(KPartition* partition, const char *type, disk_job_id job)
{
	// to be implemented by derived classes
	return B_ERROR;
}


// SetParameters
status_t
KDiskSystem::SetParameters(KPartition* partition, const char* parameters,
	disk_job_id job)
{
	// to be implemented by derived classes
	return B_ERROR;
}


// SetContentParameters
status_t
KDiskSystem::SetContentParameters(KPartition* partition,
	const char* parameters, disk_job_id job)
{
	// to be implemented by derived classes
	return B_ERROR;
}


// Initialize
status_t
KDiskSystem::Initialize(KPartition* partition, const char* name,
	const char* parameters, disk_job_id job)
{
	// to be implemented by derived classes
	return B_ERROR;
}


status_t
KDiskSystem::Uninitialize(KPartition* partition, disk_job_id job)
{
	// to be implemented by derived classes
	return B_NOT_SUPPORTED;
}


// CreateChild
status_t
KDiskSystem::CreateChild(KPartition* partition, off_t offset, off_t size,
	const char* type, const char* name, const char* parameters, disk_job_id job,
	KPartition **child, partition_id childID)
{
	// to be implemented by derived classes
	return B_ERROR;
}


// DeleteChild
status_t
KDiskSystem::DeleteChild(KPartition* child, disk_job_id job)
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


// SetShortName
status_t
KDiskSystem::SetShortName(const char *name)
{
	return set_string(fShortName, name);
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

