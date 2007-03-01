/*
	Copyright (c) 2002, Thomas Kurschel
	

	Part of Radeon accelerant
		
	Hardware accelerator management
	
	All accelerator commands go through the following steps:
	- accelerant adds command to CP buffer and updates CP write pointer
	- CP fetches command and sends it to MicroController
	- MicroController instructs 2D unit to execute command
	- 2D unit draws into 2D Destination Cache (DC)
	- 2D Destination Cache is drained to frame buffer
	
	Whenever a token is required by BeOS, a command is queued to write
	the timestamp into Scratch Register 0. I haven't fully understand
	when and how coherancy is assured by Radeon, so I assume the following:
	- when the timestamp is written, all previous commands have been issued,
	  i.e. they are read and executed by the microcontroller
	- to make sure previously issued 2D commands have been finished,
	  a WAIT_2D_IDLECLEAN command is inserted before the scratch register 
	  write
	- to flush the destination cache, a RB2D_DC_FLUSH_ALL command is
	  issued before the wait; I hope that the wait command also waits for
	  the flush command, but I'm not sure about that
	  
	Remains the cache coherency problem. It you can set various bits in
	DSTCACHE_MODE register to assure that, but first I don't really understand
	them, and second I'm not sure which other caches/FIFO may make trouble.
	Especially, Be wants to use CPU and CP accesses in parallel. Hopefully,
	they don't interfere.
	
	I know that the PAINT_MULTI commands makes trouble if you change the
	ROP to something else: CPU writes produce garbage in frame buffer for the
	next couple of accesses. Resetting the ROP to a simply copy helps, but 
	I'm not sure what happens with concurrent CPU accesses to other areas 
	of the frame buffer.
*/


#include "radeon_accelerant.h"
#include "generic.h"
#include "rbbm_regs.h"
#include "GlobalData.h"
#include "mmio.h"
#include "CP.h"

static engine_token radeon_engine_token = { 1, B_2D_ACCELERATION, NULL };

// public function: return number of hardware engine
uint32 ACCELERANT_ENGINE_COUNT(void) 
{
	// hm, is there *any* card sporting more then 
	// one hardware accelerator???
	return 1;
}

// write current sync token into CP stream;
// we instruct the CP to flush all kind of cache first to not interfere
// with subsequent host writes
static void writeSyncToken( accelerator_info *ai )
{
	// don't write token if it hasn't changed since last write
	if( ai->si->engine.count == ai->si->engine.written )
		return;

	if( ai->si->acc_dma ) {
		START_IB();
	
		// flush pending data
		WRITE_IB_REG( RADEON_RB2D_DSTCACHE_CTLSTAT, RADEON_RB2D_DC_FLUSH_ALL );
		
		// make sure commands are finished
		WRITE_IB_REG( RADEON_WAIT_UNTIL, RADEON_WAIT_2D_IDLECLEAN |
			RADEON_WAIT_3D_IDLECLEAN | RADEON_WAIT_HOST_IDLECLEAN );
			
		// write scratch register
		WRITE_IB_REG( RADEON_SCRATCH_REG0, ai->si->engine.count );
		
		ai->si->engine.written = ai->si->engine.count;
		
		SUBMIT_IB();
	} else {
		Radeon_WaitForFifo( ai, 2 );
		OUTREG( ai->regs, RADEON_RB2D_DSTCACHE_CTLSTAT, RADEON_RB2D_DC_FLUSH_ALL);
		OUTREG( ai->regs, RADEON_WAIT_UNTIL, RADEON_WAIT_2D_IDLECLEAN |
		   RADEON_WAIT_3D_IDLECLEAN |
		   RADEON_WAIT_HOST_IDLECLEAN);
		ai->si->engine.written = ai->si->engine.count;
	}
}

// public function: acquire engine for future use
//	capabilites - required 2D/3D capabilities of engine, ignored
//	max_wait - maximum time we want to wait (in ms?), ignored
//	st - when engine has been acquired, wait for this sync token
//	et - (out) specifier of the engine acquired
status_t ACQUIRE_ENGINE( uint32 capabilities, uint32 max_wait, 
	sync_token *st, engine_token **et ) 
{
	shared_info *si = ai->si;
	
	SHOW_FLOW0( 4, "" );
	
	(void)capabilities;
	(void)max_wait;
	
	ACQUIRE_BEN( si->engine.lock)

	// wait for sync
	if (st) 
		SYNC_TO_TOKEN( st );

	*et = &radeon_engine_token;
	return B_OK;
}

