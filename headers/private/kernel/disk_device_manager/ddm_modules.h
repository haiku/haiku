// ddm_modules.h
//
// Interface to be implemented by partition modules.

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
typedef bool (*partition_supports_repairing)(partition_data *partition,
	bool checkOnly);
typedef bool (*partition_supports_resizing)(partition_data *partition);
typedef bool (*partition_supports_resizing_child)(partition_data *partition,
	partition_data *child);
typedef bool (*partition_supports_moving)(partition_data *partition,
	bool *isNoOp);
typedef bool (*partition_supports_moving_child)(partition_data *partition,
	partition_data *child);
typedef bool (*partition_supports_setting_name)(partition_data *partition);
typedef bool (*partition_supports_setting_content_name)(
	partition_data *partition);
typedef bool (*partition_supports_setting_type)(partition_data *partition);
typedef bool (*partition_supports_setting_parameters)(
	partition_data *partition);
typedef bool (*partition_supports_setting_content_parameters)(
	partition_data *partition);
typedef bool (*partition_supports_initializing)(partition_data *partition);
typedef bool (*partition_supports_initializing_child)(
	partition_data *partition, const char *system);
typedef bool (*partition_supports_creating_child)(partition_data *partition);
typedef bool (*partition_supports_deleting_child)(partition_data *partition,
	partition_data *child);
typedef bool (*partition_is_sub_system_for)(partition_data *partition);

typedef bool (*partition_validate_resize)(partition_data *partition,
	off_t *size);
typedef bool (*partition_validate_resize_child)(partition_data *partition,
	partition_data *child, off_t *size);
typedef bool (*partition_validate_move)(partition_data *partition,
	off_t *start);
typedef bool (*partition_validate_move_child)(partition_data *partition,
	partition_data *child, off_t *start);
typedef bool (*partition_validate_set_name)(partition_data *partition,
											char *name);
typedef bool (*partition_validate_set_content_name)(partition_data *partition,
													char *name);
typedef bool (*partition_validate_set_type)(partition_data *partition,
											const char *type);
typedef bool (*partition_validate_set_parameters)(partition_data *partition,
	const char *parameters);
typedef bool (*partition_validate_set_content_parameters)(
	partition_data *partition, const char *parameters);
typedef bool (*partition_validate_initialize)(partition_data *partition,
	char *name, const char *parameters);
typedef bool (*partition_validate_create_child)(partition_data *partition,
	off_t *start, off_t *size, const char *type, const char *parameters,
	int32 *index);
typedef status_t (*partition_get_partitionable_spaces)(
	partition_data *partition, partitionable_space_data *buffer, int32 count,
	int32 *actualCount);
	// When not implemented, a standard algorithm is used.

typedef status_t (*partition_get_next_supported_type)(
	partition_data *partition, int32 *cookie, char *type);
typedef status_t (*partition_get_type_for_content_type)(
	const char *contentType, char *type);

// shadow partition modification
// (device is write locked)
typedef status_t (*partition_shadow_changed)(partition_data *partition,
	uint32 operation);

// writing
// (device is NOT locked)
typedef status_t (*partition_repair)(int fd, partition_id partition,
	bool checkOnly, disk_job_id job);
typedef status_t (*partition_resize)(int fd, partition_id partition,
	off_t size, disk_job_id job);
typedef status_t (*partition_resize_child)(int fd, partition_id partition,
	off_t size, disk_job_id job);
typedef status_t (*partition_move)(int fd, partition_id partition,
	off_t offset, disk_job_id job);
typedef status_t (*partition_move_child)(int fd, partition_id partition,
	partition_id child, off_t offset, disk_job_id job);
typedef status_t (*partition_set_name)(int fd, partition_id partition,
	const char *name, disk_job_id job);
typedef status_t (*partition_set_content_name)(int fd, partition_id partition,
	const char *name, disk_job_id job);
typedef status_t (*partition_set_type)(int fd, partition_id partition,
	const char *type, disk_job_id job);
typedef status_t (*partition_set_parameters)(int fd, partition_id partition,
	const char *parameters, disk_job_id job);
typedef status_t (*partition_set_content_parameters)(int fd,
	partition_id partition, const char *parameters, disk_job_id job);
typedef status_t (*partition_initialize)(int fd, partition_id partition,
	const char *name, const char *parameters, disk_job_id job);
typedef status_t (*partition_create_child)(int fd, partition_id partition,
	off_t offset, off_t size, const char *type, const char *parameters,
	disk_job_id job, partition_id *childID);
	// childID is used for the return value, but is also an optional input
	// parameter -- -1 to be ignored
typedef status_t (*partition_delete_child)(int fd, partition_id partition,
	partition_id child, disk_job_id job);

typedef struct partition_module_info {
	module_info									module;
	const char									*pretty_name;
	uint32										flags;

	// scanning
	partition_identify_partition				identify_partition;
	partition_scan_partition					scan_partition;
	partition_free_identify_partition_cookie	free_identify_partition_cookie;
	partition_free_partition_cookie				free_partition_cookie;
	partition_free_partition_content_cookie		free_partition_content_cookie;

	// querying
	partition_supports_repairing				supports_repairing;
	partition_supports_resizing					supports_resizing;
	partition_supports_resizing_child			supports_resizing_child;
	partition_supports_moving					supports_moving;
	partition_supports_moving_child				supports_moving_child;
	partition_supports_setting_name				supports_setting_name;
	partition_supports_setting_content_name		supports_setting_content_name;
	partition_supports_setting_type				supports_setting_type;
	partition_supports_setting_parameters		supports_setting_parameters;
	partition_supports_setting_content_parameters
			supports_setting_content_parameters;
	partition_supports_initializing				supports_initializing;
	partition_supports_initializing_child		supports_initializing_child;
	partition_supports_creating_child			supports_creating_child;
	partition_supports_deleting_child			supports_deleting_child;
	partition_is_sub_system_for					is_sub_system_for;

	partition_validate_resize					validate_resize;
	partition_validate_resize_child				validate_resize_child;
	partition_validate_move						validate_move;
	partition_validate_move_child				validate_move_child;
	partition_validate_set_name					validate_set_name;
	partition_validate_set_content_name			validate_set_content_name;
	partition_validate_set_type					validate_set_type;
	partition_validate_set_parameters			validate_set_parameters;
	partition_validate_set_content_parameters
			validate_set_content_parameters;
	partition_validate_initialize				validate_initialize;
	partition_validate_create_child				validate_create_child;
	partition_get_partitionable_spaces			get_partitionable_spaces;
	partition_get_next_supported_type			get_next_supported_type;
	partition_get_type_for_content_type			get_type_for_content_type;

	// shadow partition modification
	partition_shadow_changed					shadow_changed;

	// writing
	partition_repair							repair;
	partition_resize							resize;
	partition_resize_child						resize_child;
	partition_move								move;
	partition_move_child						move_child;
	partition_set_name							set_name;
	partition_set_content_name					set_content_name;
	partition_set_type							set_type;
	partition_set_parameters					set_parameters;
	partition_set_content_parameters			set_content_parameters;
	partition_initialize						initialize;
	partition_create_child						create_child;
	partition_delete_child						delete_child;
} partition_module_info;

#endif	// _K_DISK_DEVICE_MODULES_H
