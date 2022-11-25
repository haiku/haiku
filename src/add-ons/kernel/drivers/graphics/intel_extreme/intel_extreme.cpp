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

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <AreaKeeper.h>
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


static void
gen8_enable_interrupts(intel_info& info, pipe_index pipe, bool enable)
{
	ASSERT(pipe != INTEL_PIPE_ANY);
	ASSERT(info.device_type.Generation() >= 12 || pipe != INTEL_PIPE_D);

	const uint32 regMask = PCH_INTERRUPT_PIPE_MASK_BDW(pipe);
	const uint32 regEnabled = PCH_INTERRUPT_PIPE_ENABLED_BDW(pipe);
	const uint32 regIdentity = PCH_INTERRUPT_PIPE_IDENTITY_BDW(pipe);
	const uint32 value = enable ? PCH_INTERRUPT_VBLANK_BDW : 0;
	write32(info, regIdentity, ~0);
	write32(info, regEnabled, value);
	write32(info, regMask, ~value);
}


static uint32
gen11_enable_global_interrupts(intel_info& info, bool enable)
{
	write32(info, GEN11_GFX_MSTR_IRQ, enable ? GEN11_MASTER_IRQ : 0);
	return enable ? 0 : read32(info, GEN11_GFX_MSTR_IRQ);
}


static uint32
gen8_enable_global_interrupts(intel_info& info, bool enable)
{
	write32(info, PCH_MASTER_INT_CTL_BDW, enable ? PCH_MASTER_INT_CTL_GLOBAL_BDW : 0);
	return enable ? 0 : read32(info, PCH_MASTER_INT_CTL_BDW);
}


/*!
	Checks interrupt status with provided master interrupt control register.
	For Gen8 to Gen11.
*/
static int32
gen8_handle_interrupts(intel_info& info, uint32 interrupt)
{
	int32 handled = B_HANDLED_INTERRUPT;
	if ((interrupt & PCH_MASTER_INT_CTL_PIPE_PENDING_BDW(INTEL_PIPE_A)) != 0) {
		const uint32 regIdentity = PCH_INTERRUPT_PIPE_IDENTITY_BDW(INTEL_PIPE_A);
		uint32 identity = read32(info, regIdentity);
		if ((identity & PCH_INTERRUPT_VBLANK_BDW) != 0) {
			handled = release_vblank_sem(info);
			write32(info, regIdentity, identity | PCH_INTERRUPT_VBLANK_BDW);
		} else {
			dprintf("gen8_handle_interrupts unhandled interrupt on pipe A\n");
		}
		interrupt &= ~PCH_MASTER_INT_CTL_PIPE_PENDING_BDW(INTEL_PIPE_A);
	}
	if ((interrupt & PCH_MASTER_INT_CTL_PIPE_PENDING_BDW(INTEL_PIPE_B)) != 0) {
		const uint32 regIdentity = PCH_INTERRUPT_PIPE_IDENTITY_BDW(INTEL_PIPE_B);
		uint32 identity = read32(info, regIdentity);
		if ((identity & PCH_INTERRUPT_VBLANK_BDW) != 0) {
			handled = release_vblank_sem(info);
			write32(info, regIdentity, identity | PCH_INTERRUPT_VBLANK_BDW);
		} else {
			dprintf("gen8_handle_interrupts unhandled interrupt on pipe B\n");
		}
		interrupt &= ~PCH_MASTER_INT_CTL_PIPE_PENDING_BDW(INTEL_PIPE_B);
	}
	if ((interrupt & PCH_MASTER_INT_CTL_PIPE_PENDING_BDW(INTEL_PIPE_C)) != 0) {
		const uint32 regIdentity = PCH_INTERRUPT_PIPE_IDENTITY_BDW(INTEL_PIPE_C);
		uint32 identity = read32(info, regIdentity);
		if ((identity & PCH_INTERRUPT_VBLANK_BDW) != 0) {
			handled = release_vblank_sem(info);
			write32(info, regIdentity, identity | PCH_INTERRUPT_VBLANK_BDW);
		} else {
			dprintf("gen8_handle_interrupts unhandled interrupt on pipe C\n");
		}
		interrupt &= ~PCH_MASTER_INT_CTL_PIPE_PENDING_BDW(INTEL_PIPE_C);
	}

	if ((interrupt & GEN8_DE_PORT_IRQ) != 0) {
		uint32 iir = read32(info, GEN8_DE_PORT_IIR);
		if (iir != 0) {
			write32(info, GEN8_DE_PORT_IIR, iir);
		}
		interrupt &= ~GEN8_DE_PORT_IRQ;
	}

	if (info.device_type.Generation() >= 11 && (interrupt & GEN11_DE_HPD_IRQ) != 0) {
		dprintf("gen8_handle_interrupts HPD\n");
		uint32 iir = read32(info, GEN11_DE_HPD_IIR);
		if (iir != 0) {
			dprintf("gen8_handle_interrupts HPD_IIR %" B_PRIx32 "\n", iir);
			write32(info, GEN11_DE_HPD_IIR, iir);
		}
		interrupt &= ~GEN11_DE_HPD_IRQ;
	}

	if ((interrupt & GEN8_DE_PCH_IRQ) != 0) {
		dprintf("gen8_handle_interrupts PCH\n");
		uint32 iir = read32(info, SDEIIR);
		if (iir != 0) {
			dprintf("gen8_handle_interrupts PCH_IIR %" B_PRIx32 "\n", iir);
			write32(info, SDEIIR, iir);
			if (info.shared_info->pch_info >= INTEL_PCH_ICP) {
				uint32 ddiHotplug = read32(info, SHOTPLUG_CTL_DDI);
				write32(info, SHOTPLUG_CTL_DDI, ddiHotplug);
				dprintf("gen8_handle_interrupts PCH_IIR ddiHotplug %" B_PRIx32 "\n", ddiHotplug);

				uint32 tcHotplug = read32(info, SHOTPLUG_CTL_TC);
				write32(info, SHOTPLUG_CTL_TC, tcHotplug);
				dprintf("gen8_handle_interrupts PCH_IIR tcHotplug %" B_PRIx32 "\n", tcHotplug);
			}
		}
		interrupt &= ~GEN8_DE_PCH_IRQ;
	}

	interrupt &= ~PCH_MASTER_INT_CTL_GLOBAL_BDW;
	if (interrupt != 0)
		dprintf("gen8_handle_interrupts unhandled %" B_PRIx32 "\n", interrupt);
	return handled;
}



