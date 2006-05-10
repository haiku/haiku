/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef INTEL_EXTREME_H
#define INTEL_EXTREME_H


#include "lock.h"

#include <memory_manager.h>

#include <Accelerant.h>
#include <Drivers.h>
#include <PCI.h>


#define VENDOR_ID_INTEL			0x8086

enum {
	INTEL_TYPE_7xx,
	INTEL_TYPE_8xx,
	INTEL_TYPE_9xx,
};

#define DEVICE_NAME				"intel_extreme"
#define INTEL_ACCELERANT_NAME	"intel_extreme.accelerant"

#define INTEL_COOKIE_MAGIC		'intl'
#define INTEL_FREE_COOKIE_MAGIC 'itlf'

// info about PLL on graphics card
struct pll_info {
	uint32			reference_frequency;
	uint32			max_frequency;
	uint32			min_frequency;
	uint32			divisor_register;
};

struct ring_buffer {
	struct lock		lock;
	uint32			register_base;
	uint32			handle;
	uint32			offset;
	uint32			size;
	uint32			position;
	uint8			*base;
};

struct overlay_registers;

struct intel_shared_info {
	int32			type;
	area_id			mode_list_area;		// area containing display mode list
	uint32			mode_count;

	display_mode	current_mode;
	uint32			bytes_per_row;
	uint32			bits_per_pixel;
	uint32			dpms_mode;

	area_id			registers_area;			// area of memory mapped registers
	uint8			*physical_status_page;
	uint8			*physical_cursor_memory;
	area_id			graphics_memory_area;
	uint8			*graphics_memory;
	uint8			*physical_graphics_memory;
	uint32			graphics_memory_size;

	uint32			frame_buffer_offset;

	ring_buffer		primary_ring_buffer;
	ring_buffer		secondary_ring_buffer;

	int32			overlay_channel_used;
	bool			overlay_active;
	uint32			overlay_token;
	uint8*			physical_overlay_registers;

	uint32			device_type;
	char			device_identifier[32];
	struct pll_info	pll_info;
};

struct intel_info {
	uint32			cookie_magic;
	int32			open_count;
	int32			id;
	pci_info		*pci;
	uint8			*registers;
	area_id			registers_area;
	struct intel_shared_info *shared_info;
	area_id			shared_area;

	struct overlay_registers *overlay_registers;
		// update buffer, shares an area with shared_info

	uint8			*graphics_memory;
	area_id			graphics_memory_area;
	mem_info		*memory_manager;

	const char		*device_identifier;
	uint32			device_type;
};

//----------------- ioctl() interface ----------------

// magic code for ioctls
#define INTEL_PRIVATE_DATA_MAGIC		'itic'

// list ioctls
enum {
	INTEL_GET_PRIVATE_DATA = B_DEVICE_OP_CODES_END + 1,

	INTEL_GET_DEVICE_NAME,
	INTEL_ALLOCATE_GRAPHICS_MEMORY,
	INTEL_FREE_GRAPHICS_MEMORY
};

// retrieve the area_id of the kernel/accelerant shared info
struct intel_get_private_data {
	uint32	magic;				// magic number
	area_id	shared_info_area;
};

// allocate graphics memory
struct intel_allocate_graphics_memory {
	uint32	magic;
	uint32	size;
	uint32	buffer_offset;
	uint32	handle;
};

// free graphics memory
struct intel_free_graphics_memory {
	uint32 	magic;
	uint32	handle;
};

//----------------------------------------------------------
// Register definitions, taken from X driver

// PCI bridge memory management
#define INTEL_GRAPHICS_MEMORY_CONTROL	0x52
#define STOLEN_MEMORY_MASK				0x70

// models i830 and up
#define i830_LOCAL_MEMORY_ONLY			0x10
#define i830_STOLEN_512K				0x20
#define i830_STOLEN_1M					0x30
#define i830_STOLEN_8M					0x40

// models i855 and up
#define i855_STOLEN_MEMORY_1M			0x10
#define i855_STOLEN_MEMORY_4M			0x20
#define i855_STOLEN_MEMORY_8M			0x30
#define i855_STOLEN_MEMORY_16M			0x40
#define i855_STOLEN_MEMORY_32M			0x50
#define i855_STOLEN_MEMORY_48M			0x60
#define i855_STOLEN_MEMORY_64M			0x70


#define INTEL_PAGE_TABLE_CONTROL		0x02020
#define INTEL_PAGE_TABLE_ERROR			0x02024
#define INTEL_HARDWARE_STATUS_PAGE		0x02080

#define INTEL_PRIMARY_RING_BUFFER		0x02030
#define INTEL_SECONDARY_RING_BUFFER_0	0x02100
#define INTEL_SECONDARY_RING_BUFFER_1	0x02110
// offsets for the ring buffer base registers above
#define RING_BUFFER_TAIL				0x0
#define RING_BUFFER_HEAD				0x4
#define RING_BUFFER_START				0x8
#define RING_BUFFER_CONTROL				0xc
#define INTEL_RING_BUFFER_SIZE_MASK		0x000ff800
#define INTEL_RING_BUFFER_ENABLED		1

#define INTEL_DISPLAY_HTOTAL			0x60000
#define INTEL_DISPLAY_HBLANK			0x60004
#define INTEL_DISPLAY_HSYNC				0x60008
#define INTEL_DISPLAY_VTOTAL			0x6000c
#define INTEL_DISPLAY_VBLANK			0x60010
#define INTEL_DISPLAY_VSYNC				0x60014
#define INTEL_DISPLAY_IMAGE_SIZE		0x6001c

