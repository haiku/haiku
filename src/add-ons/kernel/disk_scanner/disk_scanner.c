//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file disk_scanner.c
	disk_scanner kernel module
*/

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fs_device.h>
#include <KernelExport.h>
#include <disk_scanner/fs.h>
#include <disk_scanner/partition.h>
#include <disk_scanner/disk_scanner.h>
#include <disk_scanner/session.h>

static const char *kSessionModulePrefix		= "disk_scanner/session";
static const char *kPartitionModulePrefix	= "disk_scanner/partition";
static const char *kFSModulePrefix			= "disk_scanner/fs";

#define TRACE(x) ;
//#define TRACE(x) dprintf x

// prototypes
static status_t read_block(int fd, off_t offset, size_t size, uchar **block);
static status_t get_partition_module_block(int deviceFD,
	const session_info *sessionOffset, const uchar *block,
	partition_module_info **partitionModule);


// std_ops
static
status_t
std_ops(int32 op, ...)
{
	TRACE(("disk_scanner: std_ops(0x%lx)\n", op));
	switch(op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;
	}
	return B_ERROR;
}

// get_session_module
static
status_t
disk_scanner_get_session_module(int deviceFD, off_t deviceSize, int32 blockSize,
							session_module_info **sessionModule)
{
	// Iterate through the list of session modules and return the first one
	// that thinks, it is the right one.
	status_t error = (sessionModule ? B_OK : B_BAD_VALUE);
	void *list = NULL;
	TRACE(("disk_scanner: get_session_module(%d, %lld, %ld)\n", deviceFD,
		   deviceSize, blockSize));
	if (error == B_OK && ((list = open_module_list(kSessionModulePrefix)))) {
		char moduleName[B_PATH_NAME_LENGTH];
		size_t bufferSize = sizeof(moduleName);
		for (; read_next_module_name(list, moduleName, &bufferSize) == B_OK;
			 bufferSize = sizeof(moduleName)) {
			session_module_info *module = NULL;
			if (get_module(moduleName, (module_info**)&module) == B_OK) {
				if (module->identify(deviceFD, deviceSize, blockSize)) {
					// found module
					*sessionModule = module;
					break;
				}
				put_module(moduleName);
			}
		}
		close_module_list(list);
	}
	// set result
	if (error == B_OK && !*sessionModule)
		error = B_ENTRY_NOT_FOUND;
	return error;
}

// read_block
static
status_t
read_block(int fd, off_t offset, size_t size, uchar **block)
{
	status_t error = (block && size > 0 ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		*block = malloc(size);
		if (*block) {
			if (read_pos(fd, 0, *block, size) != (ssize_t)size) {
				error = errno;
				if (error == B_OK)
					error = B_ERROR;
				free(*block);
				*block = NULL;
			}
		} else
			error = B_NO_MEMORY;
	}
	return error;
}

// get_partition_module_block
static
status_t
get_partition_module_block(int deviceFD, const session_info *sessionInfo,
						   const uchar *block,
						   partition_module_info **partitionModule)
{
	// Iterate through the list of partition modules and return the first one
	// that thinks, it is the right one.
	status_t error = (partitionModule && block ? B_OK : B_BAD_VALUE);
	void *list = NULL;
	if (error == B_OK && ((list = open_module_list(kPartitionModulePrefix)))) {
		char moduleName[B_PATH_NAME_LENGTH];
		size_t bufferSize = sizeof(moduleName);
		for (; read_next_module_name(list, moduleName, &bufferSize) == B_OK;
			 bufferSize = sizeof(moduleName)) {
			partition_module_info *module = NULL;
			TRACE(("disk_scanner: trying partition module: `%s'\n", moduleName));
			if (get_module(moduleName, (module_info**)&module) == B_OK) {
				if (module->identify(deviceFD, sessionInfo, block)) {
					// found module
					*partitionModule = module;
					break;
				}
				put_module(moduleName);
			}
		}
		close_module_list(list);
	}
	// set result
	if (error == B_OK && !*partitionModule)
		error = B_ENTRY_NOT_FOUND;
	return error;
}

// get_partition_module
static
status_t
disk_scanner_get_partition_module(int deviceFD,
								  const session_info *sessionInfo,
								  partition_module_info **partitionModule)
{
	status_t error = (partitionModule && sessionInfo ? B_OK : B_BAD_VALUE);
	TRACE(("disk_scanner: get_partition_module(%d, %lld, %lld, %ld)\n",
		   deviceFD, sessionInfo->offset, sessionInfo->size,
		   sessionInfo->logical_block_size));
	if (error == B_OK) {
		off_t sessionOffset = sessionInfo->offset;
		int32 blockSize = sessionInfo->logical_block_size;
		// read the first block of the session and let the helper function
		// do the job
		uchar *block = NULL;
		error = read_block(deviceFD, sessionOffset, blockSize, &block);
		if (error == B_OK) {
			error = get_partition_module_block(deviceFD, sessionInfo,
											   block, partitionModule);
			free(block);
		}
	}
	return error;
}