/** Get the appropriate interrupt mask for enabling or testing interrupts on
 * the given pipe.
 *
 * The bits to test or set are different depending on the hardware generation.
 *
 * \param info Intel_extreme driver information
 * \param pipe pipe to use
 * \param enable true to get the mask for enabling the interrupts, false to get
 *               the mask for testing them.
 */
static uint32
intel_get_interrupt_mask(intel_info& info, pipe_index pipe, bool enable)
{
	uint32 mask = 0;
	bool hasPCH = info.pch_info != INTEL_PCH_NONE;

	// Intel changed the PCH register mapping between Sandy Bridge and the
	// later generations (Ivy Bridge and up).
	// The PCH register itself does not exist in pre-PCH platforms, and the
	// previous interrupt register of course also had a different mapping.

	if (pipe == INTEL_PIPE_A) {
		if (info.device_type.InGroup(INTEL_GROUP_SNB)
				|| info.device_type.InGroup(INTEL_GROUP_ILK))
			mask |= PCH_INTERRUPT_VBLANK_PIPEA_SNB;
		else if (hasPCH)
			mask |= PCH_INTERRUPT_VBLANK_PIPEA;
		else
			mask |= INTERRUPT_VBLANK_PIPEA;
	}

	if (pipe == INTEL_PIPE_B) {
		if (info.device_type.InGroup(INTEL_GROUP_SNB)
				|| info.device_type.InGroup(INTEL_GROUP_ILK))
			mask |= PCH_INTERRUPT_VBLANK_PIPEB_SNB;
		else if (hasPCH)
			mask |= PCH_INTERRUPT_VBLANK_PIPEB;
		else
			mask |= INTERRUPT_VBLANK_PIPEB;
	}

#if 0 // FIXME enable when we support the 3rd pipe
	if (pipe == INTEL_PIPE_C) {
		// Older generations only had two pipes
		if (hasPCH && info.device_type.Generation() > 6)
			mask |= PCH_INTERRUPT_VBLANK_PIPEC;
	}
#endif

	// On SandyBridge, there is an extra "global enable" flag, which must also
	// be set when enabling the interrupts (but not when testing for them).
	if (enable && info.device_type.InFamily(INTEL_FAMILY_SER5))
		mask |= PCH_INTERRUPT_GLOBAL_SNB;

	return mask;
}


