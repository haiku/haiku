/*
	Copyright (c) 2002, Thomas Kurschel
	

	Part of Radeon accelerant
		
	CP initialization/sync/cleanup.
	
	It also handles command buffer synchronization.

	non-local memory is used as following:
	- 2048 dwords for ring buffer
	- 253 indirect buffers a 4k (1024 dwords)
	- 8 dwords for returned data (i.e. current read ptr)
	  & 6 dwords for "scratch registers"

	usage of scratch registers:
	- reg 0 = reached engine.count

	with a granularity of 4 KByte, we need 2+253+1=256 blocks, which is exactly 1 MB
*/

#include "radeon_driver.h"
#include "CPMicroCode.h"
#include "mmio.h"
#include "cp_regs.h"
#include "pll_regs.h"
#include "rbbm_regs.h"
#include "buscntrl_regs.h"
#include "utils.h"
#include "pll_access.h"

#include "log_coll.h"
#include "log_enum.h"

#include <string.h>

#if 0

// macros for user-space

#define ALLOC_MEM( asize, mem_type, aglobal, handle, offset ) \
	{ \
		radeon_alloc_mem am; \
\
		am.magic = RADEON_PRIVATE_DATA_MAGIC; \
		am.size = (asize) * 4; \
		am.memory_type = (mt_nonlocal); \
		am.global = (aglobal); \
\
		res = ioctl( ai->fd, RADEON_ALLOC_MEM, &am ); \
		if( res == B_OK ) \
			*(handle) = am.handle; \
			*(offset) = am.offset; \
	}

#define MEM2CPU( mem ) \
	((uint32 *)(ai->mapped_memory[(mem).memory_type].data + (mem).offset))

#define MEM2GC( mem ) ((mem).offset + si->memory[(mem).memory_type].virtual_addr_start)

#define FREE_MEM( mem_type, handle ) \
	{ \
		radeon_free_mem fm; \
\
		fm.magic = RADEON_PRIVATE_DATA_MAGIC; \
		fm.memory_type = mem_type; \
		fm.handle = offset; \
\
		ioctl( ai->fd, RADEON_FREE_MEM, &fm ); \
	}
	
#else

// macros for kernel-space

// allocate memory
// if memory_type is non-local, it is replaced with default non-local type
#define ALLOC_MEM( asize, mem_type, aglobal, handle, offset ) \
	if( mem_type == mt_nonlocal ) \
		mem_type = di->si->nonlocal_type; \
	res = mem_alloc( di->memmgr[mem_type], asize, NULL, handle, offset );

// get address as seen by program to access allocated memory
// (memory_type must _not_ be non-local, see ALLOC_MEM)
#define MEM2CPU( memory_type, offset ) \
	((uint8 *)(memory_type == mt_local ? di->si->local_mem : \
	(memory_type == mt_PCI ? di->pci_gart.buffer.ptr : di->agp_gart.buffer.ptr)) \
	+ (offset))

// get graphics card's virtual address of allocated memory
// (memory_type must _not_ be non-local, see ALLOC_MEM)
#define MEM2GC( memory_type, offset ) \
	(di->si->memory[(memory_type)].virtual_addr_start + (offset))

// free memory
// if memory_type is non-local, it is replaced with default non-local type
#define FREE_MEM( mem_type, handle ) \
	mem_free( \
		di->memmgr[ mem_type == mt_nonlocal ? di->si->nonlocal_type : mem_type], \
		handle, NULL );
	
#endif


void Radeon_DiscardAllIndirectBuffers( device_info *di );

#define RADEON_SCRATCH_REG_OFFSET	32


void Radeon_FlushPixelCache( device_info *di );

