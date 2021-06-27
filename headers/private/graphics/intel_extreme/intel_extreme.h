/*
 * Copyright 2006-2016, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Alexander von Gluck, kallisti5@unixzen.com
 */
#ifndef INTEL_EXTREME_H
#define INTEL_EXTREME_H


#include "lock.h"

#include <Accelerant.h>
#include <Drivers.h>
#include <PCI.h>

#include <edid.h>


#define VENDOR_ID_INTEL			0x8086

#define INTEL_FAMILY_MASK	0x00ff0000
#define INTEL_GROUP_MASK	0x00fffff0
#define INTEL_MODEL_MASK	0x00ffffff
#define INTEL_TYPE_MASK		0x0000000f

// families
#define INTEL_FAMILY_8xx	0x00020000	// Second Gen
#define INTEL_FAMILY_9xx	0x00040000	// Third Gen +
#define INTEL_FAMILY_SER5	0x00080000	// Intel5 Series
#define INTEL_FAMILY_SOC0	0x00200000  // Atom SOC
#define INTEL_FAMILY_LAKE	0x00400000	// Intel Lakes

// groups
#define INTEL_GROUP_83x		(INTEL_FAMILY_8xx  | 0x0010)
#define INTEL_GROUP_85x		(INTEL_FAMILY_8xx  | 0x0020)
#define INTEL_GROUP_91x		(INTEL_FAMILY_9xx  | 0x0010)
#define INTEL_GROUP_94x		(INTEL_FAMILY_9xx  | 0x0020)
#define INTEL_GROUP_96x		(INTEL_FAMILY_9xx  | 0x0040)
#define INTEL_GROUP_Gxx		(INTEL_FAMILY_9xx  | 0x0080)
#define INTEL_GROUP_G4x		(INTEL_FAMILY_9xx  | 0x0100)
#define INTEL_GROUP_PIN		(INTEL_FAMILY_9xx  | 0x0200)  // PineView
#define INTEL_GROUP_ILK		(INTEL_FAMILY_SER5 | 0x0010)  // IronLake
#define INTEL_GROUP_SNB		(INTEL_FAMILY_SER5 | 0x0020)  // SandyBridge
#define INTEL_GROUP_IVB		(INTEL_FAMILY_SER5 | 0x0040)  // IvyBridge
#define INTEL_GROUP_HAS		(INTEL_FAMILY_SER5 | 0x0080)  // Haswell
#define INTEL_GROUP_VLV		(INTEL_FAMILY_SOC0 | 0x0010)  // ValleyView
#define INTEL_GROUP_CHV		(INTEL_FAMILY_SOC0 | 0x0020)  // CherryView
#define INTEL_GROUP_BDW		(INTEL_FAMILY_SOC0 | 0x0040)  // Broadwell
#define INTEL_GROUP_SKY		(INTEL_FAMILY_LAKE | 0x0010)  // SkyLake
#define INTEL_GROUP_KBY		(INTEL_FAMILY_LAKE | 0x0020)  // KabyLake
// models
#define INTEL_TYPE_SERVER	0x0004
#define INTEL_TYPE_MOBILE	0x0008
#define INTEL_MODEL_915		(INTEL_GROUP_91x)
#define INTEL_MODEL_915M	(INTEL_GROUP_91x | INTEL_TYPE_MOBILE)
#define INTEL_MODEL_945		(INTEL_GROUP_94x)
#define INTEL_MODEL_945M	(INTEL_GROUP_94x | INTEL_TYPE_MOBILE)
#define INTEL_MODEL_965		(INTEL_GROUP_96x)
#define INTEL_MODEL_965M	(INTEL_GROUP_96x | INTEL_TYPE_MOBILE)
#define INTEL_MODEL_G33		(INTEL_GROUP_Gxx)
#define INTEL_MODEL_G45		(INTEL_GROUP_G4x)
#define INTEL_MODEL_GM45	(INTEL_GROUP_G4x | INTEL_TYPE_MOBILE)
#define INTEL_MODEL_PINE	(INTEL_GROUP_PIN)
#define INTEL_MODEL_PINEM	(INTEL_GROUP_PIN | INTEL_TYPE_MOBILE)
#define INTEL_MODEL_ILKG	(INTEL_GROUP_ILK)
#define INTEL_MODEL_ILKGM	(INTEL_GROUP_ILK | INTEL_TYPE_MOBILE)
#define INTEL_MODEL_SNBG	(INTEL_GROUP_SNB)
#define INTEL_MODEL_SNBGM	(INTEL_GROUP_SNB | INTEL_TYPE_MOBILE)
#define INTEL_MODEL_SNBGS	(INTEL_GROUP_SNB | INTEL_TYPE_SERVER)
#define INTEL_MODEL_IVBG	(INTEL_GROUP_IVB)
#define INTEL_MODEL_IVBGM	(INTEL_GROUP_IVB | INTEL_TYPE_MOBILE)
#define INTEL_MODEL_IVBGS	(INTEL_GROUP_IVB | INTEL_TYPE_SERVER)
#define INTEL_MODEL_HAS		(INTEL_GROUP_HAS)
#define INTEL_MODEL_HASM	(INTEL_GROUP_HAS | INTEL_TYPE_MOBILE)
#define INTEL_MODEL_VLV		(INTEL_GROUP_VLV)
#define INTEL_MODEL_VLVM	(INTEL_GROUP_VLV | INTEL_TYPE_MOBILE)
#define INTEL_MODEL_BDW		(INTEL_GROUP_BDW)
#define INTEL_MODEL_BDWM	(INTEL_GROUP_BDW | INTEL_TYPE_MOBILE)
#define INTEL_MODEL_SKY		(INTEL_GROUP_SKY)
#define INTEL_MODEL_SKYM	(INTEL_GROUP_SKY | INTEL_TYPE_MOBILE)
#define INTEL_MODEL_SKYS	(INTEL_GROUP_SKY | INTEL_TYPE_SERVER)

