//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <ddm_userland_interface.h>
#include <DiskSystem.h>
#include <Partition.h>

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
	return (InitCheck() == B_OK && IsFileSystem()
			&& partition && partition->_IsShadow()
			&& partition->_DiskSystem() == fID
			&& _kern_supports_defragmenting_partition(fID,
					partition->_ShadowID(), whileMounted));
}

// SupportsRepairing
bool
BDiskSystem::SupportsRepairing(BPartition *partition, bool checkOnly,
							   bool *whileMounted) const
{
	return (InitCheck() == B_OK
			&& partition && partition->_IsShadow()
			&& partition->_DiskSystem() == fID
			&& _kern_supports_repairing_partition(fID, partition->_ShadowID(),
												  checkOnly, whileMounted));
}

// SupportsResizing
bool
BDiskSystem::SupportsResizing(BPartition *partition, bool *whileMounted) const
{
	return (InitCheck() == B_OK
			&& partition && partition->_IsShadow()
			&& partition->_DiskSystem() == fID
			&& _kern_supports_resizing_partition(fID, partition->_ShadowID(),
												 whileMounted));
}

// SupportsResizingChild
bool
BDiskSystem::SupportsResizingChild(BPartition *child) const
{
	return (InitCheck() == B_OK && IsPartitioningSystem()
			&& child && child->_IsShadow() && child->Parent()
			&& child->Parent()->_DiskSystem() == fID
			&& _kern_supports_resizing_child_partition(fID,
													   child->_ShadowID()));
}

// SupportsMoving
bool
BDiskSystem::SupportsMoving(BPartition *partition, bool *whileMounted) const
{
	return (InitCheck() == B_OK
			&& partition && partition->_IsShadow()
			&& partition->_DiskSystem() == fID
			&& _kern_supports_moving_partition(fID, partition->_ShadowID(),
											   whileMounted));
}

// SupportsMovingChild
bool
BDiskSystem::SupportsMovingChild(BPartition *child) const
{
	return (InitCheck() == B_OK && IsPartitioningSystem()
			&& child && child->_IsShadow() && child->Parent()
			&& child->Parent()->_DiskSystem() == fID
			&& _kern_supports_moving_child_partition(fID, child->_ShadowID()));
}

// SupportsSettingName
bool
BDiskSystem::SupportsSettingName(BPartition *partition) const
{
	return (InitCheck() == B_OK && IsPartitioningSystem()
			&& partition && partition->_IsShadow() && partition->Parent()
			&& partition->Parent()->_DiskSystem() == fID
			&& _kern_supports_setting_partition_name(fID,
													 partition->_ShadowID()));
}

// SupportsSettingContentName
bool
BDiskSystem::SupportsSettingContentName(BPartition *partition,
										bool *whileMounted) const
{
	return (InitCheck() == B_OK
			&& partition && partition->_IsShadow()
			&& partition->_DiskSystem() == fID
			&& _kern_supports_setting_partition_content_name(fID,
					partition->_ShadowID(), whileMounted));
}

// SupportsSettingType
bool
BDiskSystem::SupportsSettingType(BPartition *partition) const
{
	return (InitCheck() == B_OK && IsPartitioningSystem()
			&& partition && partition->_IsShadow() && partition->Parent()
			&& partition->Parent()->_DiskSystem() == fID
			&& _kern_supports_setting_partition_type(fID,
													 partition->_ShadowID()));
}

// SupportsCreatingChild
bool
BDiskSystem::SupportsCreatingChild(BPartition *partition) const
{
	return (InitCheck() == B_OK && IsPartitioningSystem()
			&& partition && partition->_IsShadow()
			&& partition->_DiskSystem() == fID
			&& _kern_supports_creating_child_partition(fID,
					partition->_ShadowID()));
}

// SupportsDeletingChild
bool
BDiskSystem::SupportsDeletingChild(BPartition *child) const
{
	return (InitCheck() == B_OK && IsPartitioningSystem()
			&& child && child->_IsShadow() && child->Parent()
			&& child->Parent()->_DiskSystem() == fID
			&& _kern_supports_deleting_child_partition(fID,
													   child->_ShadowID()));
}

// SupportsInitializing
bool
BDiskSystem::SupportsInitializing(BPartition *partition) const
{
	return (InitCheck() == B_OK
			&& partition && partition->_IsShadow()
			&& _kern_supports_initializing_partition(fID,
				partition->_ShadowID()));
}

// SupportsInitializingChild
bool
BDiskSystem::SupportsInitializingChild(BPartition *child,
									   const char *diskSystem) const
{
	return (InitCheck() == B_OK && IsPartitioningSystem() && diskSystem
			&& child && child->_IsShadow() && child->Parent()
			&& child->Parent()->_DiskSystem() == fID
			&& _kern_supports_initializing_child_partition(fID,
					child->_ShadowID(), diskSystem));
}

// ValidateResize
status_t
BDiskSystem::ValidateResize(BPartition *partition, off_t *size) const
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

// ValidateMove
status_t
BDiskSystem::ValidateMove(BPartition *partition, off_t *start) const
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
BDiskSystem::ValidateSetType(BPartition *partition, const char *type) const
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

