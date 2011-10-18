/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */
#ifndef RADEON_HD_H
#define RADEON_HD_H


#include "lock.h"

#include "radeon_reg.h"

#include "rhd_regs.h"	// to phase out
#include "r500_reg.h"
#include "r600_reg.h"
#include "r800_reg.h"

#include <Accelerant.h>
#include <Drivers.h>
#include <edid.h>
#include <PCI.h>


#define VENDOR_ID_ATI	0x1002

#define RADEON_R520		0x0520	// Fudo
#define RADEON_R580		0x0580	// Rodin
#define RADEON_R600		0x0600	// Pele
#define RADEON_R700		0x0700	// Wekiva
#define RADEON_R1000	0x1000	// Evergreen
#define RADEON_R2000	0x2000  // Northern Islands
#define RADEON_R3000	0x3000	// Southern Islands
#define RADEON_R4000	0x4000	// Not yet known / used

#define RADEON_VBIOS_SIZE 0x10000

#define DEVICE_NAME				"radeon_hd"
#define RADEON_ACCELERANT_NAME	"radeon_hd.accelerant"

// Used to collect EDID from boot loader
#define EDID_BOOT_INFO "vesa_edid/v1"
#define MODES_BOOT_INFO "vesa_modes/v1"

#define RHD_POWER_ON       0
#define RHD_POWER_RESET    1   /* off temporarily */
#define RHD_POWER_SHUTDOWN 2   /* long term shutdown */
#define RHD_POWER_UNKNOWN  3   /* initial state */


struct ring_buffer {
	struct lock		lock;
	uint32			register_base;
	uint32			offset;
	uint32			size;
	uint32			position;
	uint32			space_left;
	uint8*			base;
};


struct overlay_registers;


struct radeon_shared_info {
	uint32			device_index;		// accelerant index
	uint32			device_id;			// device pciid
	area_id			mode_list_area;		// area containing display mode list
	uint32			mode_count;

	bool			has_rom;			// was rom mapped?
	area_id			rom_area;			// area of mapped rom
	uint32			rom_phys;			// rom base location
	uint32			rom_size;			// rom size
	uint8*			rom;				// cloned, memory mapped PCI ROM

	display_mode	current_mode;
	uint32			bytes_per_row;
	uint32			bits_per_pixel;
	uint32			dpms_mode;

	area_id			registers_area;			// area of memory mapped registers
	uint8*			status_page;
	addr_t			physical_status_page;
	uint32			graphics_memory_size;

	addr_t			frame_buffer_phys;		// card PCI BAR address of FB
	area_id			frame_buffer_area;		// area of memory mapped FB
	uint32			frame_buffer_int;		// card internal FB location
	uint32			frame_buffer_size;		// card internal FB aperture size
	uint8*			frame_buffer;			// virtual memory mapped FB

	bool			has_edid;
	edid1_info		edid_info;

	struct lock		accelerant_lock;
	struct lock		engine_lock;

	ring_buffer		primary_ring_buffer;

	int32			overlay_channel_used;
	bool			overlay_active;
	uint32			overlay_token;
	addr_t			physical_overlay_registers;
	uint32			overlay_offset;

	bool			hardware_cursor_enabled;
	sem_id			vblank_sem;

	uint8*			cursor_memory;
	addr_t			physical_cursor_memory;
	uint32			cursor_buffer_offset;
	uint32			cursor_format;
	bool			cursor_visible;
	uint16			cursor_hot_x;
	uint16			cursor_hot_y;

	uint16			device_chipset;
	char			device_identifier[32];
};

//----------------- ioctl() interface ----------------

// magic code for ioctls
#define RADEON_PRIVATE_DATA_MAGIC		'rdhd'

// list ioctls
enum {
	RADEON_GET_PRIVATE_DATA = B_DEVICE_OP_CODES_END + 1,

	RADEON_GET_DEVICE_NAME,
	RADEON_ALLOCATE_GRAPHICS_MEMORY,
	RADEON_FREE_GRAPHICS_MEMORY
};

// retrieve the area_id of the kernel/accelerant shared info
struct radeon_get_private_data {
	uint32	magic;				// magic number
	area_id	shared_info_area;
};

// allocate graphics memory
struct radeon_allocate_graphics_memory {
	uint32	magic;
	uint32	size;
	uint32	alignment;
	uint32	flags;
	uint32	buffer_base;
};

// free graphics memory
struct radeon_free_graphics_memory {
	uint32 	magic;
	uint32	buffer_base;
};

// registers
#define R6XX_CONFIG_APER_SIZE			0x5430	// r600>
#define OLD_CONFIG_APER_SIZE			0x0108	// <r600

#define D1CRTC_CONTROL						0x6080
#define     CRTC_EN							(1 << 0)
#define D1CRTC_STATUS						0x609c
#define D1CRTC_UPDATE_LOCK					0x60E8
#define D1GRPH_PRIMARY_SURFACE_ADDRESS		0x6110
#define D1GRPH_SECONDARY_SURFACE_ADDRESS	0x6118
#define D1GRPH_PRIMARY_SURFACE_ADDRESS_HIGH	0x6914		// r700>
#define D1GRPH_SECONDARY_SURFACE_ADDRESS_HIGH 0x691c	// r700>