static void
intel_enable_interrupts(intel_info& info, pipes which, bool enable)
{
	uint32 finalMask = 0;
	const uint32 pipeAMask = intel_get_interrupt_mask(info, INTEL_PIPE_A, true);
	const uint32 pipeBMask = intel_get_interrupt_mask(info, INTEL_PIPE_B, true);
	if (which.HasPipe(INTEL_PIPE_A))
		finalMask |= pipeAMask;
	if (which.HasPipe(INTEL_PIPE_B))
		finalMask |= pipeBMask;

	const uint32 value = enable ? finalMask : 0;

	// Clear all the interrupts
	write32(info, find_reg(info, INTEL_INTERRUPT_IDENTITY), ~0);

	// enable interrupts - we only want VBLANK interrupts
	write32(info, find_reg(info, INTEL_INTERRUPT_ENABLED), value);
	write32(info, find_reg(info, INTEL_INTERRUPT_MASK), ~value);
}


static bool
intel_check_interrupt(intel_info& info, pipes& which)
{
	which.ClearPipe(INTEL_PIPE_ANY);
	const uint32 pipeAMask = intel_get_interrupt_mask(info, INTEL_PIPE_A, false);
	const uint32 pipeBMask = intel_get_interrupt_mask(info, INTEL_PIPE_B, false);
	const uint32 regIdentity = find_reg(info, INTEL_INTERRUPT_IDENTITY);
	const uint32 interrupt = read32(info, regIdentity);
	if ((interrupt & pipeAMask) != 0)
		which.SetPipe(INTEL_PIPE_A);
	if ((interrupt & pipeBMask) != 0)
		which.SetPipe(INTEL_PIPE_B);
	return which.HasPipe(INTEL_PIPE_ANY);
}


static void
g35_clear_interrupt_status(intel_info& info, pipe_index pipe)
{
	// These registers do not exist on later GPUs.
	if (info.device_type.Generation() > 4)
		return;

	const uint32 value = DISPLAY_PIPE_VBLANK_STATUS | DISPLAY_PIPE_VBLANK_ENABLED;
	switch (pipe) {
		case INTEL_PIPE_A:
			write32(info, INTEL_DISPLAY_A_PIPE_STATUS, value);
			break;
		case INTEL_PIPE_B:
			write32(info, INTEL_DISPLAY_B_PIPE_STATUS, value);
			break;
		default:
			break;
	}
}


static void
intel_clear_pipe_interrupt(intel_info& info, pipe_index pipe)
{
	// On G35/G45, prior to clearing Display Pipe interrupt in IIR
	// the corresponding interrupt status must first be cleared.
	g35_clear_interrupt_status(info, pipe);

	const uint32 regIdentity = find_reg(info, INTEL_INTERRUPT_IDENTITY);
	const uint32 bit = intel_get_interrupt_mask(info, pipe, false);
	const uint32 identity = read32(info, regIdentity);
	write32(info, regIdentity, identity | bit);
}


/*!
	Interrupt routine for Gen8 and Gen9.
	See Gen12 Display Engine: Interrupt Service Routine chapter.
*/
static int32
gen8_interrupt_handler(void* data)
{
	intel_info& info = *(intel_info*)data;

	uint32 interrupt = gen8_enable_global_interrupts(info, false);
	if (interrupt == 0) {
		gen8_enable_global_interrupts(info, true);
		return B_UNHANDLED_INTERRUPT;
	}

	int32 handled = gen8_handle_interrupts(info, interrupt);

	gen8_enable_global_interrupts(info, true);
	return handled;
}


