/*
	Copyright (c) 2002, Thomas Kurschel


	Part of Radeon kernel driver
		
	Memory manager used for graphics mem
	
	
	It has the following features
	- doesn't access memory to be managed
	- memory block's owner is identified by tag,
	  tag is verified during free, and all memory
	  belonging to one tag can be freed at once
	- multi-threading save
*/

#include "memmgr.h"
#include <malloc.h>
#include "KernelExport.h"

#define debug_level_flow 4
#define debug_level_info 4
#define debug_level_error 4

#define DEBUG_MSG_PREFIX "Graphics Driver - "
#include "debug_ext.h"


#if 0
#ifndef _KERNEL_MODE
void    _kdprintf_(const char *format, ...);
//bool    set_dprintf_enabled(bool);	/* returns old enable flag */

#define dprintf _kdprintf_
#endif
#endif


// init manager
//   start - start of address space
//   len - len of address space
//   block_size - granularity
//   heap_entries - maximum number of blocks
mem_info *mem_init( uint32 start, uint32 len, uint32 block_size, uint32 heap_entries )
{
	mem_block *first;
	mem_info *mem;
	uint i;
	
	SHOW_FLOW( 2, "start=%lx, len=%lx, block_size=%lx, heap_entries=%ld",
		start, len, block_size, heap_entries );
	
	mem = malloc( sizeof( *mem ));
	if( mem == NULL )
		goto err;
	
	mem->block_size = block_size;
	mem->heap_entries = heap_entries;
	mem->lock = create_sem( 1, "mem_lock" );
	
	if( mem->lock < 0 )
		goto err2;
		
	mem->heap = malloc( heap_entries * sizeof( mem_block ));
	if( mem->heap == NULL )
		goto err3;
		
	for( i = 1; i < heap_entries; ++i )
		mem->heap[i].next = &mem->heap[i+1];
		
	mem->heap[heap_entries - 1].next = NULL;
	mem->unused = &mem->heap[1];

	first = &mem->heap[0];
	mem->first = first;
	
	first->base = start;
	first->size = len;
	first->prev = first->next = NULL;
	first->alloced = false;
	
	return mem;

err3:
	delete_sem( mem->lock );	
err2:
	free( mem );
err:
	return NULL;
}


// destroy heap
void mem_destroy( mem_info *mem )
{	
	SHOW_FLOW0( 2, "" );
	
	free( mem->heap );
	delete_sem( mem->lock );
	free( mem );
}


// allocate memory block
// in:
//   mem - heap handle
//   size - size in bytes
//   tag - owner tag
// out:
//   block_id - block id
//   offset - start address of block
status_t mem_alloc( mem_info *mem, uint32 size, void *tag, uint32 *block_id, uint32 *offset )
{
	mem_block *cur, *new_entry;
	
	SHOW_FLOW( 2, "size=%ld, tag=%p", size, tag );
	
	acquire_sem( mem->lock );

	// we assume block_size is power of two	
	size = (size + mem->block_size - 1) & ~(mem->block_size - 1);

	// simple first fit	
	for( cur = mem->first; cur; cur = cur->next ) {
		if( !cur->alloced && cur->size >= size )
			break;
	}
	
	if( cur == NULL ) {
		SHOW_FLOW0( 2, "out of memory" );
		goto err;
	}
	
	if( size != cur->size ) {
		new_entry = mem->unused;
		
		if( new_entry == NULL ) {
			SHOW_FLOW0( 2, "out of blocks" );
			goto err;
		}
		
		mem->unused = new_entry->next;
	
		new_entry->next = cur->next;
		new_entry->prev = cur;	
		new_entry->alloced = false;
		new_entry->base = cur->base + size;
		new_entry->size = cur->size - size;
		
		if( cur->next )
			cur->next->prev = new_entry;
			
		cur->next = new_entry;
		cur->size = size;
	}

	cur->alloced = true;
	cur->tag = tag;
	
	*block_id = cur - mem->heap + 1;
	*offset = cur->base;

	release_sem( mem->lock );
	
	SHOW_FLOW( 2, "block_id=%ld, offset=%lx", *block_id, *offset );
	
	return B_OK;
	
err:
	release_sem( mem->lock );
	return B_NO_MEMORY;
}


// merge "block" with successor
static void merge( mem_info *mem, mem_block *block )
{
	mem_block *next;
	
	next = block->next;
	
	block->size += next->size;
	
	if( next->next )
		next->next->prev = block;
	
	block->next = next->next;

	next->next = mem->unused;
	mem->unused = next;
}


// internal: free memory block including merge
static mem_block *freeblock( mem_info *mem, mem_block *block )
{
	mem_block *prev, *next;

	block->alloced = false;
	
	prev = block->prev;
	
	if( prev && !prev->alloced ) {
		block = prev;
		merge( mem, prev );
	}

	next = block->next;
		
	if( next && !next->alloced )
		merge( mem, block );
		
	return block;
}


// free memory
//   mem - heap handle
//   block_id - block id
//   tag - owner tag (must match tag passed to mem_alloc)
status_t mem_free( mem_info *mem, uint32 block_id, void *tag )
{	
	mem_block *block;
	
	SHOW_FLOW( 2, "block_id=%ld, tag=%p", block_id, tag );
	
	acquire_sem( mem->lock );
	
	--block_id;
	
	if( block_id >= mem->heap_entries ) {
		SHOW_ERROR0( 2, "invalid id" );
		goto err;
	}
		
	block = &mem->heap[block_id];
	if( !block->alloced || block->tag != tag ) {
		SHOW_ERROR0( 2, "not owner" );
		goto err;
	}
	
	freeblock( mem, block );
		
	release_sem( mem->lock );
	
	SHOW_FLOW0( 2, "success" );
	return B_OK;
	
err:
	release_sem( mem->lock );
	return B_BAD_VALUE;
}


// free all memory belonging to owner "tag"
void mem_freetag( mem_info *mem, void *tag )
{
	mem_block *cur;
	
	SHOW_FLOW( 2, "tag=%p", tag );
	
	acquire_sem( mem->lock );
	
	for( cur = mem->first; cur; cur = cur->next ) {
		if( cur->alloced && cur->tag == tag )
			cur = freeblock( mem, cur );
	}
	
	release_sem( mem->lock );
	
	SHOW_FLOW0( 2, "done" );
}
