// ddm_userland_interface.h

#ifndef _DISK_DEVICE_MANAGER_USERLAND_INTERFACE_H
#define _DISK_DEVICE_MANAGER_USERLAND_INTERFACE_H

#include <DiskDeviceDefs.h>
#include <disk_device_manager.h>
#include <OS.h>

#ifdef __cplusplus
extern "C" {
#endif

// userland partition representation
struct user_partition_data {
	partition_id		id;
	partition_id		shadow_id;
	off_t				offset;
	off_t				size;
	off_t				content_size;
	uint32				block_size;
	uint32				status;
	uint32				flags;
	dev_t				volume;
	int32				index;
	int32				change_counter;	// needed?
	disk_system_id		disk_system;
	char				*name;
	char				*content_name;
	char				*type;
	char				*content_type;
	char				*parameters;
	char				*content_parameters;
	void				*user_data;
	int32				child_count;
	user_partition_data	*children[1];
};

// userland disk device representation
struct user_disk_device_data {
	uint32				device_flags;
	char				*path;
	user_partition_data	device_partition_data;
};

// userland partitionable space representation
struct user_disk_system_info {
	disk_system_id	id;
	char			name[B_FILE_NAME_LENGTH];	// better B_PATH_NAME_LENGTH?
	char			pretty_name[B_OS_NAME_LENGTH];
	uint32			flags;
};

// userland disk device job representation
struct user_disk_device_job_info {
	disk_job_id		id;
	uint32			type;
	partition_id	partition;
	char			description[256];
};

// Max size of parameter string buffers (including NULL terminator)
#define B_DISK_DEVICE_MAX_PARAMETER_SIZE (32 * 1024)

// iterating, retrieving device/partition data
partition_id _kern_get_next_disk_device_id(int32 *cookie,
										   size_t *neededSize = NULL);
partition_id _kern_find_disk_device(const char *filename,
									size_t *neededSize = NULL);
partition_id _kern_find_partition(const char *filename,
								  size_t *neededSize = NULL);
status_t _kern_get_disk_device_data(partition_id deviceID, bool deviceOnly,
									bool shadow, user_disk_device_data *buffer,
									size_t bufferSize, size_t *neededSize);
status_t _kern_get_partition_data(partition_id partitionID, bool shadow,
								  user_partition_data *buffer,
								  size_t bufferSize, size_t *neededSize);
	// Dangerous?!

partition_id _kern_register_file_device(const char *filename);
status_t _kern_unregister_file_device(partition_id deviceID,
									  const char *filename);
	// Only a valid deviceID or filename need to be passed. The other one
	// is -1/NULL. If both is given only filename is ignored.

// disk systems
status_t _kern_get_disk_system_info(disk_system_id id,
									user_disk_system_info *info);
status_t _kern_get_next_disk_system_info(int32 *cookie,
										 user_disk_system_info *info);
status_t _kern_find_disk_system(const char *name, user_disk_system_info *info);

bool _kern_supports_defragmenting_partition(partition_id partitionID,
											int32 changeCounter,
											bool *whileMounted);
bool _kern_supports_repairing_partition(partition_id partitionID,
										int32 changeCounter, bool checkOnly,
										bool *whileMounted);
bool _kern_supports_resizing_partition(partition_id partitionID,
									   int32 changeCounter,
									   bool *canResizeContents,
									   bool *whileMounted);
bool _kern_supports_moving_partition(partition_id partitionID,
									 int32 changeCounter,
									 partition_id *unmovable,
									 partition_id *needUnmounting,
									 size_t bufferSize);
bool _kern_supports_setting_partition_name(partition_id partitionID,
										   int32 changeCounter);
bool _kern_supports_setting_partition_content_name(partition_id partitionID,
												   int32 changeCounter,
												   bool *whileMounted);
bool _kern_supports_setting_partition_type(partition_id partitionID,
										   int32 changeCounter);
bool _kern_supports_setting_partition_parameters(partition_id partitionID,
												 int32 changeCounter);
bool _kern_supports_setting_partition_content_parameters(
	partition_id partitionID, int32 changeCounter, bool *whileMounted);
bool _kern_supports_initializing_partition(partition_id partitionID,
										   int32 changeCounter,
										   const char *diskSystemName);
bool _kern_supports_creating_child_partition(partition_id partitionID,
											 int32 changeCounter);
bool _kern_supports_deleting_child_partition(partition_id partitionID,
											 int32 changeCounter);
bool _kern_is_sub_disk_system_for(disk_system_id diskSystemID,
								  partition_id partitionID,
								  int32 changeCounter);

status_t _kern_validate_resize_partition(partition_id partitionID,
										 int32 changeCounter, off_t *size);
status_t _kern_validate_move_partition(partition_id partitionID,
									   int32 changeCounter, off_t *newOffset);
status_t _kern_validate_set_partition_name(partition_id partitionID,
										   int32 changeCounter, char *name);
status_t _kern_validate_set_partition_content_name(partition_id partitionID,
												   int32 changeCounter,
												   char *name);
status_t _kern_validate_set_partition_type(partition_id partitionID,
										   int32 changeCounter,
										   const char *type);
status_t _kern_validate_initialize_partition(partition_id partitionID,
											 int32 changeCounter,
											 const char *diskSystemName,
											 char *name,
											 const char *parameters,
											 size_t parametersSize);
status_t _kern_validate_create_child_partition(partition_id partitionID,
											   int32 changeCounter,
											   off_t *offset, off_t *size,
											   const char *type,
											   const char *parameters,
											   size_t parametersSize);
status_t _kern_get_partitionable_spaces(partition_id partitionID,
										int32 changeCounter,
										partitionable_space_data *buffer,
										int32 count, int32 *actualCount);
status_t _kern_get_next_supported_partition_type(partition_id partitionID,
												 int32 changeCounter,
												 int32 *cookie, char *type);
status_t _kern_get_partition_type_for_content_type(disk_system_id diskSystemID,
												   const char *contentType,
												   char *type);

// disk device modification
status_t _kern_prepare_disk_device_modifications(partition_id deviceID);
status_t _kern_commit_disk_device_modifications(partition_id deviceID,
												port_id port, int32 token,
												bool completeProgress);
status_t _kern_cancel_disk_device_modifications(partition_id deviceID);
bool _kern_is_disk_device_modified(partition_id deviceID);

status_t _kern_defragment_partition(partition_id partitionID,
									int32 changeCounter);
status_t _kern_repair_partition(partition_id partitionID, int32 changeCounter,
								bool checkOnly);
status_t _kern_resize_partition(partition_id partitionID, int32 changeCounter,
								off_t size);
status_t _kern_move_partition(partition_id partitionID, int32 changeCounter,
							  off_t newOffset);
status_t _kern_set_partition_name(partition_id partitionID,
								  int32 changeCounter, const char *name);
status_t _kern_set_partition_content_name(partition_id partitionID,
										  int32 changeCounter,
										  const char *name);
status_t _kern_set_partition_type(partition_id partitionID,
								  int32 changeCounter, const char *type);
status_t _kern_set_partition_parameters(partition_id partitionID,
										int32 changeCounter,
										const char *parameters,
										size_t parametersSize);
status_t _kern_set_partition_content_parameters(partition_id partitionID,
												int32 changeCounter,
												const char *parameters,
												size_t parametersSize);
status_t _kern_initialize_partition(partition_id partitionID,
									int32 changeCounter,
									const char *diskSystemName,
									const char *name, const char *parameters,
									size_t parametersSize);
status_t _kern_uninitialize_partition(partition_id partitionID,
									  int32 changeCounter);
status_t _kern_create_child_partition(partition_id partitionID,
									  int32 changeCounter, off_t offset,
									  off_t size, const char *type,
									  const char *parameters,
									  size_t parametersSize,
									  partition_id *childID);
status_t _kern_delete_partition(partition_id partitionID, int32 changeCounter);

// jobs
status_t _kern_get_next_disk_device_job_info(int32 *cookie,
											 user_disk_device_job_info *info);
status_t _kern_get_disk_device_job_info(disk_job_id id,
										user_disk_device_job_info *info);
status_t _kern_get_disk_device_job_progress_info(disk_job_id id,
	disk_device_job_progress_info *info);
status_t _kern_pause_disk_device_job(disk_job_id id);
status_t _kern_cancel_disk_device_job(disk_job_id id, bool reverse);

#if 0

// watching
status_t start_disk_device_watching(port_id, int32 token, uint32 flags);
status_t start_disk_device_job_watching(disk_job_id job, port_id, int32 token,
										uint32 flags);
status_t stop_disk_device_watching(port_id, int32 token);

#endif	// 0

#ifdef __cplusplus
}
#endif

#endif	// _DISK_DEVICE_MANAGER_USERLAND_INTERFACE_H
