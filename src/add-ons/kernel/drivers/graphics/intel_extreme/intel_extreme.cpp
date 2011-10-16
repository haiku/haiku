/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "intel_extreme.h"

#include "AreaKeeper.h"
#include "driver.h"
#include "utility.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <driver_settings.h>
#include <util/kernel_cpp.h>


#define TRACE_DEVICE
#ifdef TRACE_DEVICE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


static void
init_overlay_registers(overlay_registers* registers)
{
	memset(registers, 0, B_PAGE_SIZE);

	registers->contrast_correction = 0x48;
	registers->saturation_cos_correction = 0x9a;
		// this by-passes contrast and saturation correction
}


static void
read_settings(bool &hardwareCursor)
{
	hardwareCursor = false;

	void* settings = load_driver_settings("intel_extreme");
	if (settings != NULL) {
		hardwareCursor = get_driver_boolean_parameter(settings,
			"hardware_cursor", true, true);

		unload_driver_settings(settings);
	}
}


static int32
release_vblank_sem(intel_info &info)
{
	int32 count;
	if (get_sem_count(info.shared_info->vblank_sem, &count) == B_OK
		&& count < 0) {
		release_sem_etc(info.shared_info->vblank_sem, -count,
			B_DO_NOT_RESCHEDULE);
		return B_INVOKE_SCHEDULER;
	}

	return B_HANDLED_INTERRUPT;
}


static int32
intel_interrupt_handler(void* data)
{
	intel_info &info = *(intel_info*)data;

	uint16 identity = read16(info, INTEL_INTERRUPT_IDENTITY);
	if (identity == 0)
		return B_UNHANDLED_INTERRUPT;

	int32 handled = B_HANDLED_INTERRUPT;

	// TODO: verify that these aren't actually the same
	bool isSNB = info.device_type.InGroup(INTEL_TYPE_SNB);
	uint16 mask = isSNB ? PCH_INTERRUPT_VBLANK_PIPEA : INTERRUPT_VBLANK_PIPEA;
	if ((identity & mask) != 0) {
		handled = release_vblank_sem(info);

		// make sure we'll get another one of those
		write32(info, INTEL_DISPLAY_A_PIPE_STATUS,
			DISPLAY_PIPE_VBLANK_STATUS | DISPLAY_PIPE_VBLANK_ENABLED);
	}

	mask = isSNB ? PCH_INTERRUPT_VBLANK_PIPEB : INTERRUPT_VBLANK_PIPEB;
	if ((identity & mask) != 0) {
		handled = release_vblank_sem(info);

		// make sure we'll get another one of those
		write32(info, INTEL_DISPLAY_B_PIPE_STATUS,
			DISPLAY_PIPE_VBLANK_STATUS | DISPLAY_PIPE_VBLANK_ENABLED);
	}

	// setting the bit clears it!
	write16(info, INTEL_INTERRUPT_IDENTITY, identity);

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
		|| set_sem_owner(info.shared_info->vblank_sem, threadInfo.team)
			!= B_OK) {
		status = B_ERROR;
	}

	if (status == B_OK && info.pci->u.h0.interrupt_pin != 0x00
		&& info.pci->u.h0.interrupt_line != 0xff) {
		// we've gotten an interrupt line for us to use

		info.fake_interrupts = false;

		status = install_io_interrupt_handler(info.pci->u.h0.interrupt_line,
			&intel_interrupt_handler, (void*)&info, 0);
		if (status == B_OK) {
			write32(info, INTEL_DISPLAY_A_PIPE_STATUS,
				DISPLAY_PIPE_VBLANK_STATUS | DISPLAY_PIPE_VBLANK_ENABLED);
			write32(info, INTEL_DISPLAY_B_PIPE_STATUS,
				DISPLAY_PIPE_VBLANK_STATUS | DISPLAY_PIPE_VBLANK_ENABLED);

			write16(info, INTEL_INTERRUPT_IDENTITY, ~0);

			// enable interrupts - we only want VBLANK interrupts
			bool isSNB = info.device_type.InGroup(INTEL_TYPE_SNB);
			uint16 enable = isSNB
				? (PCH_INTERRUPT_VBLANK_PIPEA | PCH_INTERRUPT_VBLANK_PIPEB)
				: (INTERRUPT_VBLANK_PIPEA | INTERRUPT_VBLANK_PIPEB);

			write16(info, INTEL_INTERRUPT_ENABLED,
				read16(info, INTEL_INTERRUPT_ENABLED) | enable);
			write16(info, INTEL_INTERRUPT_MASK, ~enable);
		}
	}
	if (status < B_OK) {
		// There is no interrupt reserved for us, or we couldn't install our
		// interrupt handler, let's fake the vblank interrupt for our clients
		// using a timer interrupt
		info.fake_interrupts = true;

		// TODO: fake interrupts!
		TRACE((DEVICE_NAME "Fake interrupt mode (no PCI interrupt line "
			"assigned)"));
		status = B_ERROR;
	}

	if (status < B_OK) {
		delete_sem(info.shared_info->vblank_sem);
		info.shared_info->vblank_sem = B_ERROR;
	}
}


