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

#include <disk_scanner.h>
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

// fs_block
typedef struct fs_block {
	off_t			offset;
	void			*data;
	struct fs_block	*previous;
	struct fs_block	*next;
} fs_block;

// fs_buffer_cache
typedef struct fs_buffer_cache {
	int					fd;
	off_t				offset;
	off_t				size;
	size_t				block_size;
	struct fs_block		*first_block;
	struct fs_block		*last_block;
} fs_buffer_cache;

// prototypes
static status_t read_block(int fd, off_t offset, size_t size, uchar **block);
static status_t get_partition_module_block(int deviceFD,
	const session_info *sessionOffset, const uchar *block,
	partition_module_info **partitionModule);
static status_t init_fs_buffer_cache(struct fs_buffer_cache *cache, int fd,
	off_t offset, off_t size, size_t blockSize);
static status_t cleanup_fs_buffer_cache(struct fs_buffer_cache *cache);
static status_t get_buffer(struct fs_buffer_cache *cache, off_t offset,
						   size_t size, void **_buffer, size_t *actualSize);


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
	if (error == B_OK)
		*sessionModule = NULL;
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
			if (read_pos(fd, offset, *block, size) != (ssize_t)size) {
				error = errno;
				if (error == B_OK)
					error = B_IO_ERROR;
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
	if (error == B_OK)
		*partitionModule = NULL;
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

// get_partition_module_for_identifier
static
status_t
get_partition_module_for_identifier(const char *identifier,
									partition_module_info **partitionModule)
{
	// find the partition module that knows the supplied identifier
	status_t error = (identifier ? B_OK : B_BAD_VALUE);
	void *list = NULL;
	if (error == B_OK)
		*partitionModule = NULL;
	if (error == B_OK && ((list = open_module_list(kPartitionModulePrefix)))) {
		char moduleName[B_PATH_NAME_LENGTH];
		size_t bufferSize = sizeof(moduleName);
		for (; read_next_module_name(list, moduleName, &bufferSize) == B_OK;
			 bufferSize = sizeof(moduleName)) {
			partition_module_info *module = NULL;
			if (get_module(moduleName, (module_info**)&module) == B_OK) {
				if (!strcmp(module->short_name, identifier)) {
					// found module
					*partitionModule = module;
					break;
				}
				put_module(moduleName);
			}
		}
		close_module_list(list);
	}
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
								  session_info *sessionInfo,
								  session_module_info **_sessionModule)
{
	status_t error = B_OK;
	session_module_info *sessionModule
		= (_sessionModule ? *_sessionModule : NULL);
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
			blockSize = geometry.bytes_per_sector;
			deviceSize = (off_t)blockSize * geometry.sectors_per_track
				* geometry.cylinder_count * geometry.head_count;
		} else
			error = errno;
	}
	// get a session module, if the device is a CD, otherwise return a
	// virtual session
	if (error == B_OK) {
		// get the session module
		if (!sessionModule) {
			error = disk_scanner_get_session_module(deviceFD, deviceSize,
													blockSize,
													&sessionModule);
			if (error == B_OK) {
				error = sessionModule->get_nth_info(deviceFD, index,
													deviceSize, blockSize,
													sessionInfo);
			} else if (error == B_ENTRY_NOT_FOUND) {
				// no session add-on found -- return a virtual session for
				// index 0
				error = B_OK;
				if (index == 0) {
					sessionInfo->offset = 0;
					sessionInfo->size = deviceSize;
					sessionInfo->logical_block_size = blockSize;
					sessionInfo->index = 0;
					sessionInfo->flags = B_VIRTUAL_SESSION;
				} else
					error = B_ENTRY_NOT_FOUND;
			}
		}
	}
	// cleanup / set results
	if (_sessionModule)
		*_sessionModule = sessionModule;
	else if (sessionModule)
		put_module(sessionModule->module.name);
	TRACE(("disk_scanner: get_nth_session_info() done: %s\n",
		   strerror(error)));
	return error;
}

