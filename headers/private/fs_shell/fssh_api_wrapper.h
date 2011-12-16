/*
 * Copyright 2007-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FSSH_API_WRAPPER_H
#define _FSSH_API_WRAPPER_H


#include <stdlib.h>

#include "fssh_dirent.h"
#include "fssh_errno.h"
#include "fssh_fcntl.h"
#include "fssh_stat.h"
#include "fssh_stdio.h"
#include "fssh_string.h"
#include "fssh_time.h"
#include "fssh_uio.h"
#include "fssh_unistd.h"

#include "fssh_atomic.h"
#include "fssh_byte_order.h"
#include "fssh_defs.h"
#include "fssh_disk_device_defs.h"
#include "fssh_disk_device_manager.h"
#include "fssh_driver_settings.h"
#include "fssh_drivers.h"
#include "fssh_errors.h"
#include "fssh_fs_attr.h"
#include "fssh_fs_cache.h"
#include "fssh_fs_index.h"
#include "fssh_fs_info.h"
#include "fssh_fs_interface.h"
#include "fssh_fs_query.h"
#include "fssh_fs_volume.h"
#include "fssh_kernel_export.h"
#include "fssh_lock.h"
#include "fssh_module.h"
#include "fssh_node_monitor.h"
#include "fssh_os.h"
#include "fssh_type_constants.h"
#include "fssh_types.h"

#include "DoublyLinkedList.h"
#include "SinglyLinkedList.h"
#include "Stack.h"


////////////////////////////////////////////////////////////////////////////////
// #pragma mark - fssh_atomic.h

#define atomic_set			fssh_atomic_set
#define atomic_test_and_set	fssh_atomic_test_and_set
#define atomic_add			fssh_atomic_add
#define atomic_and			fssh_atomic_and
#define atomic_or			fssh_atomic_or
#define atomic_get			fssh_atomic_get


////////////////////////////////////////////////////////////////////////////////
// #pragma mark - fssh_bytes_order.h

// swap directions
#define B_SWAP_HOST_TO_LENDIAN	FSSH_B_SWAP_HOST_TO_LENDIAN
#define B_SWAP_HOST_TO_BENDIAN	FSSH_B_SWAP_HOST_TO_BENDIAN
#define B_SWAP_LENDIAN_TO_HOST	FSSH_B_SWAP_LENDIAN_TO_HOST
#define B_SWAP_BENDIAN_TO_HOST	FSSH_B_SWAP_BENDIAN_TO_HOST
#define B_SWAP_ALWAYS			FSSH_B_SWAP_ALWAYS

#define swap_action	fssh_swap_action

// BSD/networking macros
#define htonl(x)	fssh_htonl(x)
#define ntohl(x)	fssh_ntohl(x)
#define htons(x)	fssh_htons(x)
#define ntohs(x)	fssh_ntohs(x)

// always swap macros
#define B_SWAP_DOUBLE(arg)	FSSH_B_SWAP_DOUBLE(arg)
#define B_SWAP_FLOAT(arg)	FSSH_B_SWAP_FLOAT(arg)
#define B_SWAP_INT64(arg)	FSSH_B_SWAP_INT64(arg)
#define B_SWAP_INT32(arg)	FSSH_B_SWAP_INT32(arg)
#define B_SWAP_INT16(arg)	FSSH_B_SWAP_INT16(arg)

#define B_HOST_IS_LENDIAN	FSSH_B_HOST_IS_LENDIAN
#define B_HOST_IS_BENDIAN	FSSH_B_HOST_IS_BENDIAN

// Host native to little endian
#define B_HOST_TO_LENDIAN_DOUBLE(arg)	FSSH_B_HOST_TO_LENDIAN_DOUBLE(arg)
#define B_HOST_TO_LENDIAN_FLOAT(arg)	FSSH_B_HOST_TO_LENDIAN_FLOAT(arg)
#define B_HOST_TO_LENDIAN_INT64(arg)	FSSH_B_HOST_TO_LENDIAN_INT64(arg)
#define B_HOST_TO_LENDIAN_INT32(arg)	FSSH_B_HOST_TO_LENDIAN_INT32(arg)
#define B_HOST_TO_LENDIAN_INT16(arg)	FSSH_B_HOST_TO_LENDIAN_INT16(arg)

// Little endian to host native
#define B_LENDIAN_TO_HOST_DOUBLE(arg)	FSSH_B_LENDIAN_TO_HOST_DOUBLE(arg)
#define B_LENDIAN_TO_HOST_FLOAT(arg)	FSSH_B_LENDIAN_TO_HOST_FLOAT(arg)
#define B_LENDIAN_TO_HOST_INT64(arg)	FSSH_B_LENDIAN_TO_HOST_INT64(arg)
#define B_LENDIAN_TO_HOST_INT32(arg)	FSSH_B_LENDIAN_TO_HOST_INT32(arg)
#define B_LENDIAN_TO_HOST_INT16(arg)	FSSH_B_LENDIAN_TO_HOST_INT16(arg)

// Host native to big endian
#define B_HOST_TO_BENDIAN_DOUBLE(arg)	FSSH_B_HOST_TO_BENDIAN_DOUBLE(arg)
#define B_HOST_TO_BENDIAN_FLOAT(arg)	FSSH_B_HOST_TO_BENDIAN_FLOAT(arg)
#define B_HOST_TO_BENDIAN_INT64(arg)	FSSH_B_HOST_TO_BENDIAN_INT64(arg)
#define B_HOST_TO_BENDIAN_INT32(arg)	FSSH_B_HOST_TO_BENDIAN_INT32(arg)
#define B_HOST_TO_BENDIAN_INT16(arg)	FSSH_B_HOST_TO_BENDIAN_INT16(arg)

// Big endian to host native
#define B_BENDIAN_TO_HOST_DOUBLE(arg)	FSSH_B_BENDIAN_TO_HOST_DOUBLE(arg)
#define B_BENDIAN_TO_HOST_FLOAT(arg)	FSSH_B_BENDIAN_TO_HOST_FLOAT(arg)
#define B_BENDIAN_TO_HOST_INT64(arg)	FSSH_B_BENDIAN_TO_HOST_INT64(arg)
#define B_BENDIAN_TO_HOST_INT32(arg)	FSSH_B_BENDIAN_TO_HOST_INT32(arg)
#define B_BENDIAN_TO_HOST_INT16(arg)	FSSH_B_BENDIAN_TO_HOST_INT16(arg)

#define swap_data			fssh_swap_data
#define is_type_swapped		fssh_is_type_swapped


// Private implementations
#define __swap_double		__fssh_swap_double
#define __swap_float		__fssh_swap_float
#define __swap_int64		__fssh_swap_int64
#define __swap_int32		__fssh_swap_int32
#define __swap_int16		__fssh_swap_int16


////////////////////////////////////////////////////////////////////////////////
// #pragma mark - fssh_defs.h

// 32/64 bitness
#undef B_HAIKU_32_BIT
#undef B_HAIKU_64_BIT
#ifdef FSSH_B_HAIKU_64_BIT
#	define B_HAIKU_64_BIT		FSSH_B_HAIKU_64_BIT
#endif
#ifdef FSSH_B_HAIKU_32_BIT
#	define B_HAIKU_32_BIT		FSSH_B_HAIKU_32_BIT
#endif

// Limits
#define B_DEV_NAME_LENGTH	FSSH_B_DEV_NAME_LENGTH
#define B_FILE_NAME_LENGTH	FSSH_B_FILE_NAME_LENGTH
#define B_PATH_NAME_LENGTH	FSSH_B_PATH_NAME_LENGTH
#define B_ATTR_NAME_LENGTH	FSSH_B_ATTR_NAME_LENGTH
#define B_MIME_TYPE_LENGTH	FSSH_B_MIME_TYPE_LENGTH
#define B_MAX_SYMLINKS		FSSH_B_MAX_SYMLINKS

// Open Modes
#define B_READ_ONLY			FSSH_B_READ_ONLY
#define B_WRITE_ONLY		FSSH_B_WRITE_ONLY
#define B_READ_WRITE		FSSH_B_READ_WRITE

#define	B_FAIL_IF_EXISTS	FSSH_B_FAIL_IF_EXISTS
#define B_CREATE_FILE		FSSH_B_CREATE_FILE
#define B_ERASE_FILE		FSSH_B_ERASE_FILE
#define B_OPEN_AT_END		FSSH_B_OPEN_AT_END

// Node Flavors
#define node_flavor			fssh_node_flavor
#define	B_FILE_NODE			FSSH_B_FILE_NODE
#define	B_SYMLINK_NODE		FSSH_B_SYMLINK_NODE
#define	B_DIRECTORY_NODE	FSSH_B_DIRECTORY_NODE
#define	B_ANY_NODE			FSSH_B_ANY_NODE

#undef	offsetof
#define offsetof(type,member)	fssh_offsetof(type,member)

#define min_c(a,b)				fssh_min_c(a,b)
#define max_c(a,b)				fssh_max_c(a,b)

#undef	_PACKED
#define _PACKED	_FSSH_PACKED


////////////////////////////////////////////////////////////////////////////////
// #pragma mark - fssh_dirent.h

#define dirent		fssh_dirent
#define dirent_t	fssh_dirent_t

#define DIR			fssh_DIR



////////////////////////////////////////////////////////////////////////////////
// #pragma mark - fssh_disk_device_defs.h

#define partition_id	fssh_partition_id
#define disk_system_id	fssh_disk_system_id
#define disk_job_id		fssh_disk_job_id

// partition flags
#define B_PARTITION_IS_DEVICE			FSSH_B_PARTITION_IS_DEVICE
#define B_PARTITION_FILE_SYSTEM			FSSH_B_PARTITION_FILE_SYSTEM
#define B_PARTITION_PARTITIONING_SYSTEM	FSSH_B_PARTITION_PARTITIONING_SYSTEM
#define B_PARTITION_READ_ONLY			FSSH_B_PARTITION_READ_ONLY
#define B_PARTITION_MOUNTED				FSSH_B_PARTITION_MOUNTED
#define B_PARTITION_BUSY				FSSH_B_PARTITION_BUSY
#define B_PARTITION_DESCENDANT_BUSY		FSSH_B_PARTITION_DESCENDANT_BUSY

// partition statuses
#define B_PARTITION_VALID			FSSH_B_PARTITION_VALID
#define B_PARTITION_CORRUPT			FSSH_B_PARTITION_CORRUPT
#define B_PARTITION_UNRECOGNIZED	FSSH_B_PARTITION_UNRECOGNIZED
#define B_PARTITION_UNINITIALIZED	FSSH_B_PARTITION_UNINITIALIZED

// partition change flags
#define B_PARTITION_CHANGED_OFFSET				FSSH_B_PARTITION_CHANGED_OFFSET
#define B_PARTITION_CHANGED_SIZE				FSSH_B_PARTITION_CHANGED_SIZE
#define B_PARTITION_CHANGED_CONTENT_SIZE		FSSH_B_PARTITION_CHANGED_CONTENT_SIZE
#define B_PARTITION_CHANGED_BLOCK_SIZE			FSSH_B_PARTITION_CHANGED_BLOCK_SIZE
#define B_PARTITION_CHANGED_STATUS				FSSH_B_PARTITION_CHANGED_STATUS
#define B_PARTITION_CHANGED_FLAGS				FSSH_B_PARTITION_CHANGED_FLAGS
#define B_PARTITION_CHANGED_VOLUME				FSSH_B_PARTITION_CHANGED_VOLUME
#define B_PARTITION_CHANGED_NAME				FSSH_B_PARTITION_CHANGED_NAME
#define B_PARTITION_CHANGED_CONTENT_NAME		FSSH_B_PARTITION_CHANGED_CONTENT_NAME
#define B_PARTITION_CHANGED_TYPE				FSSH_B_PARTITION_CHANGED_TYPE
#define B_PARTITION_CHANGED_CONTENT_TYPE		FSSH_B_PARTITION_CHANGED_CONTENT_TYPE
#define B_PARTITION_CHANGED_PARAMETERS			FSSH_B_PARTITION_CHANGED_PARAMETERS
#define B_PARTITION_CHANGED_CONTENT_PARAMETERS	FSSH_B_PARTITION_CHANGED_CONTENT_PARAMETERS
#define B_PARTITION_CHANGED_CHILDREN			FSSH_B_PARTITION_CHANGED_CHILDREN
#define B_PARTITION_CHANGED_DESCENDANTS			FSSH_B_PARTITION_CHANGED_DESCENDANTS
#define B_PARTITION_CHANGED_DEFRAGMENTATION		FSSH_B_PARTITION_CHANGED_DEFRAGMENTATION
#define B_PARTITION_CHANGED_CHECK				FSSH_B_PARTITION_CHANGED_CHECK
#define B_PARTITION_CHANGED_REPAIR				FSSH_B_PARTITION_CHANGED_REPAIR
#define B_PARTITION_CHANGED_INITIALIZATION		FSSH_B_PARTITION_CHANGED_INITIALIZATION

// disk device flags
#define B_DISK_DEVICE_REMOVABLE		FSSH_B_DISK_DEVICE_REMOVABLE
#define B_DISK_DEVICE_HAS_MEDIA		FSSH_B_DISK_DEVICE_HAS_MEDIA
#define B_DISK_DEVICE_READ_ONLY		FSSH_B_DISK_DEVICE_READ_ONLY
#define B_DISK_DEVICE_WRITE_ONCE	FSSH_B_DISK_DEVICE_WRITE_ONCE

// disk system flags
#define B_DISK_SYSTEM_IS_FILE_SYSTEM	FSSH_B_DISK_SYSTEM_IS_FILE_SYSTEM

// flags common for both file and partitioning systems
#define B_DISK_SYSTEM_SUPPORTS_CHECKING						FSSH_B_DISK_SYSTEM_SUPPORTS_CHECKING
#define B_DISK_SYSTEM_SUPPORTS_REPAIRING					FSSH_B_DISK_SYSTEM_SUPPORTS_REPAIRING
#define B_DISK_SYSTEM_SUPPORTS_RESIZING						FSSH_B_DISK_SYSTEM_SUPPORTS_RESIZING
#define B_DISK_SYSTEM_SUPPORTS_MOVING						FSSH_B_DISK_SYSTEM_SUPPORTS_MOVING
#define B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_NAME			FSSH_B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_NAME
#define B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_PARAMETERS	FSSH_B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_PARAMETERS
#define B_DISK_SYSTEM_SUPPORTS_INITIALIZING					FSSH_B_DISK_SYSTEM_SUPPORTS_INITIALIZING
#define B_DISK_SYSTEM_SUPPORTS_CONTENT_NAME					FSSH_B_DISK_SYSTEM_SUPPORTS_CONTENT_NAME

// file system specific flags
#define B_DISK_SYSTEM_SUPPORTS_DEFRAGMENTING							FSSH_B_DISK_SYSTEM_SUPPORTS_DEFRAGMENTING
#define B_DISK_SYSTEM_SUPPORTS_DEFRAGMENTING_WHILE_MOUNTED				FSSH_B_DISK_SYSTEM_SUPPORTS_DEFRAGMENTING_WHILE_MOUNTED
#define B_DISK_SYSTEM_SUPPORTS_CHECKING_WHILE_MOUNTED					FSSH_B_DISK_SYSTEM_SUPPORTS_CHECKING_WHILE_MOUNTED
#define B_DISK_SYSTEM_SUPPORTS_REPAIRING_WHILE_MOUNTED					FSSH_B_DISK_SYSTEM_SUPPORTS_REPAIRING_WHILE_MOUNTED
#define B_DISK_SYSTEM_SUPPORTS_RESIZING_WHILE_MOUNTED					FSSH_B_DISK_SYSTEM_SUPPORTS_RESIZING_WHILE_MOUNTED
#define B_DISK_SYSTEM_SUPPORTS_MOVING_WHILE_MOUNTED						FSSH_B_DISK_SYSTEM_SUPPORTS_MOVING_WHILE_MOUNTED
#define B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_NAME_WHILE_MOUNTED		FSSH_B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_NAME_WHILE_MOUNTED
#define B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_PARAMETERS_WHILE_MOUNTED	FSSH_B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_PARAMETERS_WHILE_MOUNTED
#define B_DISK_SYSTEM_SUPPORTS_WRITING									FSSH_B_DISK_SYSTEM_SUPPORTS_WRITING

// partitioning system specific flags
#define B_DISK_SYSTEM_SUPPORTS_RESIZING_CHILD		FSSH_B_DISK_SYSTEM_SUPPORTS_RESIZING_CHILD
#define B_DISK_SYSTEM_SUPPORTS_MOVING_CHILD			FSSH_B_DISK_SYSTEM_SUPPORTS_MOVING_CHILD
#define B_DISK_SYSTEM_SUPPORTS_SETTING_NAME			FSSH_B_DISK_SYSTEM_SUPPORTS_SETTING_NAME
#define B_DISK_SYSTEM_SUPPORTS_SETTING_TYPE			FSSH_B_DISK_SYSTEM_SUPPORTS_SETTING_TYPE
#define B_DISK_SYSTEM_SUPPORTS_SETTING_PARAMETERS	FSSH_B_DISK_SYSTEM_SUPPORTS_SETTING_PARAMETERS
#define B_DISK_SYSTEM_SUPPORTS_CREATING_CHILD		FSSH_B_DISK_SYSTEM_SUPPORTS_CREATING_CHILD
#define B_DISK_SYSTEM_SUPPORTS_DELETING_CHILD		FSSH_B_DISK_SYSTEM_SUPPORTS_DELETING_CHILD
#define B_DISK_SYSTEM_SUPPORTS_NAME					FSSH_B_DISK_SYSTEM_SUPPORTS_NAME

// disk device job types
#define B_DISK_DEVICE_JOB_BAD_TYPE				FSSH_B_DISK_DEVICE_JOB_BAD_TYPE
#define B_DISK_DEVICE_JOB_DEFRAGMENT			FSSH_B_DISK_DEVICE_JOB_DEFRAGMENT
#define B_DISK_DEVICE_JOB_REPAIR				FSSH_B_DISK_DEVICE_JOB_REPAIR
#define B_DISK_DEVICE_JOB_RESIZE				FSSH_B_DISK_DEVICE_JOB_RESIZE
#define B_DISK_DEVICE_JOB_MOVE					FSSH_B_DISK_DEVICE_JOB_MOVE
#define B_DISK_DEVICE_JOB_SET_NAME				FSSH_B_DISK_DEVICE_JOB_SET_NAME
#define B_DISK_DEVICE_JOB_SET_CONTENT_NAME		FSSH_B_DISK_DEVICE_JOB_SET_CONTENT_NAME
#define B_DISK_DEVICE_JOB_SET_TYPE				FSSH_B_DISK_DEVICE_JOB_SET_TYPE
#define B_DISK_DEVICE_JOB_SET_PARMETERS			FSSH_B_DISK_DEVICE_JOB_SET_PARMETERS
#define B_DISK_DEVICE_JOB_SET_CONTENT_PARMETERS	FSSH_B_DISK_DEVICE_JOB_SET_CONTENT_PARMETERS
#define B_DISK_DEVICE_JOB_INITIALIZE			FSSH_B_DISK_DEVICE_JOB_INITIALIZE
#define B_DISK_DEVICE_JOB_UNINITIALIZE			FSSH_B_DISK_DEVICE_JOB_UNINITIALIZE
#define B_DISK_DEVICE_JOB_CREATE				FSSH_B_DISK_DEVICE_JOB_CREATE
#define B_DISK_DEVICE_JOB_DELETE				FSSH_B_DISK_DEVICE_JOB_DELETE
#define B_DISK_DEVICE_JOB_SCAN					FSSH_B_DISK_DEVICE_JOB_SCAN

// disk device job statuses
#define B_DISK_DEVICE_JOB_UNINITIALIZED	FSSH_B_DISK_DEVICE_JOB_UNINITIALIZED
#define B_DISK_DEVICE_JOB_SCHEDULED		FSSH_B_DISK_DEVICE_JOB_SCHEDULED
#define B_DISK_DEVICE_JOB_IN_PROGRESS	FSSH_B_DISK_DEVICE_JOB_IN_PROGRESS
#define B_DISK_DEVICE_JOB_SUCCEEDED		FSSH_B_DISK_DEVICE_JOB_SUCCEEDED
#define B_DISK_DEVICE_JOB_FAILED		FSSH_B_DISK_DEVICE_JOB_FAILED
#define B_DISK_DEVICE_JOB_CANCELED		FSSH_B_DISK_DEVICE_JOB_CANCELED

#define disk_device_job_progress_info	fssh_disk_device_job_progress_info

// disk device job interrupt properties
#define B_DISK_DEVICE_JOB_CAN_CANCEL		FSSH_B_DISK_DEVICE_JOB_CAN_CANCEL
#define B_DISK_DEVICE_JOB_STOP_ON_CANCEL	FSSH_B_DISK_DEVICE_JOB_STOP_ON_CANCEL
#define B_DISK_DEVICE_JOB_REVERSE_ON_CANCEL	FSSH_B_DISK_DEVICE_JOB_REVERSE_ON_CANCEL
#define B_DISK_DEVICE_JOB_CAN_PAUSE			FSSH_B_DISK_DEVICE_JOB_CAN_PAUSE

// string length constants, all of which include the NULL terminator
#define B_DISK_DEVICE_TYPE_LENGTH	FSSH_B_DISK_DEVICE_TYPE_LENGTH
#define B_DISK_DEVICE_NAME_LENGTH	FSSH_B_DISK_DEVICE_NAME_LENGTH
#define B_DISK_SYSTEM_NAME_LENGTH	FSSH_B_DISK_SYSTEM_NAME_LENGTH

// max size of parameter string buffers, including NULL terminator
#define B_DISK_DEVICE_MAX_PARAMETER_SIZE	FSSH_B_DISK_DEVICE_MAX_PARAMETER_SIZE


////////////////////////////////////////////////////////////////////////////////
// #pragma mark - fssh_disk_device_manager.h

#define partition_data				fssh_partition_data
#define disk_device_data			fssh_disk_device_data
#define partitionable_space_data	fssh_partitionable_space_data

// operations on partitions
#define B_PARTITION_DEFRAGMENT				FSSH_B_PARTITION_DEFRAGMENT
#define B_PARTITION_REPAIR					FSSH_B_PARTITION_REPAIR
#define B_PARTITION_RESIZE					FSSH_B_PARTITION_RESIZE
#define B_PARTITION_RESIZE_CHILD			FSSH_B_PARTITION_RESIZE_CHILD
#define B_PARTITION_MOVE					FSSH_B_PARTITION_MOVE
#define B_PARTITION_MOVE_CHILD				FSSH_B_PARTITION_MOVE_CHILD
#define B_PARTITION_SET_NAME				FSSH_B_PARTITION_SET_NAME
#define B_PARTITION_SET_CONTENT_NAME		FSSH_B_PARTITION_SET_CONTENT_NAME
#define B_PARTITION_SET_TYPE				FSSH_B_PARTITION_SET_TYPE
#define B_PARTITION_SET_PARAMETERS			FSSH_B_PARTITION_SET_PARAMETERS
#define B_PARTITION_SET_CONTENT_PARAMETERS	FSSH_B_PARTITION_SET_CONTENT_PARAMETERS
#define B_PARTITION_INITIALIZE				FSSH_B_PARTITION_INITIALIZE
#define B_PARTITION_CREATE_CHILD			FSSH_B_PARTITION_CREATE_CHILD
#define B_PARTITION_DELETE_CHILD			FSSH_B_PARTITION_DELETE_CHILD

// disk device job cancel status
#define B_DISK_DEVICE_JOB_CONTINUE	FSSH_B_DISK_DEVICE_JOB_CONTINUE
#define B_DISK_DEVICE_JOB_CANCEL	FSSH_B_DISK_DEVICE_JOB_CANCEL
#define B_DISK_DEVICE_JOB_REVERSE	FSSH_B_DISK_DEVICE_JOB_REVERSE

// disk device locking
#define write_lock_disk_device		fssh_write_lock_disk_device
#define write_unlock_disk_device	fssh_write_unlock_disk_device
#define read_lock_disk_device		fssh_read_lock_disk_device
#define read_unlock_disk_device		fssh_read_unlock_disk_device

// getting disk devices/partitions by path
#define find_disk_device	fssh_find_disk_device
#define find_partition		fssh_find_partition

// disk device/partition read access
#define get_disk_device			fssh_get_disk_device
#define get_partition			fssh_get_partition
#define get_parent_partition	fssh_get_parent_partition
#define get_child_partition		fssh_get_child_partition

// partition write access
#define create_child_partition	fssh_create_child_partition
#define delete_partition		fssh_delete_partition
#define partition_modified		fssh_partition_modified

#define scan_partition			fssh_scan_partition

// disk systems
#define find_disk_system		fssh_find_disk_system

// jobs
#define update_disk_device_job_progress				fssh_update_disk_device_job_progress
#define update_disk_device_job_extra_progress		fssh_update_disk_device_job_extra_progress
#define set_disk_device_job_error_message			fssh_set_disk_device_job_error_message
#define update_disk_device_job_interrupt_properties	fssh_update_disk_device_job_interrupt_properties


////////////////////////////////////////////////////////////////////////////////
// #pragma mark - fssh_driver_settings.h

#define driver_parameter	fssh_driver_parameter
#define driver_settings		fssh_driver_settings

#define load_driver_settings			fssh_load_driver_settings
#define unload_driver_settings			fssh_unload_driver_settings
#define parse_driver_settings_string	fssh_parse_driver_settings_string
#define get_driver_settings_string		fssh_get_driver_settings_string
#define delete_driver_settings			fssh_delete_driver_settings
#define get_driver_parameter			fssh_get_driver_parameter
#define get_driver_boolean_parameter	fssh_get_driver_boolean_parameter
#define get_driver_settings				fssh_get_driver_settings

#define B_SAFEMODE_DRIVER_SETTINGS	FSSH_B_SAFEMODE_DRIVER_SETTINGS
#define B_SAFEMODE_SAFE_MODE		FSSH_B_SAFEMODE_SAFE_MODE


////////////////////////////////////////////////////////////////////////////////
// #pragma mark - fssh_drivers.h

#define device_open_hook		fssh_device_open_hook
#define device_close_hook		fssh_device_close_hook
#define device_free_hook		fssh_device_free_hook
#define device_control_hook		fssh_device_control_hook
#define device_read_hook		fssh_device_read_hook
#define device_write_hook		fssh_device_write_hook
#define device_select_hook		fssh_device_select_hook
#define device_deselect_hook	fssh_device_deselect_hook
#define device_read_pages_hook	fssh_device_read_pages_hook
#define device_write_pages_hook	fssh_device_write_pages_hook

#define B_CUR_DRIVER_API_VERSION	FSSH_B_CUR_DRIVER_API_VERSION

#define device_hooks	fssh_device_hooks

#define init_hardware	fssh_init_hardware
#define publish_devices	fssh_publish_devices
#define find_device		fssh_find_device
#define init_driver		fssh_init_driver
#define uninit_driver	fssh_uninit_driver

#define api_version		fssh_api_version

#define B_GET_DEVICE_SIZE			FSSH_B_GET_DEVICE_SIZE
#define B_SET_DEVICE_SIZE			FSSH_B_SET_DEVICE_SIZE
#define B_SET_NONBLOCKING_IO		FSSH_B_SET_NONBLOCKING_IO
#define B_SET_BLOCKING_IO			FSSH_B_SET_BLOCKING_IO
#define B_GET_READ_STATUS			FSSH_B_GET_READ_STATUS
#define B_GET_WRITE_STATUS			FSSH_B_GET_WRITE_STATUS
#define B_GET_GEOMETRY				FSSH_B_GET_GEOMETRY
#define B_GET_DRIVER_FOR_DEVICE		FSSH_B_GET_DRIVER_FOR_DEVICE
#define B_GET_PARTITION_INFO		FSSH_B_GET_PARTITION_INFO
#define B_SET_PARTITION				FSSH_B_SET_PARTITION
#define B_FORMAT_DEVICE				FSSH_B_FORMAT_DEVICE
#define B_EJECT_DEVICE				FSSH_B_EJECT_DEVICE
#define B_GET_ICON					FSSH_B_GET_ICON
#define B_GET_BIOS_GEOMETRY			FSSH_B_GET_BIOS_GEOMETRY
#define B_GET_MEDIA_STATUS			FSSH_B_GET_MEDIA_STATUS
#define B_LOAD_MEDIA				FSSH_B_LOAD_MEDIA
#define B_GET_BIOS_DRIVE_ID			FSSH_B_GET_BIOS_DRIVE_ID
#define B_SET_UNINTERRUPTABLE_IO	FSSH_B_SET_UNINTERRUPTABLE_IO
#define B_SET_INTERRUPTABLE_IO		FSSH_B_SET_INTERRUPTABLE_IO
#define B_FLUSH_DRIVE_CACHE			FSSH_B_FLUSH_DRIVE_CACHE
#define B_GET_PATH_FOR_DEVICE		FSSH_B_GET_PATH_FOR_DEVICE
#define B_GET_NEXT_OPEN_DEVICE		FSSH_B_GET_NEXT_OPEN_DEVICE
#define B_ADD_FIXED_DRIVER			FSSH_B_ADD_FIXED_DRIVER
#define B_REMOVE_FIXED_DRIVER		FSSH_B_REMOVE_FIXED_DRIVER
#define B_AUDIO_DRIVER_BASE			FSSH_B_AUDIO_DRIVER_BASE
#define B_MIDI_DRIVER_BASE			FSSH_B_MIDI_DRIVER_BASE
#define B_JOYSTICK_DRIVER_BASE		FSSH_B_JOYSTICK_DRIVER_BASE
#define B_GRAPHIC_DRIVER_BASE		FSSH_B_GRAPHIC_DRIVER_BASE
#define B_DEVICE_OP_CODES_END		FSSH_B_DEVICE_OP_CODES_END

#define device_geometry	fssh_device_geometry

#define B_DISK		FSSH_B_DISK
#define B_TAPE		FSSH_B_TAPE
#define B_PRINTER	FSSH_B_PRINTER
#define B_CPU		FSSH_B_CPU
#define B_WORM		FSSH_B_WORM
#define B_CD		FSSH_B_CD
#define B_SCANNER	FSSH_B_SCANNER
#define B_OPTICAL	FSSH_B_OPTICAL
#define B_JUKEBOX	FSSH_B_JUKEBOX
#define B_NETWORK	FSSH_B_NETWORK


#define partition_info			fssh_partition_info
#define driver_path				fssh_driver_path
#define open_device_iterator	fssh_open_device_iterator
#define device_icon				fssh_device_icon


////////////////////////////////////////////////////////////////////////////////
// #pragma mark - fssh_errors.h

/* Error baselines */
#define B_GENERAL_ERROR_BASE		FSSH_B_GENERAL_ERROR_BASE
#define B_OS_ERROR_BASE				FSSH_B_OS_ERROR_BASE
#define B_APP_ERROR_BASE			FSSH_B_APP_ERROR_BASE
#define B_INTERFACE_ERROR_BASE		FSSH_B_INTERFACE_ERROR_BASE
#define B_MEDIA_ERROR_BASE			FSSH_B_MEDIA_ERROR_BASE
#define B_TRANSLATION_ERROR_BASE	FSSH_B_TRANSLATION_ERROR_BASE
#define B_MIDI_ERROR_BASE			FSSH_B_MIDI_ERROR_BASE
#define B_STORAGE_ERROR_BASE		FSSH_B_STORAGE_ERROR_BASE
#define B_POSIX_ERROR_BASE			FSSH_B_POSIX_ERROR_BASE
#define B_MAIL_ERROR_BASE			FSSH_B_MAIL_ERROR_BASE
#define B_PRINT_ERROR_BASE			FSSH_B_PRINT_ERROR_BASE
#define B_DEVICE_ERROR_BASE			FSSH_B_DEVICE_ERROR_BASE

