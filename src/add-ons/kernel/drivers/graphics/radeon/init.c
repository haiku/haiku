/*
	Copyright (c) 2002, Thomas Kurschel


	Part of Radeon kernel driver
		
	Init and clean-up of devices
	
	TBD: support for multiple virtual card per device is
	not implemented yet - there is only one per device;
	apart from additional device names, we need proper
	management of graphics mem to not interfere.
*/

#include "radeon_driver.h"

#include <PCI.h>
#include <stdio.h>
#include <dac_regs.h>
#include <mmio.h>

// helper macros for easier PCI access
#define get_pci(o, s) (*pci_bus->read_pci_config)(pcii->bus, pcii->device, pcii->function, (o), (s))
#define set_pci(o, s, v) (*pci_bus->write_pci_config)(pcii->bus, pcii->device, pcii->function, (o), (s), (v))


// map frame buffer and registers
// mmio_only - true = map registers only (used during detection)
status_t Radeon_MapDevice( device_info *di, bool mmio_only ) 
{
	// framebuffer is stored in PCI range 0, 
	// register map in PCI range 2
	int regs = 2;
	int fb   = 0;
	char buffer[B_OS_NAME_LENGTH];
	shared_info *si = di->si;
	uint32	tmp;
	pci_info *pcii = &(di->pcii);
	status_t result;
	
	SHOW_FLOW( 3, "device: %02X%02X%02X",
		di->pcii.bus, di->pcii.device, di->pcii.function );
	
	si->regs_area = si->fb_area = 0;

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
		(void *) di->pcii.u.h0.base_registers[regs],
		di->pcii.u.h0.base_register_sizes[regs],
		B_ANY_KERNEL_ADDRESS,
		0,
		(void **)&(di->regs));
	if( si->regs_area < 0 ) 
		return si->regs_area;

	if( mmio_only ) 
		return B_OK;
				
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
	SHOW_INFO( 1, "physical address of framebuffer: 0x%8lx-0x%8lx", 
		di->pcii.u.h0.base_registers[fb], 
		di->pcii.u.h0.base_registers[fb] + di->pcii.u.h0.base_register_sizes[fb] - 1 );

	sprintf(buffer, "%04X_%04X_%02X%02X%02X framebuffer",
		di->pcii.vendor_id, di->pcii.device_id,
		di->pcii.bus, di->pcii.device, di->pcii.function);

	si->fb_area = map_physical_memory(
		buffer,
		(void *) di->pcii.u.h0.base_registers[fb],
		di->pcii.u.h0.base_register_sizes[fb],
		B_ANY_KERNEL_BLOCK_ADDRESS | B_MTR_WC,
		B_READ_AREA + B_WRITE_AREA,
		(void **)&(si->framebuffer));

	if (si->fb_area < 0) {
		SHOW_FLOW0( 3, "couldn't enable WC for frame buffer" );
		si->fb_area = map_physical_memory(
			buffer,
			(void *) di->pcii.u.h0.base_registers[fb],
			di->pcii.u.h0.base_register_sizes[fb],
			B_ANY_KERNEL_BLOCK_ADDRESS,
			B_READ_AREA + B_WRITE_AREA,
			(void **)&(si->framebuffer));
	}

	SHOW_FLOW( 3, "mapped frame buffer @%p", si->framebuffer );

	if (si->fb_area < 0) {
		result = si->fb_area;
		goto err;
	}
	
	// save physical address though noone can probably make
	// any use of it
	si->framebuffer_pci = (void *) di->pcii.u.h0.base_registers_pci[fb];
	
	return B_OK;

err:	
	delete_area(si->regs_area);
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
		
	if( si->fb_area > 0 )
		delete_area( si->fb_area );
		
	si->regs_area = si->fb_area = 0;
}


// initialize shared infos on first open
status_t Radeon_FirstOpen( device_info *di )
{
	status_t result;
	char buffer[B_OS_NAME_LENGTH];
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
		B_FULL_LOCK, 0);
	if (di->shared_area < 0) {
		result = di->shared_area;
		goto err8;
	}
	
	memset( di->si, 0, sizeof( *di->si ));
	
	si = di->si;
	
#ifdef ENABLE_LOGGING
	si->log = log_init( 1000000 );
#endif

	// copy all info into shared info
	si->vendor_id = di->pcii.vendor_id;
	si->device_id = di->pcii.device_id;
	si->revision = di->pcii.revision;
	
	si->has_crtc2 = di->has_crtc2;
	si->asic = di->asic;
	
	si->ports[0].disp_type = di->disp_type[0];
	si->ports[1].disp_type = di->disp_type[1];
	si->fp_port = di->fp_info;
	si->pll = di->pll;
/*	si->ram = di->ram;
	strcpy( si->ram_type, di->ram_type );*/
	si->local_mem_size = di->local_mem_size;
	
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
		B_FULL_LOCK, 0);
	if (di->virtual_card_area < 0) {
		result = di->virtual_card_area;
		goto err5;
	}

	// currently, we assign fixed ports to this virtual card
	di->vc->num_ports = si->has_crtc2 ? 2 : 1;
	di->vc->ports[0].is_crtc2 = false;	
	di->vc->ports[0].physical_port = 0;
	di->vc->ports[1].is_crtc2 = true;
	di->vc->ports[1].physical_port = 1;
		
	di->vc->fb_mem_handle = 0;
	di->vc->cursor.mem_handle = 0;
	
	// create unique id
	di->vc->id = di->virtual_card_area;
	
	result = Radeon_MapDevice( di, false );
	if (result < 0) 
		goto err4;
		
	// save dac2_cntl register
	// we need to restore that during uninit, else you only get
	// garbage on screen on reboot
	if( di->has_crtc2 )
		di->dac2_cntl = INREG( di->regs, RADEON_DAC_CNTL2 );
	
	result = Radeon_InitDMA( di );
	if( result < 0 )
		goto err3;
		
	// currently, we don't support VLI - something is broken there
	// (it doesn't change a thing apart from crashing)
/*	result = Radeon_SetupIRQ( di, buffer );
	if( result < 0 )
		goto err2;*/
		
	di->local_memmgr = mem_init( 0, di->local_mem_size, 1024, 
		di->local_mem_size / 1024 );
	if( di->local_memmgr == NULL ) {
		result = B_NO_MEMORY;
		goto err1;
	}
	
	//Radeon_Fix_AGP();

//	mem_alloc( di->local_memmgr, 0x100000, (void *)-1, &dma_block, &dma_offset );
/*	dma_offset = 15 * 1024 * 1024;
	
	si->nonlocal_mem = (uint32 *)((uint32)si->framebuffer + dma_offset);
	si->nonlocal_vm_start = (uint32)si->framebuffer_pci + dma_offset;*/
	
	return B_OK;
	
err1:
	/*Radeon_CleanupIRQ( di );
err2:*/
	Radeon_CleanupDMA( di );
err3:
	Radeon_UnmapDevice( di );
err4:
	delete_area( di->virtual_card_area );
err5:
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
	if( di->has_crtc2 )
		OUTREG( di->regs, RADEON_DAC_CNTL2, di->dac2_cntl );

	mem_destroy( di->local_memmgr );
	
//	Radeon_CleanupIRQ( di );
	Radeon_CleanupDMA( di );
	Radeon_UnmapDevice(di);
	
#ifdef ENABLE_LOGGING
	log_exit( di->si->log );
#endif

	delete_area( di->virtual_card_area );
	delete_area( di->shared_area );
}