// get_nth_partition_info
static
status_t
disk_scanner_get_nth_partition_info(int deviceFD,
									const session_info *sessionInfo,
									int32 partitionIndex,
									extended_partition_info *partitionInfo,
									char *partitionMapName,
									partition_module_info **_partitionModule)
{
	partition_module_info *partitionModule
		= (_partitionModule ? *_partitionModule : NULL);
	status_t error = (partitionInfo ? B_OK : B_BAD_VALUE);
	TRACE(("disk_scanner: get_nth_partition_info(%d, %lld, %lld, %ld, %ld, %ld)\n",
		   deviceFD, sessionInfo->offset, sessionInfo->size,
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
		if (!partitionModule) {
			error = read_block(deviceFD, sessionOffset, blockSize, &block);
			if (error == B_OK) {
				error = get_partition_module_block(deviceFD, sessionInfo,
												   block, &partitionModule);
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
		}
		// get the info
		if (error == B_OK && partitionModule) {
			error = partitionModule->get_nth_info(deviceFD, sessionInfo,
				block, partitionIndex, partitionInfo);
		}
		// set results / cleanup
		if (error == B_OK && partitionMapName) {
			// return partition module identifier
			if (partitionModule)
				strcpy(partitionMapName, partitionModule->short_name);
			else
				partitionMapName[0] = '\0';
		}
		if (_partitionModule)
			*_partitionModule = partitionModule;
		else if (partitionModule)
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
	status_t error = (partitionInfo ? B_OK : B_BAD_VALUE);
	void *list = NULL;
	fs_buffer_cache cache;
	bool cacheInitialized = false;
	TRACE(("disk_scanner: get_partition_fs_info(%d, %lld, %lld, %ld)\n", deviceFD,
		   partitionInfo->info.offset, partitionInfo->info.size,
		   partitionInfo->info.logical_block_size));
	// init the cache
	if (error == B_OK) {
		error = init_fs_buffer_cache(&cache, deviceFD,
			partitionInfo->info.offset, partitionInfo->info.size,
			partitionInfo->info.logical_block_size);
		cacheInitialized = (error == B_OK);
	}
	// Iterate through the list of FS modules and return the result of the
	// one with the highest priority that thinks, it is the right one.
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
				if (module->identify(deviceFD, &info, &priority, get_buffer,
									 &cache)) {
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
	// cleanup the cache
	if (cacheInitialized)
		cleanup_fs_buffer_cache(&cache);
	return error;
}

// get_partitioning_params
static
status_t
disk_scanner_get_partitioning_params(int deviceFD,
									 const struct session_info *sessionInfo,
									 const char *identifier, char *buffer,
									 size_t bufferSize, size_t *actualSize)
{
	// find the partition module that knows the supplied identifier
	status_t error = (sessionInfo && identifier && buffer && actualSize
					  ? B_OK : B_BAD_VALUE);
	partition_module_info *partitionModule = NULL;
	if (error == B_OK)
		get_partition_module_for_identifier(identifier, &partitionModule);
	// get the parameters from the module
	if (error == B_OK) {
		error = partitionModule->get_partitioning_params(deviceFD, sessionInfo,
			buffer, bufferSize, actualSize);
	}
	// cleanup
	if (partitionModule)
		put_module(partitionModule->module.name);
	return error;
}

// partition
static
status_t
disk_scanner_partition(int deviceFD, const struct session_info *sessionInfo,
					   const char *identifier, const char *parameters)
{
	// find the partition module that knows the supplied identifier
	status_t error = (sessionInfo && identifier ? B_OK : B_BAD_VALUE);
	partition_module_info *partitionModule = NULL;
	if (error == B_OK)
		get_partition_module_for_identifier(identifier, &partitionModule);
	// let the module do the actual partitioning
	if (error == B_OK)
		error = partitionModule->partition(deviceFD, sessionInfo, parameters);
	// cleanup
	if (partitionModule)
		put_module(partitionModule->module.name);
	return error;
}


// new_fs_block
static
status_t
new_fs_block(int fd, off_t offset, size_t size, fs_block **_block)
{
	status_t error = (_block ? B_OK : B_BAD_VALUE);
	fs_block *block = NULL;
	// alloc the block
	if (error == B_OK) {
		block = (fs_block*)malloc(sizeof(fs_block));
		if (!block)
			error = B_NO_MEMORY;
	}
	// read the block
	if (error == B_OK)
		error = read_block(fd, offset, size, (uchar**)&block->data);
	// set the fields / cleanup on error
	if (error == B_OK) {
		block->offset = offset;
		block->previous = NULL;
		block->next = NULL;
		*_block = block;
	} else {
		if (block)
			free(block);
	}
	return error;
}

// delete_fs_block
static
void
delete_fs_block(fs_block *block)
{
	if (block) {
		if (block->data)
			free(block->data);
		free(block);
	}
}

// init_fs_buffer_cache
static
status_t
init_fs_buffer_cache(struct fs_buffer_cache *cache, int fd, off_t offset,
					 off_t size, size_t blockSize)
{
	cache->fd = fd;
	cache->offset = offset;
	cache->size = size;
	cache->block_size = blockSize;
	cache->first_block = NULL;
	cache->last_block = NULL;
	return B_OK;
}

// cleanup_fs_buffer_cache
static
status_t
cleanup_fs_buffer_cache(struct fs_buffer_cache *cache)
{
	if (cache) {
		while (cache->first_block) {
			fs_block *block = cache->first_block;
			cache->first_block = block->next;
			delete_fs_block(block);
		}
	}
	return B_OK;
}

// get_buffer
static
status_t
get_buffer(struct fs_buffer_cache *cache, off_t offset, size_t size,
		   void **_buffer, size_t *actualSize)
{
	status_t error = (cache && _buffer && actualSize
					  && offset >= 0 && size > 0
					  ? B_OK : B_BAD_VALUE);
	uint8 *buffer = NULL;
	// check the bounds
	if (error == B_OK) {
		offset += cache->offset;	// make relative to beginning of device
		if (offset >= cache->offset + cache->size)
			size = 0;
		else if (offset + size > cache->offset + cache->size)
			size = cache->offset + cache->size - offset;
	}
	// allocate the buffer
	if (error == B_OK && size > 0) {
		buffer = (uint8*)malloc(size);
		if (buffer) {
			// iterate through the list of cached blocks
			off_t blockOffset = offset - offset % cache->block_size;
			size_t remainingBytes = size;
			fs_block *block = NULL;
			for (block = cache->first_block;
				 remainingBytes > 0;
				 block = block->next) {
				if (!block || block->offset >= blockOffset) {
					if (!block || block->offset > blockOffset) {
						// the block is not in cache, read it
						fs_block *newBlock = NULL;
						error = new_fs_block(cache->fd, blockOffset,
											 cache->block_size, &newBlock);
						if (error == B_OK) {
							// insert the new block
							if (block)
								newBlock->previous = block->previous;
							else
								newBlock->previous = cache->last_block;
							newBlock->next = block;
							if (newBlock->previous)
								newBlock->previous->next = newBlock;
							else
								cache->first_block = newBlock;
							if (newBlock->next)
								newBlock->next->previous = newBlock;
							else
								cache->last_block = newBlock;
							block = newBlock;
						}
					}
					if (error == B_OK) {
						// block is (now) in cache, copy the data
						off_t inBlockOffset = 0;
						size_t toCopy = cache->block_size;
						if (blockOffset < offset) {
							inBlockOffset = offset - blockOffset;
							toCopy -= inBlockOffset;
						}
						if (toCopy > remainingBytes)
							toCopy = remainingBytes;
						memcpy(buffer + blockOffset + inBlockOffset - offset,
							   (uint8*)block->data + inBlockOffset, toCopy);
						blockOffset += cache->block_size;
						remainingBytes -= toCopy;
					}
				}
				// bail out on failure
				if (error != B_OK)
					break;
			}
		} else
			error = B_NO_MEMORY;
	}
	// set results	/ cleanup on error
	if (error == B_OK) {
		*_buffer = buffer;
		*actualSize = size;
	} else {
		if (buffer)
			free(buffer);
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
	disk_scanner_get_partition_fs_info,
	disk_scanner_get_partitioning_params,
	disk_scanner_partition
};

_EXPORT disk_scanner_module_info *modules[] =
{
	&disk_scanner_module,
	NULL
};


