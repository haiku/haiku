/*
** Copyright 2002/03, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	Part of Open block device manager

	Block devices can be easily written by providing the interface
	specified hereinafter. The block device manager takes care of
	DMA and other restrictions imposed by underlying controller or
	protocol and transparently handles transmission of partial blocks
	by using a buffer (performance will suffer, though).
*/

#ifndef __BLKMAN_H__
#define __BLKMAN_H__

#include <KernelExport.h>
#include <device_manager.h>

// cookies issued by blkman
typedef struct blkman_device_info *blkman_device;
typedef struct blkman_handle_info *blkman_handle;

// cookies issued by device driver
typedef struct blkdev_device_cookie *blkdev_device_cookie;
typedef struct blkdev_handle_cookie *blkdev_handle_cookie;


// two reason why to use array of size 1:
// 1. zero-sized arrays aren't standard C
// 2. it's handy if you need a temporary global variable with num=1
typedef struct phys_vecs {
	size_t num;
	size_t total_len;
	physical_entry vec[1];
} phys_vecs;

#define PHYS_VECS(name, size) \
	uint8 name[sizeof(phys_vecs) + (size - 1)*sizeof(phys_vec)]; \
	phys_vecs *name = (phys_vecs *)name


// Block Device Node

// attributes:

// type code for block devices
#define BLKDEV_TYPE_NAME "blkdev"
// if true, this device may be a BIOS drive (uint8, optional, default: false)
#define BLKDEV_IS_BIOS_DRIVE "blkdev/is_bios_drive"
// address bits that must be 0 - must be 2^i-1 for some i (uint32, optional, default: 0)
#define BLKDEV_DMA_ALIGNMENT "blkdev/dma_alignment"
// maximum number of blocks per transfer (uint32, optional, default: unlimited)
#define BLKDEV_MAX_BLOCKS_ITEM "blkdev/max_blocks"
// mask of bits that can change in one sg block (uint32, optional, default: ~0)
#define BLKDEV_DMA_BOUNDARY "blkdev/dma_boundary"
// maximum size of one block in scatter/gather list (uint32, optional, default: ~0)
#define BLKDEV_MAX_SG_BLOCK_SIZE "blkdev/max_sg_block_size"
// maximum number of scatter/gather blocks (uint32, optional, default: unlimited)
#define BLKDEV_MAX_SG_BLOCKS "blkdev/max_sg_blocks"


// interface to be provided by device driver
typedef struct blkdev_interface {
	pnp_driver_info dinfo;

	// iovecs are physical address here
	// pos and num_blocks are in blocks; bytes_transferred in bytes
	// vecs are guaranteed to describe enough data for given block count
	status_t (*open)(blkdev_device_cookie device, blkdev_handle_cookie *handle);
	status_t (*close)(blkdev_handle_cookie handle);
	status_t (*free)(blkdev_handle_cookie handle);

	status_t (*read)(blkdev_handle_cookie handle, const phys_vecs *vecs, off_t pos,
				size_t num_blocks, uint32 block_size, size_t *bytes_transferred);
	status_t (*write)(blkdev_handle_cookie handle, const phys_vecs *vecs, off_t pos,
				size_t num_blocks, uint32 block_size, size_t *bytes_transferred);

	status_t (*ioctl)(blkdev_handle_cookie handle, int op, void *buf, size_t len);
} blkdev_interface;

#define BLKMAN_MODULE_NAME "generic/blkman/v1"


// Interface for Drivers

// blkman interface used for callbacks done by driver
typedef struct blkman_for_driver_interface {
	module_info minfo;

	// block_size - block size in bytes
	// ld_block_size - log2( block_size) (set to zero if block_size is not power of two)
	// capacity - capacity in blocks
	void (*set_media_params)(blkman_device device, uint32 block_size, uint32 ld_block_size,
				uint64 capacity);
} blkman_for_driver_interface;

#define BLKMAN_FOR_DRIVER_MODULE_NAME "generic/blkman/driver/v1"

#endif
