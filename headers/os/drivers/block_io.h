/*
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __BLOCK_IO_H__
#define __BLOCK_IO_H__

/*
	Part of Open block device manager

	Block devices can be easily written by providing the interface
	specified hereinafter. The block device manager takes care of
	DMA and other restrictions imposed by underlying controller or
	protocol and transparently handles transmission of partial blocks
	by using a buffer (performance will suffer, though).
*/


#include <KernelExport.h>
#include <device_manager.h>

// cookies issued by block_io
typedef struct block_io_device_info *block_io_device;
typedef struct block_io_handle_info *block_io_handle;

// cookies issued by device driver
typedef struct block_device_device_cookie *block_device_device_cookie;
typedef struct block_device_handle_cookie *block_device_handle_cookie;


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

// if true, this device may be a BIOS drive (uint8, optional, default: false)
#define B_BLOCK_DEVICE_IS_BIOS_DRIVE "block_device/is_bios_drive"
// address bits that must be 0 - must be 2^i-1 for some i (uint32, optional, default: 0)
#define B_BLOCK_DEVICE_DMA_ALIGNMENT "block_device/dma_alignment"
// maximum number of blocks per transfer (uint32, optional, default: unlimited)
#define B_BLOCK_DEVICE_MAX_BLOCKS_ITEM "block_device/max_blocks"
// mask of bits that can change in one sg block (uint32, optional, default: ~0)
#define B_BLOCK_DEVICE_DMA_BOUNDARY "block_device/dma_boundary"
// maximum size of one block in scatter/gather list (uint32, optional, default: ~0)
#define B_BLOCK_DEVICE_MAX_SG_BLOCK_SIZE "block_device/max_sg_block_size"
// maximum number of scatter/gather blocks (uint32, optional, default: unlimited)
#define B_BLOCK_DEVICE_MAX_SG_BLOCKS "block_device/max_sg_blocks"


// interface to be provided by device driver
typedef struct block_device_interface {
	driver_module_info info;

	// iovecs are physical address here
	// pos and num_blocks are in blocks; bytes_transferred in bytes
	// vecs are guaranteed to describe enough data for given block count
	status_t (*open)(block_device_device_cookie device, block_device_handle_cookie *handle);
	status_t (*close)(block_device_handle_cookie handle);
	status_t (*free)(block_device_handle_cookie handle);

	status_t (*read)(block_device_handle_cookie handle, const phys_vecs *vecs, off_t pos,
				size_t num_blocks, uint32 block_size, size_t *bytes_transferred);
	status_t (*write)(block_device_handle_cookie handle, const phys_vecs *vecs, off_t pos,
				size_t num_blocks, uint32 block_size, size_t *bytes_transferred);

	status_t (*ioctl)(block_device_handle_cookie handle, int op, void *buf, size_t len);
} block_device_interface;

#define B_BLOCK_IO_MODULE_NAME "generic/block_io/v1"


// Interface for Drivers

// blkman interface used for callbacks done by driver
typedef struct block_io_for_driver_interface {
	module_info info;

	// block_size - block size in bytes
	// ld_block_size - log2( block_size) (set to zero if block_size is not power of two)
	// capacity - capacity in blocks
	void (*set_media_params)(block_io_device device, uint32 block_size, uint32 ld_block_size,
				uint64 capacity);
} block_io_for_driver_interface;

#define B_BLOCK_IO_FOR_DRIVER_MODULE_NAME "generic/block_io/driver/v1"

#endif	/* __BLOCK_IO_H__ */
