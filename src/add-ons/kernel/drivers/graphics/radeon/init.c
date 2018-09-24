/*
 * Copyright (c) 2002-2004, Thomas Kurschel
 * Copyright (c) 2006-2007, Euan Kirkhope
 *
 * Distributed under the terms of the MIT license.
 */

/*
	Init and clean-up of devices

	TODO: support for multiple virtual card per device is
	not implemented yet - there is only one per device;
	apart from additional device names, we need proper
	management of graphics mem to not interfere.
*/

#include "dac_regs.h"
#include "tv_out_regs.h"
#include "fp_regs.h"
#include "mmio.h"
#include "radeon_driver.h"

#include <PCI.h>

#include <stdio.h>
#include <string.h>

// helper macros for easier PCI access
#define get_pci(o, s) (*pci_bus->read_pci_config)(pcii->bus, pcii->device, pcii->function, (o), (s))
#define set_pci(o, s, v) (*pci_bus->write_pci_config)(pcii->bus, pcii->device, pcii->function, (o), (s), (v))

extern radeon_settings current_settings;

// map frame buffer and registers
// mmio_only - true = map registers only (used during detection)
status_t Radeon_MapDevice( device_info *di, bool mmio_only )
{
	// framebuffer is stored in PCI range 0,
	// register map in PCI range 2
	int regs = 2;
	int fb   = 0;
	char buffer[100];
	shared_info *si = di->si;
	uint32	tmp;
	pci_info *pcii = &(di->pcii);
	status_t result;

	SHOW_FLOW( 3, "device: %02X%02X%02X",
		di->pcii.bus, di->pcii.device, di->pcii.function );

	si->ROM_area = si->regs_area = si->memory[mt_local].area = 0;

	// enable memory mapped IO and frame buffer
	// also, enable bus mastering (some BIOSes seem to
	// disable that, like mine)
	tmp = get_pci( PCI_command, 2 );
	SHOW_FLOW( 3, "old PCI command state: 0x%08lx", tmp );
	tmp |= PCI_command_io | PCI_command_memory | PCI_command_master;
	set_pci( PCI_command, 2, tmp );

	// registers cannot be accessed directly by user apps,
	// they need to clone area for safety reasons
	SHOW_INFO( 1, "physical address of memory-mapped I/O: 0x%8lx-0x%8lx",
		di->pcii.u.h0.base_registers[regs],
		di->pcii.u.h0.base_registers[regs] + di->pcii.u.h0.base_register_sizes[regs] - 1 );

	sprintf( buffer, "%04X_%04X_%02X%02X%02X regs",
		di->pcii.vendor_id, di->pcii.device_id,
		di->pcii.bus, di->pcii.device, di->pcii.function );

	si->regs_area = map_physical_memory(
		buffer,
		di->pcii.u.h0.base_registers[regs],
		di->pcii.u.h0.base_register_sizes[regs],
		B_ANY_KERNEL_ADDRESS,
		/*// for "poke" debugging
		B_READ_AREA + B_WRITE_AREA*/
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_USER_CLONEABLE_AREA,
		(void **)&(di->regs));
	if( si->regs_area < 0 )
		return si->regs_area;

	// that's all during detection as we have no clue about ROM or
	// frame buffer at this point
	if( mmio_only )
		return B_OK;

	// ROM must be explicetely mapped by applications too
	sprintf( buffer, "%04X_%04X_%02X%02X%02X ROM",
		di->pcii.vendor_id, di->pcii.device_id,
		di->pcii.bus, di->pcii.device, di->pcii.function );

	si->ROM_area = map_physical_memory(
		buffer,
		di->rom.phys_address,
		di->rom.size,
		B_ANY_KERNEL_ADDRESS,
		0,
		(void **)&(di->rom.rom_ptr));
	if( si->ROM_area < 0 ) {
		result = si->ROM_area;
		goto err2;
	}

	if( di->pcii.u.h0.base_register_sizes[fb] > di->local_mem_size ) {
		// Radeons allocate more address range then really needed ->
		// only map the area that contains physical memory
		SHOW_INFO( 1, "restrict frame buffer from 0x%8lx to 0x%8lx bytes",
			di->pcii.u.h0.base_register_sizes[fb],
			di->local_mem_size
		);
		di->pcii.u.h0.base_register_sizes[fb] = di->local_mem_size;
	}

	// framebuffer can be accessed by everyone
	// this is not a perfect solution; preferably, only
	// those areas owned by an application are mapped into
	// its address space
	// (this hack is needed by BeOS to write something onto screen in KDL)
	SHOW_INFO( 1, "physical address of framebuffer: 0x%8lx-0x%8lx",
		di->pcii.u.h0.base_registers[fb],
		di->pcii.u.h0.base_registers[fb] + di->pcii.u.h0.base_register_sizes[fb] - 1 );

	sprintf(buffer, "%04X_%04X_%02X%02X%02X framebuffer",
		di->pcii.vendor_id, di->pcii.device_id,
		di->pcii.bus, di->pcii.device, di->pcii.function);

	si->memory[mt_local].area = map_physical_memory(
		buffer,
		di->pcii.u.h0.base_registers[fb],
		di->pcii.u.h0.base_register_sizes[fb],
		B_ANY_KERNEL_BLOCK_ADDRESS | B_MTR_WC,
		B_READ_AREA | B_WRITE_AREA | B_USER_CLONEABLE_AREA,
		(void **)&(si->local_mem));

	if( si->memory[mt_local].area < 0 ) {
		SHOW_FLOW0( 3, "couldn't enable WC for frame buffer" );
		si->memory[mt_local].area = map_physical_memory(
			buffer,
			di->pcii.u.h0.base_registers[fb],
			di->pcii.u.h0.base_register_sizes[fb],
			B_ANY_KERNEL_BLOCK_ADDRESS,
			B_READ_AREA | B_WRITE_AREA | B_USER_CLONEABLE_AREA,
			(void **)&(si->local_mem));
	}

	SHOW_FLOW( 3, "mapped frame buffer @%p", si->local_mem );

	if( si->memory[mt_local].area < 0 ) {
		result = si->memory[mt_local].area;
		goto err;
	}

	// save physical address though noone can probably make
	// any use of it
	si->framebuffer_pci = (void *) di->pcii.u.h0.base_registers_pci[fb];

	return B_OK;

err:
	delete_area( si->ROM_area );
err2:
	delete_area( si->regs_area );
	return result;
}