#define INTEL_PCH_DEVICE_ID_MASK	0xff80
#define INTEL_PCH_IBX_DEVICE_ID		0x3b00
#define INTEL_PCH_CPT_DEVICE_ID		0x1c00
#define INTEL_PCH_PPT_DEVICE_ID		0x1e00
#define INTEL_PCH_LPT_DEVICE_ID		0x8c00
#define INTEL_PCH_LPT_LP_DEVICE_ID	0x9c00
#define INTEL_PCH_WPT_DEVICE_ID		0x8c80
#define INTEL_PCH_WPT_LP_DEVICE_ID	0x9c80
#define INTEL_PCH_SPT_DEVICE_ID		0xa100
#define INTEL_PCH_SPT_LP_DEVICE_ID	0x9d00
#define INTEL_PCH_KBP_DEVICE_ID		0xa280
#define INTEL_PCH_CNP_DEVICE_ID		0xa300
#define INTEL_PCH_CNP_LP_DEVICE_ID	0x9d80
#define INTEL_PCH_ICP_DEVICE_ID		0x3480
#define INTEL_PCH_P2X_DEVICE_ID		0x7100
#define INTEL_PCH_P3X_DEVICE_ID		0x7000

// ValleyView MMIO offset
#define VLV_DISPLAY_BASE		0x180000

#define DEVICE_NAME				"intel_extreme"
#define INTEL_ACCELERANT_NAME	"intel_extreme.accelerant"

// We encode the register block into the value and extract/translate it when
// actually accessing.
#define REGISTER_BLOCK_COUNT				6
#define REGISTER_BLOCK_SHIFT				24
#define REGISTER_BLOCK_MASK					0xff000000
#define REGISTER_REGISTER_MASK				0x00ffffff
#define REGISTER_BLOCK(x) ((x & REGISTER_BLOCK_MASK) >> REGISTER_BLOCK_SHIFT)
#define REGISTER_REGISTER(x) (x & REGISTER_REGISTER_MASK)

#define REGS_FLAT							(0 << REGISTER_BLOCK_SHIFT)
#define REGS_NORTH_SHARED					(1 << REGISTER_BLOCK_SHIFT)
#define REGS_NORTH_PIPE_AND_PORT			(2 << REGISTER_BLOCK_SHIFT)
#define REGS_NORTH_PLANE_CONTROL			(3 << REGISTER_BLOCK_SHIFT)
#define REGS_SOUTH_SHARED					(4 << REGISTER_BLOCK_SHIFT)
#define REGS_SOUTH_TRANSCODER_PORT			(5 << REGISTER_BLOCK_SHIFT)

// register blocks for (G)MCH/ICH based platforms
#define MCH_SHARED_REGISTER_BASE						0x00000
#define MCH_PIPE_AND_PORT_REGISTER_BASE					0x60000
#define MCH_PLANE_CONTROL_REGISTER_BASE					0x70000

#define ICH_SHARED_REGISTER_BASE						0x00000
#define ICH_PORT_REGISTER_BASE							0x60000

// PCH - Platform Control Hub - Some hardware moves from a MCH/ICH based
// setup to a PCH based one, that means anything that used to communicate via
// (G)MCH registers needs to use different ones on PCH based platforms
// (Ironlake, SandyBridge, IvyBridge, Some Haswell).
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
		return (type & INTEL_FAMILY_MASK) == family;
	}

	bool InGroup(uint32 group) const
	{
		return (type & INTEL_GROUP_MASK) == group;
	}

	bool IsModel(uint32 model) const
	{
		return (type & INTEL_MODEL_MASK) == model;
	}

	bool IsMobile() const
	{
		return (type & INTEL_TYPE_MASK) == INTEL_TYPE_MOBILE;
	}

	bool SupportsHDMI() const
	{
		return InGroup(INTEL_GROUP_G4x) || InFamily(INTEL_FAMILY_SER5)
			|| InFamily(INTEL_FAMILY_SOC0);
	}

	bool HasDDI() const
	{
		// Intel Digital Display Interface
		return InGroup(INTEL_GROUP_HAS) || (Generation() >= 8);
	}

	int Generation() const
	{
		if (InFamily(INTEL_FAMILY_8xx))
			return 2;
		if (InGroup(INTEL_GROUP_91x) || InGroup(INTEL_GROUP_94x)
				|| IsModel(INTEL_MODEL_G33) || InGroup(INTEL_GROUP_PIN))
			return 3;
		if (InFamily(INTEL_FAMILY_9xx))
			return 4;
		if (InGroup(INTEL_GROUP_ILK))
			return 5;
		if (InGroup(INTEL_GROUP_SNB))
			return 6;
		if (InFamily(INTEL_FAMILY_SER5) || InGroup(INTEL_GROUP_VLV))
			return 7;
		if (InGroup(INTEL_GROUP_CHV) || InGroup(INTEL_GROUP_BDW))
			return 8;
		if (InFamily(INTEL_FAMILY_LAKE))
			return 9;

		// Generation 0 means something is wrong :-)
		return 0;
	}
};

enum pch_info {
	INTEL_PCH_NONE = 0,		// No PCH present
	INTEL_PCH_IBX,			// Ibexpeak
	INTEL_PCH_CPT,			// Cougarpoint
	INTEL_PCH_LPT,			// Lynxpoint
	INTEL_PCH_SPT,			// SunrisePoint
	INTEL_PCH_KBP,			// KabyLake
	INTEL_PCH_CNP,			// CannonLake
	INTEL_PCH_ICP,			// IceLake
	INTEL_PCH_NOP
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

	display_mode	panel_mode;			// VBIOS VBT panel mode
	uint32			bytes_per_row;
	uint32			bits_per_pixel;
	uint32			dpms_mode;

	area_id			registers_area;		// area of memory mapped registers
	uint32			register_blocks[REGISTER_BLOCK_COUNT];

	uint8*			status_page;
	phys_addr_t		physical_status_page;
	uint8*			graphics_memory;
	phys_addr_t		physical_graphics_memory;
	uint32			graphics_memory_size;

	addr_t			frame_buffer;
	uint32			frame_buffer_offset;

	uint32			fdi_link_frequency;	// In Mhz

	bool			got_vbt;
	bool			single_head_locked;

	struct lock		accelerant_lock;
	struct lock		engine_lock;

	ring_buffer		primary_ring_buffer;

	int32			overlay_channel_used;
	bool			overlay_active;
	uintptr_t		overlay_token;
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

	enum pch_info	pch_info;

	edid1_info		vesa_edid_info;
	bool			has_vesa_edid_info;
};

enum pipe_index {
    INTEL_PIPE_ANY,
    INTEL_PIPE_A,
    INTEL_PIPE_B,
    INTEL_PIPE_C
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
	addr_t	buffer_base;
};

// free graphics memory
struct intel_free_graphics_memory {
	uint32 	magic;
	addr_t	buffer_base;
};

//----------------------------------------------------------
// Register definitions, taken from X driver