// wait until engine is idle;
// acquire_lock - 	true, if lock must be hold
//					false, if lock is already acquired
// keep_lock -		true, keep lock on exit (only valid if acquire_lock is true)
void Radeon_WaitForIdle( device_info *di, bool acquire_lock, bool keep_lock )
{
	if( acquire_lock )
		ACQUIRE_BEN( di->si->cp.lock );
	
	Radeon_WaitForFifo( di, 64 );
	
	while( 1 ) {
		bigtime_t start_time = system_time();
	
		do {
			if( (INREG( di->regs, RADEON_RBBM_STATUS ) & RADEON_RBBM_ACTIVE) == 0 ) {
				Radeon_FlushPixelCache( di );
				
				if( acquire_lock && !keep_lock)
					RELEASE_BEN( di->si->cp.lock );
					
				return;
			}
			
			snooze( 1 );
		} while( system_time() - start_time < 1000000 );
		
		SHOW_ERROR( 3, "Engine didn't become idle (rbbm_status=%lx, cp_stat=%lx, tlb_address=%lx, tlb_data=%lx)",
			INREG( di->regs, RADEON_RBBM_STATUS ),
			INREG( di->regs, RADEON_CP_STAT ),
			INREG( di->regs, RADEON_AIC_TLB_ADDR ),
			INREG( di->regs, RADEON_AIC_TLB_DATA ));
		
		LOG( di->si->log, _Radeon_WaitForIdle );

		Radeon_ResetEngine( di );
	}
}


// wait until "entries" FIFO entries are empty
// lock must be hold
void Radeon_WaitForFifo( device_info *di, int entries )
{
	while( 1 ) {
		bigtime_t start_time = system_time();
	
		do {
			int slots = INREG( di->regs, RADEON_RBBM_STATUS ) & RADEON_RBBM_FIFOCNT_MASK;
			
			if ( slots >= entries ) 
				return;

			snooze( 1 );
		} while( system_time() - start_time < 1000000 );
		
		LOG( di->si->log, _Radeon_WaitForFifo );
		
		Radeon_ResetEngine( di );
	}
}

// flush pixel cache of graphics card
void Radeon_FlushPixelCache( device_info *di )
{
	bigtime_t start_time;
	
	OUTREGP( di->regs, RADEON_RB2D_DSTCACHE_CTLSTAT, RADEON_RB2D_DC_FLUSH_ALL,
		~RADEON_RB2D_DC_FLUSH_ALL );

	start_time = system_time();
	
	do {
		if( (INREG( di->regs, RADEON_RB2D_DSTCACHE_CTLSTAT ) 
			 & RADEON_RB2D_DC_BUSY) == 0 ) 
			return;

		snooze( 1 );
	} while( system_time() - start_time < 1000000 );
	
	LOG( di->si->log, _Radeon_FlushPixelCache );

	SHOW_ERROR0( 0, "pixel cache didn't become empty" );
}

