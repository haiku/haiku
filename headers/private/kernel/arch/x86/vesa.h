/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _STAGE2_VESA_H
#define _STAGE2_VESA_H


#include <SupportDefs.h>


struct VBEInfoBlock {
	char   signature[4]; // should be 'VESA'
	uint16 version;
	uint32 oem_ptr;
	uint32 capabilities;
	uint32 video_ptr;
	uint16 total_memory;
	// VESA 2.x stuff
	uint16 oem_software_rev;
	uint32 oem_vendor_name_ptr;
	uint32 oem_product_name_ptr;
	uint32 oem_product_rev_ptr;
	uint8  reserved[222];
	uint8  oem_data[256];
} _PACKED;

struct VBEModeInfoBlock {
	uint16 attributes;
	uint8  wina_attributes;
	uint8  winb_attributes;
	uint16 win_granulatiry;
	uint16 win_size;
	uint16 wina_segment;
	uint16 winb_segment;
	uint32 win_function_ptr;
	uint16 bytes_per_scanline;

	uint16 x_resolution;
	uint16 y_resolution;
	uint8  x_charsize;
	uint8  y_charsize;
	uint8  num_planes;
	uint8  bits_per_pixel;
	uint8  num_banks;
	uint8  memory_model;
	uint8  bank_size;
	uint8  num_image_pages;
	uint8  _reserved;

	uint8  red_mask_size;
	uint8  red_field_position;
	uint8  green_mask_size;
	uint8  green_field_position;
	uint8  blue_mask_size;
	uint8  blue_field_position;
	uint8  reserved_mask_size;
	uint8  reserved_field_position;
	uint8  direct_color_mode_info;

	uint32 phys_base_ptr;
	uint32 offscreen_mem_offset;
	uint16 offscreen_mem_size;
	uint8  _reserved2[206];
} _PACKED;

#endif

