/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <string.h>

#include <new>

#include <fs_interface.h>

#include <AutoDeleter.h>

#include "checksumfs.h"
#include "SuperBlock.h"
#include "Volume.h"


static const char* const kCheckSumFSModuleName	= "file_systems/checksumfs"
	B_CURRENT_FS_API_VERSION;
static const char* const kCheckSumFSShortName	= "checksumfs";


// #pragma mark -


static float
checksumfs_identify_partition(int fd, partition_data* partition,
	void** _cookie)
{
	if ((uint64)partition->size < kCheckSumFSMinSize)
		return -1;

	SuperBlock* superBlock = new(std::nothrow) SuperBlock;
	if (superBlock == NULL)
		return -1;
	ObjectDeleter<SuperBlock> superBlockDeleter(superBlock);

	if (pread(fd, superBlock, sizeof(*superBlock), kCheckSumFSSuperBlockOffset)
			!= sizeof(*superBlock)) {
		return -1;
	}

	if (!superBlock->Check((uint64)partition->size / B_PAGE_SIZE))
		return -1;

	*_cookie = superBlockDeleter.Detach();
	return 0.8f;
}


static status_t
checksumfs_scan_partition(int fd, partition_data* partition, void* cookie)
{
	SuperBlock* superBlock = (SuperBlock*)cookie;

	partition->status = B_PARTITION_VALID;
	partition->flags |= B_PARTITION_FILE_SYSTEM;
	partition->content_size = superBlock->TotalBlocks() * B_PAGE_SIZE;
	partition->block_size = B_PAGE_SIZE;
	partition->content_name = strdup(superBlock->Name());
	if (partition->content_name == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


static void
checksumfs_free_identify_partition_cookie(partition_data* partition,
	void* cookie)
{
	SuperBlock* superBlock = (SuperBlock*)cookie;
	delete superBlock;
}


static status_t
checksumfs_mount(fs_volume* volume, const char* device, uint32 flags,
	const char* args, ino_t* _rootVnodeID)
{
	// TODO: Implement!
	return B_UNSUPPORTED;
}


static status_t
checksumfs_set_content_name(int fd, partition_id partition, const char* name,
	disk_job_id job)
{
	// TODO: Implement!
	return B_UNSUPPORTED;
}


static status_t
checksumfs_initialize(int fd, partition_id partition, const char* name,
	const char* parameters, off_t partitionSize, disk_job_id job)
{
	if (name == NULL || strlen(name) >= kCheckSumFSNameLength)
		return B_BAD_VALUE;

	// TODO: Forcing a non-empty name here. Superfluous when the userland disk
	// system add-on has a parameter editor for it.
	if (*name == '\0')
		name = "Unnamed";

	update_disk_device_job_progress(job, 0);

	Volume volume(0);

	status_t error = volume.Init(fd, partitionSize / B_PAGE_SIZE);
	if (error != B_OK)
		return error;

	error = volume.Initialize(name);
	if (error != B_OK)
		return error;

	// rescan partition
	error = scan_partition(partition);
	if (error != B_OK)
		return error;

	update_disk_device_job_progress(job, 1);

	return B_OK;
}


static status_t
checksumfs_std_ops(int32 operation, ...)
{
	switch (operation) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;
		default:
			return B_BAD_VALUE;
	}
}


static file_system_module_info sFSModule = {
	{
		kCheckSumFSModuleName,
		0,
		checksumfs_std_ops
	},
	kCheckSumFSShortName,
	CHECK_SUM_FS_PRETTY_NAME,
	// DDM flags
	B_DISK_SYSTEM_SUPPORTS_INITIALIZING
		| B_DISK_SYSTEM_SUPPORTS_CONTENT_NAME
		| B_DISK_SYSTEM_SUPPORTS_WRITING,

	/* scanning (the device is write locked) */
	checksumfs_identify_partition,
	checksumfs_scan_partition,
	checksumfs_free_identify_partition_cookie,
	NULL,	// free_partition_content_cookie

	/* general operations */
	checksumfs_mount,

	/* capability querying (the device is read locked) */
	NULL,	// get_supported_operations

	NULL,	// validate_resize
	NULL,	// validate_move
	NULL,	// validate_set_content_name
	NULL,	// validate_set_content_parameters
	NULL,	// validate_initialize

	/* shadow partition modification (device is write locked) */
	NULL,	// shadow_changed

	/* writing (the device is NOT locked) */
	NULL,	// defragment
	NULL,	// repair
	NULL,	// resize
	NULL,	// move
	checksumfs_set_content_name,
	NULL,	// set_content_parameters
	checksumfs_initialize
};


const module_info* modules[] = {
	(module_info*)&sFSModule,
	NULL
};
