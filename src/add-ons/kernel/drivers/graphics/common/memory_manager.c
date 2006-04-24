/*
 * Copyright 2006, Haiku Inc.
 * Copyright 2002, Thomas Kurschel.
 *
 * Distributed under the terms of the MIT license.
 */

/**	Memory manager used for graphics mem
 *
 *	It has the following features
 *	- doesn't access memory to be managed
 *	- memory block's owner is identified by tag,
 *	  tag is verified during free, and all memory
 *	  belonging to one tag can be freed at once
 *	- multi-threading save
 */

#include "memory_manager.h"

#include <KernelExport.h>
#include <stdlib.h>


//#define TRACE_MEMORY_MANAGER
#ifdef TRACE_MEMORY_MANAGER
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


// allocated memory block
typedef struct mem_block {
	struct mem_block *prev, *next;
	uint32 base;
	uint32 size;
	void *tag;
	bool allocated;
} mem_block;

// memory heap
struct mem_info {
	mem_block *first;
	area_id heap_area;
	uint32 block_size;
	sem_id lock;
	mem_block *heap;
	mem_block *unused;
	uint32 heap_entries;
};


/** merge "block" with successor */

static void
merge(mem_info *mem, mem_block *block)
{
	mem_block *next = block->next;

	block->size += next->size;

	if (next->next)
		next->next->prev = block;

	block->next = next->next;
	next->next = mem->unused;
	mem->unused = next;
}


/** free memory block including merge */

static mem_block *
freeblock(mem_info *mem, mem_block *block)
{
	mem_block *prev, *next;

	block->allocated = false;
	prev = block->prev;

	if (prev && !prev->allocated) {
		block = prev;
		merge(mem, prev);
	}

	next = block->next;

	if (next && !next->allocated)
		merge(mem, block);

	return block;
}


//	#pragma mark -


/** Init memory manager.
 *
 *	\param start start of address space
 *	\param length length of address space
 *	\param blockSize - granularity
 *	\param heapEntries - maximum number of blocks
 */

mem_info *
mem_init(const char* name, uint32 start, uint32 length,
	uint32 blockSize, uint32 heapEntries)
{
	mem_block *first;
	mem_info *mem;
	uint i;
	uint32 size;

	TRACE(("mem_init(name=%s, start=%lx, length=%lx, blockSize=%lx, heapEntries=%ld)\n",
		name, start, length, blockSize, heapEntries));

	mem = malloc(sizeof(*mem));
	if (mem == NULL)
		goto err1;

	mem->block_size = blockSize;
	mem->heap_entries = heapEntries;
	mem->lock = create_sem(1, name);
	if (mem->lock < 0)
		goto err2;

	// align size to B_PAGE_SIZE
	size = heapEntries * sizeof(mem_block);
	size = (size + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);

	mem->heap_area = create_area(name, (void **)&mem->heap,
		B_ANY_ADDRESS, size, B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA);

	if (mem->heap_area < 0 || mem->heap == NULL)
		goto err3;

	for (i = 1; i < heapEntries; ++i) {
		mem->heap[i - 1].next = &mem->heap[i];
	}

	mem->heap[heapEntries - 1].next = NULL;
	mem->unused = &mem->heap[1];

	first = &mem->heap[0];
	mem->first = first;

	first->base = start;
	first->size = length;
	first->prev = first->next = NULL;
	first->allocated = false;
	
	return mem;

err3:
	delete_sem(mem->lock);
err2:
	free(mem);
err1:
	return NULL;
}


/** destroy heap */

void
mem_destroy(mem_info *mem)
{
	TRACE(("mem_destroy(mem %p)\n", mem));

	delete_area(mem->heap_area);
	delete_sem(mem->lock);
	free(mem);
}


/** Allocate memory block
 *
 *	\param mem heap handle
 *	\param size size in bytes
 *	\param tag owner tag
 *
 *	\param blockID - returns block id
 *	\param offset - returns start address of block
 */

status_t
mem_alloc(mem_info *mem, uint32 size, void *tag, uint32 *blockID, uint32 *offset)
{
	mem_block *current, *newEntry;
	status_t status;

	TRACE(("mem_alloc(mem %p, size=%lx, tag=%p)\n", mem, size, tag));

	status = acquire_sem(mem->lock);
	if (status != B_OK)
		return status;

	// we assume block_size is power of two	
	size = (size + mem->block_size - 1) & ~(mem->block_size - 1);

	// simple first fit	
	for (current = mem->first; current; current = current->next) {
		if (!current->allocated && current->size >= size)
			break;
	}

	if (current == NULL) {
		TRACE(("mem_alloc: out of memory\n"));
		goto err;
	}

	if (size != current->size) {
		newEntry = mem->unused;

		if (newEntry == NULL) {
			TRACE(("mem_alloc: out of blocks\n"));
			goto err;
		}

		mem->unused = newEntry->next;

		newEntry->next = current->next;
		newEntry->prev = current;	
		newEntry->allocated = false;
		newEntry->base = current->base + size;
		newEntry->size = current->size - size;

		if (current->next)
			current->next->prev = newEntry;

		current->next = newEntry;
		current->size = size;
	}

	current->allocated = true;
	current->tag = tag;

	*blockID = current - mem->heap + 1;
	*offset = current->base;

	release_sem(mem->lock);

	TRACE(("mem_alloc(block_id=%ld, offset=%lx)\n", *blockID, *offset));
	return B_OK;

err:
	release_sem(mem->lock);
	return B_NO_MEMORY;
}


/** Free memory
 *	\param mem heap handle
 *	\param blockID block id
 *	\param tag owner tag (must match tag passed to mem_alloc())
 */

status_t
mem_free(mem_info *mem, uint32 blockID, void *tag)
{	
	mem_block *block;
	status_t status;

	TRACE(("mem_free(mem %p, blockID=%ld, tag=%p)\n", mem, blockID, tag));

	status = acquire_sem(mem->lock);
	if (status != B_OK)
		return status;

	--blockID;

	if (blockID >= mem->heap_entries) {
		TRACE(("mem_free: invalid ID %lu\n", blockID));
		goto err;
	}

	block = &mem->heap[blockID];
	if (!block->allocated || block->tag != tag) {
		TRACE(("mem_free: not owner\n"));
		goto err;
	}

	freeblock(mem, block);

	release_sem(mem->lock);
	return B_OK;

err:
	release_sem(mem->lock);
	return B_BAD_VALUE;
}


/** Free all memory belonging to owner "tag" */

status_t
mem_freetag(mem_info *mem, void *tag)
{
	mem_block *current;
	status_t status;

	TRACE(("mem_freetag(mem %p, tag=%p)\n", mem, tag));

	status = acquire_sem(mem->lock);
	if (status != B_OK)
		return status;
	
	for (current = mem->first; current; current = current->next) {
		if (current->allocated && current->tag == tag)
			current = freeblock(mem, current);
	}

	release_sem(mem->lock);
	return B_OK;
}
