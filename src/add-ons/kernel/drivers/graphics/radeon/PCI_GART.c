/*
	Copyright (c) 2002, Thomas Kurschel

	Part of Radeon kernel driver

	PCI GART.

	Currently, we use PCI DMA. Changing to AGP would
	only affect this file, but AGP-GART is specific to
	the chipset of the motherboard, and as DMA is really
	overkill for 2D, I cannot bother writing a dozen
	of AGP drivers just to gain little extra speedup.
*/


#include "radeon_driver.h"
#include "mmio.h"
#include "buscntrl_regs.h"
#include "memcntrl_regs.h"
#include "cp_regs.h"

#include <image.h>

#include <stdlib.h>
#include <string.h>


#if 1
//! create actual GART buffer
static status_t
createGARTBuffer(GART_info *gart, size_t size)
{
	SHOW_FLOW0( 3, "" );

	gart->buffer.size = size = (size + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);

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
	gart->buffer.area = create_area("Radeon PCI GART buffer",
		&gart->buffer.ptr, B_ANY_KERNEL_ADDRESS,
		size, B_FULL_LOCK,
		// TODO: really user read/write?
		B_READ_AREA | B_WRITE_AREA | B_CLONEABLE_AREA);
	if (gart->buffer.area < 0) {
		SHOW_ERROR(1, "cannot create PCI GART buffer (%s)",
			strerror(gart->buffer.area));
		return gart->buffer.area;
	}

	gart->buffer.unaligned_area = -1;

	memset( gart->buffer.ptr, 0, size );

	return B_OK;
}

#else

static status_t createGARTBuffer( GART_info *gart, size_t size )
{
	physical_entry map[1];
	void *unaligned_addr, *aligned_phys;

	SHOW_FLOW0( 3, "" );

	gart->buffer.size = size = (size + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);

	// we allocate an contiguous area having twice the size
	// to be able to find an aligned, contiguous range within it;
	// the graphics card doesn't care, but the CPU cannot
	// make an arbitrary area WC'ed, at least elder ones
	// question: is this necessary for a PCI GART because of bus snooping?
	gart->buffer.unaligned_area = create_area( "Radeon PCI GART buffer",
		&unaligned_addr, B_ANY_KERNEL_ADDRESS,
		2 * size, B_CONTIGUOUS/*B_FULL_LOCK*/, B_READ_AREA | B_WRITE_AREA | B_CLONEABLE_AREA );
		// TODO: Physical aligning can be done without waste using the
		// private create_area_etc().
	if (gart->buffer.unaligned_area < 0) {
		SHOW_ERROR( 1, "cannot create PCI GART buffer (%s)",
			strerror( gart->buffer.unaligned_area ));
		return gart->buffer.unaligned_area;
	}

	get_memory_map( unaligned_addr, B_PAGE_SIZE, map, 1 );

	aligned_phys =
		(void **)((map[0].address + size - 1) & ~(size - 1));

	SHOW_FLOW( 3, "aligned_phys=%p", aligned_phys );

	gart->buffer.area = map_physical_memory( "Radeon aligned PCI GART buffer",
		(addr_t)aligned_phys,
		size, B_ANY_KERNEL_BLOCK_ADDRESS | B_MTR_WC,
		B_READ_AREA | B_WRITE_AREA, &gart->buffer.ptr );

	if( gart->buffer.area < 0 ) {
		SHOW_ERROR0( 3, "cannot map buffer with WC" );
		gart->buffer.area = map_physical_memory( "Radeon aligned PCI GART buffer",
			(addr_t)aligned_phys,
			size, B_ANY_KERNEL_BLOCK_ADDRESS,
			B_READ_AREA | B_WRITE_AREA, &gart->buffer.ptr );
	}

	if( gart->buffer.area < 0 ) {
		SHOW_ERROR0( 1, "cannot map GART buffer" );
		delete_area( gart->buffer.unaligned_area );
		gart->buffer.unaligned_area = -1;
		return gart->buffer.area;
	}

	memset( gart->buffer.ptr, 0, size );

	return B_OK;
}

#endif

