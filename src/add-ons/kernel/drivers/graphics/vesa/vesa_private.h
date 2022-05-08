/*
 * Copyright 2005-2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2021, Adrien Destugues, pulkomandy@pulkomandy.tk
 * Distributed under the terms of the MIT License.
 */
#ifndef VESA_PRIVATE_H
#define VESA_PRIVATE_H


#include <Drivers.h>
#include <Accelerant.h>
#include <PCI.h>

#include <drivers/bios.h>


#define DEVICE_NAME				"vesa"
#define VESA_ACCELERANT_NAME	"vesa.accelerant"


struct vesa_get_supported_modes;
struct vesa_mode;

struct vesa_info {
	uint32			cookie_magic;
	int32			open_count;
	int32			id;
	pci_info*		pci;
	struct vesa_shared_info* shared_info;
	area_id			shared_area;
	vesa_mode*		modes;
	uint32			vbe_dpms_capabilities;
	uint8			vbe_capabilities;
	uint8			bits_per_gun;

	addr_t			frame_buffer;
	addr_t			physical_frame_buffer;
	size_t			physical_frame_buffer_size;
	bool			complete_frame_buffer_mapped;
};

extern status_t vesa_init(vesa_info& info);
extern void vesa_uninit(vesa_info& info);
extern status_t vesa_set_display_mode(vesa_info& info, uint32 mode);
extern status_t vesa_set_custom_display_mode(vesa_info& info, display_mode& mode);
extern status_t vesa_get_dpms_mode(vesa_info& info, uint32& mode);
extern status_t vesa_set_dpms_mode(vesa_info& info, uint32 mode);
extern status_t vesa_set_indexed_colors(vesa_info& info, uint8 first, uint8* colors, uint16 count);

extern void vesa_identify_bios(bios_state* state, vesa_shared_info* sharedInfo);

status_t vbe_call_prepare(bios_state** state);
status_t vbe_get_mode_info(bios_state* state, uint16 mode, struct vbe_mode_info* modeInfo);
uint32 get_color_space_for_depth(uint32 depth);
status_t vbe_set_mode(bios_state* state, uint16 mode);
status_t vbe_set_bits_per_gun(bios_state* state, vesa_info& info, uint8 bits);
status_t remap_frame_buffer(vesa_info& info, addr_t physicalBase, uint32 width,
	uint32 height, int8 depth, uint32 bytesPerRow, bool initializing);
void vbe_call_finish(bios_state* state);

#endif	/* VESA_PRIVATE_H */
