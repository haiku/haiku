/*
	Copyright (c) 2002, Thomas Kurschel


	Part of Radeon kernel driver
		
	Memory manager used for graphics mem
*/

#ifndef _MEMMGR_H
#define _MEMMGR_H

#include <OS.h>


// allocated memory block
typedef struct mem_block {
	struct mem_block *prev, *next;
	uint32 base;
	uint32 size;
	void *tag;
	bool alloced;
} mem_block;


// memory heap
typedef struct mem_info {
	mem_block *first;
	uint32 block_size;
	sem_id lock;
	mem_block *heap;
	mem_block *unused;
	uint32 heap_entries;
} mem_info;


mem_info *mem_init( uint32 start, uint32 len, uint32 block_size, uint32 heap_entries );
void mem_destroy( mem_info *mem );
status_t mem_alloc( mem_info *mem, uint32 size, void *tag, uint32 *block, uint32 *offset );
status_t mem_free( mem_info *mem, uint32 block_id, void *tag );
void mem_freetag( mem_info *mem, void *tag );

#endif
