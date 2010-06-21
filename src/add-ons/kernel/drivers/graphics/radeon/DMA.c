/*
	Copyright (c) 2002-04, Thomas Kurschel


	Part of Radeon accelerant

	DMA engine handling.

	Currently, VID DMA is always used and data is always copied from
	graphics memory to other memory.
*/

#include "radeon_driver.h"
#include "mmio.h"
#include "rbbm_regs.h"
#include "dma_regs.h"
#include <string.h>

// this is arbitrary and hopefully sufficiently high
#define RADEON_MAX_DMA_SIZE			16*1024*1024


// initialize DMA engine
status_t Radeon_InitDMA( device_info *di )
{
	status_t res;

	// allocate descriptor table in graphics mem
	// (docu says that is _must_ be in graphics mem)
	di->dma_desc_max_num = RADEON_MAX_DMA_SIZE / 4096;

	res = mem_alloc( di->memmgr[mt_local], di->dma_desc_max_num * sizeof( DMA_descriptor ), 0,
		&di->dma_desc_handle, &di->dma_desc_offset );

	if( res != B_OK )
		return res;

	// allow DMA IRQ
	OUTREGP( di->regs, RADEON_GEN_INT_CNTL, RADEON_VIDDMA_MASK, ~RADEON_VIDDMA_MASK );
	// acknowledge possibly pending IRQ
	OUTREG( di->regs, RADEON_GEN_INT_STATUS, RADEON_VIDDMA_AK );

	return B_OK;
}


// prepare DMA engine to copy data from graphics mem to other mem
static status_t Radeon_PrepareDMA(
	device_info *di, uint32 src, char *target, size_t size, bool lock_mem, bool contiguous )
{
	physical_entry map[16];
	status_t res;
	DMA_descriptor *cur_desc;
	int num_desc;

	if( lock_mem && !contiguous ) {
		res = lock_memory( target, size, B_DMA_IO | B_READ_DEVICE );

		if( res != B_OK ) {
			SHOW_ERROR( 2, "Cannot lock memory (%s)", strerror( res ));
			return res;
		}
	}

	// adjust virtual address for graphics card
	src += di->si->memory[mt_local].virtual_addr_start;

	cur_desc = (DMA_descriptor *)(di->si->local_mem + di->dma_desc_offset);
	num_desc = 0;

	// memory may be fragmented, so we create S/G list
	while( size > 0 ) {
		int i;

		if( contiguous ) {
			// if memory is contiguous, ask for start address only to reduce work
			get_memory_map( target, 1, map, 16 );
			// replace received size with total size
			map[0].size = size;
		} else {
			get_memory_map( target, size, map, 16 );
		}

		for( i = 0; i < 16; ++i ) {
			phys_addr_t address = map[i].address;
			size_t contig_size = map[i].size;

			if( contig_size == 0 )
				break;

#if B_HAIKU_PHYSICAL_BITS > 32
			if (address + contig_size > (phys_addr_t)1 << 32) {
				SHOW_ERROR(2, "Physical address > 4 GB: %#" B_PRIxPHYSADDR
					"size: %#" B_PRIxSIZE, address, size);
				res = B_BAD_VALUE;
				goto err;
			}
#endif

			target += contig_size;

			while( contig_size > 0 ) {
				size_t cur_size;

				cur_size = min( contig_size, RADEON_DMA_DESC_MAX_SIZE );

				if( ++num_desc > (int)di->dma_desc_max_num ) {
					SHOW_ERROR( 2, "Overflow of DMA descriptors, %ld bytes left", size );
					res = B_BAD_VALUE;
					goto err;
				}

				cur_desc->src_address = src;
				cur_desc->dest_address = address;
				cur_desc->command = cur_size;
				cur_desc->res = 0;

				++cur_desc;
				address += cur_size;
				contig_size -= cur_size;
				src += cur_size;
				size -= cur_size;
			}
		}
	}

	// mark last descriptor as being last one
	(cur_desc - 1)->command |= RADEON_DMA_COMMAND_EOL;

	return B_OK;

err:
	if( lock_mem && !contiguous )
		unlock_memory( target, size, B_DMA_IO| B_READ_DEVICE );

	return res;
}


// finish DMA
// caller must ensure that DMA channel has stopped
static void Radeon_FinishDMA(
	device_info *di, uint32 src, char *target, size_t size, bool lock_mem, bool contiguous )
{
	if( lock_mem && !contiguous )
		unlock_memory( target, size, B_DMA_IO| B_READ_DEVICE );
}


// copy from graphics memory to other memory via DMA
// 	src		- offset in graphics mem
//	target	- target address
//	size	- number of bytes to copy
//	lock_mem - true, if memory is not locked
//	contiguous - true, if memory is physically contiguous (implies lock_mem=false)
status_t Radeon_DMACopy(
	device_info *di, uint32 src, char *target, size_t size, bool lock_mem, bool contiguous )
{
	status_t res;

	/*SHOW_FLOW( 0, "src=%ld, target=%p, size=%ld, lock_mem=%d, contiguous=%d",
		src, target, size, lock_mem, contiguous );*/

	res =  Radeon_PrepareDMA( di, src, target, size, lock_mem, contiguous );
	if( res != B_OK )
		return res;

	//SHOW_FLOW0( 0, "2" );

	OUTREG( di->regs, RADEON_DMA_VID_TABLE_ADDR, di->si->memory[mt_local].virtual_addr_start +
		di->dma_desc_offset );

	res = acquire_sem_etc( di->dma_sem, 1, B_RELATIVE_TIMEOUT, 1000000 );

	// be sure that transmission is really finished
	while( (INREG( di->regs, RADEON_DMA_VID_STATUS ) & RADEON_DMA_STATUS_ACTIVE) != 0 ) {
		SHOW_FLOW0( 0, "DMA transmission still active" );
		snooze( 1000 );
	}

	Radeon_FinishDMA( di, src, target, size, lock_mem, contiguous );

	//SHOW_FLOW0( 0, "3" );

	return res;
}
