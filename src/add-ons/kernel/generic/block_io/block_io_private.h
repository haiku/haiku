/*
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*
	Part of Open block device manager

	Internal header.
*/

#include <block_io.h>
#include <module.h>
#include <locked_pool.h>
#include <pnp_devfs.h>
#include <device_manager.h>

#include <stdlib.h>
#include <string.h>

#include "bios_drive.h"
#include "wrapper.h"


// controller restrictions (see block_io.h)
typedef struct block_device_params {
	uint32 alignment;
	uint32 max_blocks;
	uint32 dma_boundary;
	uint32 max_sg_block_size;
	uint32 max_sg_blocks;
} block_device_params;


// device info
typedef struct block_io_device_info {
	device_node_handle node;
	block_device_interface *interface;
	block_device_device_cookie cookie;
	
	benaphore lock;					// used for access to following variables
	uint32 block_size;
	uint32 ld_block_size;
	uint64 capacity;
	
	block_device_params params;
	bool is_bios_drive;				// could be a BIOS drive
	locked_pool_cookie phys_vecs_pool;	// pool of temporary phys_vecs
	bios_drive *bios_drive;			// info about corresponding BIOS drive
} block_io_device_info;


// file handle info
typedef struct block_io_handle_info {
	block_io_device_info *device;
	block_device_handle_cookie cookie;
} block_io_handle_info;


// attribute containing BIOS drive ID (or 0, if it's no BIOS drive) (uint8)
#define B_BLOCK_DEVICE_BIOS_ID "blkdev/bios_id"

// transmission buffer data:
// size in bytes
extern uint block_io_buffer_size;
// to use the buffer, you must own this semaphore
extern sem_id block_io_buffer_lock;
// iovec
extern struct iovec block_io_buffer_vec[1];
// physical address
extern void *block_io_buffer_phys;
// virtual address
extern char *block_io_buffer;
// phys_vec of it (always linear)
extern phys_vecs block_io_buffer_phys_vec;
// area containing buffer
extern area_id block_io_buffer_area;

extern locked_pool_interface *locked_pool;
extern device_manager_info *pnp;


// io.c

status_t block_io_readv(block_io_handle_info *handle, off_t pos, struct iovec *vec, 
	size_t vec_count, size_t *len);
status_t block_io_read(block_io_handle_info *handle, off_t pos, void *buf, size_t *len);
ssize_t block_io_writev(block_io_handle_info *handle, off_t pos, struct iovec *vec,
	size_t vec_count, ssize_t *len);
ssize_t block_io_write(block_io_handle_info *handle, off_t pos, void *buf, size_t *len);
void block_io_set_media_params(block_io_device_info *device, 
	uint32 block_size, uint32 ld_block_size, uint64 capacity);
