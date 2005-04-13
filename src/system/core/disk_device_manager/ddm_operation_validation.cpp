// ddm_operation_validation.cpp

#include <KDiskDevice.h>
#include <KDiskDeviceManager.h>
#include <KDiskDeviceUtils.h>
#include <KDiskSystem.h>
#include <KShadowPartition.h>

#include "ddm_operation_validation.h"

// static functions
namespace BPrivate {
namespace DiskDevice {

static status_t check_busy_partition(const KPartition *partition, bool shadow);
static status_t check_partition(const KPartition *partition,
								int32 changeCounter, bool shadow);

} // namespace DiskDevice
} // namespace BPrivate

// get_current_team
team_id
BPrivate::DiskDevice::get_current_team()
{
	// TODO: There must be a straighter way in kernelland.
	thread_info info;
	get_thread_info(find_thread(NULL), &info);
	return info.team;
}

// check_shadow_partition
bool
BPrivate::DiskDevice::check_shadow_partition(const KPartition *partition,
											 int32 changeCounter,
											 bool requireShadow)
{
	if (!partition || !partition->Device()
		|| partition->IsShadowPartition() != requireShadow
		|| changeCounter != partition->ChangeCounter()) {
		return false;
	}
	if (!requireShadow)
		return true;
	return (partition->IsShadowPartition()
			&& partition->Device()->ShadowOwner() == get_current_team());
}

// check_busy_partition
static
status_t
BPrivate::DiskDevice::check_busy_partition(const KPartition *partition,
										   bool shadow)
{
	bool busy = (partition->IsBusy() || partition->IsDescendantBusy());
	if (shadow) {
		if (busy)
			return B_BUSY;
	} else {
		if (!busy)
			return B_BAD_VALUE;
	}
	return B_OK;
}

// check_partition
static
status_t
BPrivate::DiskDevice::check_partition(const KPartition *partition,
									  int32 changeCounter, bool shadow)
{
	if (!check_shadow_partition(partition, changeCounter, shadow))
		return B_BAD_VALUE;
	bool busy = (partition->IsBusy() || partition->IsDescendantBusy());
	if (shadow) {
		if (busy)
			return B_BUSY;
	} else {
		if (!busy)
			return B_BAD_VALUE;
	}
	return B_OK;
}


// get_unmovable_descendants
bool
BPrivate::DiskDevice::get_unmovable_descendants(KPartition *partition,
	partition_id *&unmovable, size_t &unmovableSize,
	partition_id *&needUnmounting, size_t &needUnmountingSize,
	bool requireShadow)
{
	// check parameters
	if (!partition || !unmovable || !needUnmounting || unmovableSize == 0
		|| needUnmountingSize) {
		return false;
	}
	// check partition
	KDiskSystem *diskSystem = partition->DiskSystem();
	bool isNoOp = true;
	bool supports = (diskSystem
					 && diskSystem->SupportsMoving(partition, &isNoOp));
	if (supports) {
		unmovable[0] = partition->ID();
		unmovableSize--;
	}
	if (supports && !isNoOp && diskSystem->IsFileSystem()) {
		needUnmounting[0] = partition->ID();
		needUnmountingSize--;
	}
	// check child partitions
	for (int32 i = 0; KPartition *child = partition->ChildAt(i); i++) {
		if (!get_unmovable_descendants(child, unmovable, unmovableSize,
									   needUnmounting, needUnmountingSize)) {
			return false;
		}
	}
	return true;
}

// validate_move_descendants
status_t
BPrivate::DiskDevice::validate_move_descendants(KPartition *partition,
	off_t moveBy, bool markMovable, bool requireShadow)
{
	if (!partition)
		return B_BAD_VALUE;
	// check partition
	bool uninitialized = partition->IsUninitialized();
	KDiskSystem *diskSystem = partition->DiskSystem();
	bool movable = (uninitialized || diskSystem
					|| diskSystem->SupportsMoving(partition, NULL));
	if (markMovable)
		partition->SetAlgorithmData(movable);
	// moving partition is supported in principle, now check the new offset
	if (!uninitialized) {
		if (movable) {
			off_t offset = partition->Offset() + moveBy;
			off_t newOffset = offset;
			if (!diskSystem->ValidateMove(partition, &newOffset)
				|| newOffset != offset) {
				return B_ERROR;
			}
		} else
			return B_ERROR;
		// check children
		for (int32 i = 0; KPartition *child = partition->ChildAt(i); i++) {
			status_t error = validate_move_descendants(child, moveBy);
			if (error != B_OK)
				return error;
		}
	}
	return B_OK;
}

// validate_defragment_partition
status_t
BPrivate::DiskDevice::validate_defragment_partition(KPartition *partition,
	int32 changeCounter, bool *whileMounted, bool requireShadow)
{
	status_t error = check_partition(partition, changeCounter, requireShadow);
	if (error != B_OK)
		return error;
	// get the disk system and get the info
	KDiskSystem *diskSystem = partition->DiskSystem();
	if (!diskSystem)
		return B_ENTRY_NOT_FOUND;
	if (diskSystem->SupportsDefragmenting(partition, whileMounted))
		return B_OK;
	return B_ERROR;
}