// PCI bridge memory management
#define INTEL_GRAPHICS_MEMORY_CONTROL	0x52		// i830+

	// GGC - (G)MCH Graphics Control Register
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
#define INTEL_INTERRUPT_ENABLED			0x020a0
#define INTEL_INTERRUPT_IDENTITY		0x020a4
#define INTEL_INTERRUPT_MASK			0x020a8
#define INTEL_INTERRUPT_STATUS			0x020ac
#define INTERRUPT_VBLANK_PIPEA			(1 << 7)
#define INTERRUPT_VBLANK_PIPEB			(1 << 5)

// PCH interrupts
#define PCH_INTERRUPT_STATUS			0x44000
#define PCH_INTERRUPT_MASK				0x44004
#define PCH_INTERRUPT_IDENTITY			0x44008
#define PCH_INTERRUPT_ENABLED			0x4400c

#define PCH_INTERRUPT_VBLANK_PIPEA		(1 << 0)
#define PCH_INTERRUPT_VBLANK_PIPEB		(1 << 5)
#define PCH_INTERRUPT_VBLANK_PIPEC		(1 << 10)

// SandyBridge had only two pipes, and things were shuffled aroud again with
// the introduction of pipe C.
#define PCH_INTERRUPT_VBLANK_PIPEA_SNB		(1 << 7)
#define PCH_INTERRUPT_VBLANK_PIPEB_SNB		(1 << 15)
#define PCH_INTERRUPT_GLOBAL_SNB			(1 << 31)

// graphics port control (i.e. G45)
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
#define DISPLAY_MONITOR_PORT_DETECTED	(1UL << 2) // TMDS/DisplayPort only

// Cougar Point transcoder pipe selection
// (replaces DISPLAY_MONITOR_PIPE_B)
#define  PORT_TRANS_A_SEL_CPT			0
#define  PORT_TRANS_B_SEL_CPT			(1<<29)
#define  PORT_TRANS_C_SEL_CPT			(2<<29)
#define  PORT_TRANS_SEL_MASK			(3<<29)

#define LVDS_POST2_RATE_SLOW			14 // PLL Divisors
#define LVDS_POST2_RATE_FAST			7
#define LVDS_B0B3_POWER_MASK			(3UL << 2)
#define LVDS_B0B3_POWER_UP				(3UL << 2)
#define LVDS_CLKB_POWER_MASK			(3UL << 4)
#define LVDS_CLKB_POWER_UP				(3UL << 4)
#define LVDS_A3_POWER_MASK				(3UL << 6)
#define LVDS_A3_POWER_UP				(3UL << 6)
#define LVDS_A0A2_CLKA_POWER_UP			(3UL << 8)
#define LVDS_BORDER_ENABLE				(1UL << 15)
#define LVDS_HSYNC_POLARITY				(1UL << 20)
#define LVDS_VSYNC_POLARITY				(1UL << 21)
#define LVDS_18BIT_DITHER				(1UL << 25)
#define LVDS_PORT_EN					(1UL << 31)

// PLL flags
#define DISPLAY_PLL_ENABLED				(1UL << 31)
#define DISPLAY_PLL_2X_CLOCK			(1UL << 30)
#define DISPLAY_PLL_SYNC_LOCK_ENABLED	(1UL << 29)
#define DISPLAY_PLL_NO_VGA_CONTROL		(1UL << 28)
#define DISPLAY_PLL_MODE_NORMAL			(1UL << 26)
#define DISPLAY_PLL_MODE_LVDS			(2UL << 26)
#define DISPLAY_PLL_DIVIDE_HIGH			(1UL << 24)
#define DISPLAY_PLL_DIVIDE_4X			(1UL << 23)
#define DISPLAY_PLL_POST1_DIVIDE_2		(1UL << 21)
#define DISPLAY_PLL_POST1_DIVISOR_MASK	0x001f0000
#define DISPLAY_PLL_9xx_POST1_DIVISOR_MASK	0x00ff0000
#define DISPLAY_PLL_SNB_FP0_POST1_DIVISOR_MASK	0x000000ff
#define DISPLAY_PLL_IGD_POST1_DIVISOR_MASK  0x00ff8000
#define DISPLAY_PLL_POST1_DIVISOR_SHIFT	16
#define DISPLAY_PLL_SNB_FP0_POST1_DIVISOR_SHIFT	0
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

#define INTEL_DISPLAY_OFFSET			0x1000

#define INTEL_DISPLAY_A_HTOTAL			(0x0000 | REGS_NORTH_PIPE_AND_PORT)
#define INTEL_DISPLAY_A_HBLANK			(0x0004 | REGS_NORTH_PIPE_AND_PORT)
#define INTEL_DISPLAY_A_HSYNC			(0x0008 | REGS_NORTH_PIPE_AND_PORT)
#define INTEL_DISPLAY_A_VTOTAL			(0x000c | REGS_NORTH_PIPE_AND_PORT)
#define INTEL_DISPLAY_A_VBLANK			(0x0010 | REGS_NORTH_PIPE_AND_PORT)
#define INTEL_DISPLAY_A_VSYNC			(0x0014 | REGS_NORTH_PIPE_AND_PORT)
#define INTEL_DISPLAY_B_HTOTAL			(0x1000 | REGS_NORTH_PIPE_AND_PORT)
#define INTEL_DISPLAY_B_HBLANK			(0x1004 | REGS_NORTH_PIPE_AND_PORT)
#define INTEL_DISPLAY_B_HSYNC			(0x1008 | REGS_NORTH_PIPE_AND_PORT)
#define INTEL_DISPLAY_B_VTOTAL			(0x100c | REGS_NORTH_PIPE_AND_PORT)
#define INTEL_DISPLAY_B_VBLANK			(0x1010 | REGS_NORTH_PIPE_AND_PORT)
#define INTEL_DISPLAY_B_VSYNC			(0x1014 | REGS_NORTH_PIPE_AND_PORT)

#define INTEL_DISPLAY_A_PIPE_SIZE		(0x001c | REGS_NORTH_PIPE_AND_PORT)
#define INTEL_DISPLAY_B_PIPE_SIZE		(0x101c | REGS_NORTH_PIPE_AND_PORT)

