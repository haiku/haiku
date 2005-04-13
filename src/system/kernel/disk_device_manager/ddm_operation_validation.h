// ddm_operation_validation.h

#ifndef _DISK_DEVICE_MANAGER_OPERATION_VALIDATION_H
#define _DISK_DEVICE_MANAGER_OPERATION_VALIDATION_H

#include <OS.h>

#include <disk_device_manager.h>

namespace BPrivate {
namespace DiskDevice {

class KPartition;

team_id get_current_team();
bool check_shadow_partition(const KPartition *partition, int32 changeCounter,
							bool requireShadow = true);
bool get_unmovable_descendants(KPartition *partition, partition_id *&unmovable,
							   size_t &unmovableSize,
							   partition_id *&needUnmounting,
							   size_t &needUnmountingSize,
							   bool requireShadow = true);
status_t validate_move_descendants(KPartition *partition, off_t moveBy,
								   bool markMovable = false,
								   bool requireShadow = true);
status_t validate_defragment_partition(KPartition *partition,
									   int32 changeCounter,
									   bool *whileMounted = NULL,
									   bool requireShadow = true);
status_t validate_repair_partition(KPartition *partition, int32 changeCounter,
								   bool checkOnly, bool *whileMounted = NULL,
								   bool requireShadow = true);
status_t validate_resize_partition(KPartition *partition, int32 changeCounter,
								   off_t *size, off_t *contentSize,
								   bool requireShadow = true);
status_t validate_move_partition(KPartition *partition, int32 changeCounter,
								 off_t *newOffset, bool markMovable = false,
								 bool requireShadow = true);
status_t validate_set_partition_name(KPartition *partition,
									 int32 changeCounter, char *name,
									 bool requireShadow = true);
status_t validate_set_partition_content_name(KPartition *partition,
											 int32 changeCounter, char *name,
											 bool requireShadow = true);
status_t validate_set_partition_type(KPartition *partition,
									 int32 changeCounter, const char *type,
									 bool requireShadow = true);
status_t validate_set_partition_parameters(KPartition *partition,
										   int32 changeCounter,
										   const char *parameters,
										   bool requireShadow = true);
status_t validate_set_partition_content_parameters(KPartition *partition,
												   int32 changeCounter,
												   const char *parameters,
												   bool requireShadow = true);
status_t validate_initialize_partition(KPartition *partition,
									   int32 changeCounter,
									   const char *diskSystemName, char *name,
									   const char *parameters,
									   bool requireShadow = true);
status_t validate_create_child_partition(KPartition *partition,
										 int32 changeCounter, off_t *offset,
										 off_t *size, const char *type,
										 const char *parameters,
										 int32 *index = NULL,
										 bool requireShadow = true);
status_t validate_delete_child_partition(KPartition *partition,
										 int32 changeCounter,
										 bool requireShadow = true);

} // namespace DiskDevice
} // namespace BPrivate

#endif	// _DISK_DEVICE_MANAGER_OPERATION_VALIDATION_H
