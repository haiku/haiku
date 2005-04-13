// KPartitioningSystem.cpp

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include <ddm_modules.h>
#include <KDiskDevice.h>
#include <KDiskDeviceJob.h>
#include <KDiskDeviceManager.h>
#include <KDiskDeviceUtils.h>
#include <KPartition.h>
#include <KPartitioningSystem.h>

// constructor
KPartitioningSystem::KPartitioningSystem(const char *name)
	: KDiskSystem(name),
	  fModule(NULL)
{
}

// destructor
KPartitioningSystem::~KPartitioningSystem()
{
}

// Init
status_t
KPartitioningSystem::Init()
{
	status_t error = KDiskSystem::Init();
	if (error != B_OK)
		return error;
	error = Load();
	if (error != B_OK)
		return error;
	error = SetPrettyName(fModule->pretty_name);
	SetFlags(fModule->flags & ~(uint32)B_DISK_SYSTEM_IS_FILE_SYSTEM);
	Unload();
	return error;
}

// Identify
float
KPartitioningSystem::Identify(KPartition *partition, void **cookie)
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
KPartitioningSystem::Scan(KPartition *partition, void *cookie)
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
KPartitioningSystem::FreeIdentifyCookie(KPartition *partition, void *cookie)
{
	if (!partition || !fModule || !fModule->free_identify_partition_cookie)
		return;
	fModule->free_identify_partition_cookie(partition->PartitionData(),
											cookie);
}

// FreeCookie
void
KPartitioningSystem::FreeCookie(KPartition *partition)
{
	if (!partition || !fModule || !fModule->free_partition_cookie
		|| partition->ParentDiskSystem() != this) {
		return;
	}
	fModule->free_partition_cookie(partition->PartitionData());
	partition->SetCookie(NULL);
}

// FreeContentCookie
void
KPartitioningSystem::FreeContentCookie(KPartition *partition)
{
	if (!partition || !fModule || !fModule->free_partition_content_cookie
		|| partition->DiskSystem() != this) {
		return;
	}
	fModule->free_partition_content_cookie(partition->PartitionData());
	partition->SetContentCookie(NULL);
}

// SupportsRepairing
bool
KPartitioningSystem::SupportsRepairing(KPartition *partition, bool checkOnly,
									   bool *whileMounted)
{
	bool _whileMounted = false;
	if (!whileMounted)
		whileMounted = &_whileMounted;
	if (!partition || partition->DiskSystem() != this || !fModule
		|| !fModule->supports_repairing) {
		return (*whileMounted = false);
	}
	bool result = fModule->supports_repairing(partition->PartitionData(),
											  checkOnly);
	*whileMounted = result;
	return result;
}

// SupportsResizing
bool
KPartitioningSystem::SupportsResizing(KPartition *partition,
									  bool *whileMounted)
{
	bool _whileMounted = false;
	if (!whileMounted)
		whileMounted = &_whileMounted;
	if (!partition || partition->DiskSystem() != this || !fModule ||
		!fModule->supports_resizing) {
		return (*whileMounted = false);
	}
	bool result = fModule->supports_resizing(partition->PartitionData());
	*whileMounted = result;
	return result;
}

// SupportsResizingChild
bool
KPartitioningSystem::SupportsResizingChild(KPartition *child)
{
	return (child && child->Parent() && child->ParentDiskSystem() == this
			&& fModule && fModule->supports_resizing_child
			&& fModule->supports_resizing_child(
						child->Parent()->PartitionData(),
						child->PartitionData()));
}

// SupportsMoving
bool
KPartitioningSystem::SupportsMoving(KPartition *partition, bool *isNoOp)
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

// SupportsMovingChild
bool
KPartitioningSystem::SupportsMovingChild(KPartition *child)
{
	return (child && child->Parent() && child->ParentDiskSystem() != this
			&& fModule && fModule->supports_moving_child
			&& fModule->supports_moving_child(child->Parent()->PartitionData(),
											  child->PartitionData()));
}

// SupportsSettingName
bool
KPartitioningSystem::SupportsSettingName(KPartition *partition)
{
	return (partition && partition->ParentDiskSystem() == this
			&& fModule && fModule->supports_setting_name
			&& fModule->supports_setting_name(partition->PartitionData()));
}

// SupportsSettingContentName
bool
KPartitioningSystem::SupportsSettingContentName(KPartition *partition,
												bool *whileMounted)
{
	bool _whileMounted = false;
	if (!whileMounted)
		whileMounted = &_whileMounted;
	if (!partition || partition->DiskSystem() != this || !fModule
		|| !fModule->supports_setting_content_name) {
		return (*whileMounted = false);
	}
	bool result = fModule->supports_setting_content_name(
		partition->PartitionData());
	*whileMounted = result;
	return result;
}

