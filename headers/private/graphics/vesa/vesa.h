/*
 * Copyright 2004-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef VESA_H
#define VESA_H


#include <SupportDefs.h>


/* VBE info block structure */

#define VESA_SIGNATURE	'ASEV'
#define VBE2_SIGNATURE	'2EBV'

struct vbe_info_block {
	// VBE 1.x fields
	uint32		signature;
	struct {
		uint8	minor;
		uint8	major;
	} version;
	uint32		oem_string;
	uint32		capabilities;
	uint32		mode_list;
	uint16		total_memory;	// in 64k blocks
	// VBE 2.0+ fields only
	// Note, the block is 256 bytes in size for VBE 1.x as well,
	// but doesn't define these fields. VBE 3 doesn't define
	// any additional fields.
	uint16		oem_software_revision;
	uint32		oem_vendor_name_string;
	uint32		oem_product_name_string;
	uint32		oem_product_revision_string;
	uint8		reserved[222];
	uint8		oem_data[256];
} _PACKED;

// capabilities
#define CAPABILITY_DAC_WIDTH			0x01
#define CAPABILITY_NOT_VGA_COMPATIBLE	0x02


/* VBE mode info structure */

struct vbe_mode_info {
	uint16		attributes;
	uint8		window_a_attributes;
	uint8		window_b_attributes;
	uint16		window_granularity;
	uint16		window_size;
	uint16		window_a_segment;
	uint16		window_b_segment;
	uint32		window_function;	// real mode pointer
	uint16		bytes_per_row;

	// VBE 1.2 and above
	uint16		width;
	uint16		height;
	uint8		char_width;
	uint8		char_height;
	uint8		num_planes;
	uint8		bits_per_pixel;
	uint8		num_banks;
	uint8		memory_model;
	uint8		bank_size;
	uint8		num_image_pages;
	uint8		_reserved0;

	// direct color fields
	uint8		red_mask_size;
	uint8		red_field_position;
	uint8		green_mask_size;
	uint8		green_field_position;
	uint8		blue_mask_size;
	uint8		blue_field_position;
	uint8		reserved_mask_size;
	uint8		reserved_field_position;
	uint8		direct_color_mode_info;

	// VBE 2.0 and above
	uint32		physical_base;
	uint32		_reserved1;
	uint16		_reserved2;

	// VBE 3.0 and above
	uint16		linear_bytes_per_row;
	uint8		banked_num_image_pages;
	uint8		linear_num_image_pages;

	uint8		linear_red_mask_size;
	uint8		linear_red_field_position;
	uint8		linear_green_mask_size;
	uint8		linear_green_field_position;
	uint8		linear_blue_mask_size;
	uint8		linear_blue_field_position;
	uint8		linear_reserved_mask_size;
	uint8		linear_reserved_field_position;

	uint32		max_pixel_clock;		// in Hz

	uint8		_reserved[189];
} _PACKED;

// definitions of mode info attributes
#define MODE_ATTR_AVAILABLE		1
#define MODE_ATTR_COLOR_MODE	8
#define MODE_ATTR_GRAPHICS_MODE	16
#define MODE_ATTR_LINEAR_BUFFER	128

// memory models
#define MODE_MEMORY_TEXT			0
#define MODE_MEMORY_PLANAR			3
#define MODE_MEMORY_PACKED_PIXEL	4
#define MODE_MEMORY_DIRECT_COLOR	6
#define MODE_MEMORY_YUV				7

// set mode flags
#define SET_MODE_MASK				0x01ff
#define SET_MODE_SPECIFY_CRTC		(1 << 11)
#define SET_MODE_LINEAR_BUFFER		(1 << 14)
#define SET_MODE_DONT_CLEAR_MEMORY	(1 << 15)


/* CRTC info block structure */

struct crtc_info_block {
	uint16	horizontal_total;
	uint16	horizontal_sync_start;
	uint16	horizontal_sync_end;
	uint16	vertical_total;
	uint16	vertical_sync_start;
	uint16	vertical_sync_end;
	uint8	flags;
	uint32	pixel_clock;		// in Hz
	uint16	refresh_rate;		// in 0.01 Hz

	uint8	_reserved[40];
} _PACKED;

#define CRTC_DOUBLE_SCAN		0x01
#define CRTC_INTERLACED			0x02
#define CRTC_NEGATIVE_HSYNC		0x04
#define CRTC_NEGATIVE_VSYNC		0x08


/* Power Management */

#define DPMS_ON					0x00
#define DPMS_STANDBY			0x01
#define	DPMS_SUSPEND			0x02
#define DPMS_OFF				0x04
#define DPMS_REDUCED_ON			0x08


/* VBE 3.0 protected mode interface
 * The BIOS area can be scanned for the protected mode
 * signature that identifies the structure below.
 */

#define VBE_PM_SIGNATURE 'DIMP'

struct vbe_protected_mode_info {
	uint32		signature;
	int16		entry_offset;
	int16		init_offset;
	uint16		data_selector;
	uint16		a000_selector;
	uint16		b000_selector;
	uint16		b800_selector;
	uint16		c000_selector;
	uint8		in_protected_mode;
	uint8		checksum;
} _PACKED;

#endif	/* VESA_H */
