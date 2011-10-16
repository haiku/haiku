/*
 * Copyright 2006-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef INTEL_EXTREME_H
#define INTEL_EXTREME_H


#include "lock.h"

#include <Accelerant.h>
#include <Drivers.h>
#include <PCI.h>


#define VENDOR_ID_INTEL			0x8086

#define INTEL_TYPE_FAMILY_MASK	0x000f0000
#define INTEL_TYPE_GROUP_MASK	0x000ffff0
#define INTEL_TYPE_MODEL_MASK	0x000fffff
// families
#define INTEL_TYPE_7xx			0x00010000
#define INTEL_TYPE_8xx			0x00020000
#define INTEL_TYPE_9xx			0x00040000
// groups
#define INTEL_TYPE_83x			(INTEL_TYPE_8xx | 0x0010)
#define INTEL_TYPE_85x			(INTEL_TYPE_8xx | 0x0020)
#define INTEL_TYPE_91x			(INTEL_TYPE_9xx | 0x0040)
#define INTEL_TYPE_94x			(INTEL_TYPE_9xx | 0x0080)
#define INTEL_TYPE_96x			(INTEL_TYPE_9xx | 0x0100)
#define INTEL_TYPE_Gxx			(INTEL_TYPE_9xx | 0x0200)
#define INTEL_TYPE_G4x			(INTEL_TYPE_9xx | 0x0400)
#define INTEL_TYPE_IGD			(INTEL_TYPE_9xx | 0x0800)
#define INTEL_TYPE_SNB			(INTEL_TYPE_9xx | 0x1000)
// models
#define INTEL_TYPE_MOBILE		0x0008
#define INTEL_TYPE_915			(INTEL_TYPE_91x)
#define INTEL_TYPE_915M			(INTEL_TYPE_91x | INTEL_TYPE_MOBILE)
#define INTEL_TYPE_945			(INTEL_TYPE_94x)
#define INTEL_TYPE_945M			(INTEL_TYPE_94x | INTEL_TYPE_MOBILE)
#define INTEL_TYPE_965			(INTEL_TYPE_96x)
#define INTEL_TYPE_965M			(INTEL_TYPE_96x | INTEL_TYPE_MOBILE)
#define INTEL_TYPE_G33			(INTEL_TYPE_Gxx)
#define INTEL_TYPE_G45			(INTEL_TYPE_G4x)
#define INTEL_TYPE_GM45			(INTEL_TYPE_G4x | INTEL_TYPE_MOBILE)
#define INTEL_TYPE_IGDG			(INTEL_TYPE_IGD)
#define INTEL_TYPE_IGDGM		(INTEL_TYPE_IGD | INTEL_TYPE_MOBILE)
#define INTEL_TYPE_SNBG			(INTEL_TYPE_SNB)
#define INTEL_TYPE_SNBGM		(INTEL_TYPE_SNB | INTEL_TYPE_MOBILE)

#define DEVICE_NAME				"intel_extreme"
#define INTEL_ACCELERANT_NAME	"intel_extreme.accelerant"

// We encode the register block into the value and extract/translate it when
// actually accessing.
#define REGISTER_BLOCK_COUNT				7
#define REGISTER_BLOCK_SHIFT				24
#define REGISTER_BLOCK_MASK					0xff000000
#define REGISTER_REGISTER_MASK				0x00ffffff
#define REGISTER_BLOCK(x) ((x & REGISTER_BLOCK_MASK) >> REGISTER_BLOCK_SHIFT)
#define REGISTER_REGISTER(x) (x & REGISTER_REGISTER_MASK)

#define REGS_FLAT							(0 << REGISTER_BLOCK_SHIFT)
#define REGS_INTERRUPT						(1 << REGISTER_BLOCK_SHIFT)
#define REGS_NORTH_SHARED					(2 << REGISTER_BLOCK_SHIFT)
#define REGS_NORTH_PIPE_AND_PORT			(3 << REGISTER_BLOCK_SHIFT)
#define REGS_NORTH_PLANE_CONTROL			(4 << REGISTER_BLOCK_SHIFT)
#define REGS_SOUTH_SHARED					(5 << REGISTER_BLOCK_SHIFT)
#define REGS_SOUTH_TRANSCODER_PORT			(6 << REGISTER_BLOCK_SHIFT)

// register blocks for (G)MCH/ICH based platforms
#define MCH_INTERRUPT_REGISTER_BASE						0x020a0
#define MCH_SHARED_REGISTER_BASE						0x00000
#define MCH_PIPE_AND_PORT_REGISTER_BASE					0x60000
#define MCH_PLANE_CONTROL_REGISTER_BASE					0x70000
#define ICH_SHARED_REGISTER_BASE						0x00000
#define ICH_PORT_REGISTER_BASE							0x60000

// PCH - Platform Control Hub - Newer hardware moves from a MCH/ICH based setup
// to a PCH based one, that means anything that used to communicate via (G)MCH
// registers needs to use different ones on PCH based platforms (Ironlake and
// up, SandyBridge, etc.).
#define PCH_DE_INTERRUPT_REGISTER_BASE					0x44000
#define PCH_NORTH_SHARED_REGISTER_BASE					0x40000
#define PCH_NORTH_PIPE_AND_PORT_REGISTER_BASE			0x60000
#define PCH_NORTH_PLANE_CONTROL_REGISTER_BASE			0x70000
#define PCH_SOUTH_SHARED_REGISTER_BASE					0xc0000
#define PCH_SOUTH_TRANSCODER_AND_PORT_REGISTER_BASE		0xe0000


struct DeviceType {
	uint32			type;

	DeviceType(int t)
	{
		type = t;
	}

	DeviceType& operator=(int t)
	{
		type = t;
		return *this;
	}

	bool InFamily(uint32 family) const
	{
		return (type & INTEL_TYPE_FAMILY_MASK) == family;
	}

	bool InGroup(uint32 group) const
	{
		return (type & INTEL_TYPE_GROUP_MASK) == group;
	}

	bool IsModel(uint32 model) const
	{
		return (type & INTEL_TYPE_MODEL_MASK) == model;
	}
};

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
	uint32			offset;
	uint32			size;
	uint32			position;
	uint32			space_left;
	uint8*			base;
};

struct overlay_registers;

struct intel_shared_info {
	area_id			mode_list_area;		// area containing display mode list
	uint32			mode_count;

	display_mode	current_mode;
	uint32			bytes_per_row;
	uint32			bits_per_pixel;
	uint32			dpms_mode;

	area_id			registers_area;			// area of memory mapped registers
	uint32			register_blocks[REGISTER_BLOCK_COUNT];
	uint8*			status_page;
	phys_addr_t		physical_status_page;
	uint8*			graphics_memory;
	phys_addr_t		physical_graphics_memory;
	uint32			graphics_memory_size;

	addr_t			frame_buffer;
	uint32			frame_buffer_offset;

	struct lock		accelerant_lock;
	struct lock		engine_lock;

	ring_buffer		primary_ring_buffer;

	int32			overlay_channel_used;
	bool			overlay_active;
	uint32			overlay_token;
	phys_addr_t		physical_overlay_registers;
	uint32			overlay_offset;

	bool			hardware_cursor_enabled;
	sem_id			vblank_sem;

	uint8*			cursor_memory;
	phys_addr_t		physical_cursor_memory;
	uint32			cursor_buffer_offset;
	uint32			cursor_format;
	bool			cursor_visible;
	uint16			cursor_hot_x;
	uint16			cursor_hot_y;

	DeviceType		device_type;
	char			device_identifier[32];
	struct pll_info	pll_info;
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
	uint32	alignment;
	uint32	flags;
	uint32	buffer_base;
};

// free graphics memory
struct intel_free_graphics_memory {
	uint32 	magic;
	uint32	buffer_base;
};

//----------------------------------------------------------
// Register definitions, taken from X driver

// PCI bridge memory management
#define INTEL_GRAPHICS_MEMORY_CONTROL	0x52	// GGC - (G)MCH Graphics Control Register
#define MEMORY_CONTROL_ENABLED			0x0004
#define MEMORY_MASK						0x0001
#define STOLEN_MEMORY_MASK				0x00f0
#define i965_GTT_MASK					0x000e
#define G33_GTT_MASK					0x0300
#define G4X_GTT_MASK					0x0f00	// GGMS (GSM Memory Size) mask

// models i830 and up
#define i830_LOCAL_MEMORY_ONLY			0x10
#define i830_STOLEN_512K				0x20
#define i830_STOLEN_1M					0x30
#define i830_STOLEN_8M					0x40
#define i830_FRAME_BUFFER_64M			0x01
#define i830_FRAME_BUFFER_128M			0x00

// models i855 and up
#define i855_STOLEN_MEMORY_1M			0x10
#define i855_STOLEN_MEMORY_4M			0x20
#define i855_STOLEN_MEMORY_8M			0x30
#define i855_STOLEN_MEMORY_16M			0x40
#define i855_STOLEN_MEMORY_32M			0x50
#define i855_STOLEN_MEMORY_48M			0x60
#define i855_STOLEN_MEMORY_64M			0x70
#define i855_STOLEN_MEMORY_128M			0x80
#define i855_STOLEN_MEMORY_256M			0x90

#define G4X_STOLEN_MEMORY_96MB			0xa0	// GMS - Graphics Mode Select
#define G4X_STOLEN_MEMORY_160MB			0xb0
#define G4X_STOLEN_MEMORY_224MB			0xc0
#define G4X_STOLEN_MEMORY_352MB			0xd0

// SandyBridge (SNB)
#define SNB_GRAPHICS_MEMORY_CONTROL		0x50

#define SNB_STOLEN_MEMORY_MASK			0xf8
#define SNB_STOLEN_MEMORY_32MB			(1 << 3)
#define SNB_STOLEN_MEMORY_64MB			(2 << 3)
#define SNB_STOLEN_MEMORY_96MB			(3 << 3)
#define SNB_STOLEN_MEMORY_128MB			(4 << 3)
#define SNB_STOLEN_MEMORY_160MB			(5 << 3)
#define SNB_STOLEN_MEMORY_192MB			(6 << 3)
#define SNB_STOLEN_MEMORY_224MB			(7 << 3)
#define SNB_STOLEN_MEMORY_256MB			(8 << 3)
#define SNB_STOLEN_MEMORY_288MB			(9 << 3)
#define SNB_STOLEN_MEMORY_320MB			(10 << 3)
#define SNB_STOLEN_MEMORY_352MB			(11 << 3)
#define SNB_STOLEN_MEMORY_384MB			(12 << 3)
#define SNB_STOLEN_MEMORY_416MB			(13 << 3)
#define SNB_STOLEN_MEMORY_448MB			(14 << 3)
#define SNB_STOLEN_MEMORY_480MB			(15 << 3)
#define SNB_STOLEN_MEMORY_512MB			(16 << 3)

#define SNB_GTT_SIZE_MASK				(3 << 8)
#define SNB_GTT_SIZE_NONE				(0 << 8)
#define SNB_GTT_SIZE_1MB				(1 << 8)
#define SNB_GTT_SIZE_2MB				(2 << 8)

// graphics page translation table
#define INTEL_PAGE_TABLE_CONTROL		0x02020
#define PAGE_TABLE_ENABLED				0x00000001
#define INTEL_PAGE_TABLE_ERROR			0x02024
#define INTEL_HARDWARE_STATUS_PAGE		0x02080
#define i915_GTT_BASE					0x1c
#define i830_GTT_BASE					0x10000	// (- 0x2ffff)
#define i830_GTT_SIZE					0x20000
#define i965_GTT_BASE					0x80000	// (- 0xfffff)
#define i965_GTT_SIZE					0x80000
#define i965_GTT_128K					(2 << 1)
#define i965_GTT_256K					(1 << 1)
#define i965_GTT_512K					(0 << 1)
#define G33_GTT_1M						(1 << 8)
#define G33_GTT_2M						(2 << 8)
#define G4X_GTT_NONE					0x000	// GGMS - GSM Memory Size
#define G4X_GTT_1M_NO_IVT				0x100	// no Intel Virtualization Tech.
#define G4X_GTT_2M_NO_IVT				0x300
#define G4X_GTT_2M_IVT					0x900	// with Intel Virt. Tech.
#define G4X_GTT_3M_IVT					0xa00
#define G4X_GTT_4M_IVT					0xb00


#define GTT_ENTRY_VALID					0x01
#define GTT_ENTRY_LOCAL_MEMORY			0x02
#define GTT_PAGE_SHIFT					12


// ring buffer
#define INTEL_PRIMARY_RING_BUFFER		0x02030
#define INTEL_SECONDARY_RING_BUFFER_0	0x02100
#define INTEL_SECONDARY_RING_BUFFER_1	0x02110
// offsets for the ring buffer base registers above
#define RING_BUFFER_TAIL				0x0
#define RING_BUFFER_HEAD				0x4
#define RING_BUFFER_START				0x8
#define RING_BUFFER_CONTROL				0xc
#define INTEL_RING_BUFFER_SIZE_MASK		0x001ff000
#define INTEL_RING_BUFFER_HEAD_MASK		0x001ffffc
#define INTEL_RING_BUFFER_ENABLED		1

// interrupts
#define INTEL_INTERRUPT_ENABLED			(0x0000 | REGS_INTERRUPT)
#define INTEL_INTERRUPT_IDENTITY		(0x0004 | REGS_INTERRUPT)
#define INTEL_INTERRUPT_MASK			(0x0008 | REGS_INTERRUPT)
#define INTEL_INTERRUPT_STATUS			(0x000c | REGS_INTERRUPT)
#define INTERRUPT_VBLANK_PIPEA			(1 << 7)
#define INTERRUPT_VBLANK_PIPEB			(1 << 5)
// TODO: verify that these are actually different on older versions
#define PCH_INTERRUPT_VBLANK_PIPEA		(1 << 7)
#define PCH_INTERRUPT_VBLANK_PIPEB		(1 << 15)

// display ports
#define INTEL_DISPLAY_A_ANALOG_PORT		(0x1100 | REGS_SOUTH_TRANSCODER_PORT)
#define DISPLAY_MONITOR_PORT_ENABLED	(1UL << 31)
#define DISPLAY_MONITOR_PIPE_B			(1UL << 30)
#define DISPLAY_MONITOR_VGA_POLARITY	(1UL << 15)
#define DISPLAY_MONITOR_MODE_MASK		(3UL << 10)
#define DISPLAY_MONITOR_ON				0
#define DISPLAY_MONITOR_SUSPEND			(1UL << 10)
#define DISPLAY_MONITOR_STAND_BY		(2UL << 10)
#define DISPLAY_MONITOR_OFF				(3UL << 10)
#define DISPLAY_MONITOR_POLARITY_MASK	(3UL << 3)
#define DISPLAY_MONITOR_POSITIVE_HSYNC	(1UL << 3)
#define DISPLAY_MONITOR_POSITIVE_VSYNC	(2UL << 3)
#define INTEL_DISPLAY_A_DIGITAL_PORT	(0x1120 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_DISPLAY_C_DIGITAL			(0x1160 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_DISPLAY_LVDS_PORT			(0x1180 | REGS_SOUTH_TRANSCODER_PORT)
#define LVDS_POST2_RATE_SLOW			14 // PLL Divisors
#define LVDS_POST2_RATE_FAST			7
#define LVDS_CLKB_POWER_MASK			(3 << 4)
#define LVDS_CLKB_POWER_UP				(3 << 4)
#define LVDS_PORT_EN					(1 << 31)
#define LVDS_A0A2_CLKA_POWER_UP			(3 << 8)
#define LVDS_PIPEB_SELECT				(1 << 30)
#define LVDS_B0B3PAIRS_POWER_UP			(3 << 2)
#define LVDS_PLL_MODE_LVDS				(2 << 26)
#define LVDS_18BIT_DITHER				(1 << 25)

// PLL flags
#define DISPLAY_PLL_ENABLED				(1UL << 31)
#define DISPLAY_PLL_2X_CLOCK			(1UL << 30)
#define DISPLAY_PLL_SYNC_LOCK_ENABLED	(1UL << 29)
#define DISPLAY_PLL_NO_VGA_CONTROL		(1UL << 28)
#define DISPLAY_PLL_MODE_ANALOG			(1UL << 26)
#define DISPLAY_PLL_DIVIDE_HIGH			(1UL << 24)
#define DISPLAY_PLL_DIVIDE_4X			(1UL << 23)
#define DISPLAY_PLL_POST1_DIVIDE_2		(1UL << 21)
#define DISPLAY_PLL_POST1_DIVISOR_MASK	0x001f0000
#define DISPLAY_PLL_9xx_POST1_DIVISOR_MASK	0x00ff0000
#define DISPLAY_PLL_IGD_POST1_DIVISOR_MASK  0x00ff8000
#define DISPLAY_PLL_POST1_DIVISOR_SHIFT	16
#define DISPLAY_PLL_IGD_POST1_DIVISOR_SHIFT	15
#define DISPLAY_PLL_DIVISOR_1			(1UL << 8)
#define DISPLAY_PLL_N_DIVISOR_MASK		0x001f0000
#define DISPLAY_PLL_IGD_N_DIVISOR_MASK	0x00ff0000
#define DISPLAY_PLL_M1_DIVISOR_MASK		0x00001f00
#define DISPLAY_PLL_M2_DIVISOR_MASK		0x0000001f
#define DISPLAY_PLL_IGD_M2_DIVISOR_MASK	0x000000ff
#define DISPLAY_PLL_N_DIVISOR_SHIFT		16
#define DISPLAY_PLL_M1_DIVISOR_SHIFT	8
#define DISPLAY_PLL_M2_DIVISOR_SHIFT	0
#define DISPLAY_PLL_PULSE_PHASE_SHIFT	9

// display
#define INTEL_DISPLAY_A_HTOTAL			(0x0000 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_DISPLAY_A_HBLANK			(0x0004 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_DISPLAY_A_HSYNC			(0x0008 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_DISPLAY_A_VTOTAL			(0x000c | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_DISPLAY_A_VBLANK			(0x0010 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_DISPLAY_A_VSYNC			(0x0014 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_DISPLAY_B_HTOTAL			(0x1000 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_DISPLAY_B_HBLANK			(0x1004 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_DISPLAY_B_HSYNC			(0x1008 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_DISPLAY_B_VTOTAL			(0x100c | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_DISPLAY_B_VBLANK			(0x1010 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_DISPLAY_B_VSYNC			(0x1014 | REGS_SOUTH_TRANSCODER_PORT)

#define INTEL_DISPLAY_A_IMAGE_SIZE		(0x001c | REGS_NORTH_PIPE_AND_PORT)
#define INTEL_DISPLAY_B_IMAGE_SIZE		(0x101c | REGS_NORTH_PIPE_AND_PORT)

#define INTEL_DISPLAY_B_DIGITAL_PORT	(0x1140 | REGS_SOUTH_TRANSCODER_PORT)

// planes
#define INTEL_DISPLAY_A_PIPE_CONTROL	(0x0008 | REGS_NORTH_PLANE_CONTROL)
#define INTEL_DISPLAY_B_PIPE_CONTROL	(0x1008 | REGS_NORTH_PLANE_CONTROL)
#define DISPLAY_PIPE_ENABLED			(1UL << 31)

#define INTEL_DISPLAY_A_PIPE_STATUS		(0x0024 | REGS_NORTH_PLANE_CONTROL)
#define INTEL_DISPLAY_B_PIPE_STATUS		(0x1024 | REGS_NORTH_PLANE_CONTROL)
#define DISPLAY_PIPE_VBLANK_ENABLED		(1UL << 17)
#define DISPLAY_PIPE_VBLANK_STATUS		(1UL << 1)

#define INTEL_DISPLAY_A_CONTROL			(0x0180 | REGS_NORTH_PLANE_CONTROL)
#define INTEL_DISPLAY_A_BASE			(0x0184 | REGS_NORTH_PLANE_CONTROL)
#define INTEL_DISPLAY_A_BYTES_PER_ROW	(0x0188 | REGS_NORTH_PLANE_CONTROL)
#define INTEL_DISPLAY_A_POS				(0x018c | REGS_NORTH_PLANE_CONTROL)
	// reserved on A
#define INTEL_DISPLAY_A_PIPE_SIZE		(0x0190 | REGS_NORTH_PLANE_CONTROL)
#define INTEL_DISPLAY_A_SURFACE			(0x019c | REGS_NORTH_PLANE_CONTROL)
	// i965 and up only

#define INTEL_DISPLAY_B_CONTROL			(0x1180 | REGS_NORTH_PLANE_CONTROL)
#define INTEL_DISPLAY_B_BASE			(0x1184 | REGS_NORTH_PLANE_CONTROL)
#define INTEL_DISPLAY_B_BYTES_PER_ROW	(0x1188 | REGS_NORTH_PLANE_CONTROL)
#define INTEL_DISPLAY_B_POS				(0x118c | REGS_NORTH_PLANE_CONTROL)
#define INTEL_DISPLAY_B_PIPE_SIZE		(0x1190 | REGS_NORTH_PLANE_CONTROL)
#define INTEL_DISPLAY_B_SURFACE			(0x119c | REGS_NORTH_PLANE_CONTROL)
	// i965 and up only

#define DISPLAY_CONTROL_ENABLED			(1UL << 31)
#define DISPLAY_CONTROL_GAMMA			(1UL << 30)
#define DISPLAY_CONTROL_COLOR_MASK		(0x0fUL << 26)
#define DISPLAY_CONTROL_CMAP8			(2UL << 26)
#define DISPLAY_CONTROL_RGB15			(4UL << 26)
#define DISPLAY_CONTROL_RGB16			(5UL << 26)
#define DISPLAY_CONTROL_RGB32			(6UL << 26)

// cursors
#define INTEL_CURSOR_CONTROL			(0x0080 | REGS_NORTH_PLANE_CONTROL)
#define INTEL_CURSOR_BASE				(0x0084 | REGS_NORTH_PLANE_CONTROL)
#define INTEL_CURSOR_POSITION			(0x0088 | REGS_NORTH_PLANE_CONTROL)
#define INTEL_CURSOR_PALETTE			(0x0090 | REGS_NORTH_PLANE_CONTROL)
	// (- 0x009f)
#define INTEL_CURSOR_SIZE				(0x00a0 | REGS_NORTH_PLANE_CONTROL)
#define CURSOR_ENABLED					(1UL << 31)
#define CURSOR_FORMAT_2_COLORS			(0UL << 24)
#define CURSOR_FORMAT_3_COLORS			(1UL << 24)
#define CURSOR_FORMAT_4_COLORS			(2UL << 24)
#define CURSOR_FORMAT_ARGB				(4UL << 24)
#define CURSOR_FORMAT_XRGB				(5UL << 24)
#define CURSOR_POSITION_NEGATIVE		0x8000
#define CURSOR_POSITION_MASK			0x3fff

// palette registers
#define INTEL_DISPLAY_A_PALETTE			(0xa000 | REGS_NORTH_SHARED)
#define INTEL_DISPLAY_B_PALETTE			(0xa800 | REGS_NORTH_SHARED)

// PLL registers
#define INTEL_DISPLAY_A_PLL				(0x6014 | REGS_SOUTH_SHARED)
#define INTEL_DISPLAY_B_PLL				(0x6018 | REGS_SOUTH_SHARED)
#define INTEL_DISPLAY_A_PLL_MULTIPLIER_DIVISOR \
										(0x601c | REGS_SOUTH_SHARED)
#define INTEL_DISPLAY_B_PLL_MULTIPLIER_DIVISOR \
										(0x6020 | REGS_SOUTH_SHARED)
#define INTEL_DISPLAY_A_PLL_DIVISOR_0	(0x6040 | REGS_SOUTH_SHARED)
#define INTEL_DISPLAY_A_PLL_DIVISOR_1	(0x6044 | REGS_SOUTH_SHARED)
#define INTEL_DISPLAY_B_PLL_DIVISOR_0	(0x6048 | REGS_SOUTH_SHARED)
#define INTEL_DISPLAY_B_PLL_DIVISOR_1	(0x604c | REGS_SOUTH_SHARED)

// i2c
#define INTEL_I2C_IO_A					(0x5010 | REGS_SOUTH_SHARED)
#define INTEL_I2C_IO_B					(0x5014 | REGS_SOUTH_SHARED)
#define INTEL_I2C_IO_C					(0x5018 | REGS_SOUTH_SHARED)
#define INTEL_I2C_IO_D					(0x501c | REGS_SOUTH_SHARED)
#define INTEL_I2C_IO_E					(0x5020 | REGS_SOUTH_SHARED)
#define INTEL_I2C_IO_F					(0x5024 | REGS_SOUTH_SHARED)
#define INTEL_I2C_IO_G					(0x5028 | REGS_SOUTH_SHARED)
#define INTEL_I2C_IO_H					(0x502c | REGS_SOUTH_SHARED)

#define I2C_CLOCK_DIRECTION_MASK		(1 << 0)
#define I2C_CLOCK_DIRECTION_OUT			(1 << 1)
#define I2C_CLOCK_VALUE_MASK			(1 << 2)
#define I2C_CLOCK_VALUE_OUT				(1 << 3)
#define I2C_CLOCK_VALUE_IN				(1 << 4)
#define I2C_DATA_DIRECTION_MASK			(1 << 8)
#define I2C_DATA_DIRECTION_OUT			(1 << 9)
#define I2C_DATA_VALUE_MASK				(1 << 10)
#define I2C_DATA_VALUE_OUT				(1 << 11)
#define I2C_DATA_VALUE_IN				(1 << 12)
#define I2C_RESERVED					((1 << 13) | (1 << 5))

// TODO: on IronLake this is in the north shared block at 0x41000
#define INTEL_VGA_DISPLAY_CONTROL		0x71400
#define VGA_DISPLAY_DISABLED			(1UL << 31)

// LVDS panel
#define INTEL_PANEL_STATUS				0x61200
#define PANEL_STATUS_POWER_ON			(1UL << 31)
#define INTEL_PANEL_CONTROL				0x61204
#define PANEL_CONTROL_POWER_TARGET_ON	(1UL << 0)
#define INTEL_PANEL_FIT_CONTROL			0x61230
#define INTEL_PANEL_FIT_RATIOS			0x61234

// LVDS on IronLake and up
#define PCH_PANEL_CONTROL				0xc7200
#define PCH_PANEL_STATUS				0xc7204
#define PANEL_REGISTER_UNLOCK			(0xabcd << 16)
#define PCH_LVDS_DETECTED				(1 << 1)


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
#define XY_COMMAND_SOURCE_BLIT			0x54c00006
#define XY_COMMAND_COLOR_BLIT			0x54000004
#define XY_COMMAND_SETUP_MONO_PATTERN	0x44400007
#define XY_COMMAND_SCANLINE_BLIT		0x49400001
#define COMMAND_COLOR_BLIT				0x50000003
#define COMMAND_BLIT_RGBA				0x00300000

#define COMMAND_MODE_SOLID_PATTERN		0x80
#define COMMAND_MODE_CMAP8				0x00
#define COMMAND_MODE_RGB15				0x02
#define COMMAND_MODE_RGB16				0x01
#define COMMAND_MODE_RGB32				0x03

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

// i965 overlay support is currently realized using its 3D hardware
#define INTEL_i965_OVERLAY_STATE_SIZE	36864
#define INTEL_i965_3D_CONTEXT_SIZE		32768

inline bool
intel_uses_physical_overlay(intel_shared_info &info)
{
	return !info.device_type.InGroup(INTEL_TYPE_Gxx);
}


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

#endif	/* INTEL_EXTREME_H */