// validate_repair_partition
status_t
BPrivate::DiskDevice::validate_repair_partition(KPartition *partition,
	int32 changeCounter, bool checkOnly, bool *whileMounted,
	bool requireShadow)
{
	status_t error = check_partition(partition, changeCounter, requireShadow);
	if (error != B_OK)
		return error;
	// get the disk system and get the info
	KDiskSystem *diskSystem = partition->DiskSystem();
	if (!diskSystem)
		return B_ENTRY_NOT_FOUND;
	if (diskSystem->SupportsRepairing(partition, checkOnly, whileMounted))
		return B_OK;
	return B_ERROR;
}

// validate_resize_partition
status_t
BPrivate::DiskDevice::validate_resize_partition(KPartition *partition,
	int32 changeCounter, off_t *size, off_t *contentSize, bool requireShadow)
{
	if (!partition || !size || !contentSize
		|| !check_shadow_partition(partition, changeCounter, requireShadow)
		|| !partition->Parent()) {
		return B_BAD_VALUE;
	}
	status_t error = check_busy_partition(partition->Parent(), requireShadow);
	if (error != B_OK)
		return error;
	// get the parent disk system and let it check the value
	KDiskSystem *parentDiskSystem = partition->Parent()->DiskSystem();
	if (!parentDiskSystem)
		return B_ENTRY_NOT_FOUND;
	if (!parentDiskSystem->ValidateResizeChild(partition, size))
		return B_ERROR;
	// if contents is uninitialized, then there's no need to check anything
	// more
	if (partition->IsUninitialized())
		return B_OK;
	// get the child disk system and let it check the value
	KDiskSystem *childDiskSystem = partition->DiskSystem();
	if (!childDiskSystem)
		return B_ENTRY_NOT_FOUND;
	*contentSize = *size;
	// don't worry, if the content system desires to be smaller
	if (!childDiskSystem->ValidateResize(partition, contentSize)
		|| *contentSize > *size) {
		return B_ERROR;
	}
	return B_OK;
}

// validate_move_partition
status_t
BPrivate::DiskDevice::validate_move_partition(KPartition *partition,
	int32 changeCounter, off_t *newOffset, bool markMovable,
	bool requireShadow)
{
	if (!partition || !newOffset
		|| !check_shadow_partition(partition, changeCounter, requireShadow)
		|| !partition->Parent()) {
		return B_BAD_VALUE;
	}
	status_t error = check_busy_partition(partition->Parent(), requireShadow);
	if (error != B_OK)
		return error;
	// get the parent disk system and let it check the value
	KDiskSystem *parentDiskSystem = partition->Parent()->DiskSystem();
	if (!parentDiskSystem)
		return B_ENTRY_NOT_FOUND;
	if (!parentDiskSystem->ValidateMoveChild(partition, newOffset))
		return B_ERROR;
	// let the concerned content disk systems check the value
	return validate_move_descendants(partition,
		partition->Offset() - *newOffset, markMovable);
}

// validate_set_partition_name
status_t
BPrivate::DiskDevice::validate_set_partition_name(KPartition *partition,
	int32 changeCounter, char *name, bool requireShadow)
{
	if (!partition || !name)
		return B_BAD_VALUE;
	// truncate name to maximal size
	name[B_OS_NAME_LENGTH] = '\0';
	// check the partition
	if (!check_shadow_partition(partition, changeCounter, requireShadow)
		|| !partition->Parent()) {
		return B_BAD_VALUE;
	}
	status_t error = check_busy_partition(partition->Parent(), requireShadow);
	if (error != B_OK)
		return error;
	// get the disk system
	KDiskSystem *diskSystem = partition->Parent()->DiskSystem();
	if (!diskSystem)
		return B_ENTRY_NOT_FOUND;
	// get the info
	if (diskSystem->ValidateSetName(partition, name))
		return B_OK;
	return B_ERROR;
}

// validate_set_partition_content_name
status_t
BPrivate::DiskDevice::validate_set_partition_content_name(
	KPartition *partition, int32 changeCounter, char *name, bool requireShadow)
{
	if (!partition || !name)
		return B_BAD_VALUE;
	// truncate name to maximal size
	name[B_OS_NAME_LENGTH] = '\0';
	// check the partition
	status_t error = check_partition(partition, changeCounter, requireShadow);
	if (error != B_OK)
		return error;
	// get the disk system
	KDiskSystem *diskSystem = partition->DiskSystem();
	if (!diskSystem)
		return B_ENTRY_NOT_FOUND;
	// get the info
	if (diskSystem->ValidateSetContentName(partition, name))
		return B_OK;
	return B_ERROR;
}

