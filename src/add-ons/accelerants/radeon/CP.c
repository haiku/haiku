/*
	Copyright (c) 2002, Thomas Kurschel
	

	Part of Radeon accelerant
		
	Command Processor handling
*/

#include "radeon_accelerant.h"
#include "mmio.h"
#include "CPMicroCode.h"
#include "cp_regs.h"
#include "buscntrl_regs.h"
#include "utils.h"
#include <sys/ioctl.h>

#include "log_coll.h"
#include "log_enum.h"

#include <string.h>

uint getAvailRingBuffer( accelerator_info *ai );


// non-local memory is used as following:
// - 0x10000 dwords for ring buffer
// - 8 dwords for returned data (i.e. current read ptr)
// - 6 dwords for "scratch registers"
//
// usage of scratch registers:
// - reg 0 = reached engine.count
//
// the ring buffer stuff must be at a constant offset as
// clones cannot be informed if it were changed


// upload Micro-Code of CP
static void loadMicroEngineRAMData( accelerator_info *ai )
{
	int i;
	const uint32 (*microcode)[2];
	
	SHOW_FLOW0( 3, "" );
	
	switch( ai->si->asic ) {
	case rt_r300:
	case rt_r300_4p:
	case rt_rv350:
	case rt_rv360:
	case rt_r350:
	case rt_r360:
		microcode = r300_cp_microcode;
		break;
	case rt_r200:
	//case rt_rv250:
	//case rt_m9:
		microcode = r200_cp_microcode;
		break;
	default:
		microcode = radeon_cp_microcode;
	}

	Radeon_WaitForIdle( ai );

	OUTREG( ai->regs, RADEON_CP_ME_RAM_ADDR, 0 );
	
	for ( i = 0 ; i < 256 ; i++ ) {
		OUTREG( ai->regs, RADEON_CP_ME_RAM_DATAH, microcode[i][1] );
		OUTREG( ai->regs, RADEON_CP_ME_RAM_DATAL, microcode[i][0] );
	}
}

// convert CPU's to graphics card's virtual address
#define CPU2GC( addr ) (((uint32)(addr) - (uint32)si->nonlocal_mem) + si->nonlocal_vm_start)

// initialize bus mastering
static status_t setupCPRegisters( accelerator_info *ai, int aring_size )
{
	vuint8 *regs = ai->regs;
	shared_info *si = ai->si;
	uint32 tmp;
	
#if 0
	{
		// allocate ring buffer etc. from local memory instead of PCI memory
		radeon_alloc_local_mem am;
	
		am.magic = RADEON_PRIVATE_DATA_MAGIC;
		am.size = (aring_size + 14) * 4;
		
		if( ioctl( ai->fd, RADEON_ALLOC_LOCAL_MEM, &am ) != B_OK )
			SHOW_ERROR0( 0, "Cannot allocate ring buffer from local memory" );
		else {
			si->nonlocal_vm_start = am.fb_offset;
			si->nonlocal_mem = (uint32 *)(si->framebuffer + am.fb_offset);
		}
	}
#endif
	
	memset( &si->ring, 0, sizeof( si->ring ));
	
	// set write pointer delay to zero;
	// we assume that memory synchronization is done correctly my MoBo
	// and Radeon_SendCP contains a hack that hopefully fixes such problems
	OUTREG( regs, RADEON_CP_RB_WPTR_DELAY, 0 );

	// setup CP buffer
	si->ring.start = si->nonlocal_mem;
	si->ring.size = aring_size;
	OUTREG( regs, RADEON_CP_RB_BASE, CPU2GC( si->ring.start ));
	SHOW_INFO( 3, "CP buffer address=%lx", CPU2GC( si->ring.start ));

	// setup CP read pointer buffer
	si->ring.head = si->ring.start + si->ring.size;
	OUTREG( regs, RADEON_CP_RB_RPTR_ADDR, CPU2GC( si->ring.head ));
	SHOW_INFO( 3, "CP read pointer buffer==%lx", CPU2GC( si->ring.head ));
	
	// set ring buffer size
	// (it's log2 of qwords)
	OUTREG( regs, RADEON_CP_RB_CNTL, log2( si->ring.size / 2 ));
	SHOW_INFO( 3, "CP buffer size mask=%ld", log2( si->ring.size / 2 ) );

	// set CP buffer pointers
	OUTREG( regs, RADEON_CP_RB_RPTR, 0 );
	OUTREG( regs, RADEON_CP_RB_WPTR, 0 );
	*si->ring.head = 0;
	si->ring.tail = 0;

	// setup scratch register buffer
	si->scratch_ptr = si->ring.head + RADEON_SCRATCH_REG_OFFSET / sizeof( uint32 );
	OUTREG( regs, RADEON_SCRATCH_ADDR, CPU2GC( si->scratch_ptr ));
	OUTREG( regs, RADEON_SCRATCH_UMSK, 0x3f );

	Radeon_WaitForIdle( ai );

	// enable bus mastering
#if 1
	tmp = INREG( ai->regs, RADEON_BUS_CNTL ) & ~RADEON_BUS_MASTER_DIS;
	OUTREG( regs, RADEON_BUS_CNTL, tmp );
#endif

	// sync units
	OUTREG( regs, RADEON_ISYNC_CNTL,
		(RADEON_ISYNC_ANY2D_IDLE3D |
		 RADEON_ISYNC_ANY3D_IDLE2D |
		 RADEON_ISYNC_WAIT_IDLEGUI |
		 RADEON_ISYNC_CPSCRATCH_IDLEGUI) );

	return B_OK;
}


