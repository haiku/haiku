//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <DiskSystem.h>
#include <ddm_userland_interface.h>

// constructor
BDiskSystem::BDiskSystem()
	: fID(B_NO_INIT),
	  fName(),
	  fPrettyName(),
	  fFileSystem(false)
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
	return (fID > 0 ? B_OK : fID);
}

// Name
const char *
BDiskSystem::Name() const
{
	return fName.String();
}

// PrettyName
const char *
BDiskSystem::PrettyName() const
{
	return fPrettyName.String();
}

// SupportsDefragmenting
bool
BDiskSystem::SupportsDefragmenting(BPartition *partition,
								   bool *whileMounted) const
{
	// not implemented
	return false;
}

// SupportsRepairing
bool
BDiskSystem::SupportsRepairing(BPartition *partition, bool checkOnly,
							   bool *whileMounted) const
{
	// not implemented
	return false;
}

// SupportsResizing
bool
BDiskSystem::SupportsResizing(BPartition *partition, bool *whileMounted) const
{
	// not implemented
	return false;
}

// SupportsResizingChild
bool
BDiskSystem::SupportsResizingChild(BPartition *child) const
{
	// not implemented
	return false;
}

// SupportsMoving
bool
BDiskSystem::SupportsMoving(BPartition *partition, bool *whileMounted) const
{
	// not implemented
	return false;
}

// SupportsMovingChild
bool
BDiskSystem::SupportsMovingChild(BPartition *child) const
{
	// not implemented
	return false;
}

// SupportsSettingName
bool
BDiskSystem::SupportsSettingName(BPartition *partition) const
{
	// not implemented
	return false;
}

// SupportsSettingContentName
bool
BDiskSystem::SupportsSettingContentName(BPartition *partition,
										bool *whileMounted) const
{
	// not implemented
	return false;
}

// SupportsSettingType
bool
BDiskSystem::SupportsSettingType(BPartition *partition) const
{
	// not implemented
	return false;
}

// SupportsCreatingChild
bool
BDiskSystem::SupportsCreatingChild(BPartition *partition) const
{
	// not implemented
	return false;
}

// SupportsParentSystem
bool
BDiskSystem::SupportsParentSystem(BPartition *child, const char *system) const
{
	// not implemented
	return false;
}

// SupportsChildSystem
bool
BDiskSystem::SupportsChildSystem(BPartition *child, const char *system) const
{
	// not implemented
	return false;
}

// ValidateResize
status_t
BDiskSystem::ValidateResize(BPartition *partition, off_t *size) const
{
	// not implemented
	return B_ERROR;
}

// ValidateMove
status_t
BDiskSystem::ValidateMove(BPartition *partition, off_t *start) const
{
	// not implemented
	return B_ERROR;
}

// ValidateResizeChild
status_t
BDiskSystem::ValidateResizeChild(BPartition *partition, off_t *size) const
{
	// not implemented
	return B_ERROR;
}

// ValidateMoveChild
status_t
BDiskSystem::ValidateMoveChild(BPartition *partition, off_t *start) const
{
	// not implemented
	return B_ERROR;
}

// ValidateSetName
status_t
BDiskSystem::ValidateSetName(BPartition *partition, char *name) const
{
	// not implemented
	return B_ERROR;
}

// ValidateSetContentName
status_t
BDiskSystem::ValidateSetContentName(BPartition *partition, char *name) const
{
	// not implemented
	return B_ERROR;
}

// ValidateSetType
status_t
BDiskSystem::ValidateSetType(BPartition *partition, char *type) const
{
	// not implemented
	return B_ERROR;
}

// ValidateCreateChild
status_t
BDiskSystem::ValidateCreateChild(BPartition *partition, off_t *start,
								 off_t *size, const char *type,
								 const char *parameters) const
{
	// not implemented
	return B_ERROR;
}

// GetNextSupportedType
status_t
BDiskSystem::GetNextSupportedType(BPartition *partition, int32 *cookie,
								  char *type) const
{
	// not implemented
	return B_ERROR;
}

// GetTypeForContentType
status_t
BDiskSystem::GetTypeForContentType(const char *contentType, char *type) const
{
	// not implemented
	return B_ERROR;
}

// IsPartitioningSystem
bool
BDiskSystem::IsPartitioningSystem() const
{
	return (InitCheck() == B_OK && !fFileSystem);
}

// IsFileSystem
bool
BDiskSystem::IsFileSystem() const
{
	return (InitCheck() == B_OK && fFileSystem);
}

// IsSubSystemFor
bool
BDiskSystem::IsSubSystemFor(BPartition *parent) const
{
	// not implemented
	return false;
}

// _SetTo
status_t
BDiskSystem::_SetTo(user_disk_system_info *info)
{
	_Unset();
	if (!info)
		return (fID = B_BAD_VALUE);
	fID = info->id;
	fName = info->name;
	fPrettyName = info->pretty_name;
	fFileSystem = info->file_system;
	return B_OK;
}

// _Unset
void
BDiskSystem::_Unset()
{
	fID = B_NO_INIT;
	fName = (const char*)NULL;
	fPrettyName = (const char*)NULL;
	fFileSystem = false;
}

