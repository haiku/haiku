/*
	Copyright (c) 2002, Thomas Kurschel
	

	Part of Radeon kernel driver
		
	DMA initialization/clean-up.
	
	Currently, we use PCI DMA. Changing to AGP would 
	only affect this file, but AGP-GART is specific to
	the chipset of the motherboard, and as DMA is really
	overkill for 2D, I cannot bother writing a dozen
	of AGP drivers just to gain little extra speedup.
*/


#include "radeon_driver.h"
#include <malloc.h>
#include <image.h>
#include <mmio.h>
#include <buscntrl_regs.h>
#include <memcntrl_regs.h>
#include <cp_regs.h>
#include <string.h>


#if 0
// create actual DMA buffer
static status_t createDMABuffer( DMA_buffer *buffer, size_t size )
{
	SHOW_FLOW0( 3, "" );
	
	buffer->size = size = (size + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);

	// if this buffer is used for PCI BM, cache snooping
	// takes care of syncing memory accesses; if used for AGP,
	// we'll have to access via AGP aperture (and mark aperture
	// as write-combined) as cache consistency doesn't need to 
	// be guaranteed
	
	// the specs say that some chipsets do kind of lazy flushing
	// so the graphics card may read obsolete data; up to now
	// we use PCI only where this shouldn't happen by design;
	// if we change to AGP we may tweak the pre-charge time of
	// the write buffer pointer 
	
	// as some variables in accelerant point directly into
	// the DMA buffer, we have to grant access for all apps
	buffer->buffer_area = create_area( "Radeon DMA buffer", 
		&buffer->ptr, B_ANY_KERNEL_ADDRESS, 
		size, B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA );
	if( buffer->buffer_area < 0 ) {
		SHOW_ERROR( 1, "cannot create DMA buffer (%s)", 
			strerror( buffer->buffer_area ));
		return buffer->buffer_area;
	}
	
	memset( buffer->ptr, 0, size );

	return B_OK;
}
#endif

static status_t createDMABuffer( DMA_buffer *buffer, size_t size )
{
	physical_entry map[1];
	void *unaligned_addr, *aligned_phys;
	
	SHOW_FLOW0( 3, "" );
	
	buffer->size = size = (size + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);
	
	// we allocate an contiguous area having twice the size
	// to be able to find an aligned, contiguous range within it;
	// the graphics card doesn't care, but the CPU cannot
	// make an arbitrary area WC'ed, at least elder ones
	// question: is this necessary for a PCI GART because of bus snooping?
	buffer->unaligned_area = create_area( "Radeon DMA buffer", 
		&unaligned_addr, B_ANY_KERNEL_ADDRESS, 
		2 * size, B_CONTIGUOUS/*B_FULL_LOCK*/, B_READ_AREA | B_WRITE_AREA );
	if( buffer->unaligned_area < 0 ) {
		SHOW_ERROR( 1, "cannot create DMA buffer (%s)", 
			strerror( buffer->unaligned_area ));
		return buffer->unaligned_area;
	}
	
	get_memory_map( unaligned_addr, B_PAGE_SIZE, map, 1 );
	
	aligned_phys = 
		(void **)(((uint32)map[0].address + size - 1) & ~(size - 1));
		
	SHOW_FLOW( 3, "aligned_phys=%p", aligned_phys );
	
	buffer->buffer_area = map_physical_memory( "Radeon aligned DMA buffer", 
		aligned_phys,
		size, B_ANY_KERNEL_BLOCK_ADDRESS | B_MTR_WC, 
		B_READ_AREA | B_WRITE_AREA, &buffer->ptr );
		
	if( buffer->buffer_area < 0 ) {	
		SHOW_ERROR0( 3, "cannot map buffer with WC" );
		buffer->buffer_area = map_physical_memory( "Radeon aligned DMA buffer", 
			aligned_phys,
			size, B_ANY_KERNEL_BLOCK_ADDRESS, 
			B_READ_AREA | B_WRITE_AREA, &buffer->ptr );
	}

	if( buffer->buffer_area < 0 ) {
		SHOW_ERROR0( 1, "cannot map DMA buffer" );
		delete_area( buffer->unaligned_area );
		buffer->unaligned_area = -1;
		return buffer->buffer_area;
	}
	
	memset( buffer->ptr, 0, size );

	return B_OK;
}


