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

#include <driver_settings.h>
#include <util/kernel_cpp.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>


class AreaKeeper {
	public:
		AreaKeeper();
		~AreaKeeper();

		area_id Create(const char *name, void **_virtualAddress, uint32 spec,
			size_t size, uint32 lock, uint32 protection);
		area_id Map(const char *name, void *physicalAddress, size_t numBytes,
			uint32 spec, uint32 protection, void **_virtualAddress);

		status_t InitCheck() { return fArea < B_OK ? (status_t)fArea : B_OK; }
		void Detach();

	private:
		area_id	fArea;
};


AreaKeeper::AreaKeeper()
	:
	fArea(-1)
{
}


AreaKeeper::~AreaKeeper()
{
	if (fArea >= B_OK)
		delete_area(fArea);
}


area_id
AreaKeeper::Create(const char *name, void **_virtualAddress, uint32 spec,
	size_t size, uint32 lock, uint32 protection)
{
	fArea = create_area(name, _virtualAddress, spec, size, lock, protection);
	return fArea;
}


area_id
AreaKeeper::Map(const char *name, void *physicalAddress, size_t numBytes,
	uint32 spec, uint32 protection, void **_virtualAddress)
{
	fArea = map_physical_memory(name, physicalAddress, numBytes, spec, protection,
		_virtualAddress);
	return fArea;
}


void
AreaKeeper::Detach()
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


static size_t
determine_desired_memory_size()
{
	size_t size = 8; // 8 MB

	void *settings = load_driver_settings("intel_extreme");
	if (settings != NULL) {
		size_t specified = 0;
		const char *string = get_driver_parameter(settings, "graphics_memory_size", "0", "0");
		if (string != NULL)
			specified = atoi(string);

		unload_driver_settings(settings);

		if (specified != 0) {
			// take the next power of two
			size_t desired = 1;
			while (desired < specified) {
				desired *= 2;
			}

			if (desired < 128)
				size = desired;
		}
	}

	return size << 20;
}


static size_t
determine_stolen_memory_size(intel_info &info)
{
	// read stolen memory from the PCI configuration of the PCI bridge
	uint16 memoryConfig = gPCI->read_pci_config(0, 0, 0, INTEL_GRAPHICS_MEMORY_CONTROL, 2);
	size_t memorySize = 1 << 20; // 1 MB

	// TODO: test with different models!

	if (info.device_type & INTEL_TYPE_83x) {
		switch (memoryConfig & STOLEN_MEMORY_MASK) {
			case i830_LOCAL_MEMORY_ONLY:
				// TODO: determine its size!
				break;
			case i830_STOLEN_512K:
				memorySize >>= 1;
				break;
			case i830_STOLEN_8M:
				memorySize *= 8;
				break;
		}
	} else if (info.device_type & INTEL_TYPE_85x) {
		switch (memoryConfig & STOLEN_MEMORY_MASK) {
			case i855_STOLEN_MEMORY_4M:
				memorySize *= 4;
				break;
			case i855_STOLEN_MEMORY_8M:
				memorySize *= 8;
				break;
			case i855_STOLEN_MEMORY_16M:
				memorySize *= 16;
				break;
			case i855_STOLEN_MEMORY_32M:
				memorySize *= 32;
				break;
			case i855_STOLEN_MEMORY_48M:
				memorySize *= 48;
				break;
			case i855_STOLEN_MEMORY_64M:
				memorySize *= 64;
				break;
		}
	}

	return memorySize;
}


static void
set_gtt_entry(intel_info &info, uint32 offset, uint8 *physicalAddress)
{
	write32(info.registers + INTEL_GTT_BASE + (offset >> 10),
		(uint32)physicalAddress | GTT_ENTRY_VALID);
}


static int32
release_vblank_sem(intel_info &info)
{
	int32 count;
	if (get_sem_count(info.shared_info->vblank_sem, &count) == B_OK
		&& count < 0) {
		release_sem_etc(info.shared_info->vblank_sem, -count, B_DO_NOT_RESCHEDULE);
		return B_INVOKE_SCHEDULER;
	}

	return B_HANDLED_INTERRUPT;
}


static int32
intel_interrupt_handler(void *data)
{
	intel_info &info = *(intel_info *)data;

	uint32 identity = read16(info.registers + INTEL_INTERRUPT_IDENTITY);
	if (identity == 0)
		return B_UNHANDLED_INTERRUPT;

	int32 handled = B_HANDLED_INTERRUPT;

	if ((identity & INTERRUPT_VBLANK) != 0) {
		handled = release_vblank_sem(info);

		// make sure we'll get another one of those
		write32(info.registers + INTEL_DISPLAY_PIPE_STATUS, DISPLAY_PIPE_VBLANK_STATUS);
	}

	// setting the bit clears it!
	write16(info.registers + INTEL_INTERRUPT_IDENTITY, identity);

	return handled;
}