#define INTEL_DISPLAY_CONTROL			0x70180
#define INTEL_DISPLAY_BASE				0x70184
#define INTEL_DISPLAY_BYTES_PER_ROW		0x70188
#define DISPLAY_CONTROL_ENABLED			(1UL << 31)
#define DISPLAY_CONTROL_COLOR_MASK		(0x0fUL << 26)
#define DISPLAY_CONTROL_CMAP8			(2UL << 26)
#define DISPLAY_CONTROL_RGB15			(4UL << 26)
#define DISPLAY_CONTROL_RGB16			(5UL << 26)
#define DISPLAY_CONTROL_RGB32			(7UL << 26)

#define INTEL_DISPLAY_PALETTE			0x0a000

#define INTEL_DISPLAY_PIPE_CONTROL		0x70008
#define DISPLAY_PIPE_ENABLED			(1UL << 31)

#define INTEL_DISPLAY_PLL				0x06014
#define INTEL_DISPLAY_PLL_DIVISOR_0		0x06040
#define INTEL_DISPLAY_PLL_DIVISOR_1		0x06044
#define DISPLAY_PLL_ENABLED				(1UL << 31)
#define DISPLAY_PLL_2X_CLOCK			(1UL << 30)
#define DISPLAY_PLL_SYNC_LOCK_ENABLED	(1UL << 29)
#define DISPLAY_PLL_NO_VGA_CONTROL		(1UL << 28)
#define DISPLAY_PLL_DIVIDE_4X			(1UL << 23)
#define DISPLAY_PLL_POST_DIVISOR_MASK	0x001f0000
#define DISPLAY_PLL_POST_DIVISOR_SHIFT	16
#define DISPLAY_PLL_DIVISOR_1			(1UL << 8)
#define DISPLAY_PLL_N_DIVISOR_MASK		0x001f0000
#define DISPLAY_PLL_M1_DIVISOR_MASK		0x00001f00
#define DISPLAY_PLL_M2_DIVISOR_MASK		0x0000001f
#define DISPLAY_PLL_N_DIVISOR_SHIFT		16
#define DISPLAY_PLL_M1_DIVISOR_SHIFT	8
#define DISPLAY_PLL_M2_DIVISOR_SHIFT	0

#define INTEL_DISPLAY_ANALOG_PORT		0x61100
#define DISPLAY_MONITOR_PORT_ENABLED	(1UL << 31)
#define DISPLAY_MONITOR_VGA_POLARITY	(1UL << 15)
#define DISPLAY_MONITOR_MODE_MASK		(3UL << 10)
#define DISPLAY_MONITOR_ON				0
#define DISPLAY_MONITOR_SUSPEND			(1UL << 10)
#define DISPLAY_MONITOR_STAND_BY		(2UL << 10)
#define DISPLAY_MONITOR_OFF				(3UL << 10)
#define DISPLAY_MONITOR_POLARITY_MASK	(3UL << 3)
#define DISPLAY_MONITOR_POSITIVE_HSYNC	(1UL << 3)
#define DISPLAY_MONITOR_POSITIVE_VSYNC	(2UL << 3)

// ring buffer commands

#define COMMAND_NOOP					0x00
#define COMMAND_WAIT_FOR_EVENT			(0x03 << 23)
#define COMMAND_WAIT_FOR_OVERLAY_FLIP	(1 << 16)

#define COMMAND_FLUSH					(0x04 << 23)

// overlay flip
#define COMMAND_OVERLAY_FLIP			(0x11 << 23)
#define COMMAND_OVERLAY_CONTINUE		(0 << 21)
#define COMMAND_OVERLAY_ON				(1 << 21)
#define COMMAND_OVERLAY_OFF				(2 << 21)
#define OVERLAY_UPDATE_COEFFICIENTS		0x1

// 2D acceleration
#define COMMAND_BLIT					0x54c00006

// overlay

#define INTEL_OVERLAY_UPDATE			0x30000
#define INTEL_OVERLAY_TEST				0x30004
#define INTEL_OVERLAY_STATUS			0x30008
#define INTEL_OVERLAY_EXTENDED_STATUS	0x3000c
#define INTEL_OVERLAY_GAMMA_5			0x30010
#define INTEL_OVERLAY_GAMMA_4			0x30014
#define INTEL_OVERLAY_GAMMA_3			0x30018
#define INTEL_OVERLAY_GAMMA_2			0x3001c
#define INTEL_OVERLAY_GAMMA_1			0x30020
#define INTEL_OVERLAY_GAMMA_0			0x30024

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

	// (0x70) AWINPOS - alpha blend window position
	uint32 awinpos;
	// (0x74) AWINSZ - alpha blend window size
	uint32 awinsz;

	uint32 _reserved21[10];

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
	void	*primary_ring_head_storage;
	uint32	_reserved1[3];
	void	*secondary_ring_0_head_storage;
	void	*secondary_ring_1_head_storage;
	uint32	_reserved2[2];
	void	*binning_head_storage;
	uint32	_reserved3[3];
	uint32	store[1008];
};

//----------------------------------------------------------

extern status_t intel_extreme_init(intel_info &info);
extern void intel_extreme_uninit(intel_info &info);

#endif	/* INTEL_EXTREME_H */