// reset graphics card's engine
// lock must be hold
void Radeon_ResetEngine( device_info *di )
{
	vuint8 *regs = di->regs;
	shared_info *si = di->si;
	uint32 clock_cntl_index, mclk_cntl, rbbm_soft_reset, host_path_cntl;
	uint32 cur_read_ptr;
	
	SHOW_FLOW0( 3, "" );

	Radeon_FlushPixelCache( di );

	clock_cntl_index = INREG( regs, RADEON_CLOCK_CNTL_INDEX );
	R300_PLLFix( di->regs, di->asic );
	
	// OUCH!
	// XFree disables any kind of automatic power power management 
	// because of bugs of some ASIC revision (seems like the revisions
	// cannot be read out)
	// -> this is a very bad idea, especially when it comes to laptops
	// I comment it out for now, let's hope noone takes notice
    if( di->num_crtc > 1 ) {
		Radeon_OUTPLLP( regs, di->asic, RADEON_SCLK_CNTL, 
			RADEON_CP_MAX_DYN_STOP_LAT |
			RADEON_SCLK_FORCEON_MASK,
			~RADEON_DYN_STOP_LAT_MASK );
	
/*		if( ai->si->asic == rt_rv200 ) {
		    Radeon_OUTPLLP( ai, RADEON_SCLK_MORE_CNTL, 
		    	RADEON_SCLK_MORE_FORCEON, ~0 );
		}*/
    }

	mclk_cntl = Radeon_INPLL( regs, di->asic, RADEON_MCLK_CNTL );

	// enable clock of units to be reset
	Radeon_OUTPLL( regs, di->asic, RADEON_MCLK_CNTL, mclk_cntl |
      RADEON_FORCEON_MCLKA |
      RADEON_FORCEON_MCLKB |
      RADEON_FORCEON_YCLKA |
      RADEON_FORCEON_YCLKB |
      RADEON_FORCEON_MC |
      RADEON_FORCEON_AIC );

	// do the reset
    host_path_cntl = INREG( regs, RADEON_HOST_PATH_CNTL );
	rbbm_soft_reset = INREG( regs, RADEON_RBBM_SOFT_RESET );

	switch( di->asic ) {
	case rt_r300:
		OUTREG( regs, RADEON_RBBM_SOFT_RESET, (rbbm_soft_reset |
			RADEON_SOFT_RESET_CP |
			RADEON_SOFT_RESET_HI |
			RADEON_SOFT_RESET_E2 |
			RADEON_SOFT_RESET_AIC ));
		INREG( regs, RADEON_RBBM_SOFT_RESET);
		OUTREG( regs, RADEON_RBBM_SOFT_RESET, 0);
		// this bit has no description
		OUTREGP( regs, RADEON_RB2D_DSTCACHE_MODE, (1 << 17), ~0 );

		break;
	default:
		OUTREG( regs, RADEON_RBBM_SOFT_RESET, rbbm_soft_reset |
			RADEON_SOFT_RESET_CP |
			RADEON_SOFT_RESET_HI |
			RADEON_SOFT_RESET_SE |
			RADEON_SOFT_RESET_RE |
			RADEON_SOFT_RESET_PP |
			RADEON_SOFT_RESET_E2 |
			RADEON_SOFT_RESET_RB |
			RADEON_SOFT_RESET_AIC );
		INREG( regs, RADEON_RBBM_SOFT_RESET );
		OUTREG( regs, RADEON_RBBM_SOFT_RESET, rbbm_soft_reset &
			~( RADEON_SOFT_RESET_CP |
			RADEON_SOFT_RESET_HI |
			RADEON_SOFT_RESET_SE |
			RADEON_SOFT_RESET_RE |
			RADEON_SOFT_RESET_PP |
			RADEON_SOFT_RESET_E2 |
			RADEON_SOFT_RESET_RB |
			RADEON_SOFT_RESET_AIC ) );
		INREG( regs, RADEON_RBBM_SOFT_RESET );
	}

    OUTREG( regs, RADEON_HOST_PATH_CNTL, host_path_cntl | RADEON_HDP_SOFT_RESET );
    INREG( regs, RADEON_HOST_PATH_CNTL );
    OUTREG( regs, RADEON_HOST_PATH_CNTL, host_path_cntl );

	// restore regs
	OUTREG( regs, RADEON_RBBM_SOFT_RESET, rbbm_soft_reset);

	OUTREG( regs, RADEON_CLOCK_CNTL_INDEX, clock_cntl_index );
	R300_PLLFix( regs, di->asic );
	Radeon_OUTPLL( regs, di->asic, RADEON_MCLK_CNTL, mclk_cntl );
	
	// reset ring buffer
	cur_read_ptr = INREG( regs, RADEON_CP_RB_RPTR );
	OUTREG( regs, RADEON_CP_RB_WPTR, cur_read_ptr );
	
	//if( si->cp.ring.head ) {
	// during init, there are no feedback data
	if( si->cp.feedback.mem_handle != 0 ) {
		*(uint32 *)MEM2CPU( si->cp.feedback.mem_type, si->cp.feedback.head_mem_offset) = 
			cur_read_ptr;
		//	*si->cp.ring.head = cur_read_ptr;
		si->cp.ring.tail = cur_read_ptr;
	}

	++si->engine.count;
	
	// mark all buffers as being finished
	Radeon_DiscardAllIndirectBuffers( di );

	return;
}


