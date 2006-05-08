/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "intel_extreme.h"
#include "driver.h"
#include "utility.h"

#include <util/kernel_cpp.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

// ToDo: do this correctly
extern "C" void write_isa_io(uchar bus, uchar port, uchar offset);
extern "C" uchar read_isa_io(uchar bus, uchar port, uchar offset);


class PhysicalMemoryMapper {
	public:
		PhysicalMemoryMapper();
		~PhysicalMemoryMapper();

		area_id Map(const char *name, void *physicalAddress, size_t numBytes,
			uint32 spec, uint32 protection, void **virtualAddress);
		status_t InitCheck() { return fArea < B_OK ? (status_t)fArea : B_OK; }
		void Keep();

	private:
		area_id	fArea;
};


PhysicalMemoryMapper::PhysicalMemoryMapper()
	:
	fArea(-1)
{
}


PhysicalMemoryMapper::~PhysicalMemoryMapper()
{
	if (fArea >= B_OK)
		delete_area(fArea);
}


area_id 
PhysicalMemoryMapper::Map(const char *name, void *physicalAddress, size_t numBytes,
	uint32 spec, uint32 protection, void **virtualAddress)
{
	fArea = map_physical_memory(name, physicalAddress, numBytes, spec, protection, virtualAddress);
	return fArea;
}


void 
PhysicalMemoryMapper::Keep()
{
	fArea = -1;
}


//	#pragma mark -


static void
init_overlay_registers(overlay_registers *registers)
{
	memset(registers, 0, B_PAGE_SIZE);

	registers->contrast_correction = 0x40;
	registers->saturation_cos_correction = 0x40;
		// this by-passes contrast and saturation correction
}


//	#pragma mark -


status_t
intel_extreme_init(intel_info &info)
{
	info.shared_area = create_area("intel extreme shared info",
		(void **)&info.shared_info, B_ANY_KERNEL_ADDRESS,
		ROUND_TO_PAGE_SIZE(sizeof(intel_shared_info)) + B_PAGE_SIZE,
		B_FULL_LOCK, 0);
	if (info.shared_area < B_OK)
		return info.shared_area;

	memset((void *)info.shared_info, 0, sizeof(intel_shared_info));

	// get chipset info

	// TODO: read this out of the PCI configuration of the PCI bridge
	size_t memorySize = 8 * 1024 * 1024;

	// map frame buffer, try to map it write combined

	PhysicalMemoryMapper graphicsMapper;
	info.graphics_memory_area = graphicsMapper.Map("intel extreme graphics memory",
		(void *)info.pci->u.h0.base_registers[0],
		memorySize, B_ANY_KERNEL_BLOCK_ADDRESS | B_MTR_WC,
		B_READ_AREA | B_WRITE_AREA, (void **)&info.graphics_memory);
	if (graphicsMapper.InitCheck() < B_OK) {
		// try again without write combining
		dprintf(DEVICE_NAME ": enabling write combined mode failed.\n");

		info.graphics_memory_area = graphicsMapper.Map("intel extreme graphics memory",
			(void *)info.pci->u.h0.base_registers[0],
			memorySize/*info.pci->u.h0.base_register_sizes[0]*/, B_ANY_KERNEL_BLOCK_ADDRESS,
			B_READ_AREA | B_WRITE_AREA, (void **)&info.graphics_memory);
	}
	if (graphicsMapper.InitCheck() < B_OK) {
		dprintf(DEVICE_NAME ": could not map frame buffer!\n");
		return info.graphics_memory_area;
	}

	// memory mapped I/O

	PhysicalMemoryMapper mmioMapper;
	info.registers_area = mmioMapper.Map("intel extreme mmio",
		(void *)info.pci->u.h0.base_registers[1],
		info.pci->u.h0.base_register_sizes[1],
		B_ANY_KERNEL_BLOCK_ADDRESS,
		B_READ_AREA | B_WRITE_AREA,
		(void **)&info.registers);
	if (mmioMapper.InitCheck() < B_OK) {
		dprintf(DEVICE_NAME ": could not map memory I/O!\n");
		return info.registers_area;
	}

	// init graphics memory manager

	info.memory_manager = mem_init("intel extreme memory manager", 0, memorySize, 1024, 
		min_c(memorySize / 1024, 512));
	if (info.memory_manager == NULL)
		return B_NO_MEMORY;

	// reserve ring buffer memory (currently, this memory is placed in
	// the graphics memory), but this could bring us problems with
	// write combining...
	ring_buffer &primary = info.shared_info->primary_ring_buffer;
	if (mem_alloc(info.memory_manager, B_PAGE_SIZE, &info,
			&primary.handle, &primary.offset) == B_OK) {
		primary.register_base = INTEL_PRIMARY_RING_BUFFER;
		primary.size = B_PAGE_SIZE;
		primary.base = info.shared_info->graphics_memory + primary.offset;
	}

	// no errors, so keep mappings
	graphicsMapper.Keep();
	mmioMapper.Keep();

	info.shared_info->graphics_memory_area = info.graphics_memory_area;
	info.shared_info->registers_area = info.registers_area;
	info.shared_info->graphics_memory = info.graphics_memory;
	info.shared_info->physical_graphics_memory = (uint8 *)info.pci->u.h0.base_registers[0];

	info.shared_info->graphics_memory_size = memorySize;
	info.shared_info->frame_buffer_offset = 0;
	info.shared_info->dpms_mode = B_DPMS_ON;
	info.shared_info->pll_info.reference_frequency = 48000;	// 48 kHz
	info.shared_info->pll_info.min_frequency = 25000;		// 25 MHz (not tested)
	info.shared_info->pll_info.max_frequency = 350000;		// 350 MHz RAM DAC speed
	info.shared_info->pll_info.divisor_register = INTEL_DISPLAY_PLL_DIVISOR_0;

	info.shared_info->device_type = info.device_type;
#ifdef __HAIKU__
	strlcpy(info.shared_info->device_identifier, info.device_identifier,
		sizeof(info.shared_info->device_identifier));
#else
	strcpy(info.shared_info->device_identifier, info.device_identifier);
#endif

	// setup overlay registers
	
	info.overlay_registers = (overlay_registers *)((uint8 *)info.shared_info
		+ ROUND_TO_PAGE_SIZE(sizeof(intel_shared_info)));
	init_overlay_registers(info.overlay_registers);

	physical_entry physicalEntry;
	get_memory_map(info.overlay_registers, sizeof(overlay_registers), &physicalEntry, 1);
	info.shared_info->physical_overlay_registers = (uint8 *)physicalEntry.address;

	info.cookie_magic = INTEL_COOKIE_MAGIC;
		// this makes the cookie valid to be used

	dprintf(DEVICE_NAME "intel_extreme_init() completed successfully!\n");

	return B_OK;
}


void
intel_extreme_uninit(intel_info &info)
{
	dprintf(DEVICE_NAME": intel_extreme_uninit()\n");

	mem_destroy(info.memory_manager);

	delete_area(info.graphics_memory_area);
	delete_area(info.registers_area);
	delete_area(info.shared_area);
}