static void
init_interrupt_handler(intel_info &info)
{
	info.shared_info->vblank_sem = create_sem(0, "intel extreme vblank");
	if (info.shared_info->vblank_sem < B_OK)
		return;

	status_t status = B_OK;

	// We need to change the owner of the sem to the calling team (usually the
	// app_server), because userland apps cannot acquire kernel semaphores
	thread_id thread = find_thread(NULL);
	thread_info threadInfo;
	if (get_thread_info(thread, &threadInfo) != B_OK
		|| set_sem_owner(info.shared_info->vblank_sem, threadInfo.team) != B_OK) {
		status = B_ERROR;
	}

	if (status == B_OK
		&& info.pci->u.h0.interrupt_pin != 0x00 && info.pci->u.h0.interrupt_line != 0xff) {
		// we've gotten an interrupt line for us to use

		info.fake_interrupts = false;

		status = install_io_interrupt_handler(info.pci->u.h0.interrupt_line,
			&intel_interrupt_handler, (void *)&info, 0);
		if (status == B_OK) {
			// enable interrupts - we only want VBLANK interrupts
			write16(info.registers + INTEL_INTERRUPT_ENABLED,
				read16(info.registers + INTEL_INTERRUPT_ENABLED) | INTERRUPT_VBLANK);
			write16(info.registers + INTEL_INTERRUPT_MASK, ~INTERRUPT_VBLANK);

			write32(info.registers + INTEL_DISPLAY_PIPE_STATUS,
				DISPLAY_PIPE_VBLANK_STATUS);
			write16(info.registers + INTEL_INTERRUPT_IDENTITY, ~0);
		}
	}
	if (status < B_OK) {
		// there is no interrupt reserved for us, or we couldn't install our interrupt
		// handler, let's fake the vblank interrupt for our clients using a timer
		// interrupt
		info.fake_interrupts = true;

		// TODO: fake interrupts!
		status = B_ERROR;
	}

	if (status < B_OK) {
		delete_sem(info.shared_info->vblank_sem);
		info.shared_info->vblank_sem = B_ERROR;
	}
}


//	#pragma mark -