// get_nth_session_info
static
status_t
disk_scanner_get_nth_session_info(int deviceFD, int32 index,
								  session_info *sessionInfo)
{
	status_t error = B_OK;
	session_module_info *sessionModule = NULL;
	bool isCD = false;
	int32 blockSize = 0;
	off_t deviceSize = 0;
	device_geometry geometry;
	// get the media status
	if (ioctl(deviceFD, B_GET_MEDIA_STATUS, &error) != 0)
		error = errno;
	// get the device geometry
	TRACE(("disk_scanner: get_nth_session_info(%d, %ld)\n", deviceFD, index));
	if (error == B_OK) {
		if (ioctl(deviceFD, B_GET_GEOMETRY, &geometry) == 0) {
			isCD = (geometry.device_type == B_CD);
			blockSize = geometry.bytes_per_sector;
			deviceSize = (off_t)blockSize * geometry.sectors_per_track
				* geometry.cylinder_count * geometry.head_count;
		} else
			error = errno;
	}
	// get a session module, if the device is a CD, otherwise return a
	// virtual session
	if (error == B_OK) {
		if (isCD) {
			// get the session module
			error = disk_scanner_get_session_module(deviceFD, deviceSize,
												blockSize, &sessionModule);
			// get the info
			if (error == B_OK) {
				error = sessionModule->get_nth_info(deviceFD, index,
													deviceSize, blockSize,
													sessionInfo);
			}
		} else if (index == 0) {
			// no CD -- virtual session
			sessionInfo->offset = 0;
			sessionInfo->size = deviceSize;
			sessionInfo->logical_block_size = blockSize;
			sessionInfo->index = 0;
			sessionInfo->flags = B_VIRTUAL_SESSION;
		} else	// no CD, fail after the first session
			error = B_ENTRY_NOT_FOUND;
	}
	// cleanup
	if (sessionModule)
		put_module(sessionModule->module.name);
	return error;
}

// get_nth_partition_info
static
status_t
disk_scanner_get_nth_partition_info(int deviceFD,
									const session_info *sessionInfo,
									int32 partitionIndex,
									extended_partition_info *partitionInfo)
{
	partition_module_info *partitionModule = NULL;
	status_t error = (partitionInfo ? B_OK : B_BAD_VALUE);
	TRACE(("disk_scanner: get_nth_partition_info(%d, %lld, %lld, %ld, %ld, %ld)\n",
		   deviceFD, sessionOffset, sessionSize,
		   partitionInfo->info.logical_block_size, partitionInfo->info.session,
		   partitionInfo->info.partition));
	if (error == B_OK) {
		off_t sessionOffset = sessionInfo->offset;
		off_t sessionSize = sessionInfo->size;
		int32 blockSize = sessionInfo->logical_block_size;
		uchar *block = NULL;
		// fill in the fields we do already know
		partitionInfo->info.logical_block_size = blockSize;
		partitionInfo->info.session = sessionInfo->index;
		partitionInfo->info.partition = partitionIndex;
		// read the first block of the session and get the partition module
		error = read_block(deviceFD, sessionOffset, blockSize, &block);
		if (error == B_OK) {
			error = get_partition_module_block(deviceFD, sessionInfo, block,
											   &partitionModule);
			if (error == B_ENTRY_NOT_FOUND
				&& partitionInfo->info.partition == 0) {
				// no matching partition module found: return a virtual
				// partition info
				partitionInfo->info.offset = sessionOffset;
				partitionInfo->info.size = sessionSize;
				partitionInfo->flags = B_VIRTUAL_PARTITION;
				partitionInfo->partition_name[0] = '\0';
				partitionInfo->partition_type[0] = '\0';
				partitionInfo->partition_code = 0xeb;	// doesn't matter
				error = B_OK;
			}
		}
		// get the info
		if (error == B_OK && partitionModule) {
			error = partitionModule->get_nth_info(deviceFD, sessionInfo,
				block, partitionIndex, partitionInfo);
		}
		// cleanup
		if (partitionModule)
			put_module(partitionModule->module.name);
		if (block)
			free(block);
	}
	return error;
}

// get_partition_fs_info
static
status_t
disk_scanner_get_partition_fs_info(int deviceFD,
							   extended_partition_info *partitionInfo)
{
	// Iterate through the list of FS modules and return the result of the
	// first one that thinks, it is the right one.
	status_t error = (partitionInfo ? B_OK : B_BAD_VALUE);
	void *list = NULL;
	TRACE(("disk_scanner: get_partition_fs_info(%d, %lld, %lld, %ld)\n", deviceFD,
		   partitionInfo->info.offset, partitionInfo->info.size,
		   partitionInfo->info.logical_block_size));
	if (error == B_OK && ((list = open_module_list(kFSModulePrefix)))) {
		extended_partition_info bestInfo;
		float bestPriority = -2;
		char moduleName[B_PATH_NAME_LENGTH];
		size_t bufferSize = sizeof(moduleName);
		error = B_ENTRY_NOT_FOUND;
		for (; read_next_module_name(list, moduleName, &bufferSize) == B_OK;
			 bufferSize = sizeof(moduleName)) {
			fs_module_info *module = NULL;
			TRACE(("disk_scanner: trying fs module: `%s'\n", moduleName));
			// get the module
			if (get_module(moduleName, (module_info**)&module) == B_OK) {
				// get the info
				extended_partition_info info = *partitionInfo;
				float priority = 0;
				if (module->identify(deviceFD, &info, &priority)) {
					// copy the info, if it is the first or the has a higher
					// priority than the one found before
					if (error == B_ENTRY_NOT_FOUND
						|| priority > bestPriority) {
						bestInfo = info;
						bestPriority = priority;
					}
					error = B_OK;
				}
				put_module(moduleName);
			}
			// set the return value
			if (error == B_OK)
				*partitionInfo = bestInfo;
		}
		close_module_list(list);
	}
	return error;
}

static disk_scanner_module_info disk_scanner_module = 
{
	// module_info
	{
		DISK_SCANNER_MODULE_NAME,
		0,	// better B_KEEP_LOADED?
		std_ops
	},
	disk_scanner_get_session_module,
	disk_scanner_get_partition_module,
	disk_scanner_get_nth_session_info,
	disk_scanner_get_nth_partition_info,
	disk_scanner_get_partition_fs_info
};

_EXPORT disk_scanner_module_info *modules[] =
{
	&disk_scanner_module,
	NULL
};