/* Developer-defined errors start at (B_ERRORS_END+1) */
#define B_ERRORS_END				FSSH_B_ERRORS_END

/* General Errors */
#define B_NO_MEMORY			FSSH_B_NO_MEMORY
#define B_IO_ERROR			FSSH_B_IO_ERROR
#define B_PERMISSION_DENIED	FSSH_B_PERMISSION_DENIED
#define B_BAD_INDEX			FSSH_B_BAD_INDEX
#define B_BAD_TYPE			FSSH_B_BAD_TYPE
#define B_BAD_VALUE			FSSH_B_BAD_VALUE
#define B_MISMATCHED_VALUES	FSSH_B_MISMATCHED_VALUES
#define B_NAME_NOT_FOUND	FSSH_B_NAME_NOT_FOUND
#define B_NAME_IN_USE		FSSH_B_NAME_IN_USE
#define B_TIMED_OUT			FSSH_B_TIMED_OUT
#define B_INTERRUPTED		FSSH_B_INTERRUPTED
#define B_WOULD_BLOCK		FSSH_B_WOULD_BLOCK
#define B_CANCELED			FSSH_B_CANCELED
#define B_NO_INIT			FSSH_B_NO_INIT
#define B_BUSY				FSSH_B_BUSY
#define B_NOT_ALLOWED		FSSH_B_NOT_ALLOWED
#define B_BAD_DATA			FSSH_B_BAD_DATA
#define B_DONT_DO_THAT		FSSH_B_DONT_DO_THAT

