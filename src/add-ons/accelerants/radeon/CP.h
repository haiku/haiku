/*
	Copyright (c) 2002/03, Thomas Kurschel
	

	Part of Radeon accelerant
		
	Command Processor interface.


	Buffer management:
	
	We use both the circular buffer and indirect buffers. To let the CP
	execute something, you must allocate an indirect buffer by
	Radeon_AllocIndirectBuffer(), fill it, and let it post via 
	Radeon_SendIndirectBuffer(). If you need some certain state before
	your buffer is executed, you can define a state buffer: in this
	buffer you write commands necessary to gain your whished state.
	You get this state buffer during startup via Radeon_AllocIndirectBuffer() 
	and release it by Radeon_FreeIndirectBuffer() during shutdown.
	Whenever you want to change (or free) it, call 
	Radeon_InvalidateStateBuffer() to make sure the state buffer is not
	in use. Radeon_SendIndirectBuffer() keeps track of the current 
	state and if it's different then the state necessary for execution
	of an indirect buffer, it submit the state buffer first. State
	buffers are currently used for virtual cards only, but could be
	used for things like 3D accelerator state as well.
	
	All indirect buffers have the same size: 4K (Radeons want them to
	be 4k aligned, so this is the minimum size). For 3D this may be too
	small, but for 2D it's more then enough. To not waste main memory
	(they cannot reside in graphics mem, at least my tests showed that
	you get consistency problems), there are currently 253 buffers. 
	As the ring buffer only contains calls to indirect buffers and
	each call needs at most 8 dwords, 2025 dwords would be sufficient.
	Currently, there are 4K dwords circular buffer, which is more
	then enough. Perhaps, engine synchronization code will be moved
	from indirect to ring buffer to speed things up, in which case
	the ring buffer might be too small.
	
	Indirect buffers are recycled if there is none left. To track their
	execution, each submitted buffer gets a tag (tags are numbered 0, 1...). 
	and put into a list. After execution, the tag is written to scratch 
	register 1 via CP. The recycler (Radeon_FreeIndirectBuffers()) 
	compares the tags of submitted buffers with scratch register 1 to
	detect finished buffers.
	
	When you call any public function, you don't need to own any lock.
*/

#include "cp_regs.h"

//status_t Radeon_InitCP( accelerator_info *ai );

int Radeon_AllocIndirectBuffer( accelerator_info *ai, bool keep_lock );
void Radeon_FreeIndirectBuffer( accelerator_info *ai, 
	int buffer_idx, bool never_used );
void Radeon_SendIndirectBuffer( accelerator_info *ai, 
	int buffer_idx, int buffer_size, 
	int state_buffer_idx, int state_buffer_size, bool has_lock );
void Radeon_InvalidateStateBuffer( accelerator_info *ai, int state_buffer_idx );
void Radeon_FreeIndirectBuffers( accelerator_info *ai );
void Radeon_DiscardAllIndirectBuffers( accelerator_info *ai );
	
// get CPU address of indirect buffer
static inline uint32 *Radeon_GetIndirectBufferPtr( accelerator_info *ai, int buffer_idx )
{
	return (uint32 *)(ai->mapped_memory[ai->si->cp.buffers.mem_type].data + ai->si->cp.buffers.mem_offset)
		+ buffer_idx * INDIRECT_BUFFER_SIZE;
}

// start writing into indirect buffer
#define START_IB() \
	{ \
		int buffer_idx; \
		uint32 *buffer_start, *buffer; \
\
		buffer_idx = Radeon_AllocIndirectBuffer( ai, true ); \
		buffer = buffer_start = Radeon_GetIndirectBufferPtr( ai, buffer_idx );
		
// write "write register" into indirect buffer
#define WRITE_IB_REG( reg, value ) \
	do { buffer[0] = CP_PACKET0( (reg), 1 ); \
		 buffer[1] = (value); \
		 buffer += 2; } while( 0 ) 


// submit indirect buffer specific to virtual card
// stores tag of last command in engine.count
#define SUBMIT_IB_VC() \
		Radeon_SendIndirectBuffer( ai, \
			buffer_idx, buffer - buffer_start, \
			vc->state_buffer_idx, vc->state_buffer_size, true ); \
	}

// submit indirect buffer, not specific to virtual card
#define SUBMIT_IB() \
		Radeon_SendIndirectBuffer( ai, \
			buffer_idx, buffer - buffer_start, \
			0, 0, true ); \
	}

// write PACKET3 header, restricting block count
// 	command - command code
// 	count - whished number of blocks
// 	bytes_left - number of bytes left in buffer
// 	dwords_per_block - dwords per block
// 	dwords_in_header - dwords in header (i.e. dwords before the repeating blocks)
//
// the effective count is stored in "sub_count" substracted from "count";
// further, the first dwords of the packet is written
//
// remark: it's taken care of to keep in size of the buffer and the maximum number
// of bytes per command; the dword count as written into the first dword of the header
// is "size of body(!) in dwords - 1", which means "size of packet - 2"
#define WRITE_IB_PACKET3_HEAD( command, count, bytes_left, dwords_per_block, dwords_in_header ) \
	sub_count = min( count, \
		(min( bytes_left, (1 << 14) - 1 + 2) - dwords_in_header) / dwords_per_block ); \
	count -= sub_count; \
	*buffer++ = command	| (((sub_count * dwords_per_block) + dwords_in_header - 2) << 16);