// init GATT (could be used for both PCI and AGP)
static status_t initGATT( GART_info *gart )
{
	area_id map_area;
	uint32 map_area_size;
	physical_entry *map;
	physical_entry PTB_map[1];
	size_t map_count;
	uint32 i;
	uint32 *gatt_entry;
	size_t num_pages;

	SHOW_FLOW0( 3, "" );

	num_pages = (gart->buffer.size + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);

	// GART must be contiguous
	gart->GATT.area = create_area("Radeon GATT", (void **)&gart->GATT.ptr,
		B_ANY_KERNEL_ADDRESS,
		(num_pages * sizeof( uint32 ) + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1),
		B_32_BIT_CONTIGUOUS,
			// TODO: Physical address is cast to 32 bit below! Use B_CONTIGUOUS,
			// when that is (/can be) fixed!
		// TODO: really user read/write?
		B_READ_AREA | B_WRITE_AREA | B_CLONEABLE_AREA);

	if (gart->GATT.area < 0) {
		SHOW_ERROR(1, "cannot create GATT table (%s)",
			strerror(gart->GATT.area));
		return gart->GATT.area;
	}

	get_memory_map(gart->GATT.ptr, B_PAGE_SIZE, PTB_map, 1);
	gart->GATT.phys = PTB_map[0].address;

	SHOW_INFO(3, "GATT_ptr=%p, GATT_phys=%p", gart->GATT.ptr,
		(void *)gart->GATT.phys);

	// get address mapping
	memset(gart->GATT.ptr, 0, num_pages * sizeof(uint32));

	map_count = num_pages + 1;

	// align size to B_PAGE_SIZE
	map_area_size = map_count * sizeof(physical_entry);
	if ((map_area_size / B_PAGE_SIZE) * B_PAGE_SIZE != map_area_size)
		map_area_size = ((map_area_size / B_PAGE_SIZE) + 1) * B_PAGE_SIZE;

	// temporary area where we fill in the memory map (deleted below)
	map_area = create_area("pci_gart_map_area", (void **)&map, B_ANY_ADDRESS,
		map_area_size, B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA);
		// TODO: We actually have a working malloc() in the kernel. Why create
		// an area?
	dprintf("pci_gart_map_area: %" B_PRId32 "\n", map_area);

	get_memory_map( gart->buffer.ptr, gart->buffer.size, map, map_count );

	// the following looks a bit strange as the kernel
	// combines successive entries
	gatt_entry = gart->GATT.ptr;

	for( i = 0; i < map_count; ++i ) {
		phys_addr_t addr = map[i].address;
		size_t size = map[i].size;

		if( size == 0 )
			break;

		while( size > 0 ) {
			*gatt_entry++ = addr;
			//SHOW_FLOW( 3, "%lx", *(gart_entry-1) );
			addr += ATI_PCIGART_PAGE_SIZE;
			size -= ATI_PCIGART_PAGE_SIZE;
		}
	}

	delete_area(map_area);

	if( i == map_count ) {
		// this case should never happen
		SHOW_ERROR0( 0, "memory map of GART buffer too large!" );
		delete_area( gart->GATT.area );
		gart->GATT.area = -1;
		return B_ERROR;
	}

	// this might be a bit more than needed, as
	// 1. Intel CPUs have "processor order", i.e. writes appear to external
	//    devices in program order, so a simple final write should be sufficient
	// 2. if it is a PCI GART, bus snooping should provide cache coherence
	// 3. this function is a no-op :(
	clear_caches( gart->GATT.ptr, num_pages * sizeof( uint32 ),
		B_FLUSH_DCACHE );

	// back to real live - some chipsets have write buffers that
	// proove all previous assumptions wrong
	// (don't know whether this really helps though)
	#if defined(__i386__)
	asm volatile ( "wbinvd" ::: "memory" );
	#elif defined(__POWERPC__)
	// TODO : icbi on PowerPC to flush instruction cache?
	#endif
	return B_OK;
}

// destroy GART buffer
static void destroyGARTBuffer( GART_info *gart )
{
	if( gart->buffer.area > 0 )
		delete_area( gart->buffer.area );

	if( gart->buffer.unaligned_area > 0 )
		delete_area( gart->buffer.unaligned_area );

	gart->buffer.area = gart->buffer.unaligned_area = -1;
}


// destroy GATT
static void destroyGATT( GART_info *gart )
{
	if( gart->GATT.area > 0 )
		delete_area( gart->GATT.area );

	gart->GATT.area = -1;
}


// init PCI GART
status_t Radeon_InitPCIGART( device_info *di )
{
	status_t result;

	result = createGARTBuffer( &di->pci_gart, PCI_GART_SIZE );
	if( result < 0 )
		goto err1;

	result = initGATT( &di->pci_gart );
	if( result < 0 )
		goto err2;

	return B_OK;

err2:
	destroyGARTBuffer( &di->pci_gart );

err1:
	return result;
}


// cleanup PCI GART
void Radeon_CleanupPCIGART( device_info *di )
{
	vuint8 *regs = di->regs;

	SHOW_FLOW0( 3, "" );

	// perhaps we should wait for FIFO space before messing around with registers, but
	// 1. I don't want to add all the sync stuff to the kernel driver
	// 2. I doubt that these regs are buffered by FIFO
	// but still: in worst case CP has written some commands to register FIFO,
	// which can do any kind of nasty things

	// disable CP BM
	OUTREG( regs, RADEON_CP_CSQ_CNTL, RADEON_CSQ_PRIDIS_INDDIS );
	// read-back for flushing
	INREG( regs, RADEON_CP_CSQ_CNTL );

	// disable bus mastering
	OUTREGP( regs, RADEON_BUS_CNTL, RADEON_BUS_MASTER_DIS, ~RADEON_BUS_MASTER_DIS );
	// disable PCI GART
	OUTREGP( regs, RADEON_AIC_CNTL, 0, ~RADEON_PCIGART_TRANSLATE_EN );

	destroyGATT( &di->pci_gart );
	destroyGARTBuffer( &di->pci_gart );
}
