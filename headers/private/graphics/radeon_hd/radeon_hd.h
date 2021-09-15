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

//#include "r500_reg.h"  // Not used atm. DCE 0
#include "avivo_reg.h"		// DCE 1
#include "r600_reg.h"		// DCE 2
#include "r700_reg.h"		// DCE 3
#include "evergreen_reg.h"	// DCE 4
#include "ni_reg.h"			// DCE 5
#include "si_reg.h"			// DCE 6
#include "sea_reg.h"		// DCE 8
#include "vol_reg.h"		// DCE 10
#include "car_reg.h"		// DCE 11

#include <Accelerant.h>
#include <Drivers.h>
#include <edid.h>
#include <PCI.h>


#define VENDOR_ID_ATI	0x1002

// Card chipset flags
#define CHIP_STD		(1 << 0) // Standard chipset
#define CHIP_X2			(1 << 1) // Dual cpu
#define CHIP_IGP		(1 << 2) // IGP chipset
#define CHIP_MOBILE		(1 << 3) // Mobile chipset
#define CHIP_DISCREET	(1 << 4) // Discreet chipset
#define CHIP_APU		(1 << 5) // APU chipset

#define DEVICE_NAME				"radeon_hd"
#define RADEON_ACCELERANT_NAME	"radeon_hd.accelerant"

#define MAX_NAME_LENGTH		32

// Used to collect EDID from boot loader
#define EDID_BOOT_INFO "vesa_edid/v1"
#define MODES_BOOT_INFO "vesa_modes/v1"

#define RHD_POWER_ON       0
#define RHD_POWER_RESET    1   /* off temporarily */
#define RHD_POWER_SHUTDOWN 2   /* long term shutdown */
#define RHD_POWER_UNKNOWN  3   /* initial state */


// Radeon Chipsets
// !! Must match chipset names below
enum radeon_chipset {
	RADEON_R420 = 0,	//r400, Radeon X700-X850
	RADEON_R423,
	RADEON_RV410,
	RADEON_RS400,
	RADEON_RS480,
	RADEON_RS600,
	RADEON_RS690,
	RADEON_RS740,
	RADEON_RV515,
	RADEON_R520,		//r500, DCE 1.0
	RADEON_RV530,		// DCE 1.0
	RADEON_RV560,		// DCE 1.0
	RADEON_RV570,		// DCE 1.0
	RADEON_R580,		// DCE 1.0
	RADEON_R600,		//r600, DCE 2.0
	RADEON_RV610,		// DCE 2.0
	RADEON_RV630,		// DCE 2.0
	RADEON_RV670,		// DCE 2.0
	RADEON_RV620,		// DCE 3.0
	RADEON_RV635,		// DCE 3.0
	RADEON_RS780,		// DCE 3.0
	RADEON_RS880,		// DCE 3.0
	RADEON_RV770,		//r700, DCE 3.1
	RADEON_RV730,		// DCE 3.2
	RADEON_RV710,		// DCE 3.2
	RADEON_RV740,		// DCE 3.2
	RADEON_CEDAR,		//Evergreen, DCE 4.0
	RADEON_REDWOOD,		// DCE 4.0
	RADEON_JUNIPER,		// DCE 4.0
	RADEON_CYPRESS,		// DCE 4.0
	RADEON_HEMLOCK,		// DCE 4.0?
	RADEON_PALM,		//Fusion APU (NI), DCE 4.1
	RADEON_SUMO,		// DCE 4.1
	RADEON_SUMO2,		// DCE 4.1
	RADEON_CAICOS,		//Nothern Islands, DCE 5.0
	RADEON_TURKS,		// DCE 5.0
	RADEON_BARTS,		// DCE 5.0
	RADEON_CAYMAN,		// DCE 5.0
	RADEON_ANTILLES,	// DCE 5.0?
	RADEON_CAPEVERDE,	//Southern Islands, DCE 6.0
	RADEON_PITCAIRN,	// DCE 6.0
	RADEON_TAHITI,		// DCE 6.0
	RADEON_ARUBA,		// DCE 6.1 Trinity/Richland
	RADEON_OLAND,		// DCE 6.4
	RADEON_HAINAN,		// NO DCE, only compute
	RADEON_KAVERI,		//Sea Islands, DCE 8.1
	RADEON_BONAIRE,		// DCE 8.2
	RADEON_KABINI,		// DCE 8.3
	RADEON_MULLINS,		// DCE 8.3
	RADEON_HAWAII,		// DCE 8.5
	RADEON_TOPAZ,		//Volcanic Islands, NO DCE
	RADEON_TONGA,		// DCE 10.0
	RADEON_FIJI,		// DCE 10.1?
	RADEON_CARRIZO,		// DCE 11.0
	RADEON_STONEY,		// DCE 11.1?
	RADEON_POLARIS11,	//Artic Islands, DCE 12.1?
	RADEON_POLARIS10,	// DCE 12.0?
	RADEON_POLARIS12,	// DCE 12.2?
	RADEON_VEGAM,		// DCE 13.0?
	RADEON_VEGA10,		// DCE 13.0?
	RADEON_VEGA12,		// DCE 13.0?
	RADEON_VEGA20,		// DCE 13.0?
	RADEON_RAVEN,		// DCE 13?
	RADEON_NAVI,		// DCE 13.0?
};

// !! Must match chipset families above
static const char radeon_chip_name[][MAX_NAME_LENGTH] = {
	"R420",
	"R423",
	"RV410",
	"RS400",
	"RS480",
	"RS600",
	"RS690",
	"RS740",
	"RV515",
	"R520",
	"RV530",
	"RV560",
	"RV570",
	"R580",
	"R600",
	"RV610",
	"RV630",
	"RV670",
	"RV620",
	"RV635",
	"RS780",
	"RS880",
	"RV770",
	"RV730",
	"RV710",
	"RV740",
	"Cedar",
	"Redwood",
	"Juniper",
	"Cypress",
	"Hemlock",
	"Palm",
	"Sumo",
	"Sumo2",
	"Caicos",
	"Turks",
	"Barts",
	"Cayman",
	"Antilles",
	"Cape Verde",
	"Pitcairn",
	"Tahiti",
	"Aruba",
	"Oland",
	"Hainan",
	"Kaveri",
	"Bonaire",
	"Kabini",
	"Mullins",
	"Hawaii",
	"Topaz",
	"Tonga",
	"Fiji",
	"Carrizo",
	"Stoney Ridge",
	"Polaris 11",
	"Polaris 10",
	"Polaris 12",
	"Vega Mobile",
	"Vega 10",
	"Vega 12",
	"Vega 20",
	"Raven",
	"Navi",
};


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
	uint32			deviceIndex;		// accelerant index
	uint32			pciID;				// device pci id
	uint32			pciRev;				// device pci revision
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

	uint8*			frame_buffer;			// virtual memory mapped FB
	area_id			frame_buffer_area;		// area of memory mapped FB
	addr_t			frame_buffer_phys;		// card PCI BAR address of FB
	uint32			frame_buffer_size;		// FB size mapped

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

	char			deviceName[MAX_NAME_LENGTH];
	uint16			chipsetID;
	char			chipsetName[MAX_NAME_LENGTH];
	uint32			chipsetFlags;
	uint8			dceMajor;
	uint8			dceMinor;

	uint16			color_data[3 * 256];    // colour lookup table
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
#define CONFIG_MEMSIZE                  0x5428	// r600>

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
