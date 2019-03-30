/*
	Copyright (c) 2002, Thomas Kurschel
	

	Part of Radeon accelerant
		
	Command Processor handling
	

	Something about synchronization in general: 
	
	The DDK says that only some register accesses are stored in the 
	Command FIFO, i.e. in almost all cases you don't have to wait until
	there is enough space in this FIFO. Unfortunately, ATI doesn't speak
	clearly here and doesn't tell you which registers are buffered and 
	which not (the r300 DDK provides some examples only, other DDKs refer
	to some include file where no such info could be found).
	
	Looking at pre-Radeon specs, we have the following register ranges:
		0		configuration/display/multi-media registers
		0xf00	read-only PCI configuration space 
		0x1000	CCE registers
		0x1400	FIFOed GUI-registers
		
	So, if the list is still correct, the affected registers are only 
	those used for 2D/3D drawing.
	
	This is very important as if the register you want to write is 
	buffered, you have to do a busy wait until there is enough FIFO
	space. As concurrent threads may do the same, register access should
	only be done with a lock held. We never write GUI-registers directly,
	so we never have to wait for the FIFO and thus don't need this lock.

*/

#include "radeon_accelerant.h"
#include "mmio.h"
#include "buscntrl_regs.h"
#include "utils.h"
#include <sys/ioctl.h>
#include "CP.h"

#include "log_coll.h"
#include "log_enum.h"

#include <string.h>


// get number of free entries in CP's ring buffer
static uint getAvailRingBuffer( accelerator_info *ai )
{
	CP_info *cp = &ai->si->cp;
	int space;
	
	space = 
		*(uint32 *)(ai->mapped_memory[cp->feedback.mem_type].data + cp->feedback.head_mem_offset)
		//*cp->ring.head 
		- cp->ring.tail;
	//space = INREG( ai->regs, RADEON_CP_RB_RPTR ) - cp->ring.tail;

	if( space <= 0 )
		space += cp->ring.size;
		
	// don't fill up the entire buffer as we cannot
	// distinguish between a full and an empty ring
	--space;
	
	SHOW_FLOW( 3, "head=%ld, tail=%ld, space=%ld", 
		*(uint32 *)(ai->mapped_memory[cp->feedback.mem_type].data + cp->feedback.head_mem_offset),
		//*cp->ring.head, 
		cp->ring.tail, space );

	LOG1( si->log, _GetAvailRingBufferQueue, space );
	
	cp->ring.space = space;
		
	return space;
}


