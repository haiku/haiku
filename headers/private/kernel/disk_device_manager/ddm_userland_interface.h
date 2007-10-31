// ddm_userland_interface.h

#ifndef _DISK_DEVICE_MANAGER_USERLAND_INTERFACE_H
#define _DISK_DEVICE_MANAGER_USERLAND_INTERFACE_H

#include <OS.h>

#include <DiskDeviceDefs.h>

#include <disk_device_manager.h>

#ifdef __cplusplus
extern "C" {
#endif

// userland partition representation
typedef struct user_partition_data user_partition_data;
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
typedef struct user_disk_device_data {
	uint32				device_flags;
	char				*path;
	user_partition_data	device_partition_data;
} user_disk_device_data;

// userland disk system representation
typedef struct user_disk_system_info {
	disk_system_id	id;
	char			name[B_FILE_NAME_LENGTH];	// better B_PATH_NAME_LENGTH?
	char			pretty_name[B_OS_NAME_LENGTH];
	uint32			flags;
} user_disk_system_info;

// userland disk device job representation
typedef struct user_disk_device_job_info {
	disk_job_id		id;
	uint32			type;
	partition_id	partition;
	char			description[256];
} user_disk_device_job_info;


/*! Syscalls entries */

// iterating, retrieving device/partition data
partition_id _user_get_next_disk_device_id(int32 *cookie, size_t *neededSize);
partition_id _user_find_disk_device(const char *filename, size_t *neededSize);
partition_id _user_find_partition(const char *filename, size_t *neededSize);
status_t _user_get_disk_device_data(partition_id deviceID, bool deviceOnly,
									bool shadow, user_disk_device_data *buffer,
									size_t bufferSize, size_t *neededSize);

partition_id _user_register_file_device(const char *filename);
status_t _user_unregister_file_device(partition_id deviceID,
									  const char *filename);
	// Only a valid deviceID or filename need to be passed. The other one
	// is -1/NULL. If both is given only filename is ignored.

// disk systems
status_t _user_get_disk_system_info(disk_system_id id,
									user_disk_system_info *info);
status_t _user_get_next_disk_system_info(int32 *cookie,
										 user_disk_system_info *info);
status_t _user_find_disk_system(const char *name, user_disk_system_info *info);

// disk device modification
status_t _user_defragment_partition(partition_id partitionID,
				int32* changeCounter);
status_t _user_repair_partition(partition_id partitionID, int32* changeCounter,
				bool checkOnly);
status_t _user_resize_partition(partition_id partitionID, int32* changeCounter,
				partition_id childID, int32* childChangeCounter, off_t size,
				off_t contentSize);
status_t _user_move_partition(partition_id partitionID, int32* changeCounter,
				partition_id childID, int32* childChangeCounter,
				off_t newOffset, partition_id* descendantIDs,
				int32* descendantChangeCounters, int32 descendantCount);
status_t _user_set_partition_name(partition_id partitionID,
				int32* changeCounter, partition_id childID,
				int32* childChangeCounter, const char* name);
status_t _user_set_partition_content_name(partition_id partitionID,
				int32* changeCounter, const char* name);
status_t _user_set_partition_type(partition_id partitionID,
				int32* changeCounter, partition_id childID,
				int32* childChangeCounter, const char* type);
status_t _user_set_partition_parameters(partition_id partitionID,
				int32* changeCounter, partition_id childID,
				int32* childChangeCounter, const char* parameters,
				size_t parametersSize);
status_t _user_set_partition_content_parameters(partition_id partitionID,
				int32* changeCounter, const char* parameters,
				size_t parametersSize);
status_t _user_initialize_partition(partition_id partitionID,
				int32* changeCounter, const char* diskSystemName,
				const char* name, const char* parameters,
				size_t parametersSize);
status_t _user_uninitialize_partition(partition_id partitionID,
				int32* changeCounter);

status_t _user_create_child_partition(partition_id partitionID,
				int32* changeCounter, off_t offset, off_t size,
				const char* type, const char* name, const char* parameters,
				size_t parametersSize, partition_id* childID,
				int32* childChangeCounter);
status_t _user_delete_child_partition(partition_id partitionID,
				int32* changeCounter, partition_id childID,
				int32 childChangeCounter);

#ifdef __cplusplus
}
#endif

#endif	// _DISK_DEVICE_MANAGER_USERLAND_INTERFACE_H