// init GART (could be used for both PCI and AGP)
static status_t initGART( DMA_buffer *buffer )
{
	physical_entry *map;
	physical_entry PTB_map[1];
	size_t map_count;
	int i;
	uint32 *gart_entry;
	size_t num_pages;
	
	SHOW_FLOW0( 3, "" );
	
	num_pages = (buffer->size + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);

	// GART must be contignuous
	buffer->GART_area = create_area( "Radeon GART", (void **)&buffer->GART_ptr, 
		B_ANY_KERNEL_ADDRESS, 
		(num_pages * sizeof( uint32 ) + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1), 
		B_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA );
		
	if( buffer->GART_area < 0 ) {
		SHOW_ERROR( 1, "cannot create GART table (%s)", 
			strerror( buffer->GART_area ));
		return buffer->GART_area;
	}
	
	get_memory_map( buffer->GART_ptr, B_PAGE_SIZE, PTB_map, 1 );
	buffer->GART_phys = (uint32)PTB_map[0].address;
	
	SHOW_INFO( 3, "GART_ptr=%p, GART_phys=%p", buffer->GART_ptr, 
		(void *)buffer->GART_phys );
	
	// get address mapping
	memset( buffer->GART_ptr, 0, num_pages * sizeof( uint32 ));
	
	map_count = num_pages + 1;
	map = malloc( map_count * sizeof( physical_entry ) );
	
	get_memory_map( buffer->ptr, buffer->size, map, map_count );
	
	// the following looks a bit strange as the kernel
	// combines successive entries
	gart_entry = buffer->GART_ptr;
	
	for( i = 0; i < (int)map_count; ++i ) {
		uint32 addr = (uint32)map[i].address;
		size_t size = map[i].size;
		
		if( size == 0 )
			break;
		
		while( size > 0 ) {
			*gart_entry++ = addr;
			//SHOW_FLOW( 3, "%lx", *(gart_entry-1) );
			addr += ATI_PCIGART_PAGE_SIZE;
			size -= ATI_PCIGART_PAGE_SIZE;
		}
	}
	
	free( map );
	
	if( i == (int)map_count ) {
		// this case should never happen
		SHOW_ERROR0( 0, "memory map of DMA buffer too large!" );
		delete_area( buffer->GART_area );
		buffer->GART_area = -1;
		return B_ERROR;
	}

	// this might be a bit more than needed, as
	// 1. Intel CPUs have "processor order", i.e. writes appear to external
	//    devices in program order, so a simple final write should be sufficient
	// 2. if it is a PCI GART, bus snooping should provide cache coherence
	// 3. this function is a no-op :(
	clear_caches( buffer->GART_ptr, num_pages * sizeof( uint32 ), 
		B_FLUSH_DCACHE );

	// back to real live - some chipsets have write buffers that 
	// proove all previous assumption wrong
	asm volatile ( "wbinvd" ::: "memory" );	
	return B_OK;
}


// destroy DMA buffer
static void destroyDMABuffer( DMA_buffer *buffer )
{
	if( buffer->buffer_area >= 0 )
		delete_area( buffer->buffer_area );
		
	buffer->buffer_area = -1;
}


// destroy GART
static void destroyGART( DMA_buffer *buffer )
{
	if( buffer->unaligned_area >= 0 )
		delete_area( buffer->unaligned_area );

	if( buffer->GART_area >= 0 )
		delete_area( buffer->GART_area );
		
	buffer->GART_area = -1;
}


