// ddm_modules.h
//
// Interfaces to be implemented by partition modules/FS add-ons.
// For better readibility this is not yet a module interface, but just a
// listing of functions.

#ifndef _K_DISK_DEVICE_MODULES_H
#define _K_DISK_DEVICE_MODULES_H

#include <SupportDefs.h>

// partition module interface

// scanning
// (the device is write locked)
float identify_partition(int deviceFD, partition_data *partition,
						 void **cookie);
status_t scan_partition(int deviceFD, partition_data *partition,
						void *identifyCookie);
void free_identify_partition_cookie(partition_data *partition, void *cookie);
void free_partition_cookie(partition_data *partition);
void free_partition_content_cookie(partition_data *partition);

// querying
// (the device is read locked)
bool supports_reparing_partition(partition_data *partition, bool checkOnly,
								 bool *whileMounted);
bool supports_resizing_partition(partition_data *partition,
								 bool *whileMounted);
bool supports_resizing_child_partition(partition_data *partition,
									   partition_data *child);
bool supports_moving_partition(partition_data *partition, bool *whileMounted);
bool supports_moving_child_partition(partition_data *partition,
									 partition_data *child);
bool supports_parent_system(const char *system);
bool supports_child_system(const char *system);

bool validate_resize_partition(partition_data *partition, off_t *size);
bool validate_move_partition(partition_data *partition, off_t *start);
bool validate_resize_child_partition(partition_data *partition,
									 partition_data *child, off_t *size);
bool validate_move_child_partition(partition_data *partition,
								   partition_data *child, off_t *start);
bool validate_create_child_partition(partition_data *partition,
									 partition_data *child, off_t *start,
									 off_t *size, const char *parameters);
bool validate_initialize_partition(partition_data *partition,
								   const char *parameters);
bool validate_set_partition_parameters(partition_data *partition,
									   const char *parameters);
bool validate_set_partition_content_parameters(partition_data *partition,
											   const char *parameters);
bool get_partitionable_spaces(partition_data *partition,
							  partitionable_space_data *spaces,
							  int32 count, int32 *actualCount);
	// When not implemented, a standard algorithm is used.

// Alternatively we can multiplex the supports_*()/validate_*() hooks
// (see supports_validates_parameters.h for parameters):
bool supports_partition_operation(uint32 operation, void *parameters);
bool validate_partition_operation(uint32 operation, void *parameters);
// (same holds for the FS add-on API, of course)

// writing
// (device is NOT locked)
status_t repair_partition(int deviceFD, partition_id partition,
						  bool checkOnly, disk_job_id job);
status_t resize_partition(int deviceFD, partition_id partition, off_t size,
						  disk_job_id job);
status_t resize_child_partition(int deviceFD, partition_id partition,
								off_t size, disk_job_id job);
status_t move_partition(int deviceFD, partition_id partition, off_t offset,
						disk_job_id job);
status_t move_child_partition(int deviceFD, partition_id partition,
							  partition_id child, off_t offset,
							  disk_job_id job);
status_t create_child_partition(int deviceFD, partition_id partition,
								off_t offset, off_t size,
								const char *parameters, disk_job_id job,
								partition_id *childID);
	// childID is used for the return value, but is also an optional input
	// parameter -- -1 to be ignored
status_t delete_child_partition(int deviceFD, partition_id partition,
								partition_id child, disk_job_id job);
status_t initialize_partition(int deviceFD, partition_id partition,
							  const char *parameters, disk_job_id job);
status_t set_parameters_partition(int deviceFD, partition_id partition,
								  const char *parameters, disk_job_id job);
status_t set_partition_content_parameters(int deviceFD, partition_id partition,
										  const char *parameters,
										  disk_job_id job);


// FS add-on interface

// scanning
// (the device is write locked)
float identify_partition(const char *partitionPath, partition_data *partition,
						 void **cookie);
status_t scan_partition(const char *partitionPath, partition_data *partition,
						void *identifyCookie);
void free_identify_partition_cookie(partition_data *partition, void *cookie);
void free_partition_content_cookie(partition_data *partition, void *cookie);

// querying
// (the device is read locked)
bool supports_defragmenting_partition(partition_data *partition,
									  bool *whileMounted);
bool supports_reparing_partition(partition_data *partition, bool checkOnly,
								 bool *whileMounted);
bool supports_resizing_partition(partition_data *partition,
								 bool *whileMounted);
bool supports_moving_partition(partition_data *partition, bool *whileMounted);
bool supports_parent_system(const char *system);

bool validate_resize_partition(partition_data *partition, off_t *size);
bool validate_move_partition(partition_data *partition, off_t *start);
bool validate_initialize_partition(partition_data *partition,
								   const char *parameters);
bool validate_set_partition_content_parameters(partition_data *partition,
											   const char *parameters);

// writing
// (the device is NOT locked)
status_t defragment_partition(int fd, partition_id partition, disk_job_id job);
status_t repair_partition(int fd, partition_id partition, bool checkOnly,
						  disk_job_id job);
status_t resize_partition(int fd, partition_id partition, off_t size,
						  disk_job_id job);
status_t move_partition(int fd, partition_id partition, off_t offset,
						disk_job_id job);
status_t initialize_partition(const char *partition, const char *parameters,
							  disk_job_id job);
	// This is pretty close to how the hook in R5 looked. Save the job ID, of
	// course and that the parameters were given as (void*, size_t) pair.
status_t set_partition_content_parameters(int fd, partition_id partition,
										  const char *parameters,
										  disk_job_id job);

#endif	// _K_DISK_DEVICE_MODULES_H
