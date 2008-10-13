/*
 * Copyright 2003-2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */

#include <DiskSystem.h>

#include <DiskSystemAddOn.h>
#include <Partition.h>

#include <ddm_userland_interface_defs.h>
#include <syscalls.h>

#include "DiskSystemAddOnManager.h"


// constructor
BDiskSystem::BDiskSystem()
	:
	fID(B_NO_INIT),
	fFlags(0)
{
}


// copy constructor
BDiskSystem::BDiskSystem(const BDiskSystem& other)
	:
	fID(other.fID),
	fName(other.fName),
	fShortName(other.fShortName),
	fPrettyName(other.fPrettyName),
	fFlags(other.fFlags)
{
}


// destructor
BDiskSystem::~BDiskSystem()
{
}


// InitCheck
status_t
BDiskSystem::InitCheck() const
{
	return fID > 0 ? B_OK : fID;
}


// Name
const char*
BDiskSystem::Name() const
{
	return fName.String();
}


// ShortName
const char*
BDiskSystem::ShortName() const
{
	return fShortName.String();
}


// PrettyName
const char*
BDiskSystem::PrettyName() const
{
	return fPrettyName.String();
}


// SupportsDefragmenting
bool
BDiskSystem::SupportsDefragmenting(bool* whileMounted) const
{
	if (InitCheck() != B_OK
		|| !(fFlags & B_DISK_SYSTEM_SUPPORTS_DEFRAGMENTING)) {
		if (whileMounted)
			*whileMounted = false;
		return false;
	}

	if (whileMounted) {
		*whileMounted = IsFileSystem() && (fFlags
				& B_DISK_SYSTEM_SUPPORTS_DEFRAGMENTING_WHILE_MOUNTED) != 0;
	}

	return true;
}


// SupportsRepairing
bool
BDiskSystem::SupportsRepairing(bool checkOnly, bool* whileMounted) const
{
	uint32 mainBit = B_DISK_SYSTEM_SUPPORTS_REPAIRING;
	uint32 mountedBit = B_DISK_SYSTEM_SUPPORTS_REPAIRING_WHILE_MOUNTED;

	if (checkOnly) {
		mainBit = B_DISK_SYSTEM_SUPPORTS_CHECKING;
		mountedBit = B_DISK_SYSTEM_SUPPORTS_CHECKING_WHILE_MOUNTED;
	}

	if (InitCheck() != B_OK || !(fFlags & mainBit)) {
		if (whileMounted)
			*whileMounted = false;
		return false;
	}

	if (whileMounted)
		*whileMounted = (IsFileSystem() && (fFlags & mountedBit));

	return true;
}


// SupportsResizing
bool
BDiskSystem::SupportsResizing(bool* whileMounted) const
{
	if (InitCheck() != B_OK
		|| !(fFlags & B_DISK_SYSTEM_SUPPORTS_RESIZING)) {
		if (whileMounted)
			*whileMounted = false;
		return false;
	}

	if (whileMounted) {
		*whileMounted = (IsFileSystem()
			&& (fFlags & B_DISK_SYSTEM_SUPPORTS_RESIZING_WHILE_MOUNTED));
	}

	return true;
}


// SupportsResizingChild
bool
BDiskSystem::SupportsResizingChild() const
{
	return (InitCheck() == B_OK && IsPartitioningSystem()
		&& (fFlags & B_DISK_SYSTEM_SUPPORTS_RESIZING_CHILD));
}


// SupportsMoving
bool
BDiskSystem::SupportsMoving(bool* whileMounted) const
{
	if (InitCheck() != B_OK
		|| !(fFlags & B_DISK_SYSTEM_SUPPORTS_MOVING)) {
		if (whileMounted)
			*whileMounted = false;
		return false;
	}

	if (whileMounted) {
		*whileMounted = (IsFileSystem()
			&& (fFlags & B_DISK_SYSTEM_SUPPORTS_MOVING_WHILE_MOUNTED));
	}

	return true;
}


// SupportsMovingChild
bool
BDiskSystem::SupportsMovingChild() const
{
	return (InitCheck() == B_OK && IsPartitioningSystem()
		&& (fFlags & B_DISK_SYSTEM_SUPPORTS_MOVING_CHILD));
}


// SupportsName
bool
BDiskSystem::SupportsName() const
{
	return (InitCheck() == B_OK && IsPartitioningSystem()
		&& (fFlags & B_DISK_SYSTEM_SUPPORTS_NAME));
}


// SupportsContentName
bool
BDiskSystem::SupportsContentName() const
{
	return (InitCheck() == B_OK
		&& (fFlags & B_DISK_SYSTEM_SUPPORTS_CONTENT_NAME));
}


// SupportsSettingName
bool
BDiskSystem::SupportsSettingName() const
{
	return (InitCheck() == B_OK && IsPartitioningSystem()
		&& (fFlags & B_DISK_SYSTEM_SUPPORTS_SETTING_NAME));
}