// initialize DMA (well - we initialize entire memory controller)
status_t Radeon_InitDMA( device_info *di )
{
	vuint8 *regs = di->regs;
	shared_info *si = di->si;
	status_t result;

	// any address not covered by frame buffer, PCI GART or AGP aperture
	// leads to a direct memory access
	// -> this is dangerous, so we make sure entire address space is mapped
	
	// locate PCI GART at top of address space
	si->nonlocal_vm_start = -ATI_MAX_PCIGART_PAGES * ATI_PCIGART_PAGE_SIZE;
	si->nonlocal_mem_size = ATI_MAX_PCIGART_PAGES * ATI_PCIGART_PAGE_SIZE;
	
	SHOW_INFO( 3, "Non-local memory %lx@%lx (graphics card address)", 
		si->nonlocal_mem_size, si->nonlocal_vm_start );

	// let AGP range overlap with frame buffer to hide it;
	// according to spec, frame buffer should win but we better
	// choose an unused-area to avoid trouble
	// (specs don't talk about overlapping area, let's hope
	// the memory controller won't choke if we ever access it)
	si->AGP_vm_start = si->nonlocal_vm_start - 0x10000;
	si->AGP_vm_size = 0x10000;
	
	result = createDMABuffer( &di->DMABuffer, 0x80000 );
	if( result < 0 )
		goto err1;
				
	result = initGART( &di->DMABuffer );
	if( result < 0 )
		goto err2;
		
	SHOW_INFO( 3, "GART=%p", di->DMABuffer.ptr );
	
	si->nonlocal_mem = di->DMABuffer.ptr;
		
	// set PCI GART page-table base address
	OUTREG( regs, RADEON_AIC_PT_BASE, di->DMABuffer.GART_phys );

	// set address range for PCI address translate
	OUTREG( regs, RADEON_AIC_LO_ADDR, si->nonlocal_vm_start );
	OUTREG( regs, RADEON_AIC_HI_ADDR, si->nonlocal_vm_start + 
		di->DMABuffer.size - 1 );

	// set AGP address range
	OUTREG( regs, RADEON_MC_AGP_LOCATION, 
		(si->AGP_vm_start & 0xffff0000) |
		((si->AGP_vm_start + si->AGP_vm_size - 1) >> 16 ));

	// set address range of video memory
	// (lower word = begin >> 16
	//  upper word = end >> 16)
	// let it cover all remaining addresses; 
	// addresses are wrapped 
	OUTREG( regs, RADEON_MC_FB_LOCATION,
		((si->nonlocal_vm_start - 1) & 0xffff0000) |
		 (0 >> 16) );

	// disable AGP
	OUTREG( regs, RADEON_AGP_COMMAND, 0 );
		 
	// Turn on PCI GART
#if 1
	OUTREGP( regs, RADEON_AIC_CNTL, RADEON_PCIGART_TRANSLATE_EN, 
		~RADEON_PCIGART_TRANSLATE_EN );
#endif
	
//	SHOW_FLOW0( 3, "done" );
	
	return B_OK;
	
err2:
	destroyDMABuffer( &di->DMABuffer );
	
err1:
	return result;
}


// cleanup DMA
void Radeon_CleanupDMA( device_info *di )
{
	vuint8 *regs = di->regs;
	
	SHOW_FLOW0( 3, "" );
	
	// perhaps we should wait for FIFO space before messing around with registers, but
	// 1. I don't want to add all the sync stuff to the kernel driver
	// 2. I doubt that these regs are buffered by FIFO

	// disable CP BM
	OUTREG( regs, RADEON_CP_CSQ_CNTL, RADEON_CSQ_PRIDIS_INDDIS );	
	// disable bus mastering	
	OUTREGP( regs, RADEON_BUS_CNTL, RADEON_BUS_MASTER_DIS, ~RADEON_BUS_MASTER_DIS );
	// disable PCI GART 
	OUTREGP( regs, RADEON_AIC_CNTL, 0, ~RADEON_PCIGART_TRANSLATE_EN );
	
	destroyGART( &di->DMABuffer );
	destroyDMABuffer( &di->DMABuffer );
}