// upload Micro-Code of CP
static void loadMicroEngineRAMData( device_info *di )
{
	int i;
	const uint32 (*microcode)[2];
	
	SHOW_FLOW0( 3, "" );
	
	switch( di->asic ) {
	case rt_r300:
	case rt_r300_4p:
	case rt_rv350:
	case rt_rv360:
	case rt_m11:
	case rt_r350:
	case rt_r360:
		microcode = r300_cp_microcode;
		break;
	case rt_r200:
	//case rt_rv250:
	//case rt_m9:
		microcode = r200_cp_microcode;
		break;
	case rt_rs100:
	default:
		microcode = radeon_cp_microcode;
	}

	Radeon_WaitForIdle( di, false, false );

/*	
	// HACK start
	Radeon_ResetEngine( di );
	OUTREG( di->regs, 0x30, 0x5133a3a0 );	// bus_cntl
	OUTREGP( di->regs, 0xf0c, 0xff00, ~0xff );	// latency
	Radeon_WaitForIdle( di, false, false );
	Radeon_ResetEngine( di );
	// HACK end
*/

	OUTREG( di->regs, RADEON_CP_ME_RAM_ADDR, 0 );
	
	for ( i = 0 ; i < 256 ; i++ ) {
		OUTREG( di->regs, RADEON_CP_ME_RAM_DATAH, microcode[i][1] );
		OUTREG( di->regs, RADEON_CP_ME_RAM_DATAL, microcode[i][0] );
	}
}

// aring_size - size of ring in dwords
static status_t initRingBuffer( device_info *di, int aring_size )
{
	status_t res;
	shared_info *si = di->si;
	CP_info *cp = &si->cp;
	vuint8 *regs = di->regs;
	int32 offset;
	memory_type_e memory_type;

	memset( &cp->ring, 0, sizeof( cp->ring ));

	// ring and indirect buffers can be either in AGP or PCI GART
	// (it seems that they cannot be in graphics memory, at least
	//  I had serious coherency problems when I tried that)
	memory_type = mt_nonlocal;
	
	ALLOC_MEM( aring_size * 4, memory_type, true, 
		&cp->ring.mem_handle, &offset );

	if( res != B_OK ) {
		SHOW_ERROR0( 0, "Cannot allocate ring buffer" );
		return res;
	}
	
	// setup CP buffer
	cp->ring.mem_type = memory_type;
	cp->ring.mem_offset = offset;
	cp->ring.vm_base = MEM2GC( memory_type, offset );
	cp->ring.size = aring_size;
	cp->ring.tail_mask = aring_size - 1;
	OUTREG( regs, RADEON_CP_RB_BASE, cp->ring.vm_base );
	SHOW_INFO( 3, "CP buffer address=%lx", cp->ring.vm_base );

	// set ring buffer size
	// (it's log2 of qwords)
	OUTREG( regs, RADEON_CP_RB_CNTL, log2( cp->ring.size / 2 ));
	SHOW_INFO( 3, "CP buffer size mask=%d", log2( cp->ring.size / 2 ) );

	// set write pointer delay to zero;
	// we assume that memory synchronization is done correctly my MoBo
	// and Radeon_SendCP contains a hack that hopefully fixes such problems
	OUTREG( regs, RADEON_CP_RB_WPTR_DELAY, 0 );
	
	memset( MEM2CPU( cp->ring.mem_type, cp->ring.mem_offset), 0, cp->ring.size * 4 );

	// set CP buffer pointers
	OUTREG( regs, RADEON_CP_RB_RPTR, 0 );
	OUTREG( regs, RADEON_CP_RB_WPTR, 0 );
	//*cp->ring.head = 0;
	cp->ring.tail = 0;

	return B_OK;
}

static void uninitRingBuffer( device_info *di )
{
	vuint8 *regs = di->regs;
	
	// abort any activity
	Radeon_ResetEngine( di );
	
	// disable CP BM
	OUTREG( regs, RADEON_CP_CSQ_CNTL, RADEON_CSQ_PRIDIS_INDDIS );	
	// read-back for flushing
	INREG( regs, RADEON_CP_CSQ_CNTL );
	
	FREE_MEM( mt_nonlocal, di->si->cp.ring.mem_handle );
}

