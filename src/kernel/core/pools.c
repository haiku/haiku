/* pools.c */

/* XXX - add documentation to this file! */
#include <kernel.h>
#include <OS.h>
#include <KernelExport.h>
#include <pools.h>
#include <vm.h>
#include <malloc.h>
#include <atomic.h>
#include <ktypes.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <debug.h>
#include <pools.h>

#define POOL_ALLOC_SZ 4 * 1024
#define ROUND_TO_PAGE_SIZE(x) (((x) + (POOL_ALLOC_SZ) - 1) & ~((POOL_ALLOC_SZ) - 1))


#ifdef WALK_POOL_LIST
void
walk_pool_list(struct pool_ctl *p)
{
	struct pool_mem *pb = p->list;

	dprintf("Pool: %p\n", p);
	dprintf("    -> list = %p\n", pb);
	while (pb) {
		dprintf("    -> mem_block %p, %p\n", pb, pb->next);
		pb = pb->next;
	}
}
#endif

void
pool_debug_walk(struct pool_ctl *p)
{
	struct free_blk *ptr;
	int i = 1;	
	
	dprintf("%ld byte blocks allocated, but now free:\n\n", p->alloc_size);

	#if POOL_USES_BENAPHORES
		ACQUIRE_BENAPHORE(p->lock);
	#else
		ACQUIRE_READ_LOCK(p->lock);
	#endif
	ptr = p->freelist;	
	while (ptr) {
		ASSERT(ptr->magic == FREE_MAGIC);
		ASSERT(FREE_MAGIC + (uint32)ptr->next == ptr->magic_check);
		dprintf("  %02d: %p\n", i++, ptr);
		ptr = ptr->next;
	}
	#if POOL_USES_BENAPHORES
		RELEASE_BENAPHORE(p->lock);
	#else
		RELEASE_READ_LOCK(p->lock);
	#endif
}


void
pool_debug(struct pool_ctl *p, char *name)
{
	p->debug = 1;
	strlcpy(p->name, name, POOL_DEBUG_NAME_SZ);
}