status_t
intel_extreme_init(intel_info &info)
{
	AreaKeeper sharedCreator;
	info.shared_area = sharedCreator.Create("intel extreme shared info",
		(void **)&info.shared_info, B_ANY_KERNEL_ADDRESS,
		ROUND_TO_PAGE_SIZE(sizeof(intel_shared_info)) + 3 * B_PAGE_SIZE,
		B_FULL_LOCK, 0);
	if (info.shared_area < B_OK)
		return info.shared_area;

	memset((void *)info.shared_info, 0, sizeof(intel_shared_info));

	// Determine the amount of "stolen" (ie. reserved by the BIOS) graphics memory
	// and see if we need to allocate some more.
	// TODO: make it allocate the memory on demand!

	size_t totalSize = determine_desired_memory_size();

	size_t stolenSize = determine_stolen_memory_size(info);
	totalSize = max_c(totalSize, stolenSize);

	dprintf(DEVICE_NAME ": detected %ld MB of stolen memory, reserving %ld MB total\n",
		stolenSize >> 20, totalSize >> 20);

	AreaKeeper additionalMemoryCreator;
	uint8 *additionalMemory;

	if (stolenSize < totalSize) {
		// Every device should have at least 8 MB - we could also allocate them
		// on demand only, but we're lazy here...
		info.additional_memory_area = additionalMemoryCreator.Create("intel additional memory",
			(void **)&additionalMemory, B_ANY_KERNEL_ADDRESS,
			totalSize - stolenSize, B_FULL_LOCK, 0);
		if (info.additional_memory_area < B_OK)
			return info.additional_memory_area;
	} else
		info.additional_memory_area = B_ERROR;

	// map frame buffer, try to map it write combined

	AreaKeeper graphicsMapper;
	info.graphics_memory_area = graphicsMapper.Map("intel extreme graphics memory",
		(void *)info.pci->u.h0.base_registers[0],
		totalSize, B_ANY_KERNEL_BLOCK_ADDRESS | B_MTR_WC,
		B_READ_AREA | B_WRITE_AREA, (void **)&info.graphics_memory);
	if (graphicsMapper.InitCheck() < B_OK) {
		// try again without write combining
		dprintf(DEVICE_NAME ": enabling write combined mode failed.\n");

		info.graphics_memory_area = graphicsMapper.Map("intel extreme graphics memory",
			(void *)info.pci->u.h0.base_registers[0],
			totalSize/*info.pci->u.h0.base_register_sizes[0]*/, B_ANY_KERNEL_BLOCK_ADDRESS,
			B_READ_AREA | B_WRITE_AREA, (void **)&info.graphics_memory);
	}
	if (graphicsMapper.InitCheck() < B_OK) {
		dprintf(DEVICE_NAME ": could not map frame buffer!\n");
		return info.graphics_memory_area;
	}

	// memory mapped I/O

	AreaKeeper mmioMapper;
	info.registers_area = mmioMapper.Map("intel extreme mmio",
		(void *)info.pci->u.h0.base_registers[1],
		info.pci->u.h0.base_register_sizes[1],
		B_ANY_KERNEL_ADDRESS, 0, (void **)&info.registers);
	if (mmioMapper.InitCheck() < B_OK) {
		dprintf(DEVICE_NAME ": could not map memory I/O!\n");
		return info.registers_area;
	}

	// init graphics memory manager

	info.memory_manager = mem_init("intel extreme memory manager", 0, totalSize, 1024, 
		min_c(totalSize / 1024, 512));
	if (info.memory_manager == NULL)
		return B_NO_MEMORY;

	// reserve ring buffer memory (currently, this memory is placed in
	// the graphics memory), but this could bring us problems with
	// write combining...

	ring_buffer &primary = info.shared_info->primary_ring_buffer;
	if (mem_alloc(info.memory_manager, 4 * B_PAGE_SIZE, &info,
			&primary.handle, &primary.offset) == B_OK) {
		primary.register_base = INTEL_PRIMARY_RING_BUFFER;
		primary.size = 4 * B_PAGE_SIZE;
		primary.base = info.graphics_memory + primary.offset;
	}

	ring_buffer &secondary = info.shared_info->secondary_ring_buffer;
	if (mem_alloc(info.memory_manager, B_PAGE_SIZE, &info,
			&secondary.handle, &secondary.offset) == B_OK) {
		secondary.register_base = INTEL_SECONDARY_RING_BUFFER_0;
		secondary.size = B_PAGE_SIZE;
		secondary.base = info.graphics_memory + secondary.offset;
	}

	// no errors, so keep areas and mappings
	sharedCreator.Detach();
	graphicsMapper.Detach();
	mmioMapper.Detach();

	info.shared_info->graphics_memory_area = info.graphics_memory_area;
	info.shared_info->registers_area = info.registers_area;
	info.shared_info->graphics_memory = info.graphics_memory;
	info.shared_info->physical_graphics_memory = (uint8 *)info.pci->u.h0.base_registers[0];

	info.shared_info->graphics_memory_size = totalSize;
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

	// The hardware status page and the cursor memory share one area with
	// the overlay registers and the shared info

	get_memory_map((uint8 *)info.overlay_registers + B_PAGE_SIZE,
		B_PAGE_SIZE, &physicalEntry, 1);
	info.shared_info->physical_status_page = (uint8 *)physicalEntry.address;

	get_memory_map((uint8 *)info.overlay_registers + 2 * B_PAGE_SIZE,
		B_PAGE_SIZE, &physicalEntry, 1);
	info.shared_info->physical_cursor_memory = (uint8 *)physicalEntry.address;

	// setup the GTT to point to include the additional memory

	if (stolenSize < totalSize) {
		for (size_t offset = stolenSize; offset < totalSize;) {
			physical_entry physicalEntry;
			get_memory_map(additionalMemory + offset - stolenSize,
				totalSize - offset, &physicalEntry, 1);
	
			for (size_t i = 0; i < physicalEntry.size; i += B_PAGE_SIZE) {
				set_gtt_entry(info, offset + i, (uint8 *)physicalEntry.address + i);
			}
	
			offset += physicalEntry.size;
		}
	}

	// We also need to map the cursor memory into the GTT

	// This could also be part of the usual graphics memory, but since we
	// need to make sure it's not in the graphics local memory (and I don't
	// even know yet how to determine that a chip has local memory...), we
	// keep the current strategy and put it into the shared area.
	// Unfortunately, the GTT is not readable until it has been written into
	// the double buffered register set; we cannot get its original contents.

	set_gtt_entry(info, totalSize, info.shared_info->physical_cursor_memory);
	info.shared_info->cursor_buffer_offset = totalSize;

	init_interrupt_handler(info);

	info.cookie_magic = INTEL_COOKIE_MAGIC;
		// this makes the cookie valid to be used

	dprintf(DEVICE_NAME "intel_extreme_init() completed successfully!\n");

	return B_OK;
}


void
intel_extreme_uninit(intel_info &info)
{
	dprintf(DEVICE_NAME": intel_extreme_uninit()\n");

	if (!info.fake_interrupts && info.shared_info->vblank_sem > 0) {
		// disable interrupt generation
		write16(info.registers + INTEL_INTERRUPT_ENABLED, 0);
		write16(info.registers + INTEL_INTERRUPT_MASK, ~0);

		remove_io_interrupt_handler(info.pci->u.h0.interrupt_line,
			intel_interrupt_handler, &info);
	}

	mem_destroy(info.memory_manager);

	delete_area(info.graphics_memory_area);
	delete_area(info.registers_area);
	delete_area(info.shared_area);

	// we may or may not have allocated additional graphics memory
	if (info.additional_memory_area >= B_OK)
		delete_area(info.additional_memory_area);
}

