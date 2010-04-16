/*
 * Copyright 2006-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "radeon_hd.h"

#include "AreaKeeper.h"
#include "driver.h"
#include "utility.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <driver_settings.h>
#include <util/kernel_cpp.h>
#include <vm/vm.h>


#define TRACE_DEVICE
#ifdef TRACE_DEVICE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


static void
init_overlay_registers(overlay_registers *registers)
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

	void *settings = load_driver_settings("radeon_extreme");
	if (settings != NULL) {
		hardwareCursor = get_driver_boolean_parameter(settings,
			"hardware_cursor", true, true);

		unload_driver_settings(settings);
	}
}


static int32
release_vblank_sem(radeon_info &info)
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
radeon_interrupt_handler(void *data)
{
	radeon_info &info = *(radeon_info *)data;

	uint32 identity = read16(info.registers + RADEON_INTERRUPT_IDENTITY);
	if (identity == 0)
		return B_UNHANDLED_INTERRUPT;

	int32 handled = B_HANDLED_INTERRUPT;

	if ((identity & INTERRUPT_VBLANK) != 0) {
		handled = release_vblank_sem(info);

		// make sure we'll get another one of those
		write32(info.registers + RADEON_DISPLAY_A_PIPE_STATUS,
			DISPLAY_PIPE_VBLANK_STATUS);
	}

	// setting the bit clears it!
	write16(info.registers + RADEON_INTERRUPT_IDENTITY, identity);

	return handled;
}


static void
init_interrupt_handler(radeon_info &info)
{
	info.shared_info->vblank_sem = create_sem(0, "radeon hd vblank");
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

	if (status == B_OK && info.pci->u.h0.interrupt_pin != 0x00
		&& info.pci->u.h0.interrupt_line != 0xff) {
		// we've gotten an interrupt line for us to use

		info.fake_interrupts = false;

		status = install_io_interrupt_handler(info.pci->u.h0.interrupt_line,
			&radeon_interrupt_handler, (void *)&info, 0);
		if (status == B_OK) {
			// enable interrupts - we only want VBLANK interrupts
			write16(info.registers + RADEON_INTERRUPT_ENABLED,
				read16(info.registers + RADEON_INTERRUPT_ENABLED)
				| INTERRUPT_VBLANK);
			write16(info.registers + RADEON_INTERRUPT_MASK, ~INTERRUPT_VBLANK);

			write32(info.registers + RADEON_DISPLAY_A_PIPE_STATUS,
				DISPLAY_PIPE_VBLANK_STATUS);
			write16(info.registers + RADEON_INTERRUPT_IDENTITY, ~0);
		}
	}
	if (status < B_OK) {
		// There is no interrupt reserved for us, or we couldn't install our
		// interrupt handler, let's fake the vblank interrupt for our clients
		// using a timer interrupt
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
radeon_free_memory(radeon_info &info, addr_t base)
{
	return gGART->free_memory(info.aperture, base);
}


status_t
radeon_allocate_memory(radeon_info &info, size_t size, size_t alignment,
	uint32 flags, addr_t *_base, addr_t *_physicalBase)
{
	return gGART->allocate_memory(info.aperture, size, alignment,
		flags, _base, _physicalBase);
}


enum _rs780MCRegs {
    RS78_MC_SYSTEM_STATUS	=	0x0,
    RS78_MC_FB_LOCATION		=	0x10,
    RS78_K8_FB_LOCATION		=	0x11,
    RS78_MC_MISC_UMA_CNTL       =       0x12
};


status_t
radeon_hd_init(radeon_info &info)
{
	int fbIndex = 0;
	int mmioIndex = 1;
	if (info.device_type.InFamily(RADEON_TYPE_9xx)) {
		// For some reason Intel saw the need to change the order of the
		// mappings with the introduction of the i9xx family
		mmioIndex = 0;
		fbIndex = 2;
	}

	// memory mapped I/O

	// TODO: registers are mapped twice (by us and radeon_gart), maybe we
	// can share it between the drivers

#define RHD_FB_BAR   0
#define RHD_MMIO_BAR 2

	
	AreaKeeper sharedCreator;
	info.shared_area = sharedCreator.Create("radeon hd shared info",
		(void **)&info.shared_info, B_ANY_KERNEL_ADDRESS,
		ROUND_TO_PAGE_SIZE(sizeof(radeon_shared_info)), B_FULL_LOCK, 0);
	if (info.shared_area < B_OK) {
		return info.shared_area;
	}

	memset((void *)info.shared_info, 0, sizeof(radeon_shared_info));

	AreaKeeper mmioMapper;
	info.registers_area = mmioMapper.Map("radeon hd mmio",
		(void *)info.pci->u.h0.base_registers[RHD_MMIO_BAR],
		info.pci->u.h0.base_register_sizes[RHD_MMIO_BAR],
		B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
		(void **)&info.registers);
	if (mmioMapper.InitCheck() < B_OK) {
		dprintf(DEVICE_NAME ": could not map memory I/O!\n");
		return info.registers_area;
	}

	AreaKeeper frambufferMapper;
	info.framebuffer_area = frambufferMapper.Map("radeon hd framebuffer",
		(void *)info.pci->u.h0.base_registers[RHD_FB_BAR],
		info.pci->u.h0.base_register_sizes[RHD_FB_BAR],
		B_ANY_KERNEL_ADDRESS, B_READ_AREA | B_WRITE_AREA,
		(void **)&info.shared_info->graphics_memory);
	if (frambufferMapper.InitCheck() < B_OK) {
		dprintf(DEVICE_NAME ": could not map framebuffer!\n");
		return info.framebuffer_area;
	}
	
	// Turn on write combining for the area
	vm_set_area_memory_type(info.framebuffer_area,
		info.pci->u.h0.base_registers[RHD_FB_BAR], B_MTR_WC);
	
	sharedCreator.Detach();
	mmioMapper.Detach();
	frambufferMapper.Detach();

	info.shared_info->registers_area = info.registers_area;
	info.shared_info->frame_buffer_offset = 0;
	info.shared_info->physical_graphics_memory = info.pci->u.h0.base_registers[RHD_FB_BAR];

//??	init_interrupt_handler(info);

#define R6XX_CONFIG_MEMSIZE 0x5428

	uint32 fbLocation = read32(info.registers + RS78_K8_FB_LOCATION);
	TRACE((DEVICE_NAME "radeon_hd_init() fb pci location %x, intern location %x \n",
		info.shared_info->graphics_memory, fbLocation));
	
	uint32 ramSize = read32(info.registers + R6XX_CONFIG_MEMSIZE) >> 10;
	TRACE((DEVICE_NAME "radeon_hd_init() ram size %i \n", ramSize));
	


	/*TRACE((DEVICE_NAME "radeon_hd_init() fb size %i \n",
		info.pci->u.h0.base_register_sizes[RHD_FB_BAR]));

	uint8* fbPosition = info.shared_info->graphics_memory;
	fbPosition += 90000;
	for (int i = 0; i < 90000; i++) {
		*fbPosition = 255;
		fbPosition++;
		*fbPosition = 0;
		fbPosition++;
		*fbPosition = 0;
		fbPosition++;
	}
	fbPosition += 90000;
	for (int i = 0; i < 90000; i++) {
		*fbPosition = 255;
		fbPosition++;
		*fbPosition = 0;
		fbPosition++;
		*fbPosition = 0;
		fbPosition++;
	}*/

	TRACE((DEVICE_NAME "radeon_hd_init() completed successfully!\n"));
	return B_OK;
}


void
radeon_hd_uninit(radeon_info &info)
{
	TRACE((DEVICE_NAME ": radeon_extreme_uninit()\n"));

	delete_area(info.shared_area);
	delete_area(info.registers_area);
	delete_area(info.framebuffer_area);
}