#define B_ERROR				FSSH_B_ERROR
#define B_OK				FSSH_B_OK
#define B_NO_ERROR			FSSH_B_NO_ERROR

/* Kernel Kit Errors */
#define B_BAD_SEM_ID		FSSH_B_BAD_SEM_ID
#define B_NO_MORE_SEMS		FSSH_B_NO_MORE_SEMS

#define B_BAD_THREAD_ID		FSSH_B_BAD_THREAD_ID
#define B_NO_MORE_THREADS	FSSH_B_NO_MORE_THREADS
#define B_BAD_THREAD_STATE	FSSH_B_BAD_THREAD_STATE
#define B_BAD_TEAM_ID		FSSH_B_BAD_TEAM_ID
#define B_NO_MORE_TEAMS		FSSH_B_NO_MORE_TEAMS

#define B_BAD_PORT_ID		FSSH_B_BAD_PORT_ID
#define B_NO_MORE_PORTS		FSSH_B_NO_MORE_PORTS

#define B_BAD_IMAGE_ID		FSSH_B_BAD_IMAGE_ID
#define B_BAD_ADDRESS		FSSH_B_BAD_ADDRESS
#define B_NOT_AN_EXECUTABLE	FSSH_B_NOT_AN_EXECUTABLE
#define B_MISSING_LIBRARY	FSSH_B_MISSING_LIBRARY
#define B_MISSING_SYMBOL	FSSH_B_MISSING_SYMBOL

