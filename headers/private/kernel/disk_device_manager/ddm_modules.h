// ddm_modules.h
//
// Interfaces to be implemented by partition/FS modules.

#ifndef _K_DISK_DEVICE_MODULES_H
#define _K_DISK_DEVICE_MODULES_H

#include <disk_device_manager.h>
#include <module.h>
#include <SupportDefs.h>

// partition module interface

// scanning
// (the device is write locked)
typedef float (*partition_identify_partition)(int fd,
	partition_data *partition, void **cookie);
typedef status_t (*partition_scan_partition)(int fd,
	partition_data *partition, void *identifyCookie);
typedef void (*partition_free_identify_partition_cookie)(
	partition_data *partition, void *cookie);
typedef void (*partition_free_partition_cookie)(partition_data *partition);
typedef void (*partition_free_partition_content_cookie)(
	partition_data *partition);

// querying
// (the device is read locked)
typedef bool (*partition_supports_reparing_partition)(
	partition_data *partition, bool checkOnly, bool *whileMounted);
typedef bool (*partition_supports_resizing_partition)(
	partition_data *partition, bool *whileMounted);
typedef bool (*partition_supports_resizing_child_partition)(
	partition_data *partition, partition_data *child);
typedef bool (*partition_supports_moving_partition)(partition_data *partition,
	bool *whileMounted);
typedef bool (*partition_supports_moving_child_partition)(
	partition_data *partition, partition_data *child);
typedef bool (*partition_supports_setting_name)(partition_data *partition);
typedef bool (*partition_supports_setting_content_name)(
	partition_data *partition);
typedef bool (*partition_supports_setting_type)(partition_data *partition);
typedef bool (*partition_supports_creating_child_partition)(
	partition_data *partition);
typedef bool (*partition_supports_deleting_child_partition)(
	partition_data *partition, partition_data *child);
typedef bool (*partition_supports_initializing_partition)(
	partition_data *partition);
typedef bool (*partition_supports_initializing_child_partition)(
	partition_data *partition, const char *system);

typedef bool (*partition_validate_resize_partition)(partition_data *partition,
	off_t *size);
typedef bool (*partition_validate_move_partition)(partition_data *partition,
	off_t *start);
typedef bool (*partition_validate_resize_child_partition)(
	partition_data *partition, partition_data *child, off_t *size);
typedef bool (*partition_validate_move_child_partition)(
	partition_data *partition, partition_data *child, off_t *start);
typedef bool (*partition_validate_set_name)(partition_data *partition,
											char *name);
typedef bool (*partition_validate_set_content_name)(partition_data *partition,
													char *name);
typedef bool (*partition_validate_set_type)(partition_data *partition,
											const char *type);
typedef bool (*partition_validate_create_child_partition)(
	partition_data *partition, partition_data *child, off_t *start,
	off_t *size, const char *type, const char *parameters);
typedef bool (*partition_validate_initialize_partition)(
	partition_data *partition, const char *parameters);
typedef bool (*partition_validate_set_partition_parameters)(
	partition_data *partition, const char *parameters);
typedef bool (*partition_validate_set_partition_content_parameters)(
	partition_data *partition, const char *parameters);
typedef bool (*partition_get_partitionable_spaces)(partition_data *partition,
	partitionable_space_data *spaces, int32 count, int32 *actualCount);
	// When not implemented, a standard algorithm is used.

typedef bool (*partition_get_next_supported_type)(partition_data *partition,
	int32 *cookie, char *type);
typedef bool (*partition_get_type_for_content_type)(partition_data *partition,
	const char *contentType, char *type);

// writing
// (device is NOT locked)
typedef status_t (*partition_repair_partition)(int fd,
	partition_id partition, bool checkOnly, disk_job_id job);
typedef status_t (*partition_resize_partition)(int fd,
	partition_id partition, off_t size, disk_job_id job);
typedef status_t (*partition_resize_child_partition)(int fd,
	partition_id partition, off_t size, disk_job_id job);