static status_t initCPFeedback( device_info *di )
{
	CP_info *cp = &di->si->cp;
	vuint8 *regs = di->regs;
	int32 offset;
	memory_type_e memory_type;
	status_t res;

	// status information should be in PCI memory, so CPU can
	// poll it without locking the bus (PCI memory is the only
	// cachable memory available)
	memory_type = mt_PCI;
	
	ALLOC_MEM( RADEON_SCRATCH_REG_OFFSET + 0x40, memory_type, true, 
		&cp->feedback.mem_handle, &offset );

	if( res != B_OK ) {
		SHOW_ERROR0( 0, "Cannot allocate buffers for status information" );
		return res;
	}
	
	// setup CP read pointer buffer
	cp->feedback.mem_type = memory_type;
	cp->feedback.head_mem_offset = offset;
	cp->feedback.head_vm_address = MEM2GC( memory_type, cp->feedback.head_mem_offset );
	OUTREG( regs, RADEON_CP_RB_RPTR_ADDR, cp->feedback.head_vm_address );
	SHOW_INFO( 3, "CP read pointer buffer==%lx", cp->feedback.head_vm_address );

	// setup scratch register buffer
	cp->feedback.scratch_mem_offset = offset + RADEON_SCRATCH_REG_OFFSET;
	cp->feedback.scratch_vm_start = MEM2GC( memory_type, cp->feedback.scratch_mem_offset );
	OUTREG( regs, RADEON_SCRATCH_ADDR, cp->feedback.scratch_vm_start );
	OUTREG( regs, RADEON_SCRATCH_UMSK, 0x3f );
	
	*(uint32 *)MEM2CPU( cp->feedback.mem_type, cp->feedback.head_mem_offset) = 0;
	memset( MEM2CPU( cp->feedback.mem_type, cp->feedback.scratch_mem_offset), 0, 0x40 );
	//*cp->ring.head = 0;

	return B_OK;
}

static void uninitCPFeedback( device_info *di )
{
	vuint8 *regs = di->regs;
	
	// don't allow any scratch buffer update
	OUTREG( regs, RADEON_SCRATCH_UMSK, 0x0 );
	
	FREE_MEM( mt_PCI, di->si->cp.feedback.mem_handle );
}

static status_t initIndirectBuffers( device_info *di )
{
	CP_info *cp = &di->si->cp;
	int32 offset;
	memory_type_e memory_type;
	int i;
	status_t res;
	
	memory_type = mt_nonlocal;
	
	ALLOC_MEM( NUM_INDIRECT_BUFFERS * INDIRECT_BUFFER_SIZE * 4, memory_type, 
		true, &cp->buffers.mem_handle, &offset );

	if( res != B_OK ) {
		SHOW_ERROR0( 0, "Cannot allocate indirect buffers" );
		return B_ERROR;
	}

	cp->buffers.mem_type = memory_type;
	cp->buffers.mem_offset = offset;
	cp->buffers.vm_start = MEM2GC( memory_type, cp->buffers.mem_offset );
	
	for( i = 0; i < NUM_INDIRECT_BUFFERS - 1; ++i ) {
		cp->buffers.buffers[i].next = i + 1;
	}
	
	cp->buffers.buffers[i].next = -1;
	
	cp->buffers.free_list = 0;
	cp->buffers.oldest = -1;
	cp->buffers.newest = -1;
	cp->buffers.active_state = -1;
	cp->buffers.cur_tag = 0;
	
	memset( MEM2CPU( cp->buffers.mem_type, cp->buffers.mem_offset), 0, 
		NUM_INDIRECT_BUFFERS * INDIRECT_BUFFER_SIZE * 4 );
	
	return B_OK;
}

static void uninitIndirectBuffers( device_info *di )
{
	FREE_MEM( mt_nonlocal, di->si->cp.buffers.mem_handle );
}

