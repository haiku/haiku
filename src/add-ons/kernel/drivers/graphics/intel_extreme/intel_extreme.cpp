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

#if 0
#define DUMPED_BLOCK_SIZE 16

void
dumpBlock(int file, const uint8 *buffer, int size)
{
	int i;
	
	for (i = 0; i < size;) {
		int start = i;
		char line[512];

		int pos = sprintf(line, "%06x: ", start);

		for (; i < start+DUMPED_BLOCK_SIZE; i++) {
			if (!(i % 4))
				pos += sprintf(line + pos, sizeof(line) - pos, " ");

			if (i >= size)
				break;

			pos += snprintf(line + pos, sizeof(line) - pos, "%02x",
				*(buffer + i));
		}
		pos += snprintf(line + pos, sizeof(line) - pos, "\n");
		write(file, line, pos);
	}
}
#endif

status_t
intel_extreme_init(intel_info &info)
{
	info.shared_area = create_area("intel extreme shared info",
		(void **)&info.shared_info, B_ANY_KERNEL_ADDRESS,
		ROUND_TO_PAGE_SIZE(sizeof(intel_shared_info)),
		B_FULL_LOCK, 0);
	if (info.shared_area < B_OK)
		return info.shared_area;

	memset((void *)info.shared_info, 0, sizeof(intel_shared_info));

	// get chipset info

	// TODO: read this out of the PCI configuration of the PCI bridge
	size_t memorySize = 8 * 1024 * 1024;

	// map frame buffer, try to map it write combined

	PhysicalMemoryMapper fbMapper;
	info.frame_buffer_area = fbMapper.Map("intel extreme frame buffer",
		(void *)info.pci->u.h0.base_registers[0],
		memorySize, B_ANY_KERNEL_BLOCK_ADDRESS | B_MTR_WC,
		B_READ_AREA | B_WRITE_AREA, (void **)&info.frame_buffer);
	if (fbMapper.InitCheck() < B_OK) {
		// try again without write combining
		dprintf(DEVICE_NAME ": enabling write combined mode failed.\n");

		info.frame_buffer_area = fbMapper.Map("intel extreme frame buffer",
			(void *)info.pci->u.h0.base_registers[0],
			memorySize/*info.pci->u.h0.base_register_sizes[0]*/, B_ANY_KERNEL_BLOCK_ADDRESS,
			B_READ_AREA | B_WRITE_AREA, (void **)&info.frame_buffer);
	}
	if (fbMapper.InitCheck() < B_OK) {
		dprintf(DEVICE_NAME ": could not map frame buffer!\n");
		return info.frame_buffer_area;
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

#if 0
// dump registers to a file
int file = open("/boot/home/intel_extreme_registers", O_CREAT | O_TRUNC | O_WRONLY, 0666);
if (file >= 0) {
	dumpBlock(file, info.registers, info.pci->u.h0.base_register_sizes[1]);
	close(file);
	sync();
} else
	dprintf("intel_extreme: could not log (%s)!!\n", strerror(errno));
#endif

	// no errors, so keep mappings
	fbMapper.Keep();
	mmioMapper.Keep();

	info.shared_info->frame_buffer_area = info.frame_buffer_area;
	info.shared_info->registers_area = info.registers_area;
	info.shared_info->frame_buffer = info.frame_buffer;
	info.shared_info->physical_frame_buffer = (uint8 *)info.pci->u.h0.base_registers[0];

	info.shared_info->graphics_memory_size = memorySize;
	info.shared_info->dpms_mode = B_DPMS_ON;
	info.shared_info->pll_info.reference_frequency = 48000;	// 48 kHz
	info.shared_info->pll_info.min_frequency = 100000;		// 100 MHz (not tested)
	info.shared_info->pll_info.max_frequency = 350000;		// 350 MHz RAM DAC speed
	info.shared_info->pll_info.divisor_register = INTEL_DISPLAY_PLL_DIVISOR_0;

	dprintf(DEVICE_NAME "intel_extreme_init() completed successfully!\n");

	return B_OK;
}


void
intel_extreme_uninit(intel_info &info)
{
	dprintf(DEVICE_NAME": intel_extreme_uninit()\n");

	delete_area(info.frame_buffer_area);
	delete_area(info.registers_area);
	delete_area(info.shared_area);
}