// SupportsSettingContentName
bool
BDiskSystem::SupportsSettingContentName(bool* whileMounted) const
{
	if (InitCheck() != B_OK
		|| !(fFlags & B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_NAME)) {
		if (whileMounted)
			*whileMounted = false;
		return false;
	}

	if (whileMounted) {
		*whileMounted = (IsFileSystem()
			&& (fFlags
				& B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_NAME_WHILE_MOUNTED));
	}

	return true;
}


// SupportsSettingType
bool
BDiskSystem::SupportsSettingType() const
{
	return (InitCheck() == B_OK && IsPartitioningSystem()
		&& (fFlags & B_DISK_SYSTEM_SUPPORTS_SETTING_TYPE));
}


// SupportsSettingParameters
bool
BDiskSystem::SupportsSettingParameters() const
{
	return (InitCheck() == B_OK && IsPartitioningSystem()
		&& (fFlags & B_DISK_SYSTEM_SUPPORTS_SETTING_PARAMETERS));
}


// SupportsSettingContentParameters
bool
BDiskSystem::SupportsSettingContentParameters(bool* whileMounted) const
{
	if (InitCheck() != B_OK
		|| !(fFlags & B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_PARAMETERS)) {
		if (whileMounted)
			*whileMounted = false;
		return false;
	}

	if (whileMounted) {
		uint32 whileMountedFlag
			= B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_PARAMETERS_WHILE_MOUNTED;
		*whileMounted = (IsFileSystem() && (fFlags & whileMountedFlag));
	}

	return true;
}


// SupportsCreatingChild
bool
BDiskSystem::SupportsCreatingChild() const
{
	return (InitCheck() == B_OK && IsPartitioningSystem()
		&& (fFlags & B_DISK_SYSTEM_SUPPORTS_CREATING_CHILD));
}


// SupportsDeletingChild
bool
BDiskSystem::SupportsDeletingChild() const
{
	return (InitCheck() == B_OK && IsPartitioningSystem()
		&& (fFlags & B_DISK_SYSTEM_SUPPORTS_DELETING_CHILD));
}


// SupportsInitializing
bool
BDiskSystem::SupportsInitializing() const
{
	return (InitCheck() == B_OK
		&& (fFlags & B_DISK_SYSTEM_SUPPORTS_INITIALIZING));
}


bool
BDiskSystem::SupportsWriting() const
{
	if (InitCheck() != B_OK
		|| !IsFileSystem())
		return false;

	return (fFlags & B_DISK_SYSTEM_SUPPORTS_WRITING) != 0;
}


// GetTypeForContentType
status_t
BDiskSystem::GetTypeForContentType(const char* contentType, BString* type) const
{
	if (InitCheck() != B_OK)
		return InitCheck();

	if (!contentType || !type || !IsPartitioningSystem())
		return B_BAD_VALUE;

	// get the disk system add-on
	DiskSystemAddOnManager* manager = DiskSystemAddOnManager::Default();
	BDiskSystemAddOn* addOn = manager->GetAddOn(fName.String());
	if (!addOn)
		return B_ENTRY_NOT_FOUND;

	status_t result = addOn->GetTypeForContentType(contentType, type);

	// put the add-on
	manager->PutAddOn(addOn);

	return result;
}


// IsPartitioningSystem
bool
BDiskSystem::IsPartitioningSystem() const
{
	return InitCheck() == B_OK && !(fFlags & B_DISK_SYSTEM_IS_FILE_SYSTEM);
}


// IsFileSystem
bool
BDiskSystem::IsFileSystem() const
{
	return InitCheck() == B_OK && (fFlags & B_DISK_SYSTEM_IS_FILE_SYSTEM);
}


// =
BDiskSystem&
BDiskSystem::operator=(const BDiskSystem& other)
{
	fID = other.fID;
	fName = other.fName;
	fShortName = other.fShortName;
	fPrettyName = other.fPrettyName;
	fFlags = other.fFlags;

	return *this;
}


// _SetTo
status_t
BDiskSystem::_SetTo(disk_system_id id)
{
	_Unset();

	if (id < 0)
		return fID;

	user_disk_system_info info;
	status_t error = _kern_get_disk_system_info(id, &info);
	if (error != B_OK)
		return (fID = error);

	return _SetTo(&info);
}


// _SetTo
status_t
BDiskSystem::_SetTo(const user_disk_system_info* info)
{
	_Unset();

	if (!info)
		return (fID = B_BAD_VALUE);

	fID = info->id;
	fName = info->name;
	fShortName = info->short_name;
	fPrettyName = info->pretty_name;
	fFlags = info->flags;

	return B_OK;
}


// _Unset
void
BDiskSystem::_Unset()
{
	fID = B_NO_INIT;
	fName = (const char*)NULL;
	fPrettyName = (const char*)NULL;
	fFlags = 0;
}
