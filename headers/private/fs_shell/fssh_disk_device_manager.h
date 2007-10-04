/*
 * Copyright 2003-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FSSH_DISK_DEVICE_MANAGER_H
#define _FSSH_DISK_DEVICE_MANAGER_H


#include "fssh_disk_device_defs.h"
#include "fssh_drivers.h"


#ifdef __cplusplus
extern "C" {
#endif

// C API partition representation
// Fields marked [sys] are set by the system and are not to be changed by
// the disk system modules.
typedef struct fssh_partition_data {
	fssh_partition_id	id;				// [sys]
	fssh_off_t			offset;
	fssh_off_t			size;
	fssh_off_t			content_size;
	uint32_t			block_size;
	int32_t				child_count;
	int32_t				index;			// [sys]
	uint32_t			status;
	uint32_t			flags;
	fssh_dev_t			volume;			// [sys]
	void				*mount_cookie;	// [sys] 
	char				*name;			// max: B_OS_NAME_LENGTH
	char				*content_name;	//
	char				*type;			//
	const char			*content_type;	// [sys]
	char				*parameters;
	char				*content_parameters;
	void				*cookie;
	void				*content_cookie;
} fssh_partition_data;

// C API disk device representation
typedef struct fssh_disk_device_data {
	fssh_partition_id		id;				// equal to that of the root partition
	uint32_t				flags;
	char					*path;
	fssh_device_geometry	geometry;
} fssh_disk_device_data;

// C API partitionable space representation
typedef struct fssh_partitionable_space_data {
	fssh_off_t	offset;
	fssh_off_t	size;
} fssh_partitionable_space_data;

// operations on partitions
enum {
	FSSH_B_PARTITION_DEFRAGMENT,
	FSSH_B_PARTITION_REPAIR,
	FSSH_B_PARTITION_RESIZE,
	FSSH_B_PARTITION_RESIZE_CHILD,
	FSSH_B_PARTITION_MOVE,
	FSSH_B_PARTITION_MOVE_CHILD,
	FSSH_B_PARTITION_SET_NAME,
	FSSH_B_PARTITION_SET_CONTENT_NAME,
	FSSH_B_PARTITION_SET_TYPE,
	FSSH_B_PARTITION_SET_PARAMETERS,
	FSSH_B_PARTITION_SET_CONTENT_PARAMETERS,
	FSSH_B_PARTITION_INITIALIZE,
	FSSH_B_PARTITION_CREATE_CHILD,
	FSSH_B_PARTITION_DELETE_CHILD,
};

// disk device job cancel status
enum {
	FSSH_B_DISK_DEVICE_JOB_CONTINUE,
	FSSH_B_DISK_DEVICE_JOB_CANCEL,
	FSSH_B_DISK_DEVICE_JOB_REVERSE,
};

// disk device locking
fssh_disk_device_data*	fssh_write_lock_disk_device(
								fssh_partition_id partitionID);
void					fssh_write_unlock_disk_device(
								fssh_partition_id partitionID);
fssh_disk_device_data*	fssh_read_lock_disk_device(
								fssh_partition_id partitionID);
void					fssh_read_unlock_disk_device(
								fssh_partition_id partitionID);
	// parameter is the ID of any partition on the device

// getting disk devices/partitions by path
// (no locking required)
int32_t	fssh_find_disk_device(const char *path);
int32_t	fssh_find_partition(const char *path);

// disk device/partition read access
// (read lock required)
fssh_disk_device_data*	fssh_get_disk_device(fssh_partition_id partitionID);
fssh_partition_data*	fssh_get_partition(fssh_partition_id partitionID);
fssh_partition_data*	fssh_get_parent_partition(
								fssh_partition_id partitionID);
fssh_partition_data*	fssh_get_child_partition(fssh_partition_id partitionID,
								int32_t index);

// partition write access
// (write lock required)
fssh_partition_data* 	fssh_create_child_partition(
								fssh_partition_id partitionID, int32_t index,
								fssh_partition_id childID);
	// childID is an optional input parameter -- -1 to be ignored
bool					fssh_delete_partition(fssh_partition_id partitionID);
void					fssh_partition_modified(fssh_partition_id partitionID);
	// tells the disk device manager, that the parition has been modified

fssh_status_t fssh_scan_partition(fssh_partition_id partitionID);
	// Service method for disks systems: Synchronously scans the partition.
	// Device must not be locked.

// disk systems
fssh_disk_system_id		fssh_find_disk_system(const char *name);

// jobs
bool		fssh_update_disk_device_job_progress(fssh_disk_job_id jobID,
					float progress);
bool		fssh_update_disk_device_job_extra_progress(fssh_disk_job_id jobID,
					const char *info);
bool		fssh_set_disk_device_job_error_message(fssh_disk_job_id jobID,
					const char *message);
uint32_t	fssh_update_disk_device_job_interrupt_properties(
					fssh_disk_job_id jobID, uint32_t interruptProperties);
	// returns one of B_DISK_DEVICE_JOB_{CONTINUE,CANCEL,REVERSE}

#ifdef __cplusplus
}
#endif

#endif	// _FSSH_DISK_DEVICE_MANAGER_H