static struct pool_mem *
get_mem_block(struct pool_ctl *pool)
{
	struct pool_mem *block;

	block = (struct pool_mem *)malloc(sizeof(struct pool_mem));
	if (block == NULL)
		return NULL;

	memset(block, 0, sizeof(*block));

	// ToDo: B_CONTIGUOUS for what???
	block->aid = create_area("some pool block", (void **)&block->base_addr,
		B_ANY_KERNEL_ADDRESS, pool->block_size, B_CONTIGUOUS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (block->aid < 0) {
		free(block);
		return NULL;
	}

	block->mem_size = block->avail = pool->block_size;
	block->ptr = block->base_addr;
	INIT_BENAPHORE(block->lock, "pool_mem_lock");

	if (CHECK_BENAPHORE(block->lock) >= 0) {
		#if POOL_USES_BENAPHORES
			ACQUIRE_BENAPHORE(pool->lock);
		#else
			ACQUIRE_WRITE_LOCK(pool->lock);
		#endif

		// insert block at the beginning of the pools
		block->next = pool->list;
		pool->list = block;

#ifdef WALK_POOL_LIST
		walk_pool_list(pool);
#endif

		#if POOL_USES_BENAPHORES
			RELEASE_BENAPHORE(pool->lock);
		#else
			RELEASE_WRITE_LOCK(pool->lock);
		#endif

		return block;
	}
	UNINIT_BENAPHORE(block->lock);

	delete_area(block->aid);
	free(block);

	return NULL;
}


int32
pool_init(struct pool_ctl **_newPool, size_t size)
{
	struct pool_ctl *pool = NULL;
	
	/* if the init failes, the new pool will be set to NULL */
	*_newPool = NULL;

	/* minimum block size is sizeof the free_blk structure */
	if (size < sizeof(struct free_blk)) 
		size = sizeof(struct free_blk);

	pool = (struct pool_ctl *)malloc(sizeof(struct pool_ctl));
	if (pool == NULL)
		return ENOMEM;

	memset(pool, 0, sizeof(*pool));
	
	#if POOL_USES_BENAPHORES
		INIT_BENAPHORE(pool->lock, "pool_lock");
		if (CHECK_BENAPHORE(pool->lock) < 0) {
			free(pool);
			return ENOLCK;
		}
	#else
		INIT_RW_LOCK(pool->lock, "pool_lock");
		if (CHECK_RW_LOCK(pool->lock) < 0) {
			free(pool);
			return ENOLCK;
		}
	#endif

	// 4 puddles will always fit in one pool
	pool->block_size = ROUND_TO_PAGE_SIZE(size * 8);
	pool->alloc_size = size;
	pool->list = NULL;
	pool->freelist = NULL;

	/* now add a first block */
	get_mem_block(pool);
	if (!pool->list) {
		#if POOL_USES_BENAPHORES
			UNINIT_BENAPHORE(pool->lock);
		#else
			UNINIT_RW_LOCK(pool->lock);
		#endif
		free(pool);
		return ENOMEM;
	}

	*_newPool = pool;
	return 0;
}


void *
pool_get(struct pool_ctl *p)
{
	/* ok, so now we look for a suitable block... */
	struct pool_mem *mp = p->list;
	struct free_blk *rv = NULL;

	#if POOL_USES_BENAPHORES
		ACQUIRE_BENAPHORE(p->lock);
	#else
		ACQUIRE_WRITE_LOCK(p->lock);
	#endif

	if (p->freelist) {
		/* woohoo, just grab a block! */

		rv = p->freelist;
		ASSERT(rv->magic == FREE_MAGIC);
		ASSERT(FREE_MAGIC + (uint32)rv->next == rv->magic_check);

		if (p->debug)
			dprintf("%s: allocating %p, setting freelist to %p\n",
				p->name, p->freelist, rv->next);

		p->freelist = rv->next;

		#if POOL_USES_BENAPHORES
			RELEASE_BENAPHORE(p->lock);
		#else
			RELEASE_WRITE_LOCK(p->lock);
		#endif
		
		memset(rv, 0, p->alloc_size);
		return rv;
	}		
	#if !POOL_USES_BENAPHORES
		RELEASE_WRITE_LOCK(p->lock);
		ACQUIRE_READ_LOCK(p->lock);
	#endif

	/* no free blocks, try to allocate of the top of the memory blocks
	** we must hold the global pool lock while iterating through the list!
	*/

	do {
		ACQUIRE_BENAPHORE(mp->lock);

		if (mp->avail >= p->alloc_size) {
			rv = (struct free_blk *)mp->ptr;
			mp->ptr += p->alloc_size;
			mp->avail -= p->alloc_size;
			RELEASE_BENAPHORE(mp->lock);
			break;
		}
		RELEASE_BENAPHORE(mp->lock);
	} while ((mp = mp->next) != NULL);

	#if POOL_USES_BENAPHORES
		RELEASE_BENAPHORE(p->lock);
	#else
		RELEASE_READ_LOCK(p->lock);
	#endif

	if (rv) {
		memset(rv, 0, p->alloc_size);
		return rv;
	}

	mp = get_mem_block(p);
	if (mp == NULL)
		return NULL;

	ACQUIRE_BENAPHORE(mp->lock);

	if (mp->avail >= p->alloc_size) {
		rv = (struct free_blk *)mp->ptr;
		mp->ptr += p->alloc_size;
		mp->avail -= p->alloc_size;
	}
	RELEASE_BENAPHORE(mp->lock);

	memset(rv, 0, p->alloc_size);
	return rv;
}


void
pool_put(struct pool_ctl *p, void *ptr)
{
	#if POOL_USES_BENAPHORES
		ACQUIRE_BENAPHORE(p->lock);
	#else
		ACQUIRE_WRITE_LOCK(p->lock);
	#endif
	
	memset(ptr, 0, p->alloc_size);

	((struct free_blk*)ptr)->next = p->freelist;
	((struct free_blk*)ptr)->magic = FREE_MAGIC;
	((struct free_blk*)ptr)->magic_check = FREE_MAGIC + (uint32)p->freelist;

	if (p->debug) {
		dprintf("%s: adding %p, setting next = %p\n",
			p->name, ptr, p->freelist);
	}

	p->freelist = ptr;

	if (p->debug)
		dprintf("%s: freelist = %p\n", p->name, p->freelist);

	#if POOL_USES_BENAPHORES
		RELEASE_BENAPHORE(p->lock);
	#else
		RELEASE_WRITE_LOCK(p->lock);
	#endif
}


void
pool_destroy(struct pool_ctl *p)
{
	struct pool_mem *mp,*temp;

	if (p == NULL)
		return;

	/* the semaphore will be deleted, so we don't have to unlock */
	ACQUIRE_WRITE_LOCK(p->lock);

	mp = p->list;
	while (mp != NULL) {
		delete_area(mp->aid);
		temp = mp;
		mp = mp->next;
		UNINIT_BENAPHORE(mp->lock);
		free(temp);
	}

	#if POOL_USES_BENAPHORES
		UNINIT_BENAPHORE(p->lock);
	#else
		UNINIT_RW_LOCK(p->lock);
	#endif
	free(p);
}