// on PCH we also have to set the transcoder
#define INTEL_TRANSCODER_A_HTOTAL		(0x0000 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_TRANSCODER_A_HBLANK		(0x0004 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_TRANSCODER_A_HSYNC		(0x0008 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_TRANSCODER_A_VTOTAL		(0x000c | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_TRANSCODER_A_VBLANK		(0x0010 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_TRANSCODER_A_VSYNC		(0x0014 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_TRANSCODER_B_HTOTAL		(0x1000 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_TRANSCODER_B_HBLANK		(0x1004 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_TRANSCODER_B_HSYNC		(0x1008 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_TRANSCODER_B_VTOTAL		(0x100c | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_TRANSCODER_B_VBLANK		(0x1010 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_TRANSCODER_B_VSYNC		(0x1014 | REGS_SOUTH_TRANSCODER_PORT)

// transcoder M/N DATA AND LINK VALUES (refreshrate)
#define INTEL_TRANSCODER_A_DATA_M1			(0x0030 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_TRANSCODER_A_DATA_M2			(0x0038 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_TRANSCODER_B_DATA_M1			(0x1030 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_TRANSCODER_B_DATA_M2			(0x1038 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_TRANSCODER_C_DATA_M1			(0x2030 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_TRANSCODER_C_DATA_M2			(0x2038 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_TRANSCODER_A_DATA_N1			(0x0034 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_TRANSCODER_A_DATA_N2			(0x003c | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_TRANSCODER_B_DATA_N1			(0x1034 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_TRANSCODER_B_DATA_N2			(0x103c | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_TRANSCODER_C_DATA_N1			(0x2034 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_TRANSCODER_C_DATA_N2			(0x203c | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_TRANSCODER_A_LINK_M1			(0x0040 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_TRANSCODER_A_LINK_M2			(0x0048 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_TRANSCODER_B_LINK_M1			(0x1040 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_TRANSCODER_B_LINK_M2			(0x1048 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_TRANSCODER_C_LINK_M1			(0x2040 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_TRANSCODER_C_LINK_M2			(0x2048 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_TRANSCODER_A_LINK_N1			(0x0044 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_TRANSCODER_A_LINK_N2			(0x004c | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_TRANSCODER_B_LINK_N1			(0x1044 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_TRANSCODER_B_LINK_N2			(0x104c | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_TRANSCODER_C_LINK_N1			(0x2044 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_TRANSCODER_C_LINK_N2			(0x204c | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_TRANSCODER_MN_TU_SIZE_MASK	(0x3f << 25)
#define INTEL_TRANSCODER_MN_VALUE_MASK		(0xffffff << 0)

#define INTEL_TRANSCODER_A_IMAGE_SIZE	(0x001c | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_TRANSCODER_B_IMAGE_SIZE	(0x101c | REGS_SOUTH_TRANSCODER_PORT)

// TODO: Is there consolidation that could happen here with digital ports?

#define INTEL_ANALOG_PORT				(0x1100 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_DIGITAL_PORT_A			(0x1120 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_DIGITAL_PORT_B			(0x1140 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_DIGITAL_PORT_C			(0x1160 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_DIGITAL_LVDS_PORT			(0x1180 | REGS_SOUTH_TRANSCODER_PORT)

#define INTEL_HDMI_PORT_B				(0x1140 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_HDMI_PORT_C				(0x1160 | REGS_SOUTH_TRANSCODER_PORT)

#define PCH_HDMI_PORT_B					(0x1140 | REGS_SOUTH_TRANSCODER_PORT)
#define PCH_HDMI_PORT_C					(0x1150 | REGS_SOUTH_TRANSCODER_PORT)
#define PCH_HDMI_PORT_D					(0x1160 | REGS_SOUTH_TRANSCODER_PORT)

#define GEN4_HDMI_PORT_B				(0x1140 | REGS_SOUTH_TRANSCODER_PORT)
#define GEN4_HDMI_PORT_C				(0x1160 | REGS_SOUTH_TRANSCODER_PORT)
#define CHV_HDMI_PORT_D					(0x116C | REGS_SOUTH_TRANSCODER_PORT)

// DDI Buffer Control (This replaces DP on Haswell+)
#define DDI_BUF_CTL_A					(0x4000 | REGS_NORTH_PIPE_AND_PORT)
#define DDI_BUF_CTL_B					(0x4100 | REGS_NORTH_PIPE_AND_PORT)
#define DDI_BUF_CTL_C					(0x4200 | REGS_NORTH_PIPE_AND_PORT)
#define DDI_BUF_CTL_D					(0x4300 | REGS_NORTH_PIPE_AND_PORT)
#define DDI_BUF_CTL_E					(0x4400 | REGS_NORTH_PIPE_AND_PORT)
#define 	DDI_BUF_CTL_ENABLE			(1 << 31)
#define 	DDI_BUF_TRANS_SELECT(n)		((n) << 24)
#define 	DDI_BUF_EMP_MASK			(0xf << 24)
#define 	DDI_BUF_PORT_REVERSAL		(1 << 16)
#define 	DDI_BUF_IS_IDLE				(1 << 7)
#define 	DDI_A_4_LANES				(1 << 4)
#define 	DDI_PORT_WIDTH(width)		(((width) - 1) << 1)
#define 	DDI_INIT_DISPLAY_DETECTED	(1 << 0)

#define PIPE_DDI_FUNC_CTL_A				(0x0400 | REGS_NORTH_PIPE_AND_PORT)
#define PIPE_DDI_FUNC_CTL_B				(0x1400 | REGS_NORTH_PIPE_AND_PORT)
#define PIPE_DDI_FUNC_CTL_C				(0x2400 | REGS_NORTH_PIPE_AND_PORT)
#define PIPE_DDI_FUNC_CTL_EDP			(0xF400 | REGS_NORTH_PIPE_AND_PORT)
#define PIPE_DDI_FUNC_CTL_ENABLE		(1 << 31)
#define PIPE_DDI_SELECT_SHIFT			28
#define PIPE_DDI_SELECT_PORT(x)			((x) << PIPE_DDI_SELECT_SHIFT)
#define PIPE_DDI_SELECT_MASK			(7 << PIPE_DDI_SELECT_SHIFT)
#define PIPE_DDI_PORT_NONE				0
#define PIPE_DDI_PORT_B					1
#define PIPE_DDI_PORT_C					2
#define PIPE_DDI_PORT_D					3
#define PIPE_DDI_PORT_E					4

// DP_A always @ 6xxxx, DP_B-DP_D move with PCH
#define INTEL_DISPLAY_PORT_A			(0x4000 | REGS_NORTH_PIPE_AND_PORT)
#define INTEL_DISPLAY_PORT_B			(0x4100 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_DISPLAY_PORT_C			(0x4200 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_DISPLAY_PORT_D			(0x4300 | REGS_SOUTH_TRANSCODER_PORT)