#define B_DEBUGGER_ALREADY_INSTALLED	FSSH_B_DEBUGGER_ALREADY_INSTALLED

/* Application Kit Errors */
#define B_BAD_REPLY							FSSH_B_BAD_REPLY
#define B_DUPLICATE_REPLY					FSSH_B_DUPLICATE_REPLY
#define B_MESSAGE_TO_SELF					FSSH_B_MESSAGE_TO_SELF
#define B_BAD_HANDLER						FSSH_B_BAD_HANDLER
#define B_ALREADY_RUNNING					FSSH_B_ALREADY_RUNNING
#define B_LAUNCH_FAILED						FSSH_B_LAUNCH_FAILED
#define B_AMBIGUOUS_APP_LAUNCH				FSSH_B_AMBIGUOUS_APP_LAUNCH
#define B_UNKNOWN_MIME_TYPE					FSSH_B_UNKNOWN_MIME_TYPE
#define B_BAD_SCRIPT_SYNTAX					FSSH_B_BAD_SCRIPT_SYNTAX
#define B_LAUNCH_FAILED_NO_RESOLVE_LINK		FSSH_B_LAUNCH_FAILED_NO_RESOLVE_LINK
#define B_LAUNCH_FAILED_EXECUTABLE			FSSH_B_LAUNCH_FAILED_EXECUTABLE
#define B_LAUNCH_FAILED_APP_NOT_FOUND		FSSH_B_LAUNCH_FAILED_APP_NOT_FOUND
#define B_LAUNCH_FAILED_APP_IN_TRASH		FSSH_B_LAUNCH_FAILED_APP_IN_TRASH
#define B_LAUNCH_FAILED_NO_PREFERRED_APP	FSSH_B_LAUNCH_FAILED_NO_PREFERRED_APP
#define B_LAUNCH_FAILED_FILES_APP_NOT_FOUND	FSSH_B_LAUNCH_FAILED_FILES_APP_NOT_FOUND
#define B_BAD_MIME_SNIFFER_RULE				FSSH_B_BAD_MIME_SNIFFER_RULE
#define B_NOT_A_MESSAGE						FSSH_B_NOT_A_MESSAGE
#define B_SHUTDOWN_CANCELLED				FSSH_B_SHUTDOWN_CANCELLED
#define B_SHUTTING_DOWN						FSSH_B_SHUTTING_DOWN

/* Storage Kit/File System Errors */
#define B_FILE_ERROR			FSSH_B_FILE_ERROR
#define B_FILE_NOT_FOUND		FSSH_B_FILE_NOT_FOUND
#define B_FILE_EXISTS			FSSH_B_FILE_EXISTS
#define B_ENTRY_NOT_FOUND		FSSH_B_ENTRY_NOT_FOUND
#define B_NAME_TOO_LONG			FSSH_B_NAME_TOO_LONG
#define B_NOT_A_DIRECTORY		FSSH_B_NOT_A_DIRECTORY
#define B_DIRECTORY_NOT_EMPTY	FSSH_B_DIRECTORY_NOT_EMPTY
#define B_DEVICE_FULL			FSSH_B_DEVICE_FULL
#define B_READ_ONLY_DEVICE		FSSH_B_READ_ONLY_DEVICE
#define B_IS_A_DIRECTORY		FSSH_B_IS_A_DIRECTORY
#define B_NO_MORE_FDS			FSSH_B_NO_MORE_FDS
#define B_CROSS_DEVICE_LINK		FSSH_B_CROSS_DEVICE_LINK
#define B_LINK_LIMIT			FSSH_B_LINK_LIMIT
#define B_BUSTED_PIPE			FSSH_B_BUSTED_PIPE
#define B_UNSUPPORTED			FSSH_B_UNSUPPORTED
#define B_PARTITION_TOO_SMALL	FSSH_B_PARTITION_TOO_SMALL

/* POSIX Errors */
#define E2BIG			FSSH_E2BIG
#define ECHILD			FSSH_ECHILD
#define EDEADLK			FSSH_EDEADLK
#define EFBIG			FSSH_EFBIG
#define EMLINK			FSSH_EMLINK
#define ENFILE			FSSH_ENFILE
#define ENODEV			FSSH_ENODEV
#define ENOLCK			FSSH_ENOLCK
#define ENOSYS			FSSH_ENOSYS
#define ENOTTY			FSSH_ENOTTY
#define ENXIO			FSSH_ENXIO
#define ESPIPE			FSSH_ESPIPE
#define ESRCH			FSSH_ESRCH
#define EFPOS			FSSH_EFPOS
#define ESIGPARM		FSSH_ESIGPARM
#define EDOM			FSSH_EDOM
#define ERANGE			FSSH_ERANGE
#define EPROTOTYPE		FSSH_EPROTOTYPE
#define EPROTONOSUPPORT	FSSH_EPROTONOSUPPORT
#define EPFNOSUPPORT	FSSH_EPFNOSUPPORT
#define EAFNOSUPPORT	FSSH_EAFNOSUPPORT
#define EADDRINUSE		FSSH_EADDRINUSE
#define EADDRNOTAVAIL	FSSH_EADDRNOTAVAIL
#define ENETDOWN		FSSH_ENETDOWN
#define ENETUNREACH		FSSH_ENETUNREACH
#define ENETRESET		FSSH_ENETRESET
#define ECONNABORTED	FSSH_ECONNABORTED
#define ECONNRESET		FSSH_ECONNRESET
#define EISCONN			FSSH_EISCONN
#define ENOTCONN		FSSH_ENOTCONN
#define ESHUTDOWN		FSSH_ESHUTDOWN
#define ECONNREFUSED	FSSH_ECONNREFUSED
#define EHOSTUNREACH	FSSH_EHOSTUNREACH
#define ENOPROTOOPT		FSSH_ENOPROTOOPT
#define ENOBUFS			FSSH_ENOBUFS
#define EINPROGRESS		FSSH_EINPROGRESS
#define EALREADY		FSSH_EALREADY
#define EILSEQ			FSSH_EILSEQ
#define ENOMSG			FSSH_ENOMSG
#define ESTALE			FSSH_ESTALE
#define EOVERFLOW		FSSH_EOVERFLOW
#define EMSGSIZE		FSSH_EMSGSIZE
#define EOPNOTSUPP		FSSH_EOPNOTSUPP
#define ENOTSOCK		FSSH_ENOTSOCK
#define EHOSTDOWN		FSSH_EHOSTDOWN
#define EBADMSG			FSSH_EBADMSG
#define ECANCELED		FSSH_ECANCELED
#define EDESTADDRREQ	FSSH_EDESTADDRREQ
#define EDQUOT			FSSH_EDQUOT
#define EIDRM			FSSH_EIDRM
#define EMULTIHOP		FSSH_EMULTIHOP
#define ENODATA			FSSH_ENODATA
#define ENOLINK			FSSH_ENOLINK
#define ENOSR			FSSH_ENOSR
#define ENOSTR			FSSH_ENOSTR
#define ENOTSUP			FSSH_ENOTSUP
#define EPROTO			FSSH_EPROTO
#define ETIME			FSSH_ETIME
#define ETXTBSY			FSSH_ETXTBSY

/* POSIX errors that can be mapped to BeOS error codes */
#define ENOMEM			FSSH_ENOMEM
#define EACCES			FSSH_EACCES
#define EINTR			FSSH_EINTR
#define EIO				FSSH_EIO
#define EBUSY			FSSH_EBUSY
#define EFAULT			FSSH_EFAULT
#define ETIMEDOUT		FSSH_ETIMEDOUT
#define EAGAIN			FSSH_EAGAIN
#define EWOULDBLOCK		FSSH_EWOULDBLOCK
#define EBADF			FSSH_EBADF
#define EEXIST			FSSH_EEXIST
#define EINVAL			FSSH_EINVAL
#define ENAMETOOLONG	FSSH_ENAMETOOLONG
#define ENOENT			FSSH_ENOENT
#define EPERM			FSSH_EPERM
#define ENOTDIR			FSSH_ENOTDIR
#define EISDIR			FSSH_EISDIR
#define ENOTEMPTY		FSSH_ENOTEMPTY
#define ENOSPC			FSSH_ENOSPC
#define EROFS			FSSH_EROFS
#define EMFILE			FSSH_EMFILE
#define EXDEV			FSSH_EXDEV
#define ELOOP			FSSH_ELOOP
#define ENOEXEC			FSSH_ENOEXEC
#define EPIPE			FSSH_EPIPE

/* new error codes that can be mapped to POSIX errors */
#define B_BUFFER_OVERFLOW			FSSH_B_BUFFER_OVERFLOW
#define B_TOO_MANY_ARGS				FSSH_B_TOO_MANY_ARGS
#define	B_FILE_TOO_LARGE			FSSH_B_FILE_TOO_LARGE
#define B_RESULT_NOT_REPRESENTABLE	FSSH_B_RESULT_NOT_REPRESENTABLE
#define	B_DEVICE_NOT_FOUND			FSSH_B_DEVICE_NOT_FOUND
#define B_NOT_SUPPORTED				FSSH_B_NOT_SUPPORTED

/* Media Kit Errors */
#define B_STREAM_NOT_FOUND			FSSH_B_STREAM_NOT_FOUND
#define B_SERVER_NOT_FOUND			FSSH_B_SERVER_NOT_FOUND
#define B_RESOURCE_NOT_FOUND		FSSH_B_RESOURCE_NOT_FOUND
#define B_RESOURCE_UNAVAILABLE		FSSH_B_RESOURCE_UNAVAILABLE
#define B_BAD_SUBSCRIBER			FSSH_B_BAD_SUBSCRIBER
#define B_SUBSCRIBER_NOT_ENTERED	FSSH_B_SUBSCRIBER_NOT_ENTERED
#define B_BUFFER_NOT_AVAILABLE		FSSH_B_BUFFER_NOT_AVAILABLE
#define B_LAST_BUFFER_ERROR			FSSH_B_LAST_BUFFER_ERROR

/* Mail Kit Errors */
#define B_MAIL_NO_DAEMON		FSSH_B_MAIL_NO_DAEMON
#define B_MAIL_UNKNOWN_USER		FSSH_B_MAIL_UNKNOWN_USER
#define B_MAIL_WRONG_PASSWORD	FSSH_B_MAIL_WRONG_PASSWORD
#define B_MAIL_UNKNOWN_HOST		FSSH_B_MAIL_UNKNOWN_HOST
#define B_MAIL_ACCESS_ERROR		FSSH_B_MAIL_ACCESS_ERROR
#define B_MAIL_UNKNOWN_FIELD	FSSH_B_MAIL_UNKNOWN_FIELD
#define B_MAIL_NO_RECIPIENT		FSSH_B_MAIL_NO_RECIPIENT
#define B_MAIL_INVALID_MAIL		FSSH_B_MAIL_INVALID_MAIL

/* Printing Errors */
#define B_NO_PRINT_SERVER		FSSH_B_NO_PRINT_SERVER