// initialize CP so it's ready for BM
status_t Radeon_InitCP( device_info *di )
{	
	thread_id thid;
    thread_info thinfo;
	status_t res;
	
	SHOW_FLOW0( 3, "" );
	
	// this is _really_ necessary so functions like ResetEngine() know
	// that the CP is not set up yet
	memset( &di->si->cp, 0, sizeof( di->si->cp ));
	
	if( (res = INIT_BEN( di->si->cp.lock, "Radeon CP" )) < 0 )
		return res;
	
	// HACK: change owner of benaphore semaphore to team of calling thread;
	// reason: user code cannot acquire kernel semaphores, but the accelerant
	// is in user space; interestingly, it's enough to change the semaphore's
	// owner to _any_ non-system team (that's the only security check done by
	// the kernel)
	thid = find_thread( NULL );
    get_thread_info( thid, &thinfo );
    set_sem_owner( di->si->cp.lock.sem, thinfo.team );
	
	// init raw CP
	loadMicroEngineRAMData( di );

	// do soft-reset
	Radeon_ResetEngine( di );
	
	// after warm-reset, the CP may still be active and thus react to
	// register writes during initialization unpredictably, so we better
	// stop it first
	OUTREG( di->regs, RADEON_CP_CSQ_CNTL, RADEON_CSQ_PRIDIS_INDDIS );
	INREG( di->regs, RADEON_CP_CSQ_CNTL );

	// reset CP to make disabling active
	Radeon_ResetEngine( di );

	res = initRingBuffer( di, CP_RING_SIZE );
	if( res < 0 )
		goto err4;
	
	res = initCPFeedback( di );
	if( res < 0 )
		goto err3;
		
	res = initIndirectBuffers( di );
	if( res < 0 )
		goto err2;
		
	// tell CP to use BM
	Radeon_WaitForIdle( di, false, false );
	
	// enable direct and indirect CP bus mastering
	OUTREG( di->regs, RADEON_CP_CSQ_CNTL, RADEON_CSQ_PRIBM_INDBM );
	
	// allow bus mastering in general
	OUTREGP( di->regs, RADEON_BUS_CNTL, 0, ~RADEON_BUS_MASTER_DIS );

	// don't allow mixing of 2D/3D/scratch/wait_until commands
	// (in fact, this doesn't seem to make any difference as we do a
	// manual sync in all these cases anyway)
	OUTREG( di->regs, RADEON_ISYNC_CNTL,
		RADEON_ISYNC_ANY2D_IDLE3D |
		RADEON_ISYNC_ANY3D_IDLE2D |
		RADEON_ISYNC_WAIT_IDLEGUI |
		RADEON_ISYNC_CPSCRATCH_IDLEGUI );
		 
	SHOW_FLOW( 3, "bus_cntl=%lx", INREG( di->regs, RADEON_BUS_CNTL ));

	SHOW_FLOW0( 3, "Done" );
		
	return B_OK;
	
//err:
//	uninitIndirectBuffers( ai );	
err2:
	uninitCPFeedback( di );
err3:
	uninitRingBuffer( di );
err4:
	DELETE_BEN( di->si->cp.lock );
	return res;
}


// shutdown CP, freeing any memory
void Radeon_UninitCP( device_info *di )
{
	vuint8 *regs = di->regs;

	// abort any pending commands
	Radeon_ResetEngine( di );
	
	// disable CP BM
	OUTREG( regs, RADEON_CP_CSQ_CNTL, RADEON_CSQ_PRIDIS_INDDIS );	
	// read-back for flushing
	INREG( regs, RADEON_CP_CSQ_CNTL );

	uninitRingBuffer( di );
	uninitCPFeedback( di );
	uninitIndirectBuffers( di );
	
	DELETE_BEN( di->si->cp.lock );
}


// mark all indirect buffers as being free;
// this should only be called after a reset;
// lock must be hold
void Radeon_DiscardAllIndirectBuffers( device_info *di )
{
	CP_info *cp = &di->si->cp;
	
	// during init, there is no indirect buffer
	if( cp->buffers.mem_handle == 0 )
		return;
	
	// mark all sent indirect buffers as free
	while( cp->buffers.oldest != -1 ) {
		indirect_buffer *oldest_buffer = 
			&cp->buffers.buffers[cp->buffers.oldest];
		int tmp_oldest_buffer;
			
		SHOW_FLOW( 0, "%d", cp->buffers.oldest );
		
		// remove buffer from "used" list
		tmp_oldest_buffer = oldest_buffer->next;
			
		if( tmp_oldest_buffer == -1 )
			cp->buffers.newest = -1;
			
		// put it on free list
		oldest_buffer->next = cp->buffers.free_list;
		cp->buffers.free_list = cp->buffers.oldest;
		
		cp->buffers.oldest = tmp_oldest_buffer;
	}
}
