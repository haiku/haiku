/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef VESA_INFO_H
#define VESA_INFO_H


#include <Drivers.h>
#include <Accelerant.h>
#include <PCI.h>


#define DEVICE_NAME				"vesa"
#define VESA_ACCELERANT_NAME	"vesa.accelerant"


struct vesa_shared_info {
	int32			type;
	area_id			mode_list_area;		// area containing display mode list
	uint32			mode_count;
	display_mode	current_mode;
	uint32			bytes_per_row;

	area_id			registers_area;		// area of memory mapped registers
	area_id			frame_buffer_area;	// area of frame buffer
	uint8			*frame_buffer;		// pointer to frame buffer (visible by all apps!)
	uint8			*physical_frame_buffer;
};

struct vesa_info {
	uint32			cookie_magic;
	int32			open_count;
	int32			id;
	pci_info		*pci;
	uint8			*registers;
	area_id			registers_area;
	struct vesa_shared_info *shared_info;
	area_id			shared_area;
	uint8			*reloc_io;
	area_id			reloc_io_area;
};

//----------------- ioctl() interface ----------------

// list ioctls
enum {
	VESA_GET_PRIVATE_DATA = B_DEVICE_OP_CODES_END + 1,
	VESA_GET_DEVICE_NAME,

	VGA_SET_INDEXED_COLORS,
	VGA_PLANAR_BLIT,
};

struct vga_set_indexed_colors_args {
	uint8	first;
	uint16	count;
	uint8	*colors;
};

struct vga_planar_blit_args {
	uint8	*source;
	int32	source_bytes_per_row;
	int32	left;
	int32	top;
	int32	right;
	int32	bottom;
};

//----------------------------------------------------------

extern status_t vesa_init(vesa_info &info);
extern void vesa_uninit(vesa_info &info);

#endif	/* VESA_INFO_H */
