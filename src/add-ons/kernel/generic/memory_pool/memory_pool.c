#include <stdio.h> // for the hack using malloc() / free()
#include <stdlib.h>
#include <SupportDefs.h>

#include "memory_pool.h"

// Keep in mind that pools size are rounded to B_PAGE_SIZE, and 
// node_size are align to 32 bits boundaries...

struct memory_pool {
	struct memory_pool *	next;		// for dynamic pool expansion/shrinking...
	size_t					node_size;		// in byte, rounded to 32 bits boundary
	uint32					node_count;
	// struct memory_pool_block blocks[1];		
};

typedef struct memory_pool_block {
	uint32	nb_max;			// in this pool only, may vary from pool to another for same pools chain  
	uint32	nb_free;		// in this pool only, if 0 try 'next' one...
	void *	first;			// address of first node, just next to freemap
	void *	last;			// address of last node in this pool
	// the uint32	freemap[] follow, aligned to 32 bits boundary
	// then the nodes data follow...
} memory_pool_block;

#define BITMAPSIZE(nb_nodes) (nb_nodes / 8)

status_t 		std_ops(int32 op, ...);

memory_pool *	new_pool(size_t node_size, uint32 node_count);
status_t		delete_pool(memory_pool * pool);
void *			new_pool_node(memory_pool * pool);
status_t		delete_pool_node(memory_pool * pool, void * node);
status_t		for_each_pool_node(memory_pool * pool, pool_iterate_func iterate, void * cookie);

// Keep in mind that pools size are rounded to B_PAGE_SIZE...

#define ROUND(x, y) (((x) + (y) - 1) & ~((y) - 1))

#define DEBUG	1



#ifdef CODEWARRIOR
	#pragma mark [Public functions]
#endif

// -------------------------------
memory_pool * new_pool(size_t node_size, uint32 node_count)
{
	// TODO!
	return (memory_pool *) node_size;	// quick hack
}


// -------------------------------
status_t delete_pool(memory_pool * pool)
{
  // TODO!
  
  return 0; // return B_OK;
}


// -------------------------------
void * new_pool_node(memory_pool * pool)
{
  // TODO!
  
  return malloc((size_t) pool); // quick hack
}


// -------------------------------
status_t delete_pool_node(memory_pool * pool, void * node)
{
	// TODO!
	
	free(node);	// quick hack
	return 0; // return B_OK;
}


// -------------------------------
status_t for_each_pool_node(memory_pool * pool, pool_iterate_func iterate, void * cookie)
{
	// TODO!
	return 0; // return B_OK;
}


#if 0

// -------------------------------
memory_pool * new_pool2(size_t node_size, uint32 node_count)
{
	memory_pool *	pool;
	size_t			size;
	size_t			freemap_size;
	uint8 *			ptr;


	node_size = ROUND(node_size, 4); // aligned to 32 bits

	freemap_size = ROUND(node_count, 32) / 8; // aligned to 32 bits

	size = sizeof(memory_pool);

	size += freemap_size;
	size += node_size * node_count;

	pool = (memory_pool *) malloc(size);

	pool->next		= NULL; // no secondary pool for the moment...

	pool->node_size = node_size; // aligned to 32 bits
	pool->nb_max	= node_count;
	pool->nb_free	= node_count;

	ptr = (uint8 *) (pool + 1);
	ptr += freemap_size;

	pool->first = ptr;

	ptr += (node_count-1) * node_size;

	pool->last = ptr;

	return pool;
}



// -------------------------------
status_t delete_pool2(memory_pool * pool)
{
  memory_pool * next;

  while (pool) {
	  next = pool->next;

	  my_free2("delete_pool: ", pool);

	  pool = next;
  };
  
  return 0; // return B_OK;
}


// -------------------------------
void * new_pool_node2(memory_pool * pool)
{
	memory_pool *	p;
	size_t			slot;
	sizet			nb_slots;
	uint32* 		freemap;
	uint8 *			ptr;

	// seaching pools list for one with at least one free node
	p = pool;
	while (p->nb_free == 0) {
		p = p->next;
	};

	if (! p) {
		// need to add a new pool
		p = new_pool2(pool->node_size, pool->nb_max);
		if (!p)
			return NULL;	// argh, no more memory!?!

		p->next = pool->next;
		pool->next = p;
	};

	// okay, now find the first free slot of this pool
	slot = 0;
	nb_slots = ROUND(p->nb_max, 32); // aligned to 32 bits
	freemap = (uint32 *) (p + 1);

	// fast lookup over 32 contiguous slots already in use (if any)
	while (slot < nb_slots) {
		if (*freemap != 0xFFFFFFFF)
			break;

		freemap++; // all 32 contiguous slots used
		slot += 32; 
	};

	// find the first free slot of these next 32 ones
	while(slot < nb_slots) {
		if ( *freemap & (1 << (31 - (slot % 32))) == 0)
			// free slot found :-)
			break;

		slot++;
	};

	if (slot >= nb_slots)
		// oh oh, should never happend! ;-)
		return NULL;

	*freemap |= (1 << (31 - (slot % 32)));
	p->nb_free--;

	ptr = (uint8 *) p->first;
	ptr += slot * p->node_size;

	return ptr;
}


// -------------------------------
status_t delete_pool_node2(memory_pool * pool, void * node)
{
	memory_pool *	p;
	size_t			slot;
	size_t			nb_slots;
	uint32* 		freemap;
	uint8 *			ptr;

	// seaching pools list for the one who host this node
	p = pool;
	while (pool) {
		if (node >= pool->first && node <= pool->last)
			break; // node's hosting pool found
		pool = pool->next;
	};

	if (! pool)
		return -1;	// return B_BAD_VALUE;

	// find node slot number on this pool
	slot = (node - pool->first);
	if (slot % pool->node_size)
		// oh oh, not a valid, starting node address value!
		return -1; // return B_BAD_VALUE;

	nb_slots = ROUND(p->nb_max, 32); // aligned to 32 bits
	slot /= pool->node_size;
	if (slot >= nb_slots)
		// oh oh, node slot out of range!!! 
		// something go wrong with pool->last value!?!
		return -1; // return B_BAD_VALUE;

	freemap = (uint32 *) (pool + 1);

	pool->nb_free++;
	if (pool->nb_free == pool->nb_max)
		// free this pool_block

	return 0; // return B_OK;
}

#endif

// #pragma mark -

memory_pool_module_info mpmi = {
	{
		MEMORY_POOL_MODULE_NAME,
		0, 
		std_ops
	},

	new_pool,
	delete_pool,
	new_pool_node,
	delete_pool_node,
	for_each_pool_node
};

status_t std_ops(int32 op, ...) 
{
	switch(op) {
		case B_MODULE_INIT:
			printf("memory_pool: B_MODULE_INIT\n");
			break;

		case B_MODULE_UNINIT:
			printf("memory_pool: B_MODULE_UNINIT\n");
			break;

		default:
			return B_ERROR;
	}
	return B_OK;
}


_EXPORT module_info *modules[] = {
	(module_info *) &mpmi,		// memory_pool_module_info
	NULL
};