// SupportsSettingType
bool
KPartitioningSystem::SupportsSettingType(KPartition *partition)
{
	return (partition && partition->ParentDiskSystem() == this
			&& fModule && fModule->supports_setting_type
			&& fModule->supports_setting_type(partition->PartitionData()));
}

// SupportsSettingParameters
bool
KPartitioningSystem::SupportsSettingParameters(KPartition *partition)
{
	return (partition && partition->ParentDiskSystem() == this
			&& fModule && fModule->supports_setting_parameters
			&& fModule->supports_setting_parameters(
					partition->PartitionData()));
}

// SupportsSettingContentParameters
bool
KPartitioningSystem::SupportsSettingContentParameters(KPartition *partition,
													  bool *whileMounted)
{
	bool _whileMounted = false;
	if (!whileMounted)
		whileMounted = &_whileMounted;
	if (!partition || partition->DiskSystem() != this || !fModule
		|| !fModule->supports_setting_content_parameters) {
		return (*whileMounted = false);
	}
	bool result = fModule->supports_setting_content_parameters(
		partition->PartitionData());
	*whileMounted = result;
	return result;
}

// SupportsInitializing
bool
KPartitioningSystem::SupportsInitializing(KPartition *partition)
{
	return (partition && fModule && fModule->supports_initializing
			&& fModule->supports_initializing(partition->PartitionData()));
}

// SupportsInitializingChild
bool
KPartitioningSystem::SupportsInitializingChild(KPartition *child,
											   const char *diskSystem)
{
	return (child && child->ParentDiskSystem() == this && diskSystem
			&& fModule && fModule->supports_initializing_child
			&& fModule->supports_initializing_child(child->PartitionData(),
													diskSystem));
}

// SupportsCreatingChild
bool
KPartitioningSystem::SupportsCreatingChild(KPartition *partition)
{
	return (partition && partition->DiskSystem() == this
			&& fModule && fModule->supports_creating_child
			&& fModule->supports_creating_child(partition->PartitionData()));
}

// SupportsDeletingChild
bool
KPartitioningSystem::SupportsDeletingChild(KPartition *child)
{
	return (child && child->Parent() && child->ParentDiskSystem() == this
			&& fModule && fModule->supports_deleting_child
			&& fModule->supports_deleting_child(
					child->Parent()->PartitionData(), child->PartitionData()));
}

// IsSubSystemFor
bool
KPartitioningSystem::IsSubSystemFor(KPartition *partition)
{
	return (partition && fModule && fModule->is_sub_system_for
			&& fModule->is_sub_system_for(partition->PartitionData()));
}

// ValidateResize
bool
KPartitioningSystem::ValidateResize(KPartition *partition, off_t *size)
{
	return (partition && size && partition->DiskSystem() == this && fModule
			&& fModule->validate_resize
			&& fModule->validate_resize(partition->PartitionData(), size));
}

// ValidateResizeChild
bool
KPartitioningSystem::ValidateResizeChild(KPartition *child, off_t *size)
{
	return (child && size && child->Parent()
			&& child->ParentDiskSystem() == this && fModule
			&& fModule->validate_resize_child
			&& fModule->validate_resize_child(child->Parent()->PartitionData(),
											  child->PartitionData(), size));
}

// ValidateMove
bool
KPartitioningSystem::ValidateMove(KPartition *partition, off_t *start)
{
	return (partition && start && partition->DiskSystem() == this && fModule
			&& fModule->validate_move
			&& fModule->validate_move(partition->PartitionData(), start));
}

// ValidateMoveChild
bool
KPartitioningSystem::ValidateMoveChild(KPartition *child, off_t *start)
{
	return (child && start && child->Parent()
			&& child->ParentDiskSystem() == this && fModule
			&& fModule->validate_move_child
			&& fModule->validate_move_child(child->Parent()->PartitionData(),
											child->PartitionData(), start));
}

// ValidateSetName
bool
KPartitioningSystem::ValidateSetName(KPartition *partition, char *name)
{
	return (partition && name && partition->Parent()
			&& partition->ParentDiskSystem() == this && fModule
			&& fModule->validate_set_name
			&& fModule->validate_set_name(partition->PartitionData(), name));
}

// ValidateSetContentName
bool
KPartitioningSystem::ValidateSetContentName(KPartition *partition, char *name)
{
	return (partition && name && partition->DiskSystem() == this
			&& fModule && fModule->validate_set_content_name
			&& fModule->validate_set_content_name(partition->PartitionData(),
												  name));
}

