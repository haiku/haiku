/*
** Copyright 2002/03, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	Deadlock-safe allocation of locked memory.

	Allocation/freeing is optimized for speed. Count of <sem>
	is the number of free blocks and thus should be modified 
	by each alloc() and free(). As this count is only crucial if
	an allocation is waiting for a free block, <sem> is only
	updated on demand - the correct number of free blocks is 
	stored in <free_blocks> and the difference between <free_blocks>
	and the semaphore count is stored in <diff_locks>. There
	are only three cases where <sem> is updated:

	- if an	allocation fails because there is no free block left;
	  in this case, <num_waiting> increased by one and then the 
	  thread makes <sem> up-to-date and waits for a free block 
	  via <sem> in one step; finally, <num_waiting> is decreased
	  by one
	- if a block is freed and <num_waiting> is non-zero;
	  here, count of <sem> is updated to release threads waiting 
	  for allocation
	- if a new chunk of blocks is allocated;
	  same as previous case
*/


#include <KernelExport.h>
#include <drivers/locked_pool.h>
#include <lock.h>
#include "dl_list.h"

#include <string.h>
#include <module.h>
#include <malloc.h>


//#define TRACE_LOCKED_POOL
#ifdef TRACE_LOCKED_POOL
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


// info about pool
typedef struct locked_pool {
	benaphore mutex;			// to be used whenever some variable of the first
								// block of this structure is read or modified
	int free_blocks;			// # free blocks
	int diff_locks;				// # how often <sem> must be acquired
	int num_waiting;			// # waiting allocations
	void *free_list;			// list of free blocks
	int next_ofs;				// offset of next-pointer in block

	sem_id sem;					// count=number of free blocks
	char *name;
	size_t header_size;			// effective size of chunk header
	struct chunk_header *chunks;// list of chunks
	size_t block_size;			// size of memory block
	uint32 lock_flags;			// flags for lock_memory()
	int min_free_blocks;		// min. number of free blocks
	int num_blocks;				// cur. number of blocks
	int max_blocks;				// maximum number of blocks
	int enlarge_by;				// number of blocks to enlarge pool by
	size_t alignment;			// block alignment restrictions
	locked_pool_add_hook add_hook;		// user hooks
	locked_pool_remove_hook remove_hook;
	void *hook_arg;						// arg for user hooks
	struct locked_pool *prev, *next;	// global cyclic list
} locked_pool;


// header of memory chunk
typedef struct chunk_header {
	struct chunk_header *next;	// free-list
	area_id region;				// underlying region
	int num_blocks;				// size in blocks
} chunk_header;


// global list of pools
static locked_pool *locked_pools;
// benaphore to protect locked_pools
static benaphore locked_pool_ben;
// true, if thread should shut down
static bool locked_pool_shutting_down;
// background thread to enlarge pools
static thread_id locked_pool_enlarger_thread;
// semaphore to wake up enlarger thread
static sem_id locked_pool_enlarger_sem;

// macro to access next-pointer in free block
#define NEXT_PTR(pool, a) ((void **)(((char *)a) + pool->next_ofs))


/** public: alloc memory from pool */

