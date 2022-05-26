/*
 * Copyright 2005-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef VESA_INFO_H
#define VESA_INFO_H


#include <Drivers.h>
#include <Accelerant.h>
#include <PCI.h>

#include <edid.h>


#define VESA_EDID_BOOT_INFO "vesa_edid/v1"
#define VESA_MODES_BOOT_INFO "vesa_modes/v1"

struct vesa_mode {
	uint16			mode;
	uint16			width;
	uint16			height;
	uint8			bits_per_pixel;
};

enum bios_type_enum {
	kUnknownBiosType = 0,
	kIntelBiosType,
	kNVidiaBiosType,
	kAtomBiosType1,
	kAtomBiosType2
};

struct vesa_shared_info {
	area_id			mode_list_area;		// area containing display mode list
	uint32			mode_count;
	display_mode	current_mode;
	uint32			bytes_per_row;

	area_id			frame_buffer_area;	// area of frame buffer
	uint8*			frame_buffer;
		// pointer to frame buffer (visible by all apps!)
	uint8*			physical_frame_buffer;

	uint32			vesa_mode_offset;
	uint32			vesa_mode_count;

	edid1_info		edid_info;
	bool			has_edid;
	bios_type_enum	bios_type;
	uint16			mode_table_offset;
		// Atombios only: offset to the table of video modes in the bios, used for patching in
		// extra video modes.
	uint32			dpms_capabilities;

	char			name[32];
	uint32			vram_size;
};

//----------------- ioctl() interface ----------------

// list ioctls
enum {
	VESA_GET_PRIVATE_DATA = B_DEVICE_OP_CODES_END + 1,
	VESA_GET_DEVICE_NAME,
	VESA_SET_DISPLAY_MODE,
	VESA_GET_DPMS_MODE,
	VESA_SET_DPMS_MODE,
	VESA_SET_INDEXED_COLORS,
	VESA_SET_CUSTOM_DISPLAY_MODE,

	VGA_PLANAR_BLIT,
};

struct vesa_set_indexed_colors_args {
	uint8			first;
	uint16			count;
	uint8*			colors;
};

struct vga_planar_blit_args {
	uint8*			source;
	int32			source_bytes_per_row;
	int32			left;
	int32			top;
	int32			right;
	int32			bottom;
};

#endif	/* VESA_INFO_H */
