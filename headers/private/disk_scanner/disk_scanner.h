//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file disk_scanner.h
	hook function declarations for disk_scanner module
*/


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
	int32 index, struct session_info *sessionInfo,
	struct session_module_info **sessionModule);
typedef status_t (*disk_scanner_get_nth_partition_info_hook)(int deviceFD,
	const struct session_info *sessionInfo, int32 partitionIndex,
	struct extended_partition_info *partitionInfo, char *partitionMapName,
	struct partition_module_info **partitionModule);
typedef status_t (*disk_scanner_get_partition_fs_info_hook)(int deviceFD,
	struct extended_partition_info *partitionInfo);
typedef status_t (*disk_scanner_get_partitioning_params_hook)(int deviceFD,
	const struct session_info *sessionInfo, const char *identifier,
	char *buffer, size_t bufferSize, size_t *actualSize);
typedef status_t (*disk_scanner_partition_hook)(int deviceFD,
	const struct session_info *sessionInfo, const char *identifier,
	const char *parameters);

typedef struct disk_scanner_module_info {
	module_info									module;

	disk_scanner_get_session_module_hook		get_session_module;
	disk_scanner_get_partition_module_hook		get_partition_module;
	disk_scanner_get_nth_session_info_hook		get_nth_session_info;
	disk_scanner_get_nth_partition_info_hook	get_nth_partition_info;
	disk_scanner_get_partition_fs_info_hook		get_partition_fs_info;
	disk_scanner_get_partitioning_params_hook	get_partitioning_params;
	disk_scanner_partition_hook					partition;
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
	sessionModule: pointer to session_module_info*

	The function first checks, whether the device is one that usually has
	sessions (a CD). If so, it tries to find a suitable module and delegates
	the work to that module. Otherwise, it is assumed, that the device does
	not have sessions and for index == 0 the info is filled out with data
	for a "virtual" session, i.e. one that spans the whole device.

	If sessionModule is NULL, it is ignored. Otherwise, if it points to a
	NULL pointer, then a pointer to the module that has been recognized to
	handle the session layout is returned therein (NULL in case of a
	a device other than a CD). If it points to a non-NULL pointer, the
	referred to session module is used get the session info. The caller is
	responsible to put_module() the module returned in sessionModule.
	To sum it up, the feature can be used to avoid recognizing and
	re- and unloading the respective module when iterating through the
	sessions of a device -- before the first call *sessionModule is to be
	initialized to NULL, after the last call, put_module() has to be invoked,
	if *sessionModule is not NULL.

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

	params:
	deviceFD: a device FD
	sessionInfo: a complete info about the session the partition resides on
	partitionIndex: partition index
	partitionInfo: the partition info to be filled in
	partitionMapName: Pointer to a pre-allocated char buffer of minimal
					  size B_FILE_NAME_LENGTH, into which the short name of
					  the partitioning system shall be written. The result is
					  the empty string (""), if no partitioning system is used
					  (e.g. for floppies). May be NULL.
	partitionModule: pointer to partition_module_info*

	The function first tries to find a suitable partition module and to
	delagate the work to that module. If no module could be found, for
	partition index == 0 the info is filled out with data for a "virtual"
	partition, i.e. one that spans the whole session.

	If partitionModule is NULL, it is ignored. Otherwise, if it points to a
	NULL pointer, then a pointer to the module that has been recognized to
	handle the partition layout is returned therein (NULL in case none
	could be found). If it points to a non-NULL pointer, the referred to
	partition module is used get the partition info. The caller is
	responsible to put_module() the module returned in partitionModule.
	To sum it up, the feature can be used to avoid recognizing and
	re- and unloading the respective module when iterating through the
	partitions of a session -- before the first call *partitionModule is to
	be initialized to NULL, after the last call, put_module() has to be
	invoked, if *partitionModule is not NULL.

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


	get_partitioning_params():
	-------------------------

	Returns parameters for partitioning the supplied session.
	If the session is already partitioned using the specified module, then
	the parameters describing the current layout will be returned, otherwise
	default values.

	If the supplied buffer is too small for the parameters, the function
	returns B_OK, but doesn't fill in the buffer; the required buffer
	size is returned in actualSize. If the buffer is large enough,
	actualSize is set to the actually used size. The size includes the
	terminating null.

	params:
	deviceFD: a device FD
	sessionInfo: a complete info about the session to be partitioned
	identifier: identifies the partition module to be used
	buffer: pointer to a pre-allocated buffer of size bufferSize
	bufferSize: the size of buffer
	actualSize: pointer to a pre-allocated size_t to be set to the
				actually needed buffer size

	Returns B_OK, if the parameters could be returned successfully or the
	buffer is too small, an error code otherwise.


	partition():
	-----------

	Partitions the specified session of the device using the partition module
	identified by an identifier and according to the supplied parameters.

	deviceFD: a device FD
	sessionInfo: a complete info about the session the partitions reside on
	identifier: identifies the partition module to be used
	parameters: the parameters for partitioning

	Returns B_OK, if everything went fine, an error code otherwise.
*/

#endif	// _DISKSCANNER_H