#define D2CRTC_CONTROL						0x6880
#define D2CRTC_STATUS						0x689c
#define D2CRTC_UPDATE_LOCK					0x68E8
#define D2GRPH_PRIMARY_SURFACE_ADDRESS		0x6910
#define D2GRPH_SECONDARY_SURFACE_ADDRESS	0x6918
#define D2GRPH_PRIMARY_SURFACE_ADDRESS_HIGH	0x6114		// r700>
#define D2GRPH_SECONDARY_SURFACE_ADDRESS_HIGH 0x611c	// r700>

#define D1VGA_CONTROL						0x0330
#define     DVGA_CONTROL_MODE_ENABLE		(1 << 0)
#define     DVGA_CONTROL_TIMING_SELECT		(1 << 8)
#define     DVGA_CONTROL_SYNC_POLARITY_SELECT (1 << 9)
#define     DVGA_CONTROL_OVERSCAN_TIMING_SELECT (1 << 10)
#define     DVGA_CONTROL_OVERSCAN_COLOR_EN	(1 << 16)
#define     DVGA_CONTROL_ROTATE				(1 << 24)
#define D2VGA_CONTROL						0x0338

#define VGA_HDP_CONTROL						0x328
#define     VGA_MEM_PAGE_SELECT_EN			(1 << 0)
#define     VGA_MEMORY_DISABLE				(1 << 4)
#define     VGA_RBBM_LOCK_DISABLE			(1 << 8)
#define     VGA_SOFT_RESET					(1 << 16)
#define VGA_MEMORY_BASE_ADDRESS				0x0310
#define VGA_RENDER_CONTROL					0x0300
#define     VGA_VSTATUS_CNTL_MASK			0x00030000

// PCI bridge memory management

// overlay

#define RADEON_OVERLAY_UPDATE			0x30000
#define RADEON_OVERLAY_TEST				0x30004
#define RADEON_OVERLAY_STATUS			0x30008
#define RADEON_OVERLAY_EXTENDED_STATUS	0x3000c
#define RADEON_OVERLAY_GAMMA_5			0x30010
#define RADEON_OVERLAY_GAMMA_4			0x30014
#define RADEON_OVERLAY_GAMMA_3			0x30018
#define RADEON_OVERLAY_GAMMA_2			0x3001c
#define RADEON_OVERLAY_GAMMA_1			0x30020
#define RADEON_OVERLAY_GAMMA_0			0x30024

struct overlay_scale {
	uint32 _reserved0 : 3;
	uint32 horizontal_scale_fraction : 12;
	uint32 _reserved1 : 1;
	uint32 horizontal_downscale_factor : 3;
	uint32 _reserved2 : 1;
	uint32 vertical_scale_fraction : 12;
};

#define OVERLAY_FORMAT_RGB15			0x2
#define OVERLAY_FORMAT_RGB16			0x3
#define OVERLAY_FORMAT_RGB32			0x1
#define OVERLAY_FORMAT_YCbCr422			0x8
#define OVERLAY_FORMAT_YCbCr411			0x9
#define OVERLAY_FORMAT_YCbCr420			0xc

#define OVERLAY_MIRROR_NORMAL			0x0
#define OVERLAY_MIRROR_HORIZONTAL		0x1
#define OVERLAY_MIRROR_VERTICAL			0x2

// The real overlay registers are written to using an update buffer