/* Device Kit Errors */
#define B_DEV_INVALID_IOCTL				FSSH_B_DEV_INVALID_IOCTL
#define B_DEV_NO_MEMORY					FSSH_B_DEV_NO_MEMORY
#define B_DEV_BAD_DRIVE_NUM				FSSH_B_DEV_BAD_DRIVE_NUM
#define B_DEV_NO_MEDIA					FSSH_B_DEV_NO_MEDIA
#define B_DEV_UNREADABLE				FSSH_B_DEV_UNREADABLE
#define B_DEV_FORMAT_ERROR				FSSH_B_DEV_FORMAT_ERROR
#define B_DEV_TIMEOUT					FSSH_B_DEV_TIMEOUT
#define B_DEV_RECALIBRATE_ERROR			FSSH_B_DEV_RECALIBRATE_ERROR
#define B_DEV_SEEK_ERROR				FSSH_B_DEV_SEEK_ERROR
#define B_DEV_ID_ERROR					FSSH_B_DEV_ID_ERROR
#define B_DEV_READ_ERROR				FSSH_B_DEV_READ_ERROR
#define B_DEV_WRITE_ERROR				FSSH_B_DEV_WRITE_ERROR
#define B_DEV_NOT_READY					FSSH_B_DEV_NOT_READY
#define B_DEV_MEDIA_CHANGED				FSSH_B_DEV_MEDIA_CHANGED
#define B_DEV_MEDIA_CHANGE_REQUESTED	FSSH_B_DEV_MEDIA_CHANGE_REQUESTED
#define B_DEV_RESOURCE_CONFLICT			FSSH_B_DEV_RESOURCE_CONFLICT
#define B_DEV_CONFIGURATION_ERROR		FSSH_B_DEV_CONFIGURATION_ERROR
#define B_DEV_DISABLED_BY_USER			FSSH_B_DEV_DISABLED_BY_USER
#define B_DEV_DOOR_OPEN					FSSH_B_DEV_DOOR_OPEN

#define B_DEV_INVALID_PIPE				FSSH_B_DEV_INVALID_PIPE
#define B_DEV_CRC_ERROR					FSSH_B_DEV_CRC_ERROR
#define B_DEV_STALLED					FSSH_B_DEV_STALLED
#define B_DEV_BAD_PID					FSSH_B_DEV_BAD_PID
#define B_DEV_UNEXPECTED_PID			FSSH_B_DEV_UNEXPECTED_PID
#define B_DEV_DATA_OVERRUN				FSSH_B_DEV_DATA_OVERRUN
#define B_DEV_DATA_UNDERRUN				FSSH_B_DEV_DATA_UNDERRUN
#define B_DEV_FIFO_OVERRUN				FSSH_B_DEV_FIFO_OVERRUN
#define B_DEV_FIFO_UNDERRUN				FSSH_B_DEV_FIFO_UNDERRUN
#define B_DEV_PENDING					FSSH_B_DEV_PENDING
#define B_DEV_MULTIPLE_ERRORS			FSSH_B_DEV_MULTIPLE_ERRORS
#define B_DEV_TOO_LATE					FSSH_B_DEV_TOO_LATE


////////////////////////////////////////////////////////////////////////////////
// #pragma mark - fssh_errno.h

#define ENOERR	FSSH_ENOERR
#define EOK		FSSH_EOK

#define errno	fssh_errno


////////////////////////////////////////////////////////////////////////////////
// #pragma mark - fssh_fcntl.h

/* commands that can be passed to fcntl() */
#define F_DUPFD		FSSH_F_DUPFD
#define F_GETFD		FSSH_F_GETFD
#define F_SETFD		FSSH_F_SETFD
#define F_GETFL		FSSH_F_GETFL
#define F_SETFL		FSSH_F_SETFL
#define F_GETLK		FSSH_F_GETLK
#define F_SETLK		FSSH_F_SETLK
#define F_SETLKW	FSSH_F_SETLKW

/* advisory locking types */
#define F_RDLCK	FSSH_F_RDLCK
#define F_UNLCK	FSSH_F_UNLCK
#define F_WRLCK	FSSH_F_WRLCK

/* file descriptor flags for fcntl() */
#define FD_CLOEXEC	FSSH_FD_CLOEXEC

/* file access modes for open() */
#define O_RDONLY	FSSH_O_RDONLY
#define O_WRONLY	FSSH_O_WRONLY
#define O_RDWR		FSSH_O_RDWR
#define O_ACCMODE	FSSH_O_ACCMODE
#define O_RWMASK	FSSH_O_RWMASK

/* flags for open() */
#define O_EXCL			FSSH_O_EXCL
#define O_CREAT			FSSH_O_CREAT
#define O_TRUNC			FSSH_O_TRUNC
#define O_NOCTTY		FSSH_O_NOCTTY
#define O_NOTRAVERSE	FSSH_O_NOTRAVERSE

/* flags for open() and fcntl() */
#define O_CLOEXEC	FSSH_O_CLOEXEC
#define O_NONBLOCK	FSSH_O_NONBLOCK
#define O_APPEND	FSSH_O_APPEND
#define O_TEXT		FSSH_O_TEXT
#define O_BINARY	FSSH_O_BINARY
#define O_SYNC		FSSH_O_SYNC
#define O_RSYNC		FSSH_O_RSYNC
#define O_DSYNC		FSSH_O_DSYNC

#define O_NOFOLLOW	FSSH_O_NOFOLLOW
#define O_NOCACHE	FSSH_O_NOCACHE
#define O_DIRECT	FSSH_O_DIRECT
#define O_DIRECTORY	FSSH_O_DIRECTORY
#define O_TEMPORARY	FSSH_O_TEMPORARY

#define S_IREAD		FSSH_S_IREAD
#define S_IWRITE	FSSH_S_IWRITE

#define creat	fssh_creat
#define open	fssh_open
#define fcntl	fssh_fcntl


////////////////////////////////////////////////////////////////////////////////
// #pragma mark - fssh_fs_attr.h

#define attr_info	fssh_attr_info


////////////////////////////////////////////////////////////////////////////////
// #pragma mark - fssh_fs_cache.h

#define TRANSACTION_WRITTEN				FSSH_TRANSACTION_WRITTEN
#define TRANSACTION_ABORTED				FSSH_TRANSACTION_ABORTED
#define TRANSACTION_ENDED				FSSH_TRANSACTION_ENDED
#define TRANSACTION_IDLE				FSSH_TRANSACTION_IDLE

#define FILE_MAP_CACHE_ON_DEMAND		FSSH_FILE_MAP_CACHE_ON_DEMAND
#define FILE_MAP_CACHE_ALL				FSSH_FILE_MAP_CACHE_ALL

#define transaction_notification_hook	fssh_transaction_notification_hook

/* transactions */
#define cache_start_transaction			fssh_cache_start_transaction
#define cache_sync_transaction			fssh_cache_sync_transaction
#define cache_end_transaction			fssh_cache_end_transaction
#define cache_abort_transaction			fssh_cache_abort_transaction
#define cache_detach_sub_transaction	fssh_cache_detach_sub_transaction
#define cache_abort_sub_transaction		fssh_cache_abort_sub_transaction
#define cache_start_sub_transaction		fssh_cache_start_sub_transaction
#define cache_add_transaction_listener	fssh_cache_add_transaction_listener
#define cache_remove_transaction_listener fssh_cache_remove_transaction_listener
#define cache_next_block_in_transaction	fssh_cache_next_block_in_transaction
#define cache_blocks_in_transaction		fssh_cache_blocks_in_transaction
#define cache_blocks_in_main_transaction fssh_cache_blocks_in_main_transaction
#define cache_blocks_in_sub_transaction	fssh_cache_blocks_in_sub_transaction

/* block cache */
#define block_cache_delete				fssh_block_cache_delete
#define block_cache_create				fssh_block_cache_create
#define block_cache_sync				fssh_block_cache_sync
#define block_cache_sync_etc			fssh_block_cache_sync_etc
#define block_cache_discard				fssh_block_cache_discard
#define block_cache_make_writable		fssh_block_cache_make_writable
#define block_cache_get_writable_etc	fssh_block_cache_get_writable_etc
#define block_cache_get_writable		fssh_block_cache_get_writable
#define block_cache_get_empty			fssh_block_cache_get_empty
#define block_cache_get_etc				fssh_block_cache_get_etc
#define block_cache_get					fssh_block_cache_get
#define block_cache_set_dirty			fssh_block_cache_set_dirty
#define block_cache_put					fssh_block_cache_put

/* file cache */
#define file_cache_create				fssh_file_cache_create
#define file_cache_delete				fssh_file_cache_delete
#define file_cache_enable				fssh_file_cache_enable
#define file_cache_disable				fssh_file_cache_disable
#define file_cache_set_size				fssh_file_cache_set_size
#define file_cache_sync					fssh_file_cache_sync

#define file_cache_read					fssh_file_cache_read
#define file_cache_write				fssh_file_cache_write

/* file map */
#define file_map_create					fssh_file_map_create
#define file_map_delete					fssh_file_map_delete
#define file_map_set_size				fssh_file_map_set_size
#define file_map_invalidate				fssh_file_map_invalidate
#define file_map_translate				fssh_file_map_translate

/* entry cache */
#define entry_cache_add					fssh_entry_cache_add
#define entry_cache_remove				fssh_entry_cache_remove

////////////////////////////////////////////////////////////////////////////////
// #pragma mark - fssh_fs_index.h

#define index_info	fssh_index_info


////////////////////////////////////////////////////////////////////////////////
// #pragma mark - fssh_fs_info.h

/* fs_info.flags */
#define	B_FS_IS_READONLY				FSSH_B_FS_IS_READONLY
#define	B_FS_IS_REMOVABLE				FSSH_B_FS_IS_REMOVABLE
#define	B_FS_IS_PERSISTENT				FSSH_B_FS_IS_PERSISTENT
#define	B_FS_IS_SHARED					FSSH_B_FS_IS_SHARED
#define	B_FS_HAS_MIME					FSSH_B_FS_HAS_MIME
#define	B_FS_HAS_ATTR					FSSH_B_FS_HAS_ATTR
#define	B_FS_HAS_QUERY					FSSH_B_FS_HAS_QUERY
// those additions are preliminary and may be removed
#define B_FS_HAS_SELF_HEALING_LINKS		FSSH_B_FS_HAS_SELF_HEALING_LINKS
#define B_FS_HAS_ALIASES				FSSH_B_FS_HAS_ALIASES
#define B_FS_SUPPORTS_NODE_MONITORING	FSSH_B_FS_SUPPORTS_NODE_MONITORING

#define fs_info	fssh_fs_info


////////////////////////////////////////////////////////////////////////////////
// #pragma mark - fssh_fs_interface.h

#define mount_id	fssh_mount_id
#define vnode_id	fssh_vnode_id

// TODO: These two don't belong here!
#define IORequest	FSSHIORequest
#define io_request	fssh_io_request

/* additional flags passed to write_stat() */
#define B_STAT_SIZE_INSECURE	FSSH_B_STAT_SIZE_INSECURE

/* passed to write_fs_info() */
#define FS_WRITE_FSINFO_NAME	FSSH_FS_WRITE_FSINFO_NAME

#define file_io_vec	fssh_file_io_vec

#define B_CURRENT_FS_API_VERSION	FSSH_B_CURRENT_FS_API_VERSION

// flags for publish_vnode() and fs_volume_ops::get_vnode()
#define B_VNODE_PUBLISH_REMOVED					FSSH_B_VNODE_PUBLISH_REMOVED
#define B_VNODE_DONT_CREATE_SPECIAL_SUB_NODE	FSSH_B_VNODE_DONT_CREATE_SPECIAL_SUB_NODE

#define fs_volume				fssh_fs_volume
#define fs_volume_ops			fssh_fs_volume_ops
#define fs_vnode				fssh_fs_vnode
#define fs_vnode_ops			fssh_fs_vnode_ops
#define file_system_module_info	fssh_file_system_module_info


