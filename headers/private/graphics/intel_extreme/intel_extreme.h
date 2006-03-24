/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef INTEL_EXTREME_H
#define INTEL_EXTREME_H


#include <Accelerant.h>
#include <Drivers.h>
#include <PCI.h>


#define VENDOR_ID_INTEL		0x8086
#define DEVICE_ID_i865		0x2572

#define DEVICE_NAME				"intel_extreme"
#define INTEL_ACCELERANT_NAME	"intel_extreme.accelerant"

#define INTEL_COOKIE_MAGIC		'intl'
#define INTEL_FREE_COOKIE_MAGIC 'itlf'

// info about PLL on graphics card
struct pll_info {
	uint32 reference_frequency;
	uint32 max_frequency;
	uint32 min_frequency;
	uint32 divisor_register;
};

struct intel_shared_info {
	int32			type;
	area_id			mode_list_area;		// area containing display mode list
	uint32			mode_count;

	display_mode	current_mode;
	uint32			bytes_per_row;
	uint32			dpms_mode;

	area_id			registers_area;		// area of memory mapped registers
	area_id			frame_buffer_area;	// area of frame buffer
	uint8			*frame_buffer;		// pointer to frame buffer (visible by all apps!)
	uint8			*physical_frame_buffer;

	uint32			graphics_memory_size;
	struct pll_info	pll_info;
};

struct intel_info {
	uint32		cookie_magic;
	int32		open_count;
	int32		id;
	pci_info	*pci;
	uint8		*registers;
	area_id		registers_area;
	struct intel_shared_info *shared_info;
	area_id		shared_area;
	uint8		*frame_buffer;
	area_id		frame_buffer_area;
};

//----------------- ioctl() interface ----------------

// magic code for ioctls
#define INTEL_PRIVATE_DATA_MAGIC		'itic'

// list ioctls
enum {
	INTEL_GET_PRIVATE_DATA = B_DEVICE_OP_CODES_END + 1,

	INTEL_GET_DEVICE_NAME,
	INTEL_ALLOC_LOCAL_MEMORY,
	INTEL_FREE_LOCAL_MEMORY
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
	uint32	fb_offset;
	uint32	handle;
};

// free graphics memory
struct intel_free_graphics_memory {
	uint32 	magic;
	uint32	handle;
};

//----------------------------------------------------------
// Register definitions, taken from X driver

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

//----------------------------------------------------------

extern status_t intel_extreme_init(intel_info &info);
extern void intel_extreme_uninit(intel_info &info);

#endif	/* INTEL_EXTREME_H */
