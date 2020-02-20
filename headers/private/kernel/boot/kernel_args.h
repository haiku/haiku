/*
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef KERNEL_BOOT_KERNEL_ARGS_H
#define KERNEL_BOOT_KERNEL_ARGS_H


#include <SupportDefs.h>

#include <boot/elf.h>
#include <boot/disk_identifier.h>
#include <boot/driver_settings.h>

#include <platform_kernel_args.h>
#include <arch_kernel_args.h>

#include <util/FixedWidthPointer.h>


#define CURRENT_KERNEL_ARGS_VERSION	1
#define MAX_KERNEL_ARGS_RANGE		20

// names of common boot_volume fields
#define BOOT_METHOD						"boot method"
#define BOOT_VOLUME_USER_SELECTED		"user selected"
#define BOOT_VOLUME_BOOTED_FROM_IMAGE	"booted from image"
#define BOOT_VOLUME_PACKAGED			"packaged"
#define BOOT_VOLUME_PACKAGES_STATE		"packages state"
#define BOOT_VOLUME_PARTITION_OFFSET	"partition offset"
#define BOOT_VOLUME_DISK_IDENTIFIER		"disk identifier"

// boot methods
enum {
	BOOT_METHOD_HARD_DISK	= 0,
	BOOT_METHOD_CD			= 1,
	BOOT_METHOD_NET			= 2,

	BOOT_METHOD_DEFAULT		= BOOT_METHOD_HARD_DISK
};

typedef struct kernel_args {
	uint32		kernel_args_size;
	uint32		version;

	FixedWidthPointer<struct preloaded_image> kernel_image;
	FixedWidthPointer<struct preloaded_image> preloaded_images;

	uint32		num_physical_memory_ranges;
	addr_range	physical_memory_range[MAX_PHYSICAL_MEMORY_RANGE];
	uint32		num_physical_allocated_ranges;
	addr_range	physical_allocated_range[MAX_PHYSICAL_ALLOCATED_RANGE];
	uint32		num_virtual_allocated_ranges;
	addr_range	virtual_allocated_range[MAX_VIRTUAL_ALLOCATED_RANGE];
	uint32		num_kernel_args_ranges;
	addr_range	kernel_args_range[MAX_KERNEL_ARGS_RANGE];
	uint64		ignored_physical_memory;

	uint32		num_cpus;
	addr_range	cpu_kstack[SMP_MAX_CPUS];

	// boot volume KMessage data
	FixedWidthPointer<void> boot_volume;
	int32		boot_volume_size;

	FixedWidthPointer<struct driver_settings_file> driver_settings;

	struct {
		addr_range	physical_buffer;
		uint32	bytes_per_row;
		uint16	width;
		uint16	height;
		uint8	depth;
		bool	enabled;
	} frame_buffer;

	FixedWidthPointer<void> vesa_modes;
	uint16		vesa_modes_size;
	uint8		vesa_capabilities;
	FixedWidthPointer<void> edid_info;

	FixedWidthPointer<void> debug_output;
		// If keep_debug_output_buffer, points to a ring_buffer, else to a
		// simple flat buffer. In either case it stores the debug output from
		// the boot loader.
	FixedWidthPointer<void> previous_debug_output;
		// A flat pointer to a buffer containing the debug output from the
		// previous session. May be NULL.
	uint32		debug_size;
		// If keep_debug_output_buffer, the size of the ring buffer, otherwise
		// the size of the flat buffer debug_output points to.
	uint32		previous_debug_size;
		// The size of the buffer previous_debug_output points to. Used as a
		// boolean indicator whether to save the previous session's debug output
		// until initialized for the kernel.
	bool		keep_debug_output_buffer;
		// If true, debug_output is a ring buffer, otherwise a flat buffer.

	platform_kernel_args platform_args;
	arch_kernel_args arch_args;

	// bootsplash data
	FixedWidthPointer<uint8> boot_splash;

	// optional microcode
	FixedWidthPointer<void> ucode_data;
	uint32	ucode_data_size;

} _PACKED kernel_args;


const size_t kernel_args_size_v1 = sizeof(kernel_args)
	- sizeof(FixedWidthPointer<void>) - sizeof(uint32);


#endif	/* KERNEL_BOOT_KERNEL_ARGS_H */