/* file system add-ons only prototypes */
#define iterative_io_get_vecs		fssh_iterative_io_get_vecs
#define iterative_io_finished		fssh_iterative_io_finished

#define new_vnode 					fssh_new_vnode
#define publish_vnode				fssh_publish_vnode
#define get_vnode					fssh_get_vnode
#define put_vnode					fssh_put_vnode
#define acquire_vnode				fssh_acquire_vnode
#define remove_vnode				fssh_remove_vnode
#define unremove_vnode				fssh_unremove_vnode
#define get_vnode_removed			fssh_get_vnode_removed
#define volume_for_vnode			fssh_volume_for_vnode
#define read_pages					fssh_read_pages
#define write_pages					fssh_write_pages
#define read_file_io_vec_pages		fssh_read_file_io_vec_pages
#define write_file_io_vec_pages		fssh_write_file_io_vec_pages
#define do_fd_io					fssh_do_fd_io
#define do_iterative_fd_io			fssh_do_iterative_fd_io

#define notify_entry_created		fssh_notify_entry_created
#define notify_entry_removed		fssh_notify_entry_removed
#define notify_entry_moved			fssh_notify_entry_moved
#define notify_stat_changed			fssh_notify_stat_changed
#define notify_attribute_changed	fssh_notify_attribute_changed

#define notify_query_entry_created	fssh_notify_query_entry_created
#define notify_query_entry_removed	fssh_notify_query_entry_removed
#define notify_query_attr_changed	fssh_notify_query_attr_changed


////////////////////////////////////////////////////////////////////////////////
// #pragma mark - fssh_fs_query.h

#define B_LIVE_QUERY		FSSH_B_LIVE_QUERY
#define B_QUERY_NON_INDEXED	FSSH_B_QUERY_NON_INDEXED


////////////////////////////////////////////////////////////////////////////////
// #pragma mark - fssh_fs_volume.h

#define B_MOUNT_READ_ONLY		FSSH_B_MOUNT_READ_ONLY
#define B_MOUNT_VIRTUAL_DEVICE	FSSH_B_MOUNT_VIRTUAL_DEVICE

/* unmount flags */
#define B_FORCE_UNMOUNT			FSSH_B_FORCE_UNMOUNT


////////////////////////////////////////////////////////////////////////////////
// #pragma mark - fssh_kernel_export.h

/* kernel threads */
#define spawn_kernel_thread	fssh_spawn_kernel_thread

/* misc */
#define user_memcpy			fssh_user_memcpy

/* primitive kernel debugging facilities */
#define dprintf				fssh_dprintf
#define kprintf				fssh_kprintf
#define dump_block			fssh_dump_block
#define panic				fssh_panic
#define kernel_debugger		fssh_kernel_debugger
#define parse_expression	fssh_parse_expression

#define debugger_command_hook	fssh_debugger_command_hook
#define add_debugger_command	fssh_add_debugger_command
#define remove_debugger_command	fssh_remove_debugger_command


////////////////////////////////////////////////////////////////////////////////
// #pragma mark - fssh_lock.h

#define mutex						fssh_mutex
#define rw_lock						fssh_rw_lock
#define recursive_lock				fssh_recursive_lock

#define MUTEX_FLAG_CLONE_NAME		FSSH_MUTEX_FLAG_CLONE_NAME
#define RW_LOCK_FLAG_CLONE_NAME		FSSH_RW_LOCK_FLAG_CLONE_NAME

#define ASSERT_LOCKED_RECURSIVE(r)	FSSH_ASSERT_LOCKED_RECURSIVE(r)
#define ASSERT_LOCKED_MUTEX(m)		FSSH_ASSERT_LOCKED_MUTEX(m)
#define ASSERT_WRITE_LOCKED_RW_LOCK(l) FSSH_ASSERT_WRITE_LOCKED_RW_LOCK(l)
#define ASSERT_READ_LOCKED_RW_LOCK(l) FSSH_ASSERT_READ_LOCKED_RW_LOCK(l)

#define MUTEX_INITIALIZER(name)		FSSH_MUTEX_INITIALIZER(name)
#define RECURSIVE_LOCK_INITIALIZER(name) FSSH_RECURSIVE_LOCK_INITIALIZER(name)
#define RW_LOCK_INITIALIZER(name)	FSSH_RW_LOCK_INITIALIZER(name)

#define	recursive_lock_init			fssh_recursive_lock_init
#define recursive_lock_init_etc		fssh_recursive_lock_init_etc
#define recursive_lock_destroy		fssh_recursive_lock_destroy
#define recursive_lock_lock			fssh_recursive_lock_lock
#define recursive_lock_trylock		fssh_recursive_lock_trylock
#define recursive_lock_unlock		fssh_recursive_lock_unlock
#define recursive_lock_get_recursion fssh_recursive_lock_get_recursion

#define rw_lock_init				fssh_rw_lock_init
#define rw_lock_init_etc			fssh_rw_lock_init_etc
#define rw_lock_destroy				fssh_rw_lock_destroy
#define rw_lock_read_lock			fssh_rw_lock_read_lock
#define rw_lock_read_unlock			fssh_rw_lock_read_unlock
#define rw_lock_write_lock			fssh_rw_lock_write_lock
#define rw_lock_write_unlock		fssh_rw_lock_write_unlock

#define mutex_init					fssh_mutex_init
#define mutex_init_etc				fssh_mutex_init_etc
#define mutex_destroy				fssh_mutex_destroy
#define mutex_lock					fssh_mutex_lock
#define mutex_trylock				fssh_mutex_trylock
#define mutex_unlock				fssh_mutex_unlock
#define mutex_transfer_lock			fssh_mutex_transfer_lock


////////////////////////////////////////////////////////////////////////////////
// #pragma mark - fssh_module.h

#define module_info	fssh_module_info

/* module standard operations */
#define B_MODULE_INIT	FSSH_B_MODULE_INIT
#define B_MODULE_UNINIT	FSSH_B_MODULE_UNINIT

/* module flags */
#define B_KEEP_LOADED	FSSH_B_KEEP_LOADED


#define module_dependency	fssh_module_dependency


////////////////////////////////////////////////////////////////////////////////
// #pragma mark - fssh_node_monitor.h

#define B_STOP_WATCHING			FSSH_B_STOP_WATCHING
#define B_WATCH_NAME			FSSH_B_WATCH_NAME
#define B_WATCH_STAT			FSSH_B_WATCH_STAT
#define B_WATCH_ATTR			FSSH_B_WATCH_ATTR
#define B_WATCH_DIRECTORY		FSSH_B_WATCH_DIRECTORY
#define B_WATCH_ALL				FSSH_B_WATCH_ALL
#define B_WATCH_MOUNT			FSSH_B_WATCH_MOUNT
#define B_WATCH_INTERIM_STAT	FSSH_B_WATCH_INTERIM_STAT

#define B_ENTRY_CREATED		FSSH_B_ENTRY_CREATED
#define B_ENTRY_REMOVED		FSSH_B_ENTRY_REMOVED
#define B_ENTRY_MOVED		FSSH_B_ENTRY_MOVED
#define B_STAT_CHANGED		FSSH_B_STAT_CHANGED
#define B_ATTR_CHANGED		FSSH_B_ATTR_CHANGED
#define B_DEVICE_MOUNTED	FSSH_B_DEVICE_MOUNTED
#define B_DEVICE_UNMOUNTED	FSSH_B_DEVICE_UNMOUNTED

#define B_ATTR_CREATED		FSSH_B_ATTR_CREATED
#define B_ATTR_REMOVED		FSSH_B_ATTR_REMOVED

#define B_STAT_MODE					FSSH_B_STAT_MODE
#define B_STAT_UID					FSSH_B_STAT_UID
#define B_STAT_GID					FSSH_B_STAT_GID
#define B_STAT_SIZE					FSSH_B_STAT_SIZE
#define B_STAT_ACCESS_TIME			FSSH_B_STAT_ACCESS_TIME
#define B_STAT_MODIFICATION_TIME	FSSH_B_STAT_MODIFICATION_TIME
#define B_STAT_CREATION_TIME		FSSH_B_STAT_CREATION_TIME
#define B_STAT_CHANGE_TIME			FSSH_B_STAT_CHANGE_TIME
#define B_STAT_INTERIM_UPDATE		FSSH_B_STAT_INTERIM_UPDATE


////////////////////////////////////////////////////////////////////////////////
// #pragma mark - fssh_os.h

/* System constants */

#define B_OS_NAME_LENGTH	FSSH_B_OS_NAME_LENGTH
#define B_PAGE_SIZE			FSSH_B_PAGE_SIZE
#define B_INFINITE_TIMEOUT	FSSH_B_INFINITE_TIMEOUT


/* Types */

#define area_id		fssh_area_id
#define port_id		fssh_port_id
#define sem_id		fssh_sem_id
#define team_id		fssh_team_id
#define thread_id	fssh_thread_id


/* Semaphores */

#define sem_info	fssh_sem_info

/* semaphore flags */
#define B_CAN_INTERRUPT			FSSH_B_CAN_INTERRUPT
#define B_CHECK_PERMISSION		FSSH_B_CHECK_PERMISSION
#define B_KILL_CAN_INTERRUPT	FSSH_B_KILL_CAN_INTERRUPT

#define B_DO_NOT_RESCHEDULE			FSSH_B_DO_NOT_RESCHEDULE
#define B_RELEASE_ALL				FSSH_B_RELEASE_ALL
#define B_RELEASE_IF_WAITING_ONLY	FSSH_B_RELEASE_IF_WAITING_ONLY

#define create_sem		fssh_create_sem
#define delete_sem		fssh_delete_sem
#define acquire_sem		fssh_acquire_sem
#define acquire_sem_etc	fssh_acquire_sem_etc
#define release_sem		fssh_release_sem
#define release_sem_etc	fssh_release_sem_etc
#define get_sem_count	fssh_get_sem_count
#define set_sem_owner	fssh_set_sem_owner

#define get_sem_info(sem, info)					fssh_get_sem_info(sem, info)
#define get_next_sem_info(team, cookie, info)	fssh_get_next_sem_info(team, cookie, info)

#define B_TIMEOUT			FSSH_B_TIMEOUT
#define B_RELATIVE_TIMEOUT	FSSH_B_RELATIVE_TIMEOUT
#define B_ABSOLUTE_TIMEOUT	FSSH_B_ABSOLUTE_TIMEOUT


/* Teams */

#define B_CURRENT_TEAM	FSSH_B_CURRENT_TEAM
#define B_SYSTEM_TEAM	FSSH_B_SYSTEM_TEAM


/* Threads */

#define B_THREAD_RUNNING	FSSH_B_THREAD_RUNNING
#define B_THREAD_READY		FSSH_B_THREAD_READY
#define B_THREAD_RECEIVING	FSSH_B_THREAD_RECEIVING
#define B_THREAD_ASLEEP		FSSH_B_THREAD_ASLEEP
#define B_THREAD_SUSPENDED	FSSH_B_THREAD_SUSPENDED
#define B_THREAD_WAITING	FSSH_B_THREAD_WAITING

#define thread_state		fssh_thread_state
#define thread_info			fssh_thread_info

