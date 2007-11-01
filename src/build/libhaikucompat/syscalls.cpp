
#include <stdarg.h>
#include <stdio.h>

#include <syscalls.h>


// #prama mark - iterating, retrieving device/partition data
partition_id
_kern_get_next_disk_device_id(int32 *cookie, size_t *neededSize)
{
	return -1;
}


partition_id
_kern_find_disk_device(const char *filename, size_t *neededSize)
{
	return -1;
}


partition_id
_kern_find_partition(const char *filename, size_t *neededSize)
{
	return -1;
}


status_t
_kern_get_disk_device_data(partition_id deviceID, bool deviceOnly,
	bool shadow, struct user_disk_device_data *buffer, size_t bufferSize,
	size_t *neededSize)
{
	return B_ERROR;
}


partition_id
_kern_register_file_device(const char *filename)
{
	return -1;
}


status_t
_kern_unregister_file_device(partition_id deviceID, const char *filename)
{
	return B_ERROR;
}

// #pragma mark - disk systems

status_t
_kern_get_disk_system_info(disk_system_id id,
	struct user_disk_system_info *info)
{
	return B_ERROR;
}


status_t
_kern_get_next_disk_system_info(int32 *cookie,
	struct user_disk_system_info *info)
{
	return B_ERROR;
}


status_t
_kern_find_disk_system(const char *name,
	struct user_disk_system_info *info)
{
	return B_ERROR;
}


bool
_kern_supports_defragmenting_partition(partition_id partitionID,
	int32 changeCounter, bool *whileMounted)
{
	return false;
}


bool
_kern_supports_repairing_partition(partition_id partitionID,
	int32 changeCounter, bool checkOnly, bool *whileMounted)
{
	return false;
}


bool
_kern_supports_resizing_partition(partition_id partitionID,
	int32 changeCounter, bool *canResizeContents, bool *whileMounted)
{
	return false;
}


bool
_kern_supports_moving_partition(partition_id partitionID,
	int32 changeCounter, partition_id *unmovable,
	partition_id *needUnmounting, size_t bufferSize)
{
	return false;
}


bool
_kern_supports_setting_partition_name(partition_id partitionID,
	int32 changeCounter)
{
	return false;
}


bool
_kern_supports_setting_partition_content_name(partition_id partitionID,
	int32 changeCounter, bool *whileMounted)
{
	return false;
}


bool
_kern_supports_setting_partition_type(partition_id partitionID,
	int32 changeCounter)
{
	return false;
}


bool
_kern_supports_setting_partition_parameters(partition_id partitionID,
	int32 changeCounter)
{
	return false;
}


bool
_kern_supports_setting_partition_content_parameters(
	partition_id partitionID, int32 changeCounter, bool *whileMounted)
{
	return false;
}


bool
_kern_supports_initializing_partition(partition_id partitionID,
	int32 changeCounter, const char *diskSystemName)
{
	return false;
}


bool
_kern_supports_creating_child_partition(partition_id partitionID,
	int32 changeCounter)
{
	return false;
}


bool
_kern_supports_deleting_child_partition(partition_id partitionID,
	int32 changeCounter)
{
	return false;
}


bool
_kern_is_sub_disk_system_for(disk_system_id diskSystemID,
	partition_id partitionID, int32 changeCounter)

{
	return false;
}


status_t
_kern_validate_resize_partition(partition_id partitionID, int32 changeCounter,
	off_t *size)
{
	return B_ERROR;
}


status_t
_kern_validate_move_partition(partition_id partitionID, int32 changeCounter,
	off_t *newOffset)
{
	return B_ERROR;
}


status_t
_kern_validate_set_partition_name(partition_id partitionID, int32 changeCounter,
	char *name)
{
	return B_ERROR;
}


status_t
_kern_validate_set_partition_content_name(partition_id partitionID,
	int32 changeCounter, char *name)
{
	return B_ERROR;
}


status_t
_kern_validate_set_partition_type(partition_id partitionID, int32 changeCounter,
	const char *type)
{
	return B_ERROR;
}


status_t
_kern_validate_initialize_partition(partition_id partitionID,
	int32 changeCounter, const char *diskSystemName, char *name,
	const char *parameters, size_t parametersSize)
{
	return B_ERROR;
}