typedef status_t (*partition_move_partition)(int fd,
	partition_id partition, off_t offset, disk_job_id job);
typedef status_t (*partition_move_child_partition)(int fd,
	partition_id partition, partition_id child, off_t offset, disk_job_id job);
typedef status_t (*partition_set_name)(int fd, partition_id partition,
	const char *name, disk_job_id job);
typedef status_t (*partition_set_content_name)(int fd, partition_id partition,
	const char *name, disk_job_id job);
typedef status_t (*partition_set_type)(int fd, partition_id partition,
	const char *type, disk_job_id job);
typedef status_t (*partition_create_child_partition)(int fd,
	partition_id partition, off_t offset, off_t size, const char *type,
	const char *parameters, disk_job_id job, partition_id *childID);
	// childID is used for the return value, but is also an optional input
	// parameter -- -1 to be ignored
typedef status_t (*partition_delete_child_partition)(int fd,
	partition_id partition, partition_id child, disk_job_id job);
typedef status_t (*partition_initialize_partition)(int fd,
	partition_id partition, const char *parameters, disk_job_id job);
typedef status_t (*partition_set_parameters_partition)(int fd,
	partition_id partition, const char *parameters, disk_job_id job);
typedef status_t (*partition_set_partition_content_parameters)(int fd,
	partition_id partition, const char *parameters, disk_job_id job);

typedef struct partition_module_info {
	module_info									module;
	const char									*pretty_name;

	// scanning
	partition_identify_partition				identify_partition;
	partition_scan_partition					scan_partition;
	partition_free_identify_partition_cookie	free_identify_partition_cookie;
	partition_free_partition_cookie				free_partition_cookie;
	partition_free_partition_content_cookie		free_partition_content_cookie;

	// querying
	partition_supports_reparing_partition		supports_reparing_partition;
	partition_supports_resizing_partition		supports_resizing_partition;
	partition_supports_resizing_child_partition
			supports_resizing_child_partition;
	partition_supports_moving_partition			supports_moving_partition;
	partition_supports_moving_child_partition
			supports_moving_child_partition;
	partition_supports_setting_name				supports_setting_name;
	partition_supports_setting_content_name		supports_setting_content_name;
	partition_supports_setting_type				supports_setting_type;
	partition_supports_creating_child_partition
			supports_creating_child_partition;
	partition_supports_deleting_child_partition
			supports_deleting_child_partition;
	partition_supports_initializing_partition
			supports_initializing_partition;
	partition_supports_initializing_child_partition
			supports_initializing_child_partition;

	partition_validate_resize_partition			validate_resize_partition;
	partition_validate_resize_child_partition
			validate_resize_child_partition;
	partition_validate_move_partition			validate_move_partition;
	partition_validate_move_child_partition		validate_move_child_partition;
	partition_validate_set_name					validate_set_name;
	partition_validate_set_content_name			validate_set_content_name;
	partition_validate_set_type					validate_set_type;
	partition_validate_create_child_partition
			validate_create_child_partition;
	partition_validate_initialize_partition		validate_initialize_partition;
	partition_validate_set_partition_parameters
			validate_set_partition_parameters;
	partition_validate_set_partition_content_parameters
			validate_set_partition_content_parameters;
	partition_get_partitionable_spaces			get_partitionable_spaces;
	partition_get_next_supported_type			get_next_supported_type;
	partition_get_type_for_content_type			get_type_for_content_type;

	// writing
	partition_repair_partition					repair_partition;
	partition_resize_partition					resize_partition;
	partition_resize_child_partition			resize_child_partition;
	partition_move_partition					move_partition;
	partition_move_child_partition				move_child_partition;
	partition_set_name							set_name;
	partition_set_content_name					set_content_name;
	partition_set_type							set_type;
	partition_create_child_partition			create_child_partition;
	partition_delete_child_partition			delete_child_partition;
	partition_initialize_partition				initialize_partition;
	partition_set_parameters_partition			set_parameters_partition;
	partition_set_partition_content_parameters
		set_partition_content_parameters;
} partition_module_info;