//	#pragma mark -


status_t
intel_free_memory(intel_info &info, addr_t base)
{
	return gGART->free_memory(info.aperture, base);
}


status_t
intel_allocate_memory(intel_info &info, size_t size, size_t alignment,
	uint32 flags, addr_t* _base, phys_addr_t* _physicalBase)
{
	return gGART->allocate_memory(info.aperture, size, alignment,
		flags, _base, _physicalBase);
}


status_t
intel_extreme_init(intel_info &info)
{
	info.aperture = gGART->map_aperture(info.pci->bus, info.pci->device,
		info.pci->function, 0, &info.aperture_base);
	if (info.aperture < B_OK)
		return info.aperture;

	AreaKeeper sharedCreator;
	info.shared_area = sharedCreator.Create("intel extreme shared info",
		(void**)&info.shared_info, B_ANY_KERNEL_ADDRESS,
		ROUND_TO_PAGE_SIZE(sizeof(intel_shared_info)) + 3 * B_PAGE_SIZE,
		B_FULL_LOCK, 0);
	if (info.shared_area < B_OK) {
		gGART->unmap_aperture(info.aperture);
		return info.shared_area;
	}

	memset((void*)info.shared_info, 0, sizeof(intel_shared_info));

	int fbIndex = 0;
	int mmioIndex = 1;
	if (info.device_type.InFamily(INTEL_TYPE_9xx)) {
		// For some reason Intel saw the need to change the order of the
		// mappings with the introduction of the i9xx family
		mmioIndex = 0;
		fbIndex = 2;
	}

	// evaluate driver settings, if any

	bool hardwareCursor;
	read_settings(hardwareCursor);

	// memory mapped I/O

	// TODO: registers are mapped twice (by us and intel_gart), maybe we
	// can share it between the drivers

	AreaKeeper mmioMapper;
	info.registers_area = mmioMapper.Map("intel extreme mmio",
		(void*)info.pci->u.h0.base_registers[mmioIndex],
		info.pci->u.h0.base_register_sizes[mmioIndex],
		B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
		(void**)&info.registers);
	if (mmioMapper.InitCheck() < B_OK) {
		dprintf(DEVICE_NAME ": could not map memory I/O!\n");
		gGART->unmap_aperture(info.aperture);
		return info.registers_area;
	}

	uint32* blocks = info.shared_info->register_blocks;
	blocks[REGISTER_BLOCK(REGS_FLAT)] = 0;

	// setup the register blocks for the different architectures
	if (info.device_type.InGroup(INTEL_TYPE_SNB)) {
		// PCH based platforms (IronLake and up)
		blocks[REGISTER_BLOCK(REGS_INTERRUPT)]
			= PCH_DE_INTERRUPT_REGISTER_BASE;
		blocks[REGISTER_BLOCK(REGS_NORTH_SHARED)]
			= PCH_NORTH_SHARED_REGISTER_BASE;
		blocks[REGISTER_BLOCK(REGS_NORTH_PIPE_AND_PORT)]
			= PCH_NORTH_PIPE_AND_PORT_REGISTER_BASE;
		blocks[REGISTER_BLOCK(REGS_NORTH_PLANE_CONTROL)]
			= PCH_NORTH_PLANE_CONTROL_REGISTER_BASE;
		blocks[REGISTER_BLOCK(REGS_SOUTH_SHARED)]
			= PCH_SOUTH_SHARED_REGISTER_BASE;
		blocks[REGISTER_BLOCK(REGS_SOUTH_TRANSCODER_PORT)]
			= PCH_SOUTH_TRANSCODER_AND_PORT_REGISTER_BASE;
	} else {
		// (G)MCH/ICH based platforms
		blocks[REGISTER_BLOCK(REGS_INTERRUPT)]
			= MCH_INTERRUPT_REGISTER_BASE;
		blocks[REGISTER_BLOCK(REGS_NORTH_SHARED)]
			= MCH_SHARED_REGISTER_BASE;
		blocks[REGISTER_BLOCK(REGS_NORTH_PIPE_AND_PORT)]
			= MCH_PIPE_AND_PORT_REGISTER_BASE;
		blocks[REGISTER_BLOCK(REGS_NORTH_PLANE_CONTROL)]
			= MCH_PLANE_CONTROL_REGISTER_BASE;
		blocks[REGISTER_BLOCK(REGS_SOUTH_SHARED)]
			= ICH_SHARED_REGISTER_BASE;
		blocks[REGISTER_BLOCK(REGS_SOUTH_TRANSCODER_PORT)]
			= ICH_PORT_REGISTER_BASE;
	}

	// make sure bus master, memory-mapped I/O, and frame buffer is enabled
	set_pci_config(info.pci, PCI_command, 2, get_pci_config(info.pci,
		PCI_command, 2) | PCI_command_io | PCI_command_memory
		| PCI_command_master);

	// reserve ring buffer memory (currently, this memory is placed in
	// the graphics memory), but this could bring us problems with
	// write combining...

	ring_buffer &primary = info.shared_info->primary_ring_buffer;
	if (intel_allocate_memory(info, 16 * B_PAGE_SIZE, 0, 0,
			(addr_t*)&primary.base) == B_OK) {
		primary.register_base = INTEL_PRIMARY_RING_BUFFER;
		primary.size = 16 * B_PAGE_SIZE;
		primary.offset = (addr_t)primary.base - info.aperture_base;
	}

	// Clock gating
	// Fix some problems on certain chips (taken from X driver)
	// TODO: clean this up
	if (info.pci->device_id == 0x2a02 || info.pci->device_id == 0x2a12) {
		dprintf("i965GM/i965GME quirk\n");
		write32(info, 0x6204, (1L << 29));
	} else if (info.device_type.InGroup(INTEL_TYPE_SNB)) {
		dprintf("SNB clock gating\n");
		write32(info, 0x42020, (1L << 28) | (1L << 7) | (1L << 5));
	} else if (info.device_type.InGroup(INTEL_TYPE_G4x)) {
		dprintf("G4x clock gating\n");
		write32(info, 0x6204, 0);
		write32(info, 0x6208, (1L << 9) | (1L << 7) | (1L << 6));
		write32(info, 0x6210, 0);

		uint32 gateValue = (1L << 28) | (1L << 3) | (1L << 2);
		if ((info.device_type.type & INTEL_TYPE_MOBILE) == INTEL_TYPE_MOBILE) {
			dprintf("G4x mobile clock gating\n");
		    gateValue |= 1L << 18;
		}
		write32(info, 0x6200, gateValue);
	} else {
		dprintf("i965 quirk\n");
		write32(info, 0x6204, (1L << 29) | (1L << 23));
	}
	write32(info, 0x7408, 0x10);

	// no errors, so keep areas and mappings
	sharedCreator.Detach();
	mmioMapper.Detach();

	aperture_info apertureInfo;
	gGART->get_aperture_info(info.aperture, &apertureInfo);

	info.shared_info->registers_area = info.registers_area;
	info.shared_info->graphics_memory = (uint8*)info.aperture_base;
	info.shared_info->physical_graphics_memory = apertureInfo.physical_base;
	info.shared_info->graphics_memory_size = apertureInfo.size;
	info.shared_info->frame_buffer = 0;
	info.shared_info->dpms_mode = B_DPMS_ON;

	if (info.device_type.InFamily(INTEL_TYPE_9xx)) {
		info.shared_info->pll_info.reference_frequency = 96000;	// 96 kHz
		info.shared_info->pll_info.max_frequency = 400000;
			// 400 MHz RAM DAC speed
		info.shared_info->pll_info.min_frequency = 20000;		// 20 MHz
	} else {
		info.shared_info->pll_info.reference_frequency = 48000;	// 48 kHz
		info.shared_info->pll_info.max_frequency = 350000;
			// 350 MHz RAM DAC speed
		info.shared_info->pll_info.min_frequency = 25000;		// 25 MHz
	}

	info.shared_info->pll_info.divisor_register = INTEL_DISPLAY_A_PLL_DIVISOR_0;

	info.shared_info->device_type = info.device_type;
#ifdef __HAIKU__
	strlcpy(info.shared_info->device_identifier, info.device_identifier,
		sizeof(info.shared_info->device_identifier));
#else
	strcpy(info.shared_info->device_identifier, info.device_identifier);
#endif

	// setup overlay registers

	if (intel_allocate_memory(info, B_PAGE_SIZE, 0,
			intel_uses_physical_overlay(*info.shared_info)
				? B_APERTURE_NEED_PHYSICAL : 0,
			(addr_t*)&info.overlay_registers,
			&info.shared_info->physical_overlay_registers) == B_OK) {
		info.shared_info->overlay_offset = (addr_t)info.overlay_registers
			- info.aperture_base;
	}

	init_overlay_registers(info.overlay_registers);

	// Allocate hardware status page and the cursor memory

	if (intel_allocate_memory(info, B_PAGE_SIZE, 0, B_APERTURE_NEED_PHYSICAL,
			(addr_t*)info.shared_info->status_page,
			&info.shared_info->physical_status_page) == B_OK) {
		// TODO: set status page
	}
	if (hardwareCursor) {
		intel_allocate_memory(info, B_PAGE_SIZE, 0, B_APERTURE_NEED_PHYSICAL,
			(addr_t*)&info.shared_info->cursor_memory,
			&info.shared_info->physical_cursor_memory);
	}

	init_interrupt_handler(info);

	TRACE((DEVICE_NAME "_init() completed successfully!\n"));
	return B_OK;
}


void
intel_extreme_uninit(intel_info &info)
{
	TRACE((DEVICE_NAME": intel_extreme_uninit()\n"));

	if (!info.fake_interrupts && info.shared_info->vblank_sem > 0) {
		// disable interrupt generation
		write16(info, INTEL_INTERRUPT_ENABLED, 0);
		write16(info, INTEL_INTERRUPT_MASK, ~0);

		remove_io_interrupt_handler(info.pci->u.h0.interrupt_line,
			intel_interrupt_handler, &info);
	}

	gGART->unmap_aperture(info.aperture);

	delete_area(info.registers_area);
	delete_area(info.shared_area);
}