status_t
_kern_validate_create_child_partition(partition_id partitionID,
	int32 changeCounter, off_t *offset, off_t *size, const char *type,
	const char *parameters, size_t parametersSize)
{
	return B_ERROR;
}


status_t
_kern_get_partitionable_spaces(partition_id partitionID, int32 changeCounter,
	struct partitionable_space_data *buffer, int32 count, int32 *actualCount)
{
	return B_ERROR;
}


status_t
_kern_get_next_supported_partition_type(partition_id partitionID,
	int32 changeCounter, int32 *cookie, char *type)
{
	return B_ERROR;
}


status_t
_kern_get_partition_type_for_content_type(disk_system_id diskSystemID,
	const char *contentType, char *type)
{
	return B_ERROR;
}

// #pragma mark - disk device modification

status_t
_kern_prepare_disk_device_modifications(partition_id deviceID)
{
	return B_ERROR;
}


status_t
_kern_commit_disk_device_modifications(partition_id deviceID,
	port_id port, int32 token, bool completeProgress)
{
	return B_ERROR;
}


status_t
_kern_cancel_disk_device_modifications(partition_id deviceID)
{
	return B_ERROR;
}


bool
_kern_is_disk_device_modified(partition_id deviceID)
{
	return false;
}


status_t
_kern_defragment_partition(partition_id partitionID, int32 changeCounter)
{
	return B_ERROR;
}


status_t
_kern_repair_partition(partition_id partitionID, int32 changeCounter,
	bool checkOnly)
{
	return B_ERROR;
}


status_t
_kern_resize_partition(partition_id partitionID, int32 changeCounter,
	off_t size)
{
	return B_ERROR;
}


status_t
_kern_move_partition(partition_id partitionID, int32 changeCounter,
	off_t newOffset)
{
	return B_ERROR;
}


status_t
_kern_set_partition_name(partition_id partitionID, int32 changeCounter,
	const char *name)
{
	return B_ERROR;
}


status_t
_kern_set_partition_content_name(partition_id partitionID,
	int32 changeCounter, const char *name)
{
	return B_ERROR;
}


status_t
_kern_set_partition_type(partition_id partitionID, int32 changeCounter,
	const char *type)
{
	return B_ERROR;
}


status_t
_kern_set_partition_parameters(partition_id partitionID, int32 changeCounter,
	const char *parameters, size_t parametersSize)
{
	return B_ERROR;
}


status_t
_kern_set_partition_content_parameters(partition_id partitionID,
	int32 changeCounter, const char *parameters, size_t parametersSize)
{
	return B_ERROR;
}


status_t
_kern_initialize_partition(partition_id partitionID, int32 changeCounter,
	const char *diskSystemName, const char *name, const char *parameters,
	size_t parametersSize)
{
	return B_ERROR;
}


status_t
_kern_uninitialize_partition(partition_id partitionID, int32 changeCounter)
{
	return B_ERROR;
}


status_t
_kern_create_child_partition(partition_id partitionID, int32 changeCounter,
	off_t offset, off_t size, const char *type, const char *parameters,
	size_t parametersSize, partition_id *childID)
{
	return B_ERROR;
}


status_t
_kern_delete_partition(partition_id partitionID, int32 changeCounter)
{
	return B_ERROR;
}


status_t
_kern_delete_child_partition(partition_id partitionID, int32* changeCounter,
	partition_id childID, int32 childChangeCounter)
{
	return B_ERROR;
}


// #pragma mark - jobs


status_t
_kern_get_next_disk_device_job_info(int32 *cookie,
	struct user_disk_device_job_info *info)
{
	return B_ERROR;
}


status_t
_kern_get_disk_device_job_info(disk_job_id id,
	struct user_disk_device_job_info *info)
{
	return B_ERROR;
}


status_t
_kern_get_disk_device_job_progress_info(disk_job_id id,
	struct disk_device_job_progress_info *info)
{
	return B_ERROR;
}


status_t
_kern_pause_disk_device_job(disk_job_id id)
{
	return B_ERROR;
}


status_t
_kern_cancel_disk_device_job(disk_job_id id, bool reverse)
{
	return B_ERROR;
}

// #pragma mark - other syscalls

status_t
_kern_get_safemode_option(const char *parameter, char *buffer,
	size_t *_bufferSize)
{
	return B_ERROR;
}