// unmap PCI ranges
void Radeon_UnmapDevice(device_info *di)
{
	shared_info *si = di->si;
	pci_info *pcii = &(di->pcii);
	uint32 tmp;

	SHOW_FLOW0( 3, "" );

	// disable PCI ranges (though it probably won't
	// hurt	leaving them enabled)
	tmp = get_pci( PCI_command, 2 );
	tmp &= ~PCI_command_io | PCI_command_memory | PCI_command_master;
	set_pci( PCI_command, 2, tmp );

	if( si->regs_area > 0 )
		delete_area( si->regs_area );

	if( si->ROM_area > 0 )
		delete_area( si->ROM_area );

	if( si->memory[mt_local].area > 0 )
		delete_area( si->memory[mt_local].area );

	si->regs_area = si->ROM_area = si->memory[mt_local].area = 0;
}


// initialize shared infos on first open
status_t Radeon_FirstOpen( device_info *di )
{
	status_t result;
	char buffer[100];	// B_OS_NAME_LENGTH is too short
	shared_info *si;
	//uint32 /*dma_block, */dma_offset;

	// create shared info; don't allow access by apps -
	// they'll clone it
	sprintf( buffer, "%04X_%04X_%02X%02X%02X shared",
		di->pcii.vendor_id, di->pcii.device_id,
		di->pcii.bus, di->pcii.device, di->pcii.function );

	di->shared_area = create_area(
		buffer,
		(void **)&(di->si),
		B_ANY_KERNEL_ADDRESS,
		(sizeof(shared_info) + (B_PAGE_SIZE - 1)) & ~(B_PAGE_SIZE - 1),
		B_FULL_LOCK,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_USER_CLONEABLE_AREA);
	if (di->shared_area < 0) {
		result = di->shared_area;
		goto err8;
	}

	memset( di->si, 0, sizeof( *di->si ));

	si = di->si;

	si->settings = di->settings = current_settings;

	if (di->settings.force_acc_dma)
		di->acc_dma = true;
	if (di->settings.force_acc_mmio) // force mmio will override dma... a tristate fuzzylogic, grey bool would be nice...
		di->acc_dma = false;

#ifdef ENABLE_LOGGING
#ifdef LOG_INCLUDE_STARTUP
	si->log = log_init( 1000000 );
#endif
#endif

	// copy all info into shared info
	si->vendor_id = di->pcii.vendor_id;
	si->device_id = di->pcii.device_id;
	si->revision = di->pcii.revision;

	si->asic = di->asic;
	si->is_mobility = di->is_mobility;
	si->tv_chip = di->tv_chip;
	si->new_pll = di->new_pll;
	si->is_igp = di->is_igp;
	si->has_no_i2c = di->has_no_i2c;
	si->is_atombios = di->is_atombios;
	si->is_mobility = di->is_mobility;
	si->panel_pwr_delay = di->si->panel_pwr_delay;
	si->acc_dma = di->acc_dma;

	memcpy(&si->routing, &di->routing, sizeof(disp_entity));

	// detecting theatre channel in kernel would lead to code duplication,
	// so we let the first accelerant take care of it
	si->theatre_channel = -1;

	si->crtc[0].crtc_idx = 0;
	si->crtc[0].flatpanel_port = 0;
	si->crtc[1].crtc_idx = 1;
	si->crtc[1].flatpanel_port = 1;
	si->num_crtc = di->num_crtc;

	if (di->is_mobility)
		si->flatpanels[0] = di->fp_info;

	si->pll = di->pll;

	// create virtual card info; don't allow access by apps -
	// they'll clone it
	sprintf( buffer, "%04X_%04X_%02X%02X%02X virtual card 0",
		di->pcii.vendor_id, di->pcii.device_id,
		di->pcii.bus, di->pcii.device, di->pcii.function );
	di->virtual_card_area = create_area(
		buffer,
		(void **)&(di->vc),
		B_ANY_KERNEL_ADDRESS,
		(sizeof(virtual_card) + (B_PAGE_SIZE - 1)) & ~(B_PAGE_SIZE - 1),
		B_FULL_LOCK,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_USER_CLONEABLE_AREA);
	if (di->virtual_card_area < 0) {
		result = di->virtual_card_area;
		goto err7;
	}

	// currently, we assign fixed ports to this virtual card
	di->vc->assigned_crtc[0] = true;
	di->vc->assigned_crtc[1] = si->num_crtc > 1;
	di->vc->controlled_displays =
		dd_tv_crt | dd_crt | dd_lvds | dd_dvi | dd_dvi_ext | dd_ctv | dd_stv;

	di->vc->fb_mem_handle = 0;
	di->vc->cursor.mem_handle = 0;

	// create unique id
	di->vc->id = di->virtual_card_area;

	result = Radeon_MapDevice( di, false );
	if (result < 0)
		goto err6;

	// save dac2_cntl register
	// on M6, we need to restore that during uninit, else you only get
	// garbage on screen on reboot if both CRTCs are used
	if( di->asic == rt_rv100 && di->is_mobility)
		di->dac2_cntl = INREG( di->regs, RADEON_DAC_CNTL2 );

	memcpy(&si->tmds_pll, &di->tmds_pll, sizeof(di->tmds_pll));
	si->tmds_pll_cntl = INREG( di->regs, RADEON_TMDS_PLL_CNTL);
	si->tmds_transmitter_cntl = INREG( di->regs, RADEON_TMDS_TRANSMITTER_CNTL);

	// print these out to capture bios status...
//	if ( di->is_mobility ) {
		SHOW_INFO0( 2, "Copy of Laptop Display Regs for Reference:");
		SHOW_INFO( 2, "LVDS GEN = %8lx", INREG( di->regs, RADEON_LVDS_GEN_CNTL ));
		SHOW_INFO( 2, "LVDS PLL = %8lx", INREG( di->regs, RADEON_LVDS_PLL_CNTL ));
		SHOW_INFO( 2, "TMDS PLL = %8lx", INREG( di->regs, RADEON_TMDS_PLL_CNTL ));
		SHOW_INFO( 2, "TMDS TRANS = %8lx", INREG( di->regs, RADEON_TMDS_TRANSMITTER_CNTL ));
		SHOW_INFO( 2, "FP1 GEN = %8lx", INREG( di->regs, RADEON_FP_GEN_CNTL ));
		SHOW_INFO( 2, "FP2 GEN = %8lx", INREG( di->regs, RADEON_FP2_GEN_CNTL ));
		SHOW_INFO( 2, "TV DAC = %8lx", INREG( di->regs, RADEON_TV_DAC_CNTL )); //not setup right when ext dvi
//	}

	result = Radeon_InitPCIGART( di );
	if( result < 0 )
		goto err5;

	si->memory[mt_local].size = di->local_mem_size;

	si->memory[mt_PCI].area = di->pci_gart.buffer.area;
	si->memory[mt_PCI].size = di->pci_gart.buffer.size;

	// currently, defaultnon-local memory is PCI memory
	si->nonlocal_type = mt_PCI;

	Radeon_InitMemController( di );

	// currently, we don't support VBI - something is broken there
	// (it doesn't change a thing apart from crashing)
	result = Radeon_SetupIRQ( di, buffer );
	if( result < 0 )
		goto err4;

	// resolution of 2D register is 1K, resolution of CRTC etc. is higher,
	// so 1K is the minimum block size;
	// (CP cannot use local mem)
	di->memmgr[mt_local] = mem_init("radeon local memory", 0, di->local_mem_size, 1024,
		di->local_mem_size / 1024);
	if (di->memmgr[mt_local] == NULL) {
		result = B_NO_MEMORY;
		goto err3;
	}

	// CP requires 4K alignment, which is the most restrictive I found
	di->memmgr[mt_PCI] = mem_init("radeon PCI GART memory", 0, di->pci_gart.buffer.size, 4096,
		di->pci_gart.buffer.size / 4096);
	if (di->memmgr[mt_PCI] == NULL) {
		result = B_NO_MEMORY;
		goto err2;
	}

	// no AGP support
	di->memmgr[mt_AGP] = NULL;

	// fix AGP settings for IGP chipset
	Radeon_Set_AGP( di, !di->settings.force_pci ); // disable AGP


	// time to init Command Processor
	result = Radeon_InitCP( di );
	if( result != B_OK )
		goto err;

	if ( di->acc_dma )
	{
		result = Radeon_InitDMA( di );
		if( result != B_OK )
			goto err0;
	}
	else
	{
		SHOW_INFO0( 0, "DMA is diabled using PIO mode");
	}

//	mem_alloc( di->local_memmgr, 0x100000, (void *)-1, &dma_block, &dma_offset );
/*	dma_offset = 15 * 1024 * 1024;

	si->nonlocal_mem = (uint32 *)((uint32)si->framebuffer + dma_offset);
	si->nonlocal_vm_start = (uint32)si->framebuffer_pci + dma_offset;*/

	// set dynamic clocks for Mobilty chips
	if (di->is_mobility && di->settings.dynamic_clocks)
		Radeon_SetDynamicClock( di, 1);

	return B_OK;

err0:
	Radeon_UninitCP( di );
err:
	mem_destroy( di->memmgr[mt_PCI] );
err2:
	mem_destroy( di->memmgr[mt_local] );
err3:
	Radeon_CleanupIRQ( di );
err4:
	Radeon_CleanupPCIGART( di );
err5:
	Radeon_UnmapDevice( di );
err6:
	delete_area( di->virtual_card_area );
err7:
	delete_area( di->shared_area );
err8:
	return result;
}

// clean up shared info on last close
// (we could for device destruction, but this makes
// testing easier as everythings gets cleaned up
// during tests)
void Radeon_LastClose( device_info *di )
{
	if ( di->acc_dma )
		Radeon_UninitCP( di );

	// M6 fix - unfortunately, the device is never closed by app_server,
	// not even before reboot
	if( di->asic == rt_rv100 && di->is_mobility)
		OUTREG( di->regs, RADEON_DAC_CNTL2, di->dac2_cntl );


	mem_destroy( di->memmgr[mt_local] );

	if( di->memmgr[mt_PCI] )
		mem_destroy( di->memmgr[mt_PCI] );

	if( di->memmgr[mt_AGP] )
		mem_destroy( di->memmgr[mt_AGP] );

	Radeon_CleanupIRQ( di );
	Radeon_CleanupPCIGART( di );
	Radeon_UnmapDevice(di);

#ifdef ENABLE_LOGGING
#ifdef LOG_INCLUDE_STARTUP
	log_exit( di->si->log );
#endif
#endif

	delete_area( di->virtual_card_area );
	delete_area( di->shared_area );
}