static void *
pool_alloc(locked_pool *pool)
{
	void *block;
	int num_to_lock;

	TRACE(("pool_alloc()\n"));

	benaphore_lock(&pool->mutex);

	--pool->free_blocks;

	if (pool->free_blocks > 0) {
		// there are free blocks - grab one

		// increase difference between allocations executed and those
		// reflected in pool semaphore
		++pool->diff_locks;

		TRACE(("freeblocks=%d, diff_locks=%d, free_list=%p\n",
			pool->free_blocks, pool->diff_locks, pool->free_list));

		block = pool->free_list;
		pool->free_list = *NEXT_PTR(pool, block);
		
		TRACE(("new free_list=%p\n", pool->free_list));

		benaphore_unlock(&pool->mutex);
		return block;
	}

	// entire pool is in use
	// we should do a ++free_blocks here, but this can lead to race
	// condition: when we wait for <sem> and a block gets released
	// and another thread calls alloc() before we grab the freshly freed
	// block, the other thread could overtake us and grab the free block
	// instead! by leaving free_block at a negative value, the other
	// thread cannot see the free block and thus will leave it for us

	// tell them we are waiting on semaphore
	++pool->num_waiting;

	// make pool semaphore up-to-date; 
	// add one as we want to allocate a block
	num_to_lock = pool->diff_locks+1;
	pool->diff_locks = 0;

	TRACE(("locking %d times (%d waiting allocs)\n", num_to_lock, pool->num_waiting));

	benaphore_unlock(&pool->mutex);

	// awake background thread
	release_sem_etc(locked_pool_enlarger_sem, 1, B_DO_NOT_RESCHEDULE);
	// make samphore up-to-date and wait until a block is available
	acquire_sem_etc(pool->sem, num_to_lock, 0, 0);

	benaphore_lock(&pool->mutex);

	// tell them that we don't wait on semaphore anymore	
	--pool->num_waiting;

	TRACE(("continuing alloc (%d free blocks)\n", pool->free_blocks));

	block = pool->free_list;
	pool->free_list = *NEXT_PTR(pool, block);

	benaphore_unlock(&pool->mutex);
	return block;
}


/** public: free block */

static void
pool_free(locked_pool *pool, void *block)
{
	int num_to_unlock;

	TRACE(("pool_free()\n"));

	benaphore_lock(&pool->mutex);

	// add to free list	
	*NEXT_PTR(pool, block) = pool->free_list;
	pool->free_list = block;

	++pool->free_blocks;

	// increase difference between allocation executed and those
	// reflected in pool semaphore
	--pool->diff_locks;

	TRACE(("freeblocks=%d, diff_locks=%d, free_list=%p\n",
		pool->free_blocks, pool->diff_locks, pool->free_list));

	if (pool->num_waiting == 0) {
		// if noone is waiting, this is it
		//SHOW_FLOW0( 3, "leaving" );
		benaphore_unlock(&pool->mutex);
		//SHOW_FLOW0( 3, "done" );
		return;
	}

	// someone is waiting on semaphore, so we have to make it up-to-date
	num_to_unlock = -pool->diff_locks;
	pool->diff_locks = 0;

	TRACE(("unlocking %d times (%d waiting allocs)\n",
		num_to_unlock, pool->num_waiting));

	benaphore_unlock(&pool->mutex);

	// now it is up-to-date and waiting allocations can be continued
	release_sem_etc(pool->sem, num_to_unlock, 0);
	return;
}


/** enlarge memory pool by <num_block> blocks */

static status_t
enlarge_pool(locked_pool *pool, int num_blocks)
{
	void **next;
	int i;
	status_t res;
	area_id region;
	chunk_header *chunk;
	size_t chunk_size;
	int num_to_unlock;
	void *block, *last_block;
	
	TRACE(("enlarge_pool()\n"));

	// get memory directly from VM; we don't let user code access memory
	chunk_size = num_blocks * pool->block_size + pool->header_size;
	chunk_size = (chunk_size + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);

	res = region = create_area(pool->name,
		(void **)&chunk, B_ANY_KERNEL_ADDRESS, chunk_size,
		pool->lock_flags, 0);
	if (res < 0) {
		dprintf("cannot enlarge pool (%s)\n", strerror(res));
		return res;
	}

	chunk->region = region;
	chunk->num_blocks = num_blocks;

	// create free_list and call add-hook
	// very important: we first create a freelist within the chunk,
	// going from lower to higher addresses; at the end of the loop,
	// "next" points to the head of the list and "last_block" to the
	// last list node!
	next = NULL;

	last_block = (char *)chunk + pool->header_size + 
		(num_blocks-1) * pool->block_size;

	for (i = 0, block = last_block; i < num_blocks;
		 ++i, block = (char *)block - pool->block_size) 
	{
		if (pool->add_hook) {
			if ((res = pool->add_hook(block, pool->hook_arg)) < 0)
				break;
		}

		*NEXT_PTR(pool, block) = next;		
		next = block;
	}

	if (i < num_blocks) {
		// ups - pre-init failed somehow
		// call remove-hook for blocks that we called add-hook for
		int j;

		for (block = last_block, j = 0; j < i; ++j, block = (char *)block - pool->block_size) {
			if (pool->remove_hook)
				pool->remove_hook(block, pool->hook_arg);
		}

		// destroy area and give up		
		delete_area(chunk->region);

		return res;
	}

	// add new blocks to pool
	benaphore_lock(&pool->mutex);

	// see remarks about initialising list within chunk	
	*NEXT_PTR(pool, last_block) = pool->free_list;
	pool->free_list = next;

	chunk->next = pool->chunks;
	pool->chunks = chunk;

	pool->num_blocks += num_blocks;

	pool->free_blocks += num_blocks;
	pool->diff_locks -= num_blocks;

	// update sem instantly (we could check waiting flag whether updating
	// is really necessary, but we don't go for speed here)
	num_to_unlock = -pool->diff_locks;
	pool->diff_locks = 0;

	TRACE(("done - num_blocks=%d, free_blocks=%d, num_waiting=%d, num_to_unlock=%d\n",
		pool->num_blocks, pool->free_blocks, pool->num_waiting, num_to_unlock));

	benaphore_unlock(&pool->mutex);

	// bring sem up-to-date, releasing threads that wait for empty blocks	
	release_sem_etc(pool->sem, num_to_unlock, 0);

	return B_OK;
}