#define B_IDLE_PRIORITY					FSSH_B_IDLE_PRIORITY
#define B_LOWEST_ACTIVE_PRIORITY		FSSH_B_LOWEST_ACTIVE_PRIORITY
#define B_LOW_PRIORITY					FSSH_B_LOW_PRIORITY
#define B_NORMAL_PRIORITY				FSSH_B_NORMAL_PRIORITY
#define B_DISPLAY_PRIORITY				FSSH_B_DISPLAY_PRIORITY
#define B_URGENT_DISPLAY_PRIORITY		FSSH_B_URGENT_DISPLAY_PRIORITY
#define B_REAL_TIME_DISPLAY_PRIORITY	FSSH_B_REAL_TIME_DISPLAY_PRIORITY
#define B_URGENT_PRIORITY				FSSH_B_URGENT_PRIORITY
#define B_REAL_TIME_PRIORITY			FSSH_B_REAL_TIME_PRIORITY

#define B_FIRST_REAL_TIME_PRIORITY		FSSH_B_FIRST_REAL_TIME_PRIORITY
#define B_MIN_PRIORITY					FSSH_B_MIN_PRIORITY
#define B_MAX_PRIORITY					FSSH_B_MAX_PRIORITY

#define B_SYSTEM_TIMEBASE				FSSH_B_SYSTEM_TIMEBASE

#define thread_func			fssh_thread_func
#define thread_entry		fssh_thread_entry

#define spawn_thread		fssh_spawn_thread
#define kill_thread			fssh_kill_thread
#define resume_thread		fssh_resume_thread
#define suspend_thread		fssh_suspend_thread

#define rename_thread		fssh_rename_thread
#define set_thread_priority	fssh_set_thread_priority
#define exit_thread			fssh_exit_thread
#define wait_for_thread 	fssh_wait_for_thread
#define on_exit_thread		fssh_on_exit_thread

#define find_thread			fssh_find_thread

#define send_data			fssh_send_data
#define receive_data		fssh_receive_data
#define has_data			fssh_has_data

#define snooze				fssh_snooze
#define snooze_etc			fssh_snooze_etc
#define snooze_until		fssh_snooze_until

#define get_thread_info(id, info)					fssh_get_thread_info(id, info)
#define get_next_thread_info(team, cookie, info)	fssh_get_next_thread_info(team, cookie, info)


/* Time */

#define real_time_clock			fssh_real_time_clock
#define set_real_time_clock		fssh_set_real_time_clock
#define real_time_clock_usecs	fssh_real_time_clock_usecs
#define set_timezone			fssh_set_timezone
#define system_time				fssh_system_time


////////////////////////////////////////////////////////////////////////////////
// #pragma mark - fssh_stat.h

#define stat	fssh_stat

// struct stat fields
#undef st_dev
#undef st_ino
#undef st_mode
#undef st_nlink
#undef st_uid
#undef st_gid
#undef st_size
#undef st_rdev
#undef st_blksize
#undef st_atime
#undef st_mtime
#undef st_ctime
#undef st_crtime
#undef st_type

#define st_dev		fssh_st_dev
#define st_ino		fssh_st_ino
#define st_mode		fssh_st_mode
#define st_nlink	fssh_st_nlink
#define st_uid		fssh_st_uid
#define st_gid		fssh_st_gid
#define st_size		fssh_st_size
#define st_rdev		fssh_st_rdev
#define st_blksize	fssh_st_blksize
#define st_atim		fssh_st_atim
#define st_mtim		fssh_st_mtim
#define st_ctim		fssh_st_ctim
#define st_crtim	fssh_st_crtim
#define st_atime	fssh_st_atime
#define st_mtime	fssh_st_mtime
#define st_ctime	fssh_st_ctime
#define st_crtime	fssh_st_crtime
#define st_type		fssh_st_type
#define st_blocks	fssh_st_blocks

/* extended file types */
#define S_ATTR_DIR			FSSH_S_ATTR_DIR
#define S_ATTR				FSSH_S_ATTR
#define S_INDEX_DIR			FSSH_S_INDEX_DIR
#define S_STR_INDEX			FSSH_S_STR_INDEX
#define S_INT_INDEX			FSSH_S_INT_INDEX
#define S_UINT_INDEX		FSSH_S_UINT_INDEX
#define S_LONG_LONG_INDEX	FSSH_S_LONG_LONG_INDEX
#define S_ULONG_LONG_INDEX	FSSH_S_ULONG_LONG_INDEX
#define S_FLOAT_INDEX		FSSH_S_FLOAT_INDEX
#define S_DOUBLE_INDEX		FSSH_S_DOUBLE_INDEX
#define S_ALLOW_DUPS		FSSH_S_ALLOW_DUPS

/* link types */
#define S_LINK_SELF_HEALING	FSSH_S_LINK_SELF_HEALING
#define S_LINK_AUTO_DELETE	FSSH_S_LINK_AUTO_DELETE

/* standard file types */
#define S_IFMT	FSSH_S_IFMT
#define S_IFLNK	FSSH_S_IFLNK
#define S_IFREG	FSSH_S_IFREG
#define S_IFBLK	FSSH_S_IFBLK
#define S_IFDIR	FSSH_S_IFDIR
#define S_IFCHR	FSSH_S_IFCHR
#define S_IFIFO	FSSH_S_IFIFO

#define S_ISREG(mode)	FSSH_S_ISREG(mode)
#define S_ISLNK(mode)	FSSH_S_ISLNK(mode)
#define S_ISBLK(mode)	FSSH_S_ISBLK(mode)
#define S_ISDIR(mode)	FSSH_S_ISDIR(mode)
#define S_ISCHR(mode)	FSSH_S_ISCHR(mode)
#define S_ISFIFO(mode)	FSSH_S_ISFIFO(mode)
#define S_ISINDEX(mode)	FSSH_S_ISINDEX(mode)

#define S_IUMSK	FSSH_S_IUMSK

#define S_ISUID	FSSH_S_ISUID
#define S_ISGID	FSSH_S_ISGID

#define S_ISVTX	FSSH_S_ISVTX

#define S_IRWXU	FSSH_S_IRWXU
#define S_IRUSR	FSSH_S_IRUSR
#define S_IWUSR	FSSH_S_IWUSR
#define S_IXUSR	FSSH_S_IXUSR
#define S_IRWXG	FSSH_S_IRWXG
#define S_IRGRP	FSSH_S_IRGRP
#define S_IWGRP	FSSH_S_IWGRP
#define S_IXGRP	FSSH_S_IXGRP
#define S_IRWXO	FSSH_S_IRWXO
#define S_IROTH	FSSH_S_IROTH
#define S_IWOTH	FSSH_S_IWOTH
#define S_IXOTH	FSSH_S_IXOTH

#define ACCESSPERMS	FSSH_ACCESSPERMS
#define ALLPERMS	FSSH_ALLPERMS
#define DEFFILEMODE	FSSH_DEFFILEMODE

#define chmod	fssh_chmod
#define fchmod	fssh_fchmod
#define mkdir	fssh_mkdir
#define mkfifo	fssh_mkfifo
#define umask	fssh_umask

//#define stat	fssh_stat
	// Already mapped above for "struct stat".
#define fstat	fssh_fstat
#define lstat	fssh_lstat


////////////////////////////////////////////////////////////////////////////////
// #pragma mark - fssh_stdio.h

#define EOF	FSSH_EOF

/* file operations */
#define remove		fssh_remove
#define rename		fssh_rename

/* formatted I/O */
#define sprintf		fssh_sprintf
#define snprintf	fssh_snprintf
#define vsprintf	fssh_vsprintf
#define vsnprintf	fssh_vsnprintf

#define sscanf		fssh_sscanf
#define vsscanf		fssh_vsscanf


////////////////////////////////////////////////////////////////////////////////
// #pragma mark - fssh_string.h

/* memXXX() functions */
#define memchr		fssh_memchr
#define memcmp		fssh_memcmp
#define memcpy		fssh_memcpy
#define memccpy		fssh_memccpy
#define memmove		fssh_memmove
#define memset		fssh_memset

/* string functions */
#define strcpy		fssh_strcpy
#define strncpy		fssh_strncpy
#define strcat		fssh_strcat
#define strncat		fssh_strncat
#define strlen		fssh_strlen
#define strcmp		fssh_strcmp
#define strncmp		fssh_strncmp
#define strchr		fssh_strchr
#define strrchr		fssh_strrchr
#define strstr		fssh_strstr
#define strchrnul	fssh_strchrnul
#define strpbrk		fssh_strpbrk
#define strtok		fssh_strtok
#define strtok_r	fssh_strtok_r
#define strspn		fssh_strspn
#define strcspn		fssh_strcspn
#define strcoll		fssh_strcoll
#define strxfrm		fssh_strxfrm
#define strerror	fssh_strerror
#define strerror_r	fssh_strerror_r

/* non-standard string functions */
#define strcasecmp	fssh_strcasecmp
#define strncasecmp	fssh_strncasecmp
#define strcasestr	fssh_strcasestr
#define strdup		fssh_strdup
#define stpcpy		fssh_stpcpy
#define strtcopy	fssh_strtcopy
#define strlcat		fssh_strlcat
#define strlcpy		fssh_strlcpy
#define strnlen		fssh_strnlen

#define ffs			fssh_ffs
#define index		fssh_index
#define rindex		fssh_rindex


////////////////////////////////////////////////////////////////////////////////
// #pragma mark - fssh_time.h

#define clock_t		fssh_clock_t
#define time_t		fssh_time_t
#define suseconds_t	fssh_suseconds_t
#define useconds_t	fssh_useconds_t

#undef CLOCKS_PER_SEC
#undef CLK_TCK
#undef MAX_TIMESTR

#define CLOCKS_PER_SEC	FSSH_CLOCKS_PER_SEC
#define CLK_TCK			FSSH_CLK_TCK
#define MAX_TIMESTR		FSSH_MAX_TIMESTR

#define timespec	fssh_timespec
#define itimerspec	fssh_itimerspec
#define tm			fssh_tm

/* special timezone support */
#define tzname		fssh_tzname
#define daylight	fssh_daylight
#define timezone	fssh_timezone

#define clock		fssh_clock
#define difftime	fssh_difftime
#define mktime		fssh_mktime
#define time		fssh_time
#define asctime		fssh_asctime
#define asctime_r	fssh_asctime_r
#define ctime		fssh_ctime
#define ctime_r		fssh_ctime_r
#define gmtime		fssh_gmtime
#define gmtime_r	fssh_gmtime_r
#define localtime	fssh_localtime
#define localtime_r	fssh_localtime_r
#define strftime	fssh_strftime
#define strptime	fssh_strptime

/* special timezone support */
#define tzset	fssh_tzset
#define stime	fssh_stime


////////////////////////////////////////////////////////////////////////////////
// #pragma mark - fssh_type_constants.h