// validate_set_partition_type
status_t
BPrivate::DiskDevice::validate_set_partition_type(KPartition *partition,
	int32 changeCounter, const char *type, bool requireShadow)
{
	if (!partition || !type)
		return B_BAD_VALUE;
	// check the partition
	if (!check_shadow_partition(partition, changeCounter, requireShadow)
		|| !partition->Parent()) {
		return B_BAD_VALUE;
	}
	status_t error = check_busy_partition(partition->Parent(), requireShadow);
	if (error != B_OK)
		return error;
	// get the disk system
	KDiskSystem *diskSystem = partition->Parent()->DiskSystem();
	if (!diskSystem)
		return B_ENTRY_NOT_FOUND;
	// get the info
	if (diskSystem->ValidateSetType(partition, type))
		return B_OK;
	return B_ERROR;
}

// validate_set_partition_parameters
status_t
BPrivate::DiskDevice::validate_set_partition_parameters(KPartition *partition,
	int32 changeCounter, const char *parameters, bool requireShadow)
{
	if (!partition || !parameters)
		return B_BAD_VALUE;
	// check the partition
	if (!check_shadow_partition(partition, changeCounter, requireShadow)
		|| !partition->Parent()) {
		return B_BAD_VALUE;
	}
	status_t error = check_busy_partition(partition->Parent(), requireShadow);
	if (error != B_OK)
		return error;
	// get the disk system
	KDiskSystem *diskSystem = partition->Parent()->DiskSystem();
	if (!diskSystem)
		return B_ENTRY_NOT_FOUND;
	// get the info
	if (diskSystem->ValidateSetParameters(partition, parameters))
		return B_OK;
	return B_ERROR;
}

// validate_set_partition_content_parameters
status_t
BPrivate::DiskDevice::validate_set_partition_content_parameters(
	KPartition *partition, int32 changeCounter, const char *parameters,
	bool requireShadow)
{
	// check the partition
	status_t error = check_partition(partition, changeCounter, requireShadow);
	if (error != B_OK)
		return error;
	// get the disk system
	KDiskSystem *diskSystem = partition->DiskSystem();
	if (!diskSystem)
		return B_ENTRY_NOT_FOUND;
	// get the info
	if (diskSystem->ValidateSetContentParameters(partition, parameters))
		return B_OK;
	return B_ERROR;
}

// validate_initialize_partition
status_t
BPrivate::DiskDevice::validate_initialize_partition(KPartition *partition,
	int32 changeCounter, const char *diskSystemName, char *name,
	const char *parameters, bool requireShadow)
{
	if (!partition || !diskSystemName || !name)
		return B_BAD_VALUE;
	// truncate name to maximal size
	name[B_OS_NAME_LENGTH] = '\0';
	// check the partition
	status_t error = check_partition(partition, changeCounter, requireShadow);
	if (error != B_OK)
		return error;
	// get the disk system
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	KDiskSystem *diskSystem = manager->LoadDiskSystem(diskSystemName);
	if (!diskSystem)
		return B_ENTRY_NOT_FOUND;
	DiskSystemLoader loader(diskSystem, true);
	// get the info
	if (diskSystem->ValidateInitialize(partition, name, parameters))
		return B_OK;
	return B_ERROR;
}

// validate_create_child_partition
status_t
BPrivate::DiskDevice::validate_create_child_partition(KPartition *partition,
	int32 changeCounter, off_t *offset, off_t *size, const char *type,
	const char *parameters, int32 *index, bool requireShadow)
{
	if (!partition || !offset || !size || !type)
		return B_BAD_VALUE;
	// check the partition
	status_t error = check_partition(partition, changeCounter, requireShadow);
	if (error != B_OK)
		return error;
	// get the disk system
	KDiskSystem *diskSystem = partition->DiskSystem();
	if (!diskSystem)
		return B_ENTRY_NOT_FOUND;
	// get the info
	if (diskSystem->ValidateCreateChild(partition, offset, size, type,
										parameters, index)) {
		return B_OK;
	}
	return B_ERROR;
}

// validate_delete_child_partition
status_t
BPrivate::DiskDevice::validate_delete_child_partition(KPartition *partition,
	int32 changeCounter, bool requireShadow)
{
	if (!partition)
		return B_BAD_VALUE;
	// check the partition
	if (!check_shadow_partition(partition, changeCounter, requireShadow)
		|| !partition->Parent()) {
		return B_BAD_VALUE;
	}
	status_t error = check_busy_partition(partition->Parent(), requireShadow);
	if (error != B_OK)
		return error;
	// get the disk system
	KDiskSystem *diskSystem = partition->Parent()->DiskSystem();
	if (!diskSystem)
		return B_ENTRY_NOT_FOUND;
	// get the info
	if (diskSystem->SupportsDeletingChild(partition))
		return B_OK;
	return B_ERROR;
}

//} // namespace DiskDevice
//} // namespace BPrivate