#define INTEL_TRANSCODER_A_DP_CTL		(0x0300 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_TRANSCODER_B_DP_CTL		(0x1300 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_TRANSCODER_C_DP_CTL		(0x2300 | REGS_SOUTH_TRANSCODER_PORT)

#define INTEL_TRANS_DP_COLOR_SHIFT		9
#define INTEL_TRANS_DP_BPC(x)			((x) << INTEL_PIPE_COLOR_SHIFT)
#define INTEL_TRANS_DP_BPC_MASK			(7 << INTEL_PIPE_COLOR_SHIFT)

// Unless you're a damn Valley/CherryView unicorn :-(
#define VLV_DISPLAY_PORT_B				(VLV_DISPLAY_BASE + 0x64100)
#define VLV_DISPLAY_PORT_C				(VLV_DISPLAY_BASE + 0x64200)
#define CHV_DISPLAY_PORT_D				(VLV_DISPLAY_BASE + 0x64300)

// DP AUX channels
#define INTEL_DP_AUX_CTL_A				(0x4010 | REGS_NORTH_PIPE_AND_PORT)
#define INTEL_DP_AUX_CTL_B				(0x4110 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_DP_AUX_CTL_C				(0x4210 | REGS_SOUTH_TRANSCODER_PORT)
#define INTEL_DP_AUX_CTL_D				(0x4310 | REGS_SOUTH_TRANSCODER_PORT)

#define VLV_DP_AUX_CTL_B				(VLV_DISPLAY_BASE + 0x64110)
#define VLV_DP_AUX_CTL_C				(VLV_DISPLAY_BASE + 0x64210)
#define CHV_DP_AUX_CTL_D				(VLV_DISPLAY_BASE + 0x64310)

#define INTEL_DP_AUX_CTL_BUSY			(1 << 31)
#define INTEL_DP_AUX_CTL_DONE			(1 << 30)
#define INTEL_DP_AUX_CTL_INTERRUPT		(1 << 29)
#define INTEL_DP_AUX_CTL_TIMEOUT_ERROR	(1 << 28)
#define INTEL_DP_AUX_CTL_TIMEOUT_400us	(0 << 26)
#define INTEL_DP_AUX_CTL_TIMEOUT_600us	(1 << 26)
#define INTEL_DP_AUX_CTL_TIMEOUT_800us	(2 << 26)
#define INTEL_DP_AUX_CTL_TIMEOUT_1600us (3 << 26)
#define INTEL_DP_AUX_CTL_TIMEOUT_MASK	(3 << 26)
#define INTEL_DP_AUX_CTL_RECEIVE_ERROR	(1 << 25)
#define INTEL_DP_AUX_CTL_MSG_SIZE_MASK	(0x1f << 20)
#define INTEL_DP_AUX_CTL_MSG_SIZE_SHIFT 20
#define INTEL_DP_AUX_CTL_PRECHARGE_2US_MASK (0xf << 16)
#define INTEL_DP_AUX_CTL_PRECHARGE_2US_SHIFT 16
#define INTEL_DP_AUX_CTL_BIT_CLOCK_2X_MASK (0x7ff)
#define INTEL_DP_AUX_CTL_BIT_CLOCK_2X_SHIFT 0
#define INTEL_DP_AUX_CTL_SYNC_PULSE_SKL(c)   ((c) - 1)

// planes
#define INTEL_PIPE_ENABLED				(1UL << 31)
#define INTEL_PIPE_STATE				(1UL << 30)

#define INTEL_PLANE_OFFSET				0x1000

#define INTEL_DISPLAY_A_PIPE_CONTROL	(0x0008	| REGS_NORTH_PLANE_CONTROL)
#define INTEL_DISPLAY_B_PIPE_CONTROL	(0x1008 | REGS_NORTH_PLANE_CONTROL)
#define INTEL_DISPLAY_C_PIPE_CONTROL	(0x2008 | REGS_NORTH_PLANE_CONTROL)
#define INTEL_DISPLAY_A_PIPE_STATUS		(0x0024 | REGS_NORTH_PLANE_CONTROL)
#define INTEL_DISPLAY_B_PIPE_STATUS		(0x1024 | REGS_NORTH_PLANE_CONTROL)

#define INTEL_DISPLAY_A_PIPE_WATERMARK	(0x5100 | REGS_NORTH_PLANE_CONTROL)
#define INTEL_DISPLAY_B_PIPE_WATERMARK	(0x5104 | REGS_NORTH_PLANE_CONTROL)
#define INTEL_DISPLAY_C_PIPE_WATERMARK	(0x5200 | REGS_NORTH_PLANE_CONTROL)

#define DISPLAY_PIPE_VBLANK_ENABLED		(1UL << 17)
#define DISPLAY_PIPE_VBLANK_STATUS		(1UL << 1)

#define INTEL_DISPLAY_A_CONTROL			(0x0180 | REGS_NORTH_PLANE_CONTROL)
#define INTEL_DISPLAY_A_BASE			(0x0184 | REGS_NORTH_PLANE_CONTROL)
#define INTEL_DISPLAY_A_BYTES_PER_ROW	(0x0188 | REGS_NORTH_PLANE_CONTROL)
#define INTEL_DISPLAY_A_POS				(0x018c | REGS_NORTH_PLANE_CONTROL)
	// reserved on A
#define INTEL_DISPLAY_A_IMAGE_SIZE		(0x0190 | REGS_NORTH_PLANE_CONTROL)
#define INTEL_DISPLAY_A_SURFACE			(0x019c | REGS_NORTH_PLANE_CONTROL)
	// i965 and up only

#define INTEL_DISPLAY_B_CONTROL			(0x1180 | REGS_NORTH_PLANE_CONTROL)
#define INTEL_DISPLAY_B_BASE			(0x1184 | REGS_NORTH_PLANE_CONTROL)
#define INTEL_DISPLAY_B_BYTES_PER_ROW	(0x1188 | REGS_NORTH_PLANE_CONTROL)
#define INTEL_DISPLAY_B_POS				(0x118c | REGS_NORTH_PLANE_CONTROL)
#define INTEL_DISPLAY_B_IMAGE_SIZE		(0x1190 | REGS_NORTH_PLANE_CONTROL)
#define INTEL_DISPLAY_B_SURFACE			(0x119c | REGS_NORTH_PLANE_CONTROL)
	// i965 and up only