// get number of free entries in CP's ring buffer
uint getAvailRingBuffer( accelerator_info *ai )
{
	shared_info *si = ai->si;
	int space;
	
//	space = *si->ring.head - si->ring.tail;
	space = INREG( ai->regs, RADEON_CP_RB_RPTR ) - si->ring.tail;

	if( space <= 0 )
		space += si->ring.size;
		
	// don't fill up the entire buffer as we cannot
	// distinguish between a full and an empty ring
	--space;
	
	SHOW_FLOW( 4, "head=%ld, tail=%ld, space=%ld", *si->ring.head, si->ring.tail, space );

	LOG1( si->log, _GetAvailRingBufferQueue, space );
		
	return space;
}

// initialize CP so it's ready for BM
status_t Radeon_InitCP( accelerator_info *ai )
{	
//	shared_info *si = ai->si;
	status_t result;
	
	SHOW_FLOW0( 3, "" );
	
	// init raw CP
	loadMicroEngineRAMData( ai );

	// do soft-reset
	Radeon_ResetEngine( ai );
	
	// after warm-reset, the CP may still be active and thus react to
	// register writes during initialization unpredictably, so we better
	// stop it first
	OUTREG( ai->regs, RADEON_CP_CSQ_CNTL, RADEON_CSQ_PRIDIS_INDDIS );
	INREG( ai->regs, RADEON_CP_CSQ_CNTL );
	
	// reset CP to make disabling active
	Radeon_ResetEngine( ai );

	// setup CP memory ranges
	result = setupCPRegisters( ai, 0x10000 );
	if( result < 0 )
		return result;

	// tell CP to use BM
	Radeon_WaitForIdle( ai );
	OUTREG( ai->regs, RADEON_CP_CSQ_CNTL, RADEON_CSQ_PRIBM_INDBM );

	// this may be a bit too much	
	Radeon_SendPurgeCache( ai );
	Radeon_SendWaitUntilIdle( ai );
	
	return B_OK;
}


// write to register via CP
void Radeon_WriteRegCP( accelerator_info *ai, uint32 reg, uint32 value )
{
	uint32 buffer[2];
	
	SHOW_FLOW0( 4, "" );
	
	LOG2( ai->si->log, _Radeon_WriteRegFifo, reg, value );
	
	buffer[0] = CP_PACKET0( reg, 0 );
	buffer[1] = value;
	
	Radeon_SendCP( ai, buffer, 2 );
}


// send packets to CP
void Radeon_SendCP( accelerator_info *ai, uint32 *buffer, uint32 num_dwords )
{
	shared_info *si = ai->si;
	
	SHOW_FLOW( 4, "num_dwords=%d", num_dwords );

	while( num_dwords > 0 ) {
		uint32 space;
		uint32 max_copy;
//		uint i;
		
		space = getAvailRingBuffer( ai );
		
		if( space == 0 )
			continue;
			
		max_copy = min( space, num_dwords );

#ifdef ENABLE_LOGGING		
		for( i = 0; i < max_copy; ++i )
			LOG1( si->log, _Radeon_SendCP, buffer[i] );
#endif
					
		if( si->ring.tail + max_copy >= si->ring.size ) {
			uint32 sub_len;
			
			sub_len = si->ring.size - si->ring.tail;
			memcpy( si->ring.start + si->ring.tail, buffer, sub_len * sizeof( uint32 ));
			buffer += sub_len;
			num_dwords -= sub_len;
			max_copy -= sub_len;
			si->ring.tail = 0;
		}
		
		memcpy( si->ring.start + si->ring.tail, buffer, max_copy * sizeof( uint32 ) );
		buffer += max_copy;
		num_dwords -= max_copy;
		if( si->ring.tail + max_copy < si->ring.size )
			si->ring.tail += max_copy;
		else
			si->ring.tail = 0;
	}
	
	// some chipsets have problems with write buffers; effectively, the command
	// list we've just created gets delayed in some queue and the graphics chip
	// reads out-dated commands, which don't make sense and thus crash the
	// graphics card
	
	// flush writes to ring
	// (this code is a bit of a overkill - currently, only some WinChip/Cyrix 
	//  CPU's support out-of-order writes, but we are prepared)
	__asm__ __volatile__ ("lock; addl $0,0(%%esp)": : :"memory");
	// make sure the chipset has flushed its write buffer by
	// reading some uncached memory
	(void)*si->ring.head;

	// now, the command list should really be written to memory,
	// so it's safe to instruct the graphics card to read it
	OUTREG( ai->regs, RADEON_CP_RB_WPTR, si->ring.tail );

	// read from PCI bus to ensure correct posting
	INREG( ai->regs, RADEON_CP_RB_RPTR );
}