/** background thread that adjusts pool size */

static int32
enlarger_threadproc(void *arg)
{	
	while (1) {
		locked_pool *pool;

		acquire_sem(locked_pool_enlarger_sem);

		if (locked_pool_shutting_down)
			break;

		// protect traversing of global list and
		// block destroy_pool() to not clean up a pool we are enlarging
		benaphore_lock(&locked_pool_ben);

		for (pool = locked_pools; pool; pool = pool->next) {
			int num_free;

			// this mutex is probably not necessary (at least on 80x86)
			// but I'm not sure about atomicity of other architectures
			// (anyway - this routine is not performance critical)
			benaphore_lock(&pool->mutex);
			num_free = pool->free_blocks;
			benaphore_unlock(&pool->mutex);

			// perhaps blocks got freed meanwhile, i.e. pool is large enough
			if (num_free > pool->min_free_blocks)
				continue;

			// enlarge pool as much as possible
			// never create more blocks then defined - the caller may have
			// a good reason for choosing the limit
			if (pool->num_blocks < pool->max_blocks) {
				enlarge_pool(pool, 
					min(pool->enlarge_by, pool->max_blocks - pool->num_blocks));
			}
		}

		benaphore_unlock(&locked_pool_ben);
	}

	return 0;
}


/** free all chunks belonging to pool */

static void
free_chunks(locked_pool *pool)
{
	chunk_header *chunk, *next;

	for (chunk = pool->chunks; chunk; chunk = next) {
		int i;
		void *block, *last_block;

		next = chunk->next;

		last_block = (char *)chunk + pool->header_size + 
			(chunk->num_blocks-1) * pool->block_size;

		// don't forget to call remove-hook		
		for (i = 0, block = last_block; i < pool->num_blocks; ++i, block = (char *)block - pool->block_size) {
			if (pool->remove_hook)
				pool->remove_hook(block, pool->hook_arg);
		}

		delete_area(chunk->region);
	}

	pool->chunks = NULL;
}


/** public: create pool */