#define B_ANY_TYPE						FSSH_B_ANY_TYPE
#define B_ATOM_TYPE						FSSH_B_ATOM_TYPE
#define B_ATOMREF_TYPE					FSSH_B_ATOMREF_TYPE
#define B_BOOL_TYPE						FSSH_B_BOOL_TYPE
#define B_CHAR_TYPE						FSSH_B_CHAR_TYPE
#define B_COLOR_8_BIT_TYPE				FSSH_B_COLOR_8_BIT_TYPE
#define B_DOUBLE_TYPE					FSSH_B_DOUBLE_TYPE
#define B_FLOAT_TYPE					FSSH_B_FLOAT_TYPE
#define B_GRAYSCALE_8_BIT_TYPE			FSSH_B_GRAYSCALE_8_BIT_TYPE
#define B_INT16_TYPE					FSSH_B_INT16_TYPE
#define B_INT32_TYPE					FSSH_B_INT32_TYPE
#define B_INT64_TYPE					FSSH_B_INT64_TYPE
#define B_INT8_TYPE						FSSH_B_INT8_TYPE
#define B_LARGE_ICON_TYPE				FSSH_B_LARGE_ICON_TYPE
#define B_MEDIA_PARAMETER_GROUP_TYPE	FSSH_B_MEDIA_PARAMETER_GROUP_TYPE
#define B_MEDIA_PARAMETER_TYPE			FSSH_B_MEDIA_PARAMETER_TYPE
#define B_MEDIA_PARAMETER_WEB_TYPE		FSSH_B_MEDIA_PARAMETER_WEB_TYPE
#define B_MESSAGE_TYPE					FSSH_B_MESSAGE_TYPE
#define B_MESSENGER_TYPE				FSSH_B_MESSENGER_TYPE
#define B_MIME_TYPE						FSSH_B_MIME_TYPE
#define B_MINI_ICON_TYPE				FSSH_B_MINI_ICON_TYPE
#define B_MONOCHROME_1_BIT_TYPE			FSSH_B_MONOCHROME_1_BIT_TYPE
#define B_OBJECT_TYPE					FSSH_B_OBJECT_TYPE
#define B_OFF_T_TYPE					FSSH_B_OFF_T_TYPE
#define B_PATTERN_TYPE					FSSH_B_PATTERN_TYPE
#define B_POINTER_TYPE					FSSH_B_POINTER_TYPE
#define B_POINT_TYPE					FSSH_B_POINT_TYPE
#define B_PROPERTY_INFO_TYPE			FSSH_B_PROPERTY_INFO_TYPE
#define B_RAW_TYPE						FSSH_B_RAW_TYPE
#define B_RECT_TYPE						FSSH_B_RECT_TYPE
#define B_REF_TYPE						FSSH_B_REF_TYPE
#define B_RGB_32_BIT_TYPE				FSSH_B_RGB_32_BIT_TYPE
#define B_RGB_COLOR_TYPE				FSSH_B_RGB_COLOR_TYPE
#define B_SIZE_T_TYPE					FSSH_B_SIZE_T_TYPE
#define B_SSIZE_T_TYPE					FSSH_B_SSIZE_T_TYPE
#define B_STRING_TYPE					FSSH_B_STRING_TYPE
#define B_TIME_TYPE						FSSH_B_TIME_TYPE
#define B_UINT16_TYPE					FSSH_B_UINT16_TYPE
#define B_UINT32_TYPE					FSSH_B_UINT32_TYPE
#define B_UINT64_TYPE					FSSH_B_UINT64_TYPE
#define B_UINT8_TYPE					FSSH_B_UINT8_TYPE
#define B_VECTOR_ICON_TYPE				FSSH_B_VECTOR_ICON_TYPE
#define B_ASCII_TYPE					FSSH_B_ASCII_TYPE
#define B_XATTR_TYPE					FSSH_B_XATTR_TYPE
#define B_NETWORK_ADDRESS_TYPE			FSSH_B_NETWORK_ADDRESS_TYPE
#define B_MIME_STRING_TYPE				FSSH_B_MIME_STRING_TYPE

//----- System-wide MIME types for handling URL's ------------------------------

#define B_URL_HTTP		FSSH_B_URL_HTTP
#define B_URL_HTTPS		FSSH_B_URL_HTTPS
#define B_URL_FTP		FSSH_B_URL_FTP
#define B_URL_GOPHER	FSSH_B_URL_GOPHER
#define B_URL_MAILTO	FSSH_B_URL_MAILTO
#define B_URL_NEWS		FSSH_B_URL_NEWS
#define B_URL_NNTP		FSSH_B_URL_NNTP
#define B_URL_TELNET	FSSH_B_URL_TELNET
#define B_URL_RLOGIN	FSSH_B_URL_RLOGIN
#define B_URL_TN3270	FSSH_B_URL_TN3270
#define B_URL_WAIS		FSSH_B_URL_WAIS
#define B_URL_FILE		FSSH_B_URL_FILE


////////////////////////////////////////////////////////////////////////////////
// #pragma mark - fssh_types.h

#define addr_t			fssh_addr_t
#define phys_addr_t		fssh_phys_addr_t
#define phys_size_t		fssh_phys_size_t
#define generic_addr_t	fssh_generic_addr_t
#define generic_size_t	fssh_generic_size_t

#define dev_t			fssh_dev_t
#define ino_t			fssh_ino_t

#define size_t			fssh_size_t
#define ssize_t			fssh_ssize_t
#define off_t			fssh_off_t

#define bigtime_t		fssh_bigtime_t

#define status_t		fssh_status_t
#define type_code		fssh_type_code

#define mode_t			fssh_mode_t
#define nlink_t			fssh_nlink_t
#define uid_t			fssh_uid_t
#define gid_t			fssh_gid_t
#define pid_t			fssh_pid_t

/* printf()/scanf() format strings for [u]int* types */
#define B_PRId8			FSSH_B_PRId8
#define B_PRIi8			FSSH_B_PRIi8
#define B_PRId16		FSSH_B_PRId16
#define B_PRIi16		FSSH_B_PRIi16
#define B_PRId32		FSSH_B_PRId32
#define B_PRIi32		FSSH_B_PRIi32
#define B_PRId64		FSSH_B_PRId64
#define B_PRIi64		FSSH_B_PRIi64
#define B_PRIu8			FSSH_B_PRIu8
#define B_PRIo8			FSSH_B_PRIo8
#define B_PRIx8			FSSH_B_PRIx8
#define B_PRIX8			FSSH_B_PRIX8
#define B_PRIu16		FSSH_B_PRIu16
#define B_PRIo16		FSSH_B_PRIo16
#define B_PRIx16		FSSH_B_PRIx16
#define B_PRIX16		FSSH_B_PRIX16
#define B_PRIu32		FSSH_B_PRIu32
#define B_PRIo32		FSSH_B_PRIo32
#define B_PRIx32		FSSH_B_PRIx32
#define B_PRIX32		FSSH_B_PRIX32
#define B_PRIu64		FSSH_B_PRIu64
#define B_PRIo64		FSSH_B_PRIo64
#define B_PRIx64		FSSH_B_PRIx64
#define B_PRIX64		FSSH_B_PRIX64

#define B_SCNd8			FSSH_B_SCNd8
#define B_SCNi8			FSSH_B_SCNi8
#define B_SCNd16		FSSH_B_SCNd16
#define B_SCNi16		FSSH_B_SCNi16
#define B_SCNd32		FSSH_B_SCNd32
#define B_SCNi32		FSSH_B_SCNi32
#define B_SCNd64		FSSH_B_SCNd64
#define B_SCNi64		FSSH_B_SCNi64
#define B_SCNu8			FSSH_B_SCNu8
#define B_SCNo8			FSSH_B_SCNo8
#define B_SCNx8			FSSH_B_SCNx8
#define B_SCNu16		FSSH_B_SCNu16
#define B_SCNu16		FSSH_B_SCNu16
#define B_SCNx16		FSSH_B_SCNx16
#define B_SCNu32		FSSH_B_SCNu32
#define B_SCNo32		FSSH_B_SCNo32
#define B_SCNx32		FSSH_B_SCNx32
#define B_SCNu64		FSSH_B_SCNu64
#define B_SCNo64		FSSH_B_SCNo64
#define B_SCNx64		FSSH_B_SCNx64

/* printf() format strings for some standard types */
/* size_t */
#define B_PRIuSIZE		FSSH_B_PRIuSIZE
#define B_PRIoSIZE		FSSH_B_PRIoSIZE
#define B_PRIxSIZE		FSSH_B_PRIxSIZE
#define B_PRIXSIZE		FSSH_B_PRIXSIZE
/* ssize_t */
#define B_PRIdSSIZE		FSSH_B_PRIdSSIZE
#define B_PRIiSSIZE		FSSH_B_PRIiSSIZE
/* addr_t */
#define B_PRIuADDR		FSSH_B_PRIuADDR
#define B_PRIoADDR		FSSH_B_PRIoADDR
#define B_PRIxADDR		FSSH_B_PRIxADDR
#define B_PRIXADDR		FSSH_B_PRIXADDR
/* phys_addr_t */
#define B_PRIuPHYSADDR	FSSH_B_PRIuPHYSADDR
#define B_PRIoPHYSADDR	FSSH_B_PRIoPHYSADDR
#define B_PRIxPHYSADDR	FSSH_B_PRIxPHYSADDR
#define B_PRIXPHYSADDR	FSSH_B_PRIXPHYSADDR
/* generic_addr_t */
#define B_PRIuGENADDR	FSSH_B_PRIuGENADDR
#define B_PRIoGENADDR	FSSH_B_PRIoGENADDR
#define B_PRIxGENADDR	FSSH_B_PRIxGENADDR
#define B_PRIXGENADDR	FSSH_B_PRIXGENADDR
/* off_t */
#define B_PRIdOFF		FSSH_B_PRIdOFF
#define B_PRIiOFF		FSSH_B_PRIiOFF
/* dev_t */
#define B_PRIdDEV		FSSH_B_PRIdDEV
#define B_PRIiDEV		FSSH_B_PRIiDEV
/* ino_t */
#define B_PRIdINO		FSSH_B_PRIdINO
#define B_PRIiINO		FSSH_B_PRIiINO
/* time_t */
#define B_PRIdTIME		FSSH_B_PRId32
#define B_PRIiTIME		FSSH_B_PRIi32


////////////////////////////////////////////////////////////////////////////////
// #pragma mark - fssh_uio.h

#define iovec		fssh_iovec

#define readv		fssh_readv
#define readv_pos	fssh_readv_pos
#define writev		fssh_writev
#define writev_pos	fssh_writev_pos


////////////////////////////////////////////////////////////////////////////////
// #pragma mark - fssh_unistd.h

/* access modes */
#define R_OK	FSSH_R_OK
#define W_OK	FSSH_W_OK
#define X_OK	FSSH_X_OK
#define F_OK	FSSH_F_OK

/* standard file descriptors */
#define STDIN_FILENO	FSSH_STDIN_FILENO
#define STDOUT_FILENO	FSSH_STDOUT_FILENO
#define STDERR_FILENO	FSSH_STDERR_FILENO

/* lseek() constants */
#define SEEK_SET	FSSH_SEEK_SET
#define SEEK_CUR	FSSH_SEEK_CUR
#define SEEK_END	FSSH_SEEK_END

/* file functions */
#define access		fssh_access

#define chdir		fssh_chdir
#define fchdir		fssh_fchdir
#define getcwd		fssh_getcwd

#define dup			fssh_dup
#define dup2		fssh_dup2
#define close		fssh_close
#define link		fssh_link
#define unlink		fssh_unlink
#define rmdir		fssh_rmdir

#define readlink	fssh_readlink
#define symlink		fssh_symlink

#define ftruncate	fssh_ftruncate
#define truncate	fssh_truncate
#define ioctl		fssh_ioctl

#define read		fssh_read
#define read_pos	fssh_read_pos
#define pread		fssh_pread
#define write		fssh_write
#define write_pos	fssh_write_pos
#define pwrite		fssh_pwrite
#define lseek		fssh_lseek

#define sync		fssh_sync
#define fsync		fssh_fsync

/* access permissions */
#define getegid		fssh_getegid
#define geteuid		fssh_geteuid
#define getgid		fssh_getgid
#define getgroups	fssh_getgroups
#define getuid		fssh_getuid


////////////////////////////////////////////////////////////////////////////////
// int types

#define int8	int8_t
#define uint8	uint8_t
#define int16	int16_t
#define uint16	uint16_t
#define int32	int32_t
#define uint32	uint32_t
#define int64	int64_t
#define uint64	uint64_t

#define vint32	vint32_t


#endif	// _FSSH_API_WRAPPER_H