// INTEL_DISPLAY_A_CONTROL source pixel format
#define DISPLAY_CONTROL_ENABLED			(1UL << 31)
#define DISPLAY_CONTROL_GAMMA			(1UL << 30)
#define DISPLAY_CONTROL_COLOR_MASK		(0x0fUL << 26)
#define DISPLAY_CONTROL_CMAP8			(2UL << 26)
#define DISPLAY_CONTROL_RGB15			(4UL << 26)
#define DISPLAY_CONTROL_RGB16			(5UL << 26)
#define DISPLAY_CONTROL_RGB32			(6UL << 26)

// INTEL_DISPLAY_A_PIPE_CONTROL ILK+
#define INTEL_PIPE_DITHER_TYPE_MASK		(0x0000000c)
#define INTEL_PIPE_DITHER_TYPE_SP		(0 << 2)
#define INTEL_PIPE_DITHER_TYPE_ST1		(1 << 2)
#define INTEL_PIPE_DITHER_TYPE_ST2		(2 << 2)
#define INTEL_PIPE_DITHER_TYPE_TEMP		(3 << 2)
#define INTEL_PIPE_DITHER_EN			(1 << 4)
#define INTEL_PIPE_COLOR_SHIFT			5
#define INTEL_PIPE_BPC(x)				((x) << INTEL_PIPE_COLOR_SHIFT)
#define INTEL_PIPE_BPC_MASK				(7 << INTEL_PIPE_COLOR_SHIFT)
#define INTEL_PIPE_8BPC					0
#define INTEL_PIPE_10BPC				1
#define INTEL_PIPE_6BPC					2
#define INTEL_PIPE_12BPC				3
#define INTEL_PIPE_PROGRESSIVE			(0 << 21)

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

// Ironlake PCH reference clk control
#define PCH_DREF_CONTROL					(0x6200 | REGS_SOUTH_SHARED)
#define DREF_CONTROL_MASK					0x7fc3
#define DREF_CPU_SOURCE_OUTPUT_DISABLE		(0 << 13)
#define DREF_CPU_SOURCE_OUTPUT_DOWNSPREAD	(2 << 13)
#define DREF_CPU_SOURCE_OUTPUT_NONSPREAD	(3 << 13)
#define DREF_CPU_SOURCE_OUTPUT_MASK			(3 << 13)
#define DREF_SSC_SOURCE_DISABLE				(0 << 11)
#define DREF_SSC_SOURCE_ENABLE				(2 << 11)
#define DREF_SSC_SOURCE_MASK				(3 << 11)
#define DREF_NONSPREAD_SOURCE_DISABLE		(0 << 9)
#define DREF_NONSPREAD_CK505_ENABLE			(1 << 9)
#define DREF_NONSPREAD_SOURCE_ENABLE		(2 << 9)
#define DREF_NONSPREAD_SOURCE_MASK			(3 << 9)
#define DREF_SUPERSPREAD_SOURCE_DISABLE 	(0 << 7)
#define DREF_SUPERSPREAD_SOURCE_ENABLE		(2 << 7)
#define DREF_SUPERSPREAD_SOURCE_MASK		(3 << 7)
#define DREF_SSC4_DOWNSPREAD				(0 << 6)
#define DREF_SSC4_CENTERSPREAD				(1 << 6)
#define DREF_SSC1_DISABLE					(0 << 1)
#define DREF_SSC1_ENABLE					(1 << 1)
#define DREF_SSC4_DISABLE					(0 << 0)
#define DREF_SSC4_ENABLE					(1 << 0)

// PLL registers
//  Multiplier Divisor
#define INTEL_DISPLAY_A_PLL				(0x6014 | REGS_SOUTH_SHARED)
#define INTEL_DISPLAY_B_PLL				(0x6018 | REGS_SOUTH_SHARED)
#define INTEL_DISPLAY_A_PLL_MD			(0x601C | REGS_SOUTH_SHARED)
#define INTEL_DISPLAY_B_PLL_MD			(0x6020 | REGS_SOUTH_SHARED)
#define CHV_DISPLAY_C_PLL				(0x6030 | REGS_SOUTH_SHARED)
#define CHV_DISPLAY_B_PLL_MD			(0x603C | REGS_SOUTH_SHARED)

#define INTEL_DISPLAY_A_PLL_DIVISOR_0	(0x6040 | REGS_SOUTH_SHARED)
#define INTEL_DISPLAY_A_PLL_DIVISOR_1	(0x6044 | REGS_SOUTH_SHARED)
#define INTEL_DISPLAY_B_PLL_DIVISOR_0	(0x6048 | REGS_SOUTH_SHARED)
#define INTEL_DISPLAY_B_PLL_DIVISOR_1	(0x604c | REGS_SOUTH_SHARED)

#define SNB_DPLL_SEL					(0x7000 | REGS_SOUTH_SHARED)

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
#define INTEL_VGA_DISPLAY_CONTROL		(0x1400 | REGS_NORTH_PLANE_CONTROL)
#define VGA_DISPLAY_DISABLED			(1UL << 31)

// LVDS panel
#define INTEL_PANEL_STATUS				(0x1200 | REGS_NORTH_PIPE_AND_PORT)
#define INTEL_PANEL_CONTROL				(0x1204 | REGS_NORTH_PIPE_AND_PORT)
#define INTEL_PANEL_FIT_CONTROL			(0x1230 | REGS_NORTH_PIPE_AND_PORT)
#define INTEL_PANEL_FIT_RATIOS			(0x1234 | REGS_NORTH_PIPE_AND_PORT)

// LVDS on IronLake and up
#define PCH_PANEL_STATUS				(0x7200 | REGS_SOUTH_SHARED)
#define PCH_PANEL_CONTROL				(0x7204 | REGS_SOUTH_SHARED)
#define PCH_PANEL_ON_DELAYS				(0x7208 | REGS_SOUTH_SHARED)
#define PCH_PANEL_OFF_DELAYS			(0x720c | REGS_SOUTH_SHARED)
#define PCH_PANEL_DIVISOR				(0x7210 | REGS_SOUTH_SHARED)
#define PCH_LVDS_DETECTED				(1 << 1)

#define PANEL_STATUS_POWER_ON			(1UL << 31)
#define PANEL_CONTROL_POWER_TARGET_OFF	(0UL << 0)
#define PANEL_CONTROL_POWER_TARGET_ON	(1UL << 0)
#define PANEL_CONTROL_POWER_TARGET_RST	(1UL << 1)
#define PANEL_REGISTER_UNLOCK			(0xabcd << 16)