static locked_pool *
create_pool(int block_size, int alignment, int next_ofs,
	int chunk_size, int max_blocks, int min_free_blocks, 
	const char *name, uint32 lock_flags,
	locked_pool_add_hook add_hook, 
	locked_pool_remove_hook remove_hook, void *hook_arg)
{
	locked_pool *pool;
	status_t res;
	
	TRACE(("create_pool()\n"));

	pool = (locked_pool *)malloc(sizeof(*pool));
	if (pool == NULL)
		return NULL;

	memset(pool, sizeof(*pool), 0);

	if ((res = benaphore_init(&pool->mutex, "locked_pool")) < 0)
		goto err;

	if ((res = pool->sem = create_sem(0, "locked_pool")) < 0)
		goto err1;

	if ((pool->name = strdup(name)) == NULL) {
		res = B_NO_MEMORY;
		goto err3;
	}
	
	pool->alignment = alignment;
	
	// take care that there is always enough space to fulfill alignment	
	pool->block_size = (block_size + pool->alignment) & ~pool->alignment;
	
	pool->next_ofs = next_ofs;
	pool->lock_flags = lock_flags;
	
	pool->header_size = max((sizeof( chunk_header ) + pool->alignment) & ~pool->alignment, 
		pool->alignment + 1);

	pool->enlarge_by = (((chunk_size + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1)) - pool->header_size)
		/ pool->block_size;

	pool->max_blocks = max_blocks;
	pool->min_free_blocks = min_free_blocks;
	pool->free_blocks = 0;
	pool->num_blocks = 0;
	pool->diff_locks = 0;
	pool->num_waiting = 0;
	pool->free_list = NULL;
	pool->add_hook = add_hook;
	pool->remove_hook = remove_hook;
	pool->hook_arg = hook_arg;
	pool->chunks = NULL;

	TRACE(("block_size=%d, alignment=%d, next_ofs=%d, wiring_flags=%d, header_size=%d, enlarge_by=%d\n",
		(int)pool->block_size, (int)pool->alignment, (int)pool->next_ofs,
		(int)pool->lock_flags, (int)pool->header_size, pool->enlarge_by));

	// if there is a minimum size, enlarge pool right now
	if (min_free_blocks > 0) {
		if ((res = enlarge_pool(pool, min(pool->enlarge_by, pool->max_blocks))) < 0)
			goto err4;
	}

	// add to global list, so enlarger thread takes care of pool
	benaphore_lock(&locked_pool_ben);
	ADD_DL_LIST_HEAD(pool, locked_pools, );
	benaphore_unlock(&locked_pool_ben);

	return pool;

err4:
	free(pool->name);
err3:
	delete_sem(pool->sem);
err1:
	benaphore_destroy(&pool->mutex);
err:
	free(pool);
	return NULL;
}


// public: destroy pool
static void destroy_pool( locked_pool *pool )
{
	TRACE(("destroy_pool()\n"));

	// first, remove from global list, so enlarger thread
	// won't touch this pool anymore
	benaphore_lock(&locked_pool_ben);
	REMOVE_DL_LIST(pool, locked_pools, );
	benaphore_unlock(&locked_pool_ben);

	// then cleanup pool	
	free_chunks(pool);

	free(pool->name);
	delete_sem(pool->sem);
	benaphore_destroy(&pool->mutex);

	free(pool);
}


/** global init, executed when module is loaded */

static status_t
init_locked_pool(void)
{
	status_t res;
	
	if ((res = benaphore_init(&locked_pool_ben, "locked_pool_global_list")) < 0)
		goto err;

	if ((res = locked_pool_enlarger_sem = create_sem(0, "locked_pool_enlarger")) < 0)
		goto err2;

	locked_pools = NULL;
	locked_pool_shutting_down = false;

	if ((res = locked_pool_enlarger_thread = spawn_kernel_thread(enlarger_threadproc,
					"locked_pool_enlarger", B_NORMAL_PRIORITY, NULL)) < 0)
		goto err3;

	resume_thread(locked_pool_enlarger_thread);

	return B_OK;

err3:
	delete_sem(locked_pool_enlarger_sem);
err2:
	benaphore_destroy(&locked_pool_ben);
err:
	return res;
}


/** global uninit, executed before module is unloaded */

static status_t
uninit_locked_pool()
{
	int32 retcode;
	locked_pool_shutting_down = true;
	
	release_sem(locked_pool_enlarger_sem);

	wait_for_thread(locked_pool_enlarger_thread, &retcode);

	delete_sem(locked_pool_enlarger_sem);
	benaphore_destroy(&locked_pool_ben);

	return B_OK;
}


static status_t
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return init_locked_pool();
		case B_MODULE_UNINIT:
			return uninit_locked_pool();

		default:
			return B_ERROR;
	}
}


locked_pool_interface interface = {
	{
		LOCKED_POOL_MODULE_NAME,
		0,
		std_ops
	},

	pool_alloc, 
	pool_free,

	create_pool, 
	destroy_pool
};

#if !_BUILDING_kernel && !BOOT
_EXPORT 
module_info *modules[] = {
	&interface.minfo,
	NULL
};
#endif
