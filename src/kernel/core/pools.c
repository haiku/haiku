/* pools.c */

/* XXX - add documentation to this file! */
#include <kernel.h>
#include <OS.h>
#include <pools.h>
#include <vm.h>
#include <memheap.h>
#include <atomic.h>
#include <ktypes.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <debug.h>
#include <pools.h>

static sem_id init_sem = -1;
#define POOL_ALLOC_SZ 4 * 1024
#define ROUND_TO_PAGE_SIZE(x) (((x) + (POOL_ALLOC_SZ) - 1) & ~((POOL_ALLOC_SZ) - 1))

#ifdef WALK_POOL_LIST
void walk_pool_list(struct pool_ctl *p)
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

void pool_debug_walk(struct pool_ctl *p)
{
	char *ptr;
	int i = 1;	
	
	dprintf("%ld byte blocks allocated, but now free:\n\n", p->alloc_size);

	#if POOL_USES_BENAPHORES
		ACQUIRE_BENAPHORE(p->lock);
	#else
		ACQUIRE_READ_LOCK(p->lock);
	#endif
	ptr = p->freelist;	
	while (ptr) {
		dprintf("  %02d: %p\n", i++, ptr);
		ptr = ((struct free_blk*)ptr)->next;
	}
	#if POOL_USES_BENAPHORES
		RELEASE_BENAPHORE(p->lock);
	#else
		RELEASE_READ_LOCK(p->lock);
	#endif
}

void pool_debug(struct pool_ctl *p, char *name)
{
	p->debug = 1;
	if (strlen(name) < POOL_DEBUG_NAME_SZ)
		strncpy(p->name, name, strlen(name));
	else
		strncpy(p->name, name, POOL_DEBUG_NAME_SZ);
}

static struct pool_mem *get_mem_block(struct pool_ctl *pool)
{
	struct pool_mem *block;

	block = (struct pool_mem *)kmalloc(sizeof(struct pool_mem));
	if (block == NULL)
		return NULL;

	memset(block, 0, sizeof(*block));

	block->aid = vm_create_anonymous_region(vm_get_kernel_aspace_id(),
	                    "net_stack_pools_block",
						(void**)&block->base_addr,
						REGION_ADDR_ANY_ADDRESS, pool->block_size,
						REGION_WIRING_WIRED_CONTIG, 
						LOCK_KERNEL|LOCK_RW);
	if (block->aid < 0) {
		kfree(block);
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
		if (pool->list)
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
	
	vm_delete_region(vm_get_kernel_aspace_id(), block->aid);
	kfree(block);

	return NULL;
}


int32 pool_init(struct pool_ctl **_newPool, size_t size)
{
	struct pool_ctl *pool = NULL;

	if (init_sem == -1)
		create_sem(1, "pool_init_sem");

	/* minimum block size is sizeof the free_blk structure */
	if (size < sizeof(struct free_blk)) 
		return EINVAL;

//	acquire_sem_etc(init_sem, 1, B_CAN_INTERRUPT, 0);
	
	pool = (struct pool_ctl*)kmalloc(sizeof(struct pool_ctl));
	if (pool == NULL)
		return ENOMEM;

	memset(pool, 0, sizeof(*pool));
	
	#if POOL_USES_BENAPHORES
		INIT_BENAPHORE(pool->lock, "pool_lock");
		if (CHECK_BENAPHORE(pool->lock) < 0) {
			kfree(pool);
			return ENOLCK;
		}
	#else
		INIT_RW_LOCK(pool->lock, "pool_lock");
		if (CHECK_RW_LOCK(pool->lock) < 0) {
			kfree(pool);
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
		kfree(pool);
		return ENOMEM;
	}

	*_newPool = pool;

//	release_sem_etc(init_sem, 1, B_CAN_INTERRUPT);	
	return 0;
}


char *pool_get(struct pool_ctl *p)
{
	/* ok, so now we look for a suitable block... */
	struct pool_mem *mp = p->list;
	char *rv = NULL;

	#if POOL_USES_BENAPHORES
		ACQUIRE_BENAPHORE(p->lock);
	#else
		ACQUIRE_WRITE_LOCK(p->lock);
	#endif

	if (p->freelist) {
		/* woohoo, just grab a block! */

		rv = p->freelist;

		if (p->debug)
			dprintf("%s: allocating %p, setting freelist to %p\n",
				p->name, p->freelist, 
				((struct free_blk*)rv)->next);

		p->freelist = ((struct free_blk*)rv)->next;

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
			rv = mp->ptr;
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
		rv = mp->ptr;
		mp->ptr += p->alloc_size;
		mp->avail -= p->alloc_size;
	}
	RELEASE_BENAPHORE(mp->lock);

	memset(rv, 0, p->alloc_size);
	return rv;
}


void pool_put(struct pool_ctl *p, void *ptr)
{
	#if POOL_USES_BENAPHORES
		ACQUIRE_BENAPHORE(p->lock);
	#else
		ACQUIRE_WRITE_LOCK(p->lock);
	#endif
	
	memset(ptr, 0, p->alloc_size);
	((struct free_blk*)ptr)->next = p->freelist;

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


void pool_destroy(struct pool_ctl *p)
{
	struct pool_mem *mp,*temp;

	if (p == NULL)
		return;

	/* the semaphore will be deleted, so we don't have to unlock */
	ACQUIRE_WRITE_LOCK(p->lock);

	mp = p->list;
	while (mp != NULL) {
		vm_delete_region(vm_get_kernel_aspace_id(), mp->aid);
		temp = mp;
		mp = mp->next;
		UNINIT_BENAPHORE(mp->lock);
		kfree(temp);
	}

	#if POOL_USES_BENAPHORES
		UNINIT_BENAPHORE(p->lock);
	#else
		UNINIT_RW_LOCK(p->lock);
	#endif
	kfree(p);
}