// FS module interface

// scanning
// (the device is write locked)
typedef float (*fs_identify_partition)(int fd, partition_data *partition,
	void **cookie);
typedef status_t (*fs_scan_partition)(int fd, partition_data *partition,
	void *cookie);
typedef void (*fs_free_identify_partition_cookie)(partition_data *partition,
	void *cookie);
typedef void (*fs_free_partition_content_cookie)(partition_data *partition);

// querying
// (the device is read locked)
typedef bool (*fs_supports_defragmenting_partition)(partition_data *partition,
	bool *whileMounted);
typedef bool (*fs_supports_reparing_partition)(partition_data *partition,
	bool checkOnly, bool *whileMounted);
typedef bool (*fs_supports_resizing_partition)(partition_data *partition,
	bool *whileMounted);
typedef bool (*fs_supports_moving_partition)(partition_data *partition,
	bool *whileMounted);
typedef bool (*fs_supports_setting_content_name)(partition_data *partition,
	bool *whileMounted);
typedef bool (*fs_supports_initializing_partition)(partition_data *partition);

typedef bool (*fs_validate_resize_partition)(partition_data *partition,
	off_t *size);
typedef bool (*fs_validate_move_partition)(partition_data *partition,
	off_t *start);
typedef bool (*fs_validate_set_content_name)(partition_data *partition,
	char *name);
typedef bool (*fs_validate_initialize_partition)(partition_data *partition,
	const char *parameters);
typedef bool (*fs_validate_set_partition_content_parameters)(
	partition_data *partition, const char *parameters);

// writing
// (the device is NOT locked)
typedef status_t (*fs_defragment_partition)(int fd, partition_id partition,
	disk_job_id job);
typedef status_t (*fs_repair_partition)(int fd, partition_id partition,
	bool checkOnly, disk_job_id job);
typedef status_t (*fs_resize_partition)(int fd, partition_id partition,
	off_t size, disk_job_id job);
typedef status_t (*fs_move_partition)(int fd, partition_id partition,
	off_t offset, disk_job_id job);
typedef status_t (*fs_set_content_name)(int fd, partition_id partition,
	const char *name, disk_job_id job);
typedef status_t (*fs_initialize_partition)(const char *partition,
	const char *parameters, disk_job_id job);
	// This is pretty close to how the hook in R5 looked. Save the job ID, of
	// course and that the parameters were given as (void*, size_t) pair.
typedef status_t (*fs_set_partition_content_parameters)(int fd,
	partition_id partition, const char *parameters, disk_job_id job);

typedef struct fs_module_info {
	module_info									module;
	const char									*pretty_name;

	// scanning
	fs_identify_partition						identify_partition;
	fs_scan_partition							scan_partition;
	fs_free_identify_partition_cookie			free_identify_partition_cookie;
	fs_free_partition_content_cookie			free_partition_content_cookie;

	// querying
	fs_supports_defragmenting_partition
			supports_defragmenting_partition;
	fs_supports_reparing_partition				supports_reparing_partition;
	fs_supports_resizing_partition				supports_resizing_partition;
	fs_supports_moving_partition				supports_moving_partition;
	fs_supports_setting_content_name			supports_setting_content_name;
	fs_supports_initializing_partition
			supports_initializing_partition;

	fs_validate_resize_partition				validate_resize_partition;
	fs_validate_move_partition					validate_move_partition;
	fs_validate_set_content_name				validate_set_content_name;
	fs_validate_initialize_partition			validate_initialize_partition;
	fs_validate_set_partition_content_parameters
			validate_set_partition_content_parameters;

	// writing
	fs_defragment_partition						defragment_partition;
	fs_repair_partition							repair_partition;
	fs_resize_partition							resize_partition;
	fs_move_partition							move_partition;
	fs_set_content_name							set_content_name;
	fs_initialize_partition						initialize_partition;
	fs_set_partition_content_parameters
			set_partition_content_parameters;
} fs_module_info;

#endif	// _K_DISK_DEVICE_MODULES_H
