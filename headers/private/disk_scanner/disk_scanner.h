// disk_scanner.h

#ifndef _DISKSCANNER_H
#define _DISKSCANNER_H

#include <Drivers.h>
#include <module.h>

struct extended_partition_info;
struct partition_module_info;
struct session_info;
struct session_module_info;

#define DISK_SCANNER_MODULE_NAME		"disk_scanner/disk_scanner/v1"

typedef status_t (*disk_scanner_get_session_module_hook)(int deviceFD,
	off_t deviceSize, int32 blockSize,
	struct session_module_info **sessionModule);
typedef status_t (*disk_scanner_get_partition_module_hook)(int deviceFD,
	const struct session_info *sessionInfo,
	struct partition_module_info **partitionModule);

typedef status_t (*disk_scanner_get_nth_session_info_hook)(int deviceFD,
	int32 index, struct session_info *sessionInfo);
typedef status_t (*disk_scanner_get_nth_partition_info_hook)(int deviceFD,
	const struct session_info *sessionInfo, int32 partitionIndex,
	struct extended_partition_info *partitionInfo);
typedef status_t (*disk_scanner_get_partition_fs_info_hook)(int deviceFD,
	struct extended_partition_info *partitionInfo);

typedef struct disk_scanner_module_info {
	module_info								module;

	disk_scanner_get_session_module_hook		get_session_module;
	disk_scanner_get_partition_module_hook		get_partition_module;
	disk_scanner_get_nth_session_info_hook		get_nth_session_info;
	disk_scanner_get_nth_partition_info_hook	get_nth_partition_info;
	disk_scanner_get_partition_fs_info_hook		get_partition_fs_info;
} disk_scanner_module_info;

/*
	get_session_module:
	------------------

	Searches for a module that can deal with the sessions on the specified
	device and returns a module_info for it. put_module() must be called, when
	done with the module.

	params:
	deviceFD: a device FD
	deviceSize: size of the device in bytes
	blockSize: the logical block size
	sessionModule: buffer the pointer to the found module_info shall be
				   written into

	Returns B_OK, if a module could be found, B_ENTRY_NOT_FOUND, if no
	module was suitable for the job.


	get_partition_module:
	--------------------

	Searches for a module that can deal with the partitions on the specified
	session and returns a module_info for it. put_module() must be called, when
	done with the module.

	params:
	deviceFD: a device FD
	sessionInfo: a complete info about the session the partition resides on
	partitionModule: buffer the pointer to the found module_info shall be
					 written into

	Returns B_OK, if a module could be found, B_ENTRY_NOT_FOUND, if no
	module was suitable for the job.


	get_nth_session_info():
	----------------------

	Fills in all fields of sessionInfo with information about
	the indexth session on the specified device.

	params:
	deviceFD: a device FD
	index: the session index
	sessionInfo: the session info

	The function first checks, whether the device is one that usually has
	sessions (a CD). If so, it tries to find a suitable module and delegates
	the work to that module. Otherwise, it is assumed, that the device does
	not have sessions and for index == 0 the info is filled out with data
	for a "virtual" session, i.e. one that spans the whole device.

	Returns B_OK, if successful, B_ENTRY_NOT_FOUND, if no suitable session
	module could be found or if the session index is out of range.


	get_nth_partition_info():
	------------------------

	Fills in the following fields of partitionInfo with information about
	the indexth partition on the specified session:
	* offset
	* size
	* logical_block_size
	* session
	* partition
	* flags
	* partition_name
	* partition_type
	* partition_code

	params:
	deviceFD: a device FD
	sessionInfo: a complete info about the session the partition resides on
	partitionIndex: partition index
	partitionInfo: the partition info to be filled in

	The function first tries to find a suitable partition module and to
	delagate the work to that module. If no module could be found, for
	partition index == 0 the info is filled out with data for a "virtual"
	partition, i.e. one that spans the whole session.

	Returns B_OK, if successful, B_ENTRY_NOT_FOUND, if the index is out of
	range.


	get_partition_fs_info():
	-----------------------

	Expects partitionInfo to be partially initialized and, if a module could
	be found, that recognizes the FS on the partition, fills in
	the fields:
	* file_system_short_name
	* file_system_long_name
	* volume_name

	The minimally required fields are:
	* offset
	* size
	* logical_block_size

	params:
	deviceFD: a device FD
	partitionInfo: the partition info

	Returns B_OK, if successful (i.e. the FS was recognized),
	B_ENTRY_NOT_FOUND, if no suitable module could be found.
*/

#endif	// _DISKSCANNER_H
