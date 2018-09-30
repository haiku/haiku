/*
 * Copyright 2006-2018, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */


#include "intel_extreme.h"

#include "AreaKeeper.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <boot_item.h>
#include <driver_settings.h>
#include <util/kernel_cpp.h>

#include <vesa_info.h>

#include "driver.h"
#include "power.h"
#include "utility.h"


#define TRACE_INTELEXTREME
#ifdef TRACE_INTELEXTREME
#	define TRACE(x...) dprintf("intel_extreme: " x)
#else
#	define TRACE(x) ;
#endif

#define ERROR(x...) dprintf("intel_extreme: " x)
#define CALLED(x...) TRACE("intel_extreme: CALLED %s\n", __PRETTY_FUNCTION__)


static void
init_overlay_registers(overlay_registers* _registers)
{
	user_memset(_registers, 0, B_PAGE_SIZE);

	overlay_registers registers;
	memset(&registers, 0, sizeof(registers));
	registers.contrast_correction = 0x48;
	registers.saturation_cos_correction = 0x9a;
		// this by-passes contrast and saturation correction

	user_memcpy(_registers, &registers, sizeof(overlay_registers));
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


/** Get the appropriate interrupt mask for enabling or testing interrupts on
 * the given pipes.
 *
 * The bits to test or set are different depending on the hardware generation.
 *
 * \param info Intel_extreme driver information
 * \param pipes bit mask of the pipes to use
 * \param enable true to get the mask for enabling the interrupts, false to get
 *               the mask for testing them.
 */
static uint32
intel_get_interrupt_mask(intel_info& info, int pipes, bool enable)
{
	uint32 mask = 0;
	bool hasPCH = info.pch_info != INTEL_PCH_NONE;

	// Intel changed the PCH register mapping between Sandy Bridge and the
	// later generations (Ivy Bridge and up).

	if ((pipes & INTEL_PIPE_A) != 0) {
		if (info.device_type.InGroup(INTEL_GROUP_SNB))
			mask |= PCH_INTERRUPT_VBLANK_PIPEA_SNB;
		else if (hasPCH)
			mask |= PCH_INTERRUPT_VBLANK_PIPEA;
		else
			mask |= INTERRUPT_VBLANK_PIPEA;
	}

	if ((pipes & INTEL_PIPE_B) != 0) {
		if (info.device_type.InGroup(INTEL_GROUP_SNB))
			mask |= PCH_INTERRUPT_VBLANK_PIPEB_SNB;
		else if (hasPCH)
			mask |= PCH_INTERRUPT_VBLANK_PIPEB;
		else
			mask |= INTERRUPT_VBLANK_PIPEB;
	}

	// On SandyBridge, there is an extra "global enable" flag, which must also
	// be set when enabling the interrupts (but not when testing for them).
	if (enable && info.device_type.InGroup(INTEL_GROUP_SNB))
		mask |= PCH_INTERRUPT_GLOBAL_SNB;

	return mask;
}


static int32
intel_interrupt_handler(void* data)
{
	intel_info &info = *(intel_info*)data;
	uint32 reg = find_reg(info, INTEL_INTERRUPT_IDENTITY);
	bool hasPCH = (info.pch_info != INTEL_PCH_NONE);
	uint32 identity;

	if (hasPCH)
		identity = read32(info, reg);
	else
		identity = read16(info, reg);

	if (identity == 0)
		return B_UNHANDLED_INTERRUPT;

	int32 handled = B_HANDLED_INTERRUPT;

	while (identity != 0) {

		uint32 mask = intel_get_interrupt_mask(info, INTEL_PIPE_A, false);

		if ((identity & mask) != 0) {
			handled = release_vblank_sem(info);

			// make sure we'll get another one of those
			write32(info, INTEL_DISPLAY_A_PIPE_STATUS,
				DISPLAY_PIPE_VBLANK_STATUS | DISPLAY_PIPE_VBLANK_ENABLED);
		}

		mask = intel_get_interrupt_mask(info, INTEL_PIPE_B, false);
		if ((identity & mask) != 0) {
			handled = release_vblank_sem(info);

			// make sure we'll get another one of those
			write32(info, INTEL_DISPLAY_B_PIPE_STATUS,
				DISPLAY_PIPE_VBLANK_STATUS | DISPLAY_PIPE_VBLANK_ENABLED);
		}

#if 0
		// FIXME we don't have supprot for the 3rd pipe yet
		mask = hasPCH ? PCH_INTERRUPT_VBLANK_PIPEC
			: 0;
		if ((identity & mask) != 0) {
			handled = release_vblank_sem(info);

			// make sure we'll get another one of those
			write32(info, INTEL_DISPLAY_C_PIPE_STATUS,
				DISPLAY_PIPE_VBLANK_STATUS | DISPLAY_PIPE_VBLANK_ENABLED);
		}
#endif

		// setting the bit clears it!
		if (hasPCH) {
			write32(info, reg, identity);
			identity = read32(info, reg);
		} else {
			write16(info, reg, identity);
			identity = read16(info, reg);
		}
	}

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

	// Find the right interrupt vector, using MSIs if available.
	info.irq = 0xff;
	info.use_msi = false;
	if (info.pci->u.h0.interrupt_pin != 0x00)
		info.irq = info.pci->u.h0.interrupt_line;
	if (gPCIx86Module != NULL && gPCIx86Module->get_msi_count(info.pci->bus,
			info.pci->device, info.pci->function) >= 1) {
		uint8 msiVector = 0;
		if (gPCIx86Module->configure_msi(info.pci->bus, info.pci->device,
				info.pci->function, 1, &msiVector) == B_OK
			&& gPCIx86Module->enable_msi(info.pci->bus, info.pci->device,
				info.pci->function) == B_OK) {
			ERROR("using message signaled interrupts\n");
			info.irq = msiVector;
			info.use_msi = true;
		}
	}

	if (status == B_OK && info.irq != 0xff) {
		// we've gotten an interrupt line for us to use

		info.fake_interrupts = false;

		status = install_io_interrupt_handler(info.irq,
			&intel_interrupt_handler, (void*)&info, 0);
		if (status == B_OK) {
			write32(info, INTEL_DISPLAY_A_PIPE_STATUS,
				DISPLAY_PIPE_VBLANK_STATUS | DISPLAY_PIPE_VBLANK_ENABLED);
			write32(info, INTEL_DISPLAY_B_PIPE_STATUS,
				DISPLAY_PIPE_VBLANK_STATUS | DISPLAY_PIPE_VBLANK_ENABLED);

			bool hasPCH = (info.pch_info != INTEL_PCH_NONE);

			uint32 enable = intel_get_interrupt_mask(info,
				INTEL_PIPE_A | INTEL_PIPE_B, true);

			if (hasPCH) {
				// Clear all the interrupts
				write32(info, find_reg(info, INTEL_INTERRUPT_IDENTITY), ~0);

				// enable interrupts - we only want VBLANK interrupts
				write32(info, find_reg(info, INTEL_INTERRUPT_ENABLED), enable);
				write32(info, find_reg(info, INTEL_INTERRUPT_MASK), ~enable);
			} else {
				// Clear all the interrupts
				write16(info, find_reg(info, INTEL_INTERRUPT_IDENTITY), ~0);

				// enable interrupts - we only want VBLANK interrupts
				write16(info, find_reg(info, INTEL_INTERRUPT_ENABLED), enable);
				write16(info, find_reg(info, INTEL_INTERRUPT_MASK), ~enable);
			}

		}
	}
	if (status < B_OK) {
		// There is no interrupt reserved for us, or we couldn't install our
		// interrupt handler, let's fake the vblank interrupt for our clients
		// using a timer interrupt
		info.fake_interrupts = true;

		// TODO: fake interrupts!
		TRACE("Fake interrupt mode (no PCI interrupt line assigned\n");
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
	CALLED();
	info.aperture = gGART->map_aperture(info.pci->bus, info.pci->device,
		info.pci->function, 0, &info.aperture_base);
	if (info.aperture < B_OK) {
		ERROR("error: could not map GART aperture! (%s)\n",
			strerror(info.aperture));
		return info.aperture;
	}

	AreaKeeper sharedCreator;
	info.shared_area = sharedCreator.Create("intel extreme shared info",
		(void**)&info.shared_info, B_ANY_KERNEL_ADDRESS,
		ROUND_TO_PAGE_SIZE(sizeof(intel_shared_info)) + 3 * B_PAGE_SIZE,
		B_FULL_LOCK,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_USER_CLONEABLE_AREA);
	if (info.shared_area < B_OK) {
		ERROR("error: could not create shared area!\n");
		gGART->unmap_aperture(info.aperture);
		return info.shared_area;
	}

	memset((void*)info.shared_info, 0, sizeof(intel_shared_info));

	int mmioIndex = 1;
	if (info.device_type.Generation() >= 3) {
		// For some reason Intel saw the need to change the order of the
		// mappings with the introduction of the i9xx family
		mmioIndex = 0;
	}

	// evaluate driver settings, if any

	bool hardwareCursor;
	read_settings(hardwareCursor);

	// memory mapped I/O

	// TODO: registers are mapped twice (by us and intel_gart), maybe we
	// can share it between the drivers

	AreaKeeper mmioMapper;
	info.registers_area = mmioMapper.Map("intel extreme mmio",
		info.pci->u.h0.base_registers[mmioIndex],
		info.pci->u.h0.base_register_sizes[mmioIndex],
		B_ANY_KERNEL_ADDRESS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_USER_CLONEABLE_AREA,
		(void**)&info.registers);
	if (mmioMapper.InitCheck() < B_OK) {
		ERROR("error: could not map memory I/O!\n");
		gGART->unmap_aperture(info.aperture);
		return info.registers_area;
	}

	bool hasPCH = (info.pch_info != INTEL_PCH_NONE);

	ERROR("Init Intel generation %d GPU %s PCH split.\n",
		info.device_type.Generation(), hasPCH ? "with" : "without");

	uint32* blocks = info.shared_info->register_blocks;
	blocks[REGISTER_BLOCK(REGS_FLAT)] = 0;

	// setup the register blocks for the different architectures
	if (hasPCH) {
		// PCH based platforms (IronLake through ultra-low-power Broadwells)
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

	// Everything in the display PRM gets +0x180000
	if (info.device_type.InGroup(INTEL_GROUP_VLV)) {
		// "I nearly got violent with the hw guys when they told me..."
		blocks[REGISTER_BLOCK(REGS_SOUTH_SHARED)] += VLV_DISPLAY_BASE;
		blocks[REGISTER_BLOCK(REGS_SOUTH_TRANSCODER_PORT)] += VLV_DISPLAY_BASE;
	}

	TRACE("REGS_NORTH_SHARED: 0x%" B_PRIx32 "\n",
		blocks[REGISTER_BLOCK(REGS_NORTH_SHARED)]);
	TRACE("REGS_NORTH_PIPE_AND_PORT: 0x%" B_PRIx32 "\n",
		blocks[REGISTER_BLOCK(REGS_NORTH_PIPE_AND_PORT)]);
	TRACE("REGS_NORTH_PLANE_CONTROL: 0x%" B_PRIx32 "\n",
		blocks[REGISTER_BLOCK(REGS_NORTH_PLANE_CONTROL)]);
	TRACE("REGS_SOUTH_SHARED: 0x%" B_PRIx32 "\n",
		blocks[REGISTER_BLOCK(REGS_SOUTH_SHARED)]);
	TRACE("REGS_SOUTH_TRANSCODER_PORT: 0x%" B_PRIx32 "\n",
		blocks[REGISTER_BLOCK(REGS_SOUTH_TRANSCODER_PORT)]);

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

	// Enable clock gating
	intel_en_gating(info);

	// Enable automatic gpu downclocking if we can to save power
	intel_en_downclock(info);

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

	// Pull VBIOS panel mode for later use
	info.shared_info->got_vbt = get_lvds_mode_from_bios(
		&info.shared_info->panel_mode);

	/* at least 855gm can't drive more than one head at time */
	if (info.device_type.InFamily(INTEL_FAMILY_8xx))
		info.shared_info->single_head_locked = 1;

	if (info.device_type.InFamily(INTEL_FAMILY_SER5)) {
		info.shared_info->pll_info.reference_frequency = 120000;	// 120 MHz
		info.shared_info->pll_info.max_frequency = 350000;
			// 350 MHz RAM DAC speed
		info.shared_info->pll_info.min_frequency = 20000;		// 20 MHz
	} else if (info.device_type.InFamily(INTEL_FAMILY_9xx)) {
		info.shared_info->pll_info.reference_frequency = 96000;	// 96 MHz
		info.shared_info->pll_info.max_frequency = 400000;
			// 400 MHz RAM DAC speed
		info.shared_info->pll_info.min_frequency = 20000;		// 20 MHz
	} else {
		info.shared_info->pll_info.reference_frequency = 48000;	// 48 MHz
		info.shared_info->pll_info.max_frequency = 350000;
			// 350 MHz RAM DAC speed
		info.shared_info->pll_info.min_frequency = 25000;		// 25 MHz
	}

	info.shared_info->pll_info.divisor_register = INTEL_DISPLAY_A_PLL_DIVISOR_0;

	info.shared_info->pch_info = info.pch_info;

	info.shared_info->device_type = info.device_type;
#ifdef __HAIKU__
	strlcpy(info.shared_info->device_identifier, info.device_identifier,
		sizeof(info.shared_info->device_identifier));
#else
	strcpy(info.shared_info->device_identifier, info.device_identifier);
#endif

	// setup overlay registers

	status_t status = intel_allocate_memory(info, B_PAGE_SIZE, 0,
		intel_uses_physical_overlay(*info.shared_info)
				? B_APERTURE_NEED_PHYSICAL : 0,
		(addr_t*)&info.overlay_registers,
		&info.shared_info->physical_overlay_registers);
	if (status == B_OK) {
		info.shared_info->overlay_offset = (addr_t)info.overlay_registers
			- info.aperture_base;
		init_overlay_registers(info.overlay_registers);
	} else {
		ERROR("error: could not allocate overlay memory! %s\n",
			strerror(status));
	}

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

	edid1_info* edidInfo = (edid1_info*)get_boot_item(VESA_EDID_BOOT_INFO,
		NULL);
	if (edidInfo != NULL) {
		info.shared_info->has_vesa_edid_info = true;
		memcpy(&info.shared_info->vesa_edid_info, edidInfo, sizeof(edid1_info));
	}

	init_interrupt_handler(info);

	if (hasPCH) {
		if (info.device_type.Generation() == 5) {
			info.shared_info->fdi_link_frequency = (read32(info, FDI_PLL_BIOS_0)
				& FDI_PLL_FB_CLOCK_MASK) + 2;
			info.shared_info->fdi_link_frequency *= 100;
		} else {
			info.shared_info->fdi_link_frequency = 2700;
		}
	} else {
		info.shared_info->fdi_link_frequency = 0;
	}

	TRACE("%s: completed successfully!\n", __func__);
	return B_OK;
}


void
intel_extreme_uninit(intel_info &info)
{
	CALLED();

	if (!info.fake_interrupts && info.shared_info->vblank_sem > 0) {
		bool hasPCH = (info.pch_info != INTEL_PCH_NONE);

		// disable interrupt generation
		if (hasPCH) {
			write32(info, find_reg(info, INTEL_INTERRUPT_ENABLED), 0);
			write32(info, find_reg(info, INTEL_INTERRUPT_MASK), ~0);
		} else {
			write16(info, find_reg(info, INTEL_INTERRUPT_ENABLED), 0);
			write16(info, find_reg(info, INTEL_INTERRUPT_MASK), ~0);
		}

		remove_io_interrupt_handler(info.irq, intel_interrupt_handler, &info);

		if (info.use_msi && gPCIx86Module != NULL) {
			gPCIx86Module->disable_msi(info.pci->bus,
				info.pci->device, info.pci->function);
			gPCIx86Module->unconfigure_msi(info.pci->bus,
				info.pci->device, info.pci->function);
		}
	}

	gGART->unmap_aperture(info.aperture);

	delete_area(info.registers_area);
	delete_area(info.shared_area);
}