// mark all indirect buffers that have been processed as being free;
// lock must be hold
void Radeon_FreeIndirectBuffers( accelerator_info *ai )
{
	CP_info *cp = &ai->si->cp;
	int32 cur_processed_tag = 
		((uint32 *)(ai->mapped_memory[cp->feedback.mem_type].data + cp->feedback.scratch_mem_offset))[1];
		//ai->si->cp.scratch.ptr[1];
	//INREG( ai->regs, RADEON_SCRATCH_REG1 );
	
	SHOW_FLOW( 3, "processed_tag=%d", cur_processed_tag );
	
	// mark all sent indirect buffers as free
	while( cp->buffers.oldest != -1 ) {
		indirect_buffer *oldest_buffer = 
			&cp->buffers.buffers[cp->buffers.oldest];
		int tmp_oldest_buffer;
		
		SHOW_FLOW( 3, "oldset buffer's tag: %d", oldest_buffer->send_tag );
			
		// this is a tricky calculation to handle wrap-arounds correctly,
		// so don't change it unless you really understand the signess problem
		if( (int32)(cur_processed_tag - oldest_buffer->send_tag) < 0 )
			break;
			
		SHOW_FLOW( 3, "mark %d as being free", oldest_buffer->send_tag );
			
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


// wait until an indirect buffer becomes available;
// lock must be hold
static void Radeon_WaitForFreeIndirectBuffers( accelerator_info *ai )
{
	bigtime_t start_time;
	CP_info *cp = &ai->si->cp;
	
	SHOW_FLOW0( 3, "" );
	
	start_time = system_time();

	while( 1 ) {
		bigtime_t sample_time;
		
		Radeon_FreeIndirectBuffers( ai );
		
		if( cp->buffers.free_list >= 0 )
			return;
		
		sample_time = system_time();
		
		if( sample_time - start_time > 100000 )
			break;
			
		RELEASE_BEN( cp->lock );

		// use exponential fall-off
		// in the beginning do busy-waiting, later on we let the thread sleep;
		// the micro-spin is used to reduce PCI load
		if( sample_time - start_time > 5000 ) 
			snooze( (sample_time - start_time) / 10 );
		else
			Radeon_Spin( 1 );

		ACQUIRE_BEN( cp->lock );
	}
	
	SHOW_ERROR0( 0, "All buffers are in use and engine doesn't finish any of them" );

	// lock must be released during reset (reset acquires it automatically)
	RELEASE_BEN( cp->lock );
	Radeon_ResetEngine( ai );
	ACQUIRE_BEN( cp->lock );
}

// allocate an indirect buffer
int Radeon_AllocIndirectBuffer( accelerator_info *ai, bool keep_lock )
{
	CP_info *cp = &ai->si->cp;
	int buffer_idx;
	
	SHOW_FLOW0( 3, "" );
	
	ACQUIRE_BEN( cp->lock );
	
	if( cp->buffers.free_list == -1 )
		Radeon_WaitForFreeIndirectBuffers( ai );
		
	buffer_idx = cp->buffers.free_list;
	cp->buffers.free_list = cp->buffers.buffers[buffer_idx].next;

	//if( !keep_lock )
		RELEASE_BEN( cp->lock );
	(void)keep_lock;
	
	SHOW_FLOW( 3, "got %d", buffer_idx );
	
	return buffer_idx;
}


// explicitely free an indirect buffer;
// this is not needed if the buffer was send via SendIndirectBuffer()
// never_used	- 	set to true if the buffer wasn't even sent indirectly
//					as a state buffer
// !Warning! 
// if never_used is false, execution may take very long as all buffers 
// must be flushed!
void Radeon_FreeIndirectBuffer( accelerator_info *ai, int buffer_idx, bool never_used )
{
	CP_info *cp = &ai->si->cp;
	
	SHOW_FLOW( 3, "buffer_idx=%d, never_used=%d", buffer_idx, never_used );
	
	// if the buffer was used as a state buffer, we don't record its usage,
	// so we don't know if the buffer was/is/will be used;
	// the only way to be sure is to let the CP run dry
	if( !never_used ) 
		Radeon_WaitForIdle( ai, false );

	ACQUIRE_BEN( cp->lock );

	cp->buffers.buffers[buffer_idx].next = cp->buffers.free_list;
	cp->buffers.free_list = buffer_idx;	

	RELEASE_BEN( cp->lock );
	
	SHOW_FLOW0( 3, "done" );
}

// this function must be moved to end of file to avoid inlining
void Radeon_WaitForRingBufferSpace( accelerator_info *ai, uint num_dwords );


// start writing to ring buffer
// num_dwords - number of dwords to write (must be precise!)
// !Warning! 
// during wait, CP's benaphore is released
#define WRITE_RB_START( num_dwords ) \
	{ \
		uint32 *ring_start; \
		uint32 ring_tail, ring_tail_mask; \
		uint32 ring_tail_increment = (num_dwords); \
		if( cp->ring.space < ring_tail_increment ) \
			Radeon_WaitForRingBufferSpace( ai, ring_tail_increment ); \
		ring_start = \
		(uint32 *)(ai->mapped_memory[cp->ring.mem_type].data + cp->ring.mem_offset); \
			/*cp->ring.start;*/ \
		ring_tail = cp->ring.tail; \
		ring_tail_mask = cp->ring.tail_mask;

// write single dword to ring buffer
#define WRITE_RB( value ) \
	{ \
		uint32 val = (value); \
		SHOW_FLOW( 3, "@%d: %x", ring_tail, val ); \
		ring_start[ring_tail++] = val; \
		ring_tail &= ring_tail_mask; \
	}
	
// finish writing to ring buffer
#define WRITE_RB_FINISH \
		cp->ring.tail = ring_tail; \
		cp->ring.space -= ring_tail_increment; \
	}

// submit indirect buffer for execution.
// the indirect buffer must not be used afterwards!
// buffer_idx			- index of indirect buffer to submit
// buffer_size  		- size of indirect buffer in 32 bits
// state_buffer_idx		- index of indirect buffer to restore required state
// state_buffer_size	- size of indirect buffer to restore required state
// returns:				  tag of buffer (so you can wait for its execution)
// if no special state is required, set state_buffer_size to zero
void Radeon_SendIndirectBuffer( accelerator_info *ai, 
	int buffer_idx, int buffer_size, 
	int state_buffer_idx, int state_buffer_size, bool has_lock )
{
	CP_info *cp = &ai->si->cp;
	bool need_stateupdate;
	
	SHOW_FLOW( 3, "buffer_idx=%d, buffer_size=%d, state_buffer_idx=%d, state_buffer_size=%d", 
		buffer_idx, buffer_size, state_buffer_idx, state_buffer_size );
	
	if( (buffer_size & 1) != 0 ) {
		SHOW_FLOW( 3, "buffer has uneven size (%d)", buffer_size );
		// size of indirect buffers _must_ be multiple of 64 bits, so
		// add a nop to fulfil alignment
		Radeon_GetIndirectBufferPtr( ai, buffer_idx )[buffer_size] = RADEON_CP_PACKET2;
		buffer_size += 1;
	}

	//if( !has_lock )
		ACQUIRE_BEN( cp->lock );
	(void)has_lock;

	need_stateupdate = 
		state_buffer_size > 0 && state_buffer_idx != cp->buffers.active_state;
		
	WRITE_RB_START( 5 + (need_stateupdate ? 3 : 0) );
	
	// if the indirect buffer to submit requires a special state and the
	// hardware is in wrong state then execute state buffer
	if( need_stateupdate ) {
		SHOW_FLOW0( 3, "update state" );
		
		WRITE_RB( CP_PACKET0( RADEON_CP_IB_BASE, 2 ));
		WRITE_RB( cp->buffers.vm_start + 
			state_buffer_idx * INDIRECT_BUFFER_SIZE * sizeof( uint32 ));
		WRITE_RB( state_buffer_size );
		
		cp->buffers.active_state = state_buffer_idx;
	}

	// execute indirect buffer
	WRITE_RB( CP_PACKET0( RADEON_CP_IB_BASE, 2 ));
	WRITE_RB( cp->buffers.vm_start + buffer_idx * INDIRECT_BUFFER_SIZE * sizeof( uint32 ));
	WRITE_RB( buffer_size );
	
	// give buffer a tag so it can be freed after execution
	WRITE_RB( CP_PACKET0( RADEON_SCRATCH_REG1, 1 ));
	WRITE_RB( cp->buffers.buffers[buffer_idx].send_tag = (int32)++cp->buffers.cur_tag );
	
	SHOW_FLOW( 3, "Assigned tag %d", cp->buffers.buffers[buffer_idx].send_tag );
	
	WRITE_RB_FINISH;

	// append buffer to list of submitted buffers
	if( cp->buffers.newest > 0 )
		cp->buffers.buffers[cp->buffers.newest].next = buffer_idx;
	else
		cp->buffers.oldest = buffer_idx;
		
	cp->buffers.newest = buffer_idx;
	cp->buffers.buffers[buffer_idx].next = -1;
	
	// flush writes to CP buffers
	// (this code is a bit of a overkill - currently, only some WinChip/Cyrix 
	//  CPU's support out-of-order writes, but we are prepared)
	// TODO : Other Architectures? PowerPC?
	#ifdef __i386__
	__asm__ __volatile__ ("lock; addl $0,0(%%esp)": : :"memory");
	#endif
	// make sure the motherboard chipset has flushed its write buffer by
	// reading some uncached memory
	//(void)*(volatile int *)si->framebuffer;
	INREG( ai->regs, RADEON_CP_RB_RPTR );

	//SHOW_FLOW( 3, "new tail: %d", cp->ring.tail );
	
	//snooze( 100 );
	
	// now, the command list should really be written to memory,
	// so it's safe to instruct the graphics card to read it
	OUTREG( ai->regs, RADEON_CP_RB_WPTR, cp->ring.tail );

	// read from PCI bus to ensure correct posting
	//INREG( ai->regs, RADEON_CP_RB_RPTR );
	
	RELEASE_BEN( cp->lock );
		
	SHOW_FLOW0( 3, "done" );
}


// mark state buffer as being invalid;
// this must be done _before_ modifying the state buffer as the
// state buffer may be in use
void Radeon_InvalidateStateBuffer( accelerator_info *ai, int state_buffer_idx )
{
	CP_info *cp = &ai->si->cp;

	// make sure state buffer is not used anymore
	Radeon_WaitForIdle( ai, false );

	ACQUIRE_BEN( cp->lock );

	// mark state as being invalid
	if( cp->buffers.active_state == state_buffer_idx )
		cp->buffers.active_state = -1;

	RELEASE_BEN( cp->lock );
}


// wait until there is enough space in ring buffer
// num_dwords - number of dwords needed in ring buffer
// must be called with benaphore hold
void Radeon_WaitForRingBufferSpace( accelerator_info *ai, uint num_dwords )
{
	bigtime_t start_time;
	CP_info *cp = &ai->si->cp;
	
	start_time = system_time();

	while( getAvailRingBuffer( ai ) < num_dwords ) {
		bigtime_t sample_time;
		
		sample_time = system_time();
		
		if( sample_time - start_time > 100000 )
			break;
			
		RELEASE_BEN( cp->lock );

		// use exponential fall-off
		// in the beginning do busy-waiting, later on we let the thread sleep;
		// the micro-spin is used to reduce PCI load
		if( sample_time - start_time > 5000 ) 
			snooze( (sample_time - start_time) / 10 );
		else
			Radeon_Spin( 1 );

		ACQUIRE_BEN( cp->lock );
	}
}
