/*
** Copyright 2002/03, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	Part of Open block device manager

	Internal header.
*/

#include <blkman.h>
#include <string.h>
#include <module.h>
#include <locked_pool.h>
#include <malloc.h>
#include <pnp_devfs.h>
#include <device_manager.h>
#include <bios_drive.h>
#include "wrapper.h"


// controller restrictions (see blkman.h)
typedef struct blkdev_params {
	uint32 alignment;
	uint32 max_blocks;
	uint32 dma_boundary;
	uint32 max_sg_block_size;
	uint32 max_sg_blocks;
} blkdev_params;


// device info
typedef struct blkman_device_info {
	pnp_node_handle node;
	blkdev_interface *interface;
	blkdev_device_cookie cookie;
	
	benaphore lock;					// used for access to following variables
	uint32 block_size;
	uint32 ld_block_size;
	uint64 capacity;
	
	blkdev_params params;
	bool is_bios_drive;				// could be a BIOS drive
	locked_pool_cookie phys_vecs_pool;	// pool of temporary phys_vecs
	bios_drive *bios_drive;			// info about corresponding BIOS drive
} blkman_device_info;


// file handle info
typedef struct blkman_handle_info {
	blkman_device_info *device;
	blkdev_handle_cookie cookie;
} blkman_handle_info;


// attribute containing BIOS drive ID (or 0, if it's no BIOS drive) (uint8)
#define BLKDEV_BIOS_ID "blkdev/bios_id"

// transmission buffer data:
// size in bytes
extern uint blkman_buffer_size;
// to use the buffer, you must own this semaphore
extern sem_id blkman_buffer_lock;
// iovec
extern struct iovec blkman_buffer_vec[1];
// physical address
extern void *blkman_buffer_phys;
// virtual address
extern char *blkman_buffer;
// phys_vec of it (always linear)
extern phys_vecs blkman_buffer_phys_vec;
// area containing buffer
extern area_id blkman_buffer_area;

extern locked_pool_interface *locked_pool;
extern device_manager_info *pnp;


// io.c

status_t blkman_readv( blkman_handle_info *handle, off_t pos, struct iovec *vec, 
	size_t vec_count, size_t *len );
status_t blkman_read( blkman_handle_info *handle, off_t pos, void *buf, size_t *len );
ssize_t blkman_writev( blkman_handle_info *handle, off_t pos, struct iovec *vec, 
	size_t vec_count, ssize_t *len );
ssize_t blkman_write( blkman_handle_info *handle, off_t pos, void *buf, size_t *len );
void blkman_set_media_params( blkman_device_info *device, 
	uint32 block_size, uint32 ld_block_size, uint64 capacity );