// public function: release accelerator
//	et - engine to release
//	st - (out) sync token to be filled out
status_t RELEASE_ENGINE( engine_token *et, sync_token *st ) 
{
	shared_info *si = ai->si;

	SHOW_FLOW0( 4, "" );
	
	// fill out sync token
	if (st) {
		writeSyncToken( ai );
		
		st->engine_id = et->engine_id;
		st->counter = si->engine.count;
	}

	RELEASE_BEN( ai->si->engine.lock )
	
	return B_OK;
}

// public function: wait until engine is idle 
// ??? which engine to wait for? Is there anyone using this function?
//     is lock hold?
void WAIT_ENGINE_IDLE(void) 
{
	SHOW_FLOW0( 4, "" );
	
	Radeon_WaitForIdle( ai, false );
}

// public function: get sync token
//	et - engine to wait for
//	st - (out) sync token to be filled out
status_t GET_SYNC_TOKEN( engine_token *et, sync_token *st )
{
	shared_info *si = ai->si;

	SHOW_FLOW0( 4, "" );
	
	writeSyncToken( ai );
	
	st->engine_id = et->engine_id;
	st->counter = si->engine.count;
	
	SHOW_FLOW( 4, "got counter=%d", si->engine.count );
	
	return B_OK;
}

// this is the same as the corresponding kernel function
void Radeon_Spin( uint32 delay )
{
	bigtime_t start_time;
	
	start_time = system_time();
	
	while( system_time() - start_time < delay )
		;
}

// public: sync to token
//	st - token to wait for
status_t SYNC_TO_TOKEN( sync_token *st ) 
{
	shared_info *si = ai->si;
	bigtime_t start_time, sample_time;
	
	SHOW_FLOW0( 4, "" );
	
	if ( !ai->si->acc_dma )
	{
		Radeon_WaitForFifo( ai, 64 );
		Radeon_WaitForIdle( ai, false );
		return B_OK;
	}
	
	start_time = system_time();

	while( 1 ) {
		SHOW_FLOW( 4, "passed counter=%d", 
			((uint32 *)(ai->mapped_memory[si->cp.feedback.mem_type].data + si->cp.feedback.scratch_mem_offset))[0] );
			//si->cp.scratch.ptr[0] );
		
		// a bit nasty: counter is 64 bit, but we have 32 bit only,
		// this is a tricky calculation to handle wrap-arounds correctly
		if( (int32)(
			((uint32 *)(ai->mapped_memory[si->cp.feedback.mem_type].data + si->cp.feedback.scratch_mem_offset))[0]
			//si->cp.scratch.ptr[0] 
			- st->counter) >= 0 )
			return B_OK;
		/*if( (int32)(INREG( ai->regs, RADEON_SCRATCH_REG0 ) - st->counter) >= 0 )
			return B_OK;*/
		
		// commands have not been finished;
		// this is a good time to free completed buffers as we have to
		// busy-wait anyway
		ACQUIRE_BEN( si->cp.lock );
		Radeon_FreeIndirectBuffers( ai );
		RELEASE_BEN( si->cp.lock );

		sample_time = system_time();
		
		if( sample_time - start_time > 100000 )
			break;

		// use exponential fall-off
		// in the beginning do busy-waiting, later on we let thread sleep
		// the micro-spin is used to reduce PCI load
		if( sample_time - start_time > 5000 ) 
			snooze( (sample_time - start_time) / 10 );
		else
			Radeon_Spin( 1 );
	} 

	// we could reset engine now, but caller doesn't need to acquire
	// engine before calling this function, so we either reset it
	// without sync (ouch!) or acquire engine first and risk deadlocking
	SHOW_ERROR( 0, "Failed waiting for token %d (active token: %d)",
		st->counter, /*INREG( ai->regs, RADEON_SCRATCH_REG0 )*/
		((uint32 *)(ai->mapped_memory[si->cp.feedback.mem_type].data + si->cp.feedback.scratch_mem_offset))[0] );
		//si->cp.scratch.ptr[0] );
		
	Radeon_ResetEngine( ai );
		
	return B_ERROR;
}