// ValidateSetType
bool
KPartitioningSystem::ValidateSetType(KPartition *partition, const char *type)
{
	return (partition && type && partition->ParentDiskSystem() == this
			&& fModule && fModule->validate_set_type
			&& fModule->validate_set_type(partition->PartitionData(), type));
}

// ValidateSetParameters
bool
KPartitioningSystem::ValidateSetParameters(KPartition *partition,
										   const char *parameters)
{
	return (partition && partition->ParentDiskSystem() == this 
			&& fModule && fModule->validate_set_parameters
			&& fModule->validate_set_parameters(partition->PartitionData(),
					parameters));
}

// ValidateSetContentParameters
bool
KPartitioningSystem::ValidateSetContentParameters(KPartition *partition,
												  const char *parameters)
{
	return (partition && partition->DiskSystem() == this && fModule
			&& fModule->validate_set_content_parameters
			&& fModule->validate_set_content_parameters(
					partition->PartitionData(), parameters));
}

// ValidateInitialize
bool
KPartitioningSystem::ValidateInitialize(KPartition *partition, char *name,
										const char *parameters)
{
	return (partition && name && fModule && fModule->validate_initialize
			&& fModule->validate_initialize(partition->PartitionData(), name,
											parameters));
}

// ValidateCreateChild
bool
KPartitioningSystem::ValidateCreateChild(KPartition *partition, off_t *start,
										 off_t *size, const char *type,
										 const char *parameters, int32 *index)
{
	int32 _index = 0;
	if (!index)
		index = &_index;
	return (partition && start && size && type
			&& partition->DiskSystem() == this && fModule
			&& fModule->validate_create_child
			&& fModule->validate_create_child(partition->PartitionData(),
											  start, size, type, parameters,
											  index));
}

// CountPartitionableSpaces
int32
KPartitioningSystem::CountPartitionableSpaces(KPartition *partition)
{
	if (!partition || partition->DiskSystem() != this || !fModule)
		return 0;
	if (!fModule->get_partitionable_spaces) {
		// TODO: Fallback algorithm.
		return 0;
	}
	int32 count = 0;
	status_t error = fModule->get_partitionable_spaces(
		partition->PartitionData(), NULL, 0, &count);
	return (error == B_OK || error == B_BUFFER_OVERFLOW ? count : 0);
}

// GetPartitionableSpaces
status_t
KPartitioningSystem::GetPartitionableSpaces(KPartition *partition,
											partitionable_space_data *buffer,
											int32 count, int32 *actualCount)
{
	if (!partition || partition->DiskSystem() != this || count > 0 && !buffer
		|| !actualCount || !fModule) {
		return B_BAD_VALUE;
	}
	if (!fModule->get_partitionable_spaces) {
		// TODO: Fallback algorithm.
		return B_ENTRY_NOT_FOUND;
	}
	return fModule->get_partitionable_spaces(partition->PartitionData(),
											 buffer, count, actualCount);
}

// GetNextSupportedType
status_t
KPartitioningSystem::GetNextSupportedType(KPartition *partition, int32 *cookie,
										  char *type)
{
	if (!partition || partition->DiskSystem() != this || !cookie || !type
		|| !fModule) {
		return B_BAD_VALUE;
	}
	if (!fModule->get_next_supported_type)
		return B_ENTRY_NOT_FOUND;
	return fModule->get_next_supported_type(partition->PartitionData(), cookie,
											type);
}

// GetTypeForContentType
status_t
KPartitioningSystem::GetTypeForContentType(const char *contentType, char *type)
{
	if (!contentType || !type || !fModule)
		return B_BAD_VALUE;
	if (!fModule->get_type_for_content_type)
		return B_ENTRY_NOT_FOUND;
	return fModule->get_type_for_content_type(contentType, type);
}

// ShadowPartitionChanged
status_t
KPartitioningSystem::ShadowPartitionChanged(KPartition *partition,
											uint32 operation)
{
	if (!partition)
		return B_BAD_VALUE;
	if (!fModule)
		return B_ERROR;
	// If not implemented, we assume, that the partitioning system doesn't
	// have to make any additional changes.
	if (!fModule->shadow_changed)
		return B_OK;
	return fModule->shadow_changed(partition->PartitionData(), operation);
}

// Repair
status_t
KPartitioningSystem::Repair(KPartition *partition, bool checkOnly,
							KDiskDeviceJob *job)
{
	// to be implemented
	return B_ERROR;
}