// PCH_PANEL_ON_DELAYS
#define PANEL_DELAY_PORT_SELECT_MASK	(3 << 30)
#define PANEL_DELAY_PORT_SELECT_LVDS	(0 << 30)
#define PANEL_DELAY_PORT_SELECT_DPA		(1 << 30)
#define PANEL_DELAY_PORT_SELECT_DPC		(2 << 30)
#define PANEL_DELAY_PORT_SELECT_DPD		(3 << 30)

// PCH_PANEL_DIVISOR
#define PANEL_DIVISOR_REFERENCE_DIV_MASK 0xffffff00
#define PANEL_DIVISOR_REFERENCE_DIV_SHIFT 8
#define PANEL_DIVISOR_POW_CYCLE_DLY_MASK 0x1f
#define PANEL_DIVISOR_POW_CYCLE_DLY_SHIFT 0x1f

// Backlight control registers
#define PCH_BLC_PWM_CTL2                (0x8250 | REGS_NORTH_SHARED)
#define PCH_BLC_PWM_CTL                 (0x8254 | REGS_NORTH_SHARED)
#define PCH_SBLC_PWM_CTL2               (0x8254 | REGS_SOUTH_SHARED)

#define MCH_BLC_PWM_CTL                 (0x1254 | REGS_NORTH_PIPE_AND_PORT)

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

// FDI - Flexible Display Interface, the interface between the (CPU-internal)
// GPU and the PCH display outputs. Proprietary interface, based on DisplayPort
// though, so similar link training and all...
// There's an FDI transmitter (TX) on the CPU and an FDI receiver (RX) on the
// PCH for each display pipe.
// FDI receiver A is hooked up to transcoder A, FDI receiver B is hooked up to
// transcoder B, so we have the same mapping as with the display pipes.
#define PCH_FDI_RX_BASE_REGISTER		0xf0000
#define PCH_FDI_RX_PIPE_OFFSET			0x01000
#define PCH_FDI_RX_CONTROL				0x00c
#define PCH_FDI_RX_MISC					0x010
#define PCH_FDI_RX_IIR					0x014
#define PCH_FDI_RX_IMR					0x018

#define FDI_RX_ENABLE					(1 << 31)
#define FDI_RX_PLL_ENABLED				(1 << 13)

#define FDI_RX_LINK_COLOR_SHIFT			16
#define FDI_RX_LINK_BPC(x)				((x) << FDI_RX_LINK_COLOR_SHIFT)
#define FDI_RX_LINK_BPC_MASK			(7 << FDI_RX_LINK_COLOR_SHIFT)

// Transcoder - same base as FDI_RX
#define PCH_TRANS_CONF_A				0x0008
#define PCH_TRANS_CONF_B				0x1008
#define PCH_TRANS_CONF_C				0x2008

#define TRANS_ENABLE					(1 << 31)
#define TRANS_ENABLED					(1 << 30)

// FDI_tX interrupt register
#define FDI_RX_INTER_LANE_ALIGN         (1 << 10)
#define FDI_RX_SYMBOL_LOCK              (1 << 9)
#define FDI_RX_BIT_LOCK                 (1 << 8)
#define FDI_RX_TRAIN_PATTERN_2_FAIL     (1 << 7)
#define FDI_RX_FS_CODE_ERR              (1 << 6)
#define FDI_RX_FE_CODE_ERR              (1 << 5)
#define FDI_RX_SYMBOL_ERR_RATE_ABOVE    (1 << 4)
#define FDI_RX_HDCP_LINK_FAIL           (1 << 3)
#define FDI_RX_PIXEL_FIFO_OVERFLOW      (1 << 2)
#define FDI_RX_CROSS_CLOCK_OVERFLOW     (1 << 1)
#define FDI_RX_SYMBOL_QUEUE_OVERFLOW    (1 << 0)

#define FDI_FS_ERRC_ENABLE				(1 << 27)
#define FDI_FE_ERRC_ENABLE				(1 << 26)

#define PCH_FDI_RX_TRANS_UNIT_SIZE_1	0x30
#define PCH_FDI_RX_TRANS_UNIT_SIZE_2	0x38
#define FDI_RX_TRANS_UNIT_SIZE(x)		((x - 1) << 25)
#define FDI_RX_TRANS_UNIT_MASK			0x7e000000

#define FDI_RX_ENHANCE_FRAME_ENABLE		(1 << 6)
#define FDI_RX_CLOCK_MASK				(1 << 4)
#define FDI_RX_CLOCK_RAW				(0 << 4)
#define FDI_RX_CLOCK_PCD				(1 << 4)

// FDI RX MISC
#define FDI_RX_PWRDN_LANE1_MASK		(3 << 26)
#define FDI_RX_PWRDN_LANE1_VAL(x)	((x) << 26)
#define FDI_RX_PWRDN_LANE0_MASK		(3 << 24)
#define FDI_RX_PWRDN_LANE0_VAL(x)	((x) << 24)
#define FDI_RX_TP1_TO_TP2_48		(2 << 20)
#define FDI_RX_TP1_TO_TP2_64		(3 << 20)
#define FDI_RX_FDI_DELAY_90			(0x90 << 0)

#define PCH_FDI_TX_BASE_REGISTER			0x60000
#define PCH_FDI_TX_PIPE_OFFSET				0x01000
#define PCH_FDI_TX_CONTROL					0x100
#define FDI_TX_ENABLE						(1 << 31)
#define FDI_LINK_TRAIN_PATTERN_1			(0 << 28)
#define FDI_LINK_TRAIN_PATTERN_2			(1 << 28)
#define FDI_LINK_TRAIN_PATTERN_IDLE			(2 << 28)
#define FDI_LINK_TRAIN_NONE					(3 << 28)
#define FDI_LINK_TRAIN_VOLTAGE_0_4V			(0 << 25)
#define FDI_LINK_TRAIN_VOLTAGE_0_6V			(1 << 25)
#define FDI_LINK_TRAIN_VOLTAGE_0_8V			(2 << 25)
#define FDI_LINK_TRAIN_VOLTAGE_1_2V			(3 << 25)
#define FDI_LINK_TRAIN_PRE_EMPHASIS_NONE	(0 << 22)
#define FDI_LINK_TRAIN_PRE_EMPHASIS_1_5X	(1 << 22)
#define FDI_LINK_TRAIN_PRE_EMPHASIS_2X		(2 << 22)
#define FDI_LINK_TRAIN_PRE_EMPHASIS_3X		(3 << 22)