/*!
	Interrupt routine for Gen11.
	See Gen12 Display Engine: Interrupt Service Routine chapter.
*/
static int32
gen11_interrupt_handler(void* data)
{
	intel_info& info = *(intel_info*)data;

	uint32 interrupt = gen11_enable_global_interrupts(info, false);

	if (interrupt == 0) {
		gen11_enable_global_interrupts(info, true);
		return B_UNHANDLED_INTERRUPT;
	}

	int32 handled = B_HANDLED_INTERRUPT;
	if ((interrupt & GEN11_DISPLAY_IRQ) != 0)
		handled = gen8_handle_interrupts(info, read32(info, GEN11_DISPLAY_INT_CTL));

	gen11_enable_global_interrupts(info, true);
	return handled;
}


static int32
intel_interrupt_handler(void* data)
{
	intel_info &info = *(intel_info*)data;

	pipes which;
	bool shouldHandle = intel_check_interrupt(info, which);

	if (!shouldHandle)
		return B_UNHANDLED_INTERRUPT;

	int32 handled = B_HANDLED_INTERRUPT;

	while (shouldHandle) {
		if (which.HasPipe(INTEL_PIPE_A)) {
			handled = release_vblank_sem(info);

			intel_clear_pipe_interrupt(info, INTEL_PIPE_A);
		}

		if (which.HasPipe(INTEL_PIPE_B)) {
			handled = release_vblank_sem(info);

			intel_clear_pipe_interrupt(info, INTEL_PIPE_B);
		}

#if 0
		// FIXME we don't have support for the 3rd pipe yet
		if (which.HasPipe(INTEL_PIPE_C)) {
			handled = release_vblank_sem(info);

			intel_clear_pipe_interrupt(info, INTEL_PIPE_C);
		}
#endif

		shouldHandle = intel_check_interrupt(info, which);
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
			TRACE("using message signaled interrupts\n");
			info.irq = msiVector;
			info.use_msi = true;
		}
	}

	if (status == B_OK && info.irq != 0xff) {
		// we've gotten an interrupt line for us to use

		info.fake_interrupts = false;

		if (info.device_type.Generation() >= 8) {
			interrupt_handler handler = &gen8_interrupt_handler;
			if (info.device_type.Generation() >= 11)
				handler = &gen11_interrupt_handler;
			status = install_io_interrupt_handler(info.irq,
				handler, (void*)&info, 0);
			if (status == B_OK) {
				gen8_enable_interrupts(info, INTEL_PIPE_A, true);
				gen8_enable_interrupts(info, INTEL_PIPE_B, true);
				if (info.device_type.Generation() >= 11)
					gen8_enable_interrupts(info, INTEL_PIPE_C, true);
				gen8_enable_global_interrupts(info, true);

				if (info.device_type.Generation() >= 11) {
					if (info.shared_info->pch_info >= INTEL_PCH_ICP) {
						read32(info, SDEIIR);
						write32(info, SDEIER, 0xffffffff);
						write32(info, SDEIMR, ~SDE_GMBUS_ICP);
						read32(info, SDEIMR);
					}

					uint32 mask = GEN8_AUX_CHANNEL_A;
					mask |= GEN9_AUX_CHANNEL_B | GEN9_AUX_CHANNEL_C | GEN9_AUX_CHANNEL_D;
					mask |= CNL_AUX_CHANNEL_F;
					mask |= ICL_AUX_CHANNEL_E;
					read32(info, GEN8_DE_PORT_IIR);
					write32(info, GEN8_DE_PORT_IER, mask);
					write32(info, GEN8_DE_PORT_IMR, ~mask);
					read32(info, GEN8_DE_PORT_IMR);

					read32(info, GEN8_DE_MISC_IIR);
					write32(info, GEN8_DE_MISC_IER, GEN8_DE_EDP_PSR);
					write32(info, GEN8_DE_MISC_IMR, ~GEN8_DE_EDP_PSR);
					read32(info, GEN8_DE_MISC_IMR);

					read32(info, GEN11_GU_MISC_IIR);
					write32(info, GEN11_GU_MISC_IER, GEN11_GU_MISC_GSE);
					write32(info, GEN11_GU_MISC_IMR, ~GEN11_GU_MISC_GSE);
					read32(info, GEN11_GU_MISC_IMR);

					read32(info, GEN11_DE_HPD_IIR);
					write32(info, GEN11_DE_HPD_IER,
						GEN11_DE_TC_HOTPLUG_MASK | GEN11_DE_TBT_HOTPLUG_MASK);
					write32(info, GEN11_DE_HPD_IMR, 0xffffffff);
					read32(info, GEN11_DE_HPD_IMR);

					write32(info, GEN11_TC_HOTPLUG_CTL, 0);
					write32(info, GEN11_TBT_HOTPLUG_CTL, 0);

					if (info.shared_info->pch_info >= INTEL_PCH_ICP) {
						if (info.shared_info->pch_info <= INTEL_PCH_TGP)
							write32(info, SHPD_FILTER_CNT, SHPD_FILTER_CNT_500_ADJ);
						read32(info, SDEIMR);
						write32(info, SDEIMR, 0x3f023f07);
						read32(info, SDEIMR);

						uint32 ctl = read32(info, SHOTPLUG_CTL_DDI);
						// we enable everything, should come from the VBT
						ctl |= SHOTPLUG_CTL_DDI_HPD_ENABLE(HPD_PORT_A)
							| SHOTPLUG_CTL_DDI_HPD_ENABLE(HPD_PORT_B)
							| SHOTPLUG_CTL_DDI_HPD_ENABLE(HPD_PORT_C)
							| SHOTPLUG_CTL_DDI_HPD_ENABLE(HPD_PORT_D);
						write32(info, SHOTPLUG_CTL_DDI, ctl);
						ctl = read32(info, SHOTPLUG_CTL_TC);
						// we enable everything, should come from the VBT
						ctl |= SHOTPLUG_CTL_TC_HPD_ENABLE(HPD_PORT_TC1)
							| SHOTPLUG_CTL_TC_HPD_ENABLE(HPD_PORT_TC2)
							| SHOTPLUG_CTL_TC_HPD_ENABLE(HPD_PORT_TC3)
							| SHOTPLUG_CTL_TC_HPD_ENABLE(HPD_PORT_TC4)
							| SHOTPLUG_CTL_TC_HPD_ENABLE(HPD_PORT_TC5)
							| SHOTPLUG_CTL_TC_HPD_ENABLE(HPD_PORT_TC6);
						write32(info, SHOTPLUG_CTL_TC, ctl);
					}

					gen11_enable_global_interrupts(info, true);
				}
			}
		} else {
			status = install_io_interrupt_handler(info.irq,
				&intel_interrupt_handler, (void*)&info, 0);
			if (status == B_OK) {
				g35_clear_interrupt_status(info, INTEL_PIPE_A);
				g35_clear_interrupt_status(info, INTEL_PIPE_B);

				pipes which;
				which.SetPipe(INTEL_PIPE_A);
				which.SetPipe(INTEL_PIPE_B);
				intel_enable_interrupts(info, which, true);
			}
		}
	}
	if (status < B_OK) {
		// There is no interrupt reserved for us, or we couldn't install our
		// interrupt handler, let's fake the vblank interrupt for our clients
		// using a timer interrupt
		info.fake_interrupts = true;

		// TODO: fake interrupts!
		ERROR("Fake interrupt mode (no PCI interrupt line assigned\n");
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
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_CLONEABLE_AREA);
	if (info.shared_area < B_OK) {
		ERROR("error: could not create shared area!\n");
		gGART->unmap_aperture(info.aperture);
		return info.shared_area;
	}

	// enable power
	gPCI->set_powerstate(info.pci->bus, info.pci->device, info.pci->function,
		PCI_pm_state_d0);

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

	phys_addr_t addr = info.pci->u.h0.base_registers[mmioIndex];
	uint64 barSize = info.pci->u.h0.base_register_sizes[mmioIndex];
	if ((info.pci->u.h0.base_register_flags[mmioIndex] & PCI_address_type) == PCI_address_type_64) {
		addr |= (uint64)info.pci->u.h0.base_registers[mmioIndex + 1] << 32;
		barSize |= (uint64)info.pci->u.h0.base_register_sizes[mmioIndex + 1] << 32;
	}
	AreaKeeper mmioMapper;
	info.registers_area = mmioMapper.Map("intel extreme mmio", addr, barSize,
		B_ANY_KERNEL_ADDRESS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_CLONEABLE_AREA,
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
	info.shared_info->min_brightness = 2;
	info.shared_info->internal_crt_support = true;
	info.shared_info->pch_info = info.pch_info;
	info.shared_info->device_type = info.device_type;

	// Pull VBIOS info for later use
	info.shared_info->got_vbt = parse_vbt_from_bios(info.shared_info);

	/* at least 855gm can't drive more than one head at time */
	if (info.device_type.InFamily(INTEL_FAMILY_8xx))
		info.shared_info->single_head_locked = 1;

	if (info.device_type.InFamily(INTEL_FAMILY_SER5)) {
		info.shared_info->pll_info.reference_frequency = 120000;// 120 MHz
		info.shared_info->pll_info.max_frequency = 350000;
			// 350 MHz RAM DAC speed
		info.shared_info->pll_info.min_frequency = 20000;		// 20 MHz
	} else if (info.device_type.InFamily(INTEL_FAMILY_9xx)) {
		info.shared_info->pll_info.reference_frequency = 96000;	// 96 MHz
		info.shared_info->pll_info.max_frequency = 400000;
			// 400 MHz RAM DAC speed
		info.shared_info->pll_info.min_frequency = 20000;		// 20 MHz
	} else if (info.device_type.HasDDI() && (info.device_type.Generation() <= 8)) {
		info.shared_info->pll_info.reference_frequency = 135000;// 135 MHz
		info.shared_info->pll_info.max_frequency = 350000;
			// 350 MHz RAM DAC speed
		info.shared_info->pll_info.min_frequency = 25000;		// 25 MHz
	} else if ((info.device_type.Generation() >= 9) &&
				info.device_type.InGroup(INTEL_GROUP_SKY)) {
		info.shared_info->pll_info.reference_frequency = 24000;	// 24 MHz
		info.shared_info->pll_info.max_frequency = 350000;
			// 350 MHz RAM DAC speed
		info.shared_info->pll_info.min_frequency = 25000;		// 25 MHz
	} else if (info.device_type.Generation() >= 9) {
		uint32 refInfo =
			(read32(info, ICL_DSSM) & ICL_DSSM_REF_FREQ_MASK) >> ICL_DSSM_REF_FREQ_SHIFT;
		switch (refInfo) {
			case ICL_DSSM_24000:
				info.shared_info->pll_info.reference_frequency = 24000;	// 24 MHz
				break;
			case ICL_DSSM_19200:
				info.shared_info->pll_info.reference_frequency = 19200;	// 19.2 MHz
				break;
			case ICL_DSSM_38400:
				info.shared_info->pll_info.reference_frequency = 38400;	// 38.4 MHz
				break;
			default:
				ERROR("error: unknown ref. freq. strap, using 24Mhz! %" B_PRIx32 "\n", refInfo);
				info.shared_info->pll_info.reference_frequency = 24000;	// 24 MHz
				break;
		}
		info.shared_info->pll_info.max_frequency = 350000;
			// 350 MHz RAM DAC speed
		info.shared_info->pll_info.min_frequency = 25000;		// 25 MHz
	} else {
		info.shared_info->pll_info.reference_frequency = 48000;	// 48 MHz
		info.shared_info->pll_info.max_frequency = 350000;
			// 350 MHz RAM DAC speed
		info.shared_info->pll_info.min_frequency = 25000;		// 25 MHz
	}

	info.shared_info->pll_info.divisor_register = INTEL_DISPLAY_A_PLL_DIVISOR_0;

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
		TRACE("Overlay registers mapped at 0x%" B_PRIx32 " = %p - %"
			B_PRIxADDR " (%" B_PRIxPHYSADDR ")\n",
			info.shared_info->overlay_offset, info.overlay_registers,
			info.aperture_base, info.shared_info->physical_overlay_registers);
		init_overlay_registers(info.overlay_registers);
	} else {
		ERROR("error: could not allocate overlay memory! %s\n",
			strerror(status));
	}

	// Allocate hardware status page and the cursor memory
	TRACE("Allocating hardware status page");

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
		if (info.shared_info->pch_info >= INTEL_PCH_CNP) {
			// TODO read/write info.shared_info->hraw_clock
		} else {
			info.shared_info->hraw_clock = (read32(info, PCH_RAWCLK_FREQ)
				& RAWCLK_FREQ_MASK) * 1000;
			TRACE("%s: rawclk rate: %" B_PRIu32 " kHz\n", __func__, info.shared_info->hraw_clock);
		}
	} else {
		// TODO read info.shared_info->hraw_clock
		info.shared_info->fdi_link_frequency = 0;
	}

	if (info.device_type.InGroup(INTEL_GROUP_BDW)) {
		uint32 lcpll = read32(info, LCPLL_CTL);
		if ((lcpll & LCPLL_CD_SOURCE_FCLK) != 0)
			info.shared_info->hw_cdclk = 800000;
		else if ((read32(info, FUSE_STRAP) & HSW_CDCLK_LIMIT) != 0)
			info.shared_info->hw_cdclk = 450000;
		else if ((lcpll & LCPLL_CLK_FREQ_MASK) == LCPLL_CLK_FREQ_450)
			info.shared_info->hw_cdclk = 450000;
		else if ((lcpll & LCPLL_CLK_FREQ_MASK) == LCPLL_CLK_FREQ_54O_BDW)
			info.shared_info->hw_cdclk = 540000;
		else if ((lcpll & LCPLL_CLK_FREQ_MASK) == LCPLL_CLK_FREQ_337_5_BDW)
			info.shared_info->hw_cdclk = 337500;
		else
			info.shared_info->hw_cdclk = 675000;
	} else if (info.device_type.InGroup(INTEL_GROUP_HAS)) {
		uint32 lcpll = read32(info, LCPLL_CTL);
		if ((lcpll & LCPLL_CD_SOURCE_FCLK) != 0)
			info.shared_info->hw_cdclk = 800000;
		else if ((read32(info, FUSE_STRAP) & HSW_CDCLK_LIMIT) != 0)
			info.shared_info->hw_cdclk = 450000;
		else if ((lcpll & LCPLL_CLK_FREQ_MASK) == LCPLL_CLK_FREQ_450)
			info.shared_info->hw_cdclk = 450000;
		/* ULT type is missing
		else if (IS_ULT)
			info.shared_info->hw_cdclk = 337500;
		*/
		else
			info.shared_info->hw_cdclk = 540000;
	} else if (info.device_type.InGroup(INTEL_GROUP_SNB)
		|| info.device_type.InGroup(INTEL_GROUP_IVB)) {
		info.shared_info->hw_cdclk = 400000;
	} else if (info.device_type.InGroup(INTEL_GROUP_ILK)) {
		info.shared_info->hw_cdclk = 450000;
	}
	TRACE("%s: hw_cdclk: %" B_PRIu32 " kHz\n", __func__, info.shared_info->hw_cdclk);

	TRACE("%s: completed successfully!\n", __func__);
	return B_OK;
}


void
intel_extreme_uninit(intel_info &info)
{
	CALLED();

	if (!info.fake_interrupts && info.shared_info->vblank_sem > 0) {
		// disable interrupt generation
		if (info.device_type.Generation() >= 8) {
			if (info.device_type.Generation() >= 11) {
				gen11_enable_global_interrupts(info, false);
			}
			gen8_enable_global_interrupts(info, false);
			interrupt_handler handler = &gen8_interrupt_handler;
			if (info.device_type.Generation() >= 11)
				handler = &gen11_interrupt_handler;
			remove_io_interrupt_handler(info.irq, handler, &info);
		} else {
			write32(info, find_reg(info, INTEL_INTERRUPT_ENABLED), 0);
			write32(info, find_reg(info, INTEL_INTERRUPT_MASK), ~0);
			remove_io_interrupt_handler(info.irq, intel_interrupt_handler, &info);
		}

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