// Resize
status_t
KPartitioningSystem::Resize(KPartition *partition, off_t size,
							KDiskDeviceJob *job)
{
	// check parameters
	if (!partition || !job || size < 0)
		return B_BAD_VALUE;
	if (!fModule->resize)
		return B_ENTRY_NOT_FOUND;
	// lock partition and open partition device
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	KPartition *_partition = manager->ReadLockPartition(partition->ID());
	if (!_partition)
		return B_ERROR;
	int fd = -1;
	{
		PartitionRegistrar registrar(_partition, true);
		PartitionRegistrar deviceRegistrar(_partition->Device(), true);
		DeviceReadLocker locker(_partition->Device(), true);
		if (partition != _partition)
			return B_ERROR;
		status_t result = partition->Open(O_RDONLY, &fd);
		if (result != B_OK)
			return result;
	}
	// let the module do its job
	status_t result = fModule->resize(fd, partition->ID(), size, job->ID());
	// cleanup and return
	close(fd);
	return result;
}

// ResizeChild
status_t
KPartitioningSystem::ResizeChild(KPartition *child, off_t size,
								 KDiskDeviceJob *job)
{
	// check parameters
	if (!child || !job || size < 0)
		return B_BAD_VALUE;
	if (!fModule->resize_child)
		return B_ENTRY_NOT_FOUND;
	// lock partition and open (parent) partition device
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	KPartition *_partition = manager->ReadLockPartition(child->ID());
	if (!_partition)
		return B_ERROR;
	int fd = -1;
	{
		PartitionRegistrar registrar(_partition, true);
		PartitionRegistrar deviceRegistrar(_partition->Device(), true);
		DeviceReadLocker locker(_partition->Device(), true);
		if (child != _partition)
			return B_ERROR;
		if (!child->Parent())
			return B_BAD_VALUE;
		status_t result = child->Parent()->Open(O_RDONLY, &fd);
		if (result != B_OK)
			return result;
	}
	// let the module do its job
	status_t result = fModule->resize_child(fd, child->ID(), size, job->ID());
	// cleanup and return
	close(fd);
	return result;
}

// Move
status_t
KPartitioningSystem::Move(KPartition *partition, off_t offset,
						  KDiskDeviceJob *job)
{
	// to be implemented
	return B_ERROR;
}

// MoveChild
status_t
KPartitioningSystem::MoveChild(KPartition *child, off_t offset,
							   KDiskDeviceJob *job)
{
	// to be implemented
	return B_ERROR;
}

// SetName
status_t
KPartitioningSystem::SetName(KPartition *partition, char *name,
							 KDiskDeviceJob *job)
{
	// to be implemented
	return B_ERROR;
}

// SetContentName
status_t
KPartitioningSystem::SetContentName(KPartition *partition, char *name,
									KDiskDeviceJob *job)
{
	// to be implemented
	return B_ERROR;
}

// SetType
status_t
KPartitioningSystem::SetType(KPartition *partition, char *type,
							 KDiskDeviceJob *job)
{
	// to be implemented
	return B_ERROR;
}

// SetParameters
status_t
KPartitioningSystem::SetParameters(KPartition *partition,
								   const char *parameters, KDiskDeviceJob *job)
{
	// to be implemented
	return B_ERROR;
}

// SetContentParameters
status_t
KPartitioningSystem::SetContentParameters(KPartition *partition,
										  const char *parameters,
										  KDiskDeviceJob *job)
{
	// to be implemented
	return B_ERROR;
}

// Initialize
status_t
KPartitioningSystem::Initialize(KPartition *partition, const char *name,
								const char *parameters, KDiskDeviceJob *job)
{
	// to be implemented
	return B_ERROR;
}

// CreateChild
status_t
KPartitioningSystem::CreateChild(KPartition *partition, off_t offset,
								 off_t size, const char *type,
								 const char *parameters, KDiskDeviceJob *job,
								 KPartition **child, partition_id childID)
{
	// to be implemented
	return B_ERROR;
}

// DeleteChild
status_t
KPartitioningSystem::DeleteChild(KPartition *child, KDiskDeviceJob *job)
{
	// to be implemented
	return B_ERROR;
}

// LoadModule
status_t
KPartitioningSystem::LoadModule()
{
	if (fModule)		// shouldn't happen
		return B_OK;
	return get_module(Name(), (module_info**)&fModule);
}

// UnloadModule
void
KPartitioningSystem::UnloadModule()
{
	if (fModule) {
		put_module(fModule->module.name);
		fModule = NULL;
	}
}