//FDI PIPE M/N DATA AND LINK VALUES (refreshrate)
#define PCH_FDI_PIPE_A_DATA_M1				0x0030
#define PCH_FDI_PIPE_A_DATA_M2				0x0038
#define PCH_FDI_PIPE_B_DATA_M1				0x1030
#define PCH_FDI_PIPE_B_DATA_M2				0x1038
#define PCH_FDI_PIPE_C_DATA_M1				0x2030
#define PCH_FDI_PIPE_C_DATA_M2				0x2038
#define PCH_FDI_PIPE_A_DATA_N1				0x0034
#define PCH_FDI_PIPE_A_DATA_N2				0x003c
#define PCH_FDI_PIPE_B_DATA_N1				0x1034
#define PCH_FDI_PIPE_B_DATA_N2				0x103c
#define PCH_FDI_PIPE_C_DATA_N1				0x2034
#define PCH_FDI_PIPE_C_DATA_N2				0x203c
#define PCH_FDI_PIPE_A_LINK_M1				0x0040
#define PCH_FDI_PIPE_A_LINK_M2				0x0048
#define PCH_FDI_PIPE_B_LINK_M1				0x1040
#define PCH_FDI_PIPE_B_LINK_M2				0x1048
#define PCH_FDI_PIPE_C_LINK_M1				0x2040
#define PCH_FDI_PIPE_C_LINK_M2				0x2048
#define PCH_FDI_PIPE_A_LINK_N1				0x0044
#define PCH_FDI_PIPE_A_LINK_N2				0x004c
#define PCH_FDI_PIPE_B_LINK_N1				0x1044
#define PCH_FDI_PIPE_B_LINK_N2				0x104c
#define PCH_FDI_PIPE_C_LINK_N1				0x2044
#define PCH_FDI_PIPE_C_LINK_N2				0x204c
#define FDI_PIPE_MN_TU_SIZE_MASK			(0x3f << 25)
#define FDI_PIPE_MN_VALUE_MASK				(0xffffff << 0)

// SNB A stepping
#define FDI_LINK_TRAIN_400MV_0DB_SNB_A		(0x38 << 22)
#define FDI_LINK_TRAIN_400MV_6DB_SNB_A		(0x02 << 22)
#define FDI_LINK_TRAIN_600MV_3_5DB_SNB_A	(0x01 << 22)
#define FDI_LINK_TRAIN_800MV_0DB_SNB_A		(0x00 << 22)

// SNB B stepping
#define FDI_LINK_TRAIN_400MV_0DB_SNB_B		(0x00 << 22)
#define FDI_LINK_TRAIN_400MV_6DB_SNB_B		(0x3a << 22)
#define FDI_LINK_TRAIN_600MV_3_5DB_SNB_B	(0x39 << 22)
#define FDI_LINK_TRAIN_800MV_0DB_SNB_B		(0x38 << 22)
#define FDI_LINK_TRAIN_VOL_EMP_MASK			(0x3f << 22)
#define FDI_TX_ENHANCE_FRAME_ENABLE			(1 << 18)
#define FDI_TX_PLL_ENABLED					(1 << 14)

#define FDI_DP_PORT_WIDTH_SHIFT			19
#define FDI_DP_PORT_WIDTH_MASK			(7 << FDI_DP_PORT_WIDTH_SHIFT)
#define FDI_DP_PORT_WIDTH(width)		(((width) - 1) << FDI_DP_PORT_WIDTH_SHIFT)

#define FDI_PLL_BIOS_0					0x46000
#define FDI_PLL_FB_CLOCK_MASK			0xff
#define FDI_PLL_BIOS_1					0x46004
#define FDI_PLL_BIOS_2					0x46008

#define FDI_AUTO_TRAINING				(1 << 10)
#define FDI_AUTO_TRAIN_DONE				(1 << 1)

#define FDI_LINK_TRAIN_PATTERN_1_CPT	(0 << 8)
#define FDI_LINK_TRAIN_PATTERN_2_CPT	(1 << 8)
#define FDI_LINK_TRAIN_PATTERN_IDLE_CPT	(2 << 8)
#define FDI_LINK_TRAIN_NORMAL_CPT		(3 << 8)
#define FDI_LINK_TRAIN_PATTERN_MASK_CPT	(3 << 8)

// IvyBridge changes it up because... they hate developers?
#define FDI_LINK_TRAIN_PATTERN_1_IVB	(0 << 8)
#define FDI_LINK_TRAIN_PATTERN_2_IVB	(1 << 8)
#define FDI_LINK_TRAIN_PATTERN_IDLE_IVB	(2 << 8)
#define FDI_LINK_TRAIN_NONE_IVB			(3 << 8)

#define PCH_FDI_RXA_CHICKEN				(0x200c | REGS_SOUTH_SHARED)
#define PCH_FDI_RXB_CHICKEN				(0x2010 | REGS_SOUTH_SHARED)
#define FDI_RX_PHASE_SYNC_POINTER_EN	(1 << 0)
#define FDI_RX_PHASE_SYNC_POINTER_OVR	(1 << 1)

// CPU Panel Fitters - These are for IronLake and up and are the CPU internal
// panel fitters.
#define PCH_PANEL_FITTER_BASE_REGISTER	0x68000
#define PCH_PANEL_FITTER_PIPE_OFFSET	0x00800

#define PCH_PANEL_FITTER_WINDOW_POS		0x70
#define PCH_PANEL_FITTER_WINDOW_SIZE	0x74
#define PCH_PANEL_FITTER_CONTROL		0x80
//not on IvyBridge:
#define PCH_PANEL_FITTER_V_SCALE		0x84
#define PCH_PANEL_FITTER_H_SCALE		0x90

#define PANEL_FITTER_ENABLED			(1 << 31)
#define PANEL_FITTER_PIPE_MASK			(3 << 29)
#define PANEL_FITTER_PIPE_A				(0 << 29)
#define PANEL_FITTER_PIPE_B				(1 << 29)
#define PANEL_FITTER_PIPE_C				(2 << 29)
#define PANEL_FITTER_SCALING_MODE_MASK	(7 << 26)
#define PANEL_FITTER_FILTER_MASK		(3 << 24)

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
	return !info.device_type.InGroup(INTEL_GROUP_Gxx);
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