struct overlay_registers {
	uint32 buffer_rgb0;
	uint32 buffer_rgb1;
	uint32 buffer_u0;
	uint32 buffer_v0;
	uint32 buffer_u1;
	uint32 buffer_v1;
	// (0x18) OSTRIDE - overlay stride
	uint16 stride_rgb;
	uint16 stride_uv;
	// (0x1c) YRGB_VPH - Y/RGB vertical phase
	uint16 vertical_phase0_rgb;
	uint16 vertical_phase1_rgb;
	// (0x20) UV_VPH - UV vertical phase
	uint16 vertical_phase0_uv;
	uint16 vertical_phase1_uv;
	// (0x24) HORZ_PH - horizontal phase
	uint16 horizontal_phase_rgb;
	uint16 horizontal_phase_uv;
	// (0x28) INIT_PHS - initial phase shift
	uint32 initial_vertical_phase0_shift_rgb0 : 4;
	uint32 initial_vertical_phase1_shift_rgb0 : 4;
	uint32 initial_horizontal_phase_shift_rgb0 : 4;
	uint32 initial_vertical_phase0_shift_uv : 4;
	uint32 initial_vertical_phase1_shift_uv : 4;
	uint32 initial_horizontal_phase_shift_uv : 4;
	uint32 _reserved0 : 8;
	// (0x2c) DWINPOS - destination window position
	uint16 window_left;
	uint16 window_top;
	// (0x30) DWINSZ - destination window size
	uint16 window_width;
	uint16 window_height;
	// (0x34) SWIDTH - source width
	uint16 source_width_rgb;
	uint16 source_width_uv;
	// (0x38) SWITDHSW - source width in 8 byte steps
	uint16 source_bytes_per_row_rgb;
	uint16 source_bytes_per_row_uv;
	uint16 source_height_rgb;
	uint16 source_height_uv;
	overlay_scale scale_rgb;
	overlay_scale scale_uv;
	// (0x48) OCLRC0 - overlay color correction 0
	uint32 brightness_correction : 8;		// signed, -128 to 127
	uint32 _reserved1 : 10;
	uint32 contrast_correction : 9;			// fixed point: 3.6 bits
	uint32 _reserved2 : 5;
	// (0x4c) OCLRC1 - overlay color correction 1
	uint32 saturation_cos_correction : 10;	// fixed point: 3.7 bits
	uint32 _reserved3 : 6;
	uint32 saturation_sin_correction : 11;	// signed fixed point: 3.7 bits
	uint32 _reserved4 : 5;
	// (0x50) DCLRKV - destination color key value
	uint32 color_key_blue : 8;
	uint32 color_key_green : 8;
	uint32 color_key_red : 8;
	uint32 _reserved5 : 8;
	// (0x54) DCLRKM - destination color key mask
	uint32 color_key_mask_blue : 8;
	uint32 color_key_mask_green : 8;
	uint32 color_key_mask_red : 8;
	uint32 _reserved6 : 7;
	uint32 color_key_enabled : 1;
	// (0x58) SCHRKVH - source chroma key high value
	uint32 source_chroma_key_high_red : 8;
	uint32 source_chroma_key_high_blue : 8;
	uint32 source_chroma_key_high_green : 8;
	uint32 _reserved7 : 8;
	// (0x5c) SCHRKVL - source chroma key low value
	uint32 source_chroma_key_low_red : 8;
	uint32 source_chroma_key_low_blue : 8;
	uint32 source_chroma_key_low_green : 8;
	uint32 _reserved8 : 8;
	// (0x60) SCHRKEN - source chroma key enable
	uint32 _reserved9 : 24;
	uint32 source_chroma_key_red_enabled : 1;
	uint32 source_chroma_key_blue_enabled : 1;
	uint32 source_chroma_key_green_enabled : 1;
	uint32 _reserved10 : 5;
	// (0x64) OCONFIG - overlay configuration
	uint32 _reserved11 : 3;
	uint32 color_control_output_mode : 1;
	uint32 yuv_to_rgb_bypass : 1;
	uint32 _reserved12 : 11;
	uint32 gamma2_enabled : 1;
	uint32 _reserved13 : 1;
	uint32 select_pipe : 1;
	uint32 slot_time : 8;
	uint32 _reserved14 : 5;
	// (0x68) OCOMD - overlay command
	uint32 overlay_enabled : 1;
	uint32 active_field : 1;
	uint32 active_buffer : 2;
	uint32 test_mode : 1;
	uint32 buffer_field_mode : 1;
	uint32 _reserved15 : 1;
	uint32 tv_flip_field_enabled : 1;
	uint32 _reserved16 : 1;
	uint32 tv_flip_field_parity : 1;
	uint32 source_format : 4;
	uint32 ycbcr422_order : 2;
	uint32 _reserved18 : 1;
	uint32 mirroring_mode : 2;
	uint32 _reserved19 : 13;

	uint32 _reserved20;

	uint32 start_0y;
	uint32 start_1y;
	uint32 start_0u;
	uint32 start_0v;
	uint32 start_1u;
	uint32 start_1v;
	uint32 _reserved21[6];
#if 0
	// (0x70) AWINPOS - alpha blend window position
	uint32 awinpos;
	// (0x74) AWINSZ - alpha blend window size
	uint32 awinsz;

	uint32 _reserved21[10];
#endif

	// (0xa0) FASTHSCALE - fast horizontal downscale (strangely enough,
	// the next two registers switch the usual Y/RGB vs. UV order)
	uint16 horizontal_scale_uv;
	uint16 horizontal_scale_rgb;
	// (0xa4) UVSCALEV - vertical downscale
	uint16 vertical_scale_uv;
	uint16 vertical_scale_rgb;

	uint32 _reserved22[86];

	// (0x200) polyphase filter coefficients
	uint16 vertical_coefficients_rgb[128];
	uint16 horizontal_coefficients_rgb[128];

	uint32	_reserved23[64];

	// (0x500)
	uint16 vertical_coefficients_uv[128];
	uint16 horizontal_coefficients_uv[128];
};


struct hardware_status {
	uint32	interrupt_status_register;
	uint32	_reserved0[3];
	void*	primary_ring_head_storage;
	uint32	_reserved1[3];
	void*	secondary_ring_0_head_storage;
	void*	secondary_ring_1_head_storage;
	uint32	_reserved2[2];
	void*	binning_head_storage;
	uint32	_reserved3[3];
	uint32	store[1008];
};

#endif	/* RADEON_HD_H */
