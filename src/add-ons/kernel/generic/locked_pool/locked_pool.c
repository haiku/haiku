/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

/*!	Deadlock-safe allocation of locked memory.

	Allocation/freeing is optimized for speed. Count of <sem>
	is the number of free blocks and thus should be modified
	by each alloc() and free(). As this count is only crucial if
	an allocation is waiting for a free block, <sem> is only
	updated on demand - the correct number of free blocks is
	stored in <free_blocks>. There are only three cases where
	<sem> is updated:

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
	struct mutex mutex;			// to be used whenever some variable of the first
								// block of this structure is read or modified
	int free_blocks;			// # free blocks
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
	area_id area;				// underlying area
	int num_blocks;				// size in blocks
} chunk_header;


// global list of pools
static locked_pool *sLockedPools;
// mutex to protect sLockedPools
static mutex sLockedPoolsLock;
// true, if thread should shut down
static bool sShuttingDown;
// background thread to enlarge pools
static thread_id sEnlargerThread;
// semaphore to wake up enlarger thread
static sem_id sEnlargerSemaphore;

// macro to access next-pointer in free block
#define NEXT_PTR(pool, a) ((void **)(((char *)a) + pool->next_ofs))


/*! Enlarge memory pool by <num_block> blocks */
static status_t
enlarge_pool(locked_pool *pool, int numBlocks)
{
	void **next;
	int i;
	int numWaiting;
	status_t status;
	area_id area;
	chunk_header *chunk;
	size_t chunkSize;
	void *block, *lastBlock;

	TRACE(("enlarge_pool()\n"));

	// get memory directly from VM; we don't let user code access memory
	chunkSize = numBlocks * pool->block_size + pool->header_size;
	chunkSize = (chunkSize + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);

	status = area = create_area(pool->name,
		(void **)&chunk, B_ANY_KERNEL_ADDRESS, chunkSize,
		pool->lock_flags, 0);
	if (status < B_OK) {
		dprintf("cannot enlarge pool (%s)\n", strerror(status));
		// TODO: we should wait a bit and try again!
		return status;
	}

	chunk->area = area;
	chunk->num_blocks = numBlocks;

	// create free_list and call add-hook
	// very important: we first create a freelist within the chunk,
	// going from lower to higher addresses; at the end of the loop,
	// "next" points to the head of the list and "lastBlock" to the
	// last list node!
	next = NULL;

	lastBlock = (char *)chunk + pool->header_size +
		(numBlocks-1) * pool->block_size;

	for (i = 0, block = lastBlock; i < numBlocks;
		 ++i, block = (char *)block - pool->block_size)
	{
		if (pool->add_hook) {
			if ((status = pool->add_hook(block, pool->hook_arg)) < B_OK)
				break;
		}

		*NEXT_PTR(pool, block) = next;
		next = block;
	}

	if (i < numBlocks) {
		// ups - pre-init failed somehow
		// call remove-hook for blocks that we called add-hook for
		int j;

		for (block = lastBlock, j = 0; j < i; ++j,
				block = (char *)block - pool->block_size) {
			if (pool->remove_hook)
				pool->remove_hook(block, pool->hook_arg);
		}

		// destroy area and give up
		delete_area(chunk->area);

		return status;
	}

	// add new blocks to pool
	mutex_lock(&pool->mutex);

	// see remarks about initialising list within chunk
	*NEXT_PTR(pool, lastBlock) = pool->free_list;
	pool->free_list = next;

	chunk->next = pool->chunks;
	pool->chunks = chunk;

	pool->num_blocks += numBlocks;
	pool->free_blocks += numBlocks;

	TRACE(("done - num_blocks=%d, free_blocks=%d, num_waiting=%d\n",
		pool->num_blocks, pool->free_blocks, pool->num_waiting));

	numWaiting = min_c(pool->num_waiting, numBlocks);
	pool->num_waiting -= numWaiting;

	mutex_unlock(&pool->mutex);

	// release threads that wait for empty blocks
	release_sem_etc(pool->sem, numWaiting, 0);

	return B_OK;
}


/*! Background thread that adjusts pool size */
static int32
enlarger_thread(void *arg)
{
	while (1) {
		locked_pool *pool;

		acquire_sem(sEnlargerSemaphore);

		if (sShuttingDown)
			break;

		// protect traversing of global list and
		// block destroy_pool() to not clean up a pool we are enlarging
		mutex_lock(&sLockedPoolsLock);

		for (pool = sLockedPools; pool; pool = pool->next) {
			int num_free;

			// this mutex is probably not necessary (at least on 80x86)
			// but I'm not sure about atomicity of other architectures
			// (anyway - this routine is not performance critical)
			mutex_lock(&pool->mutex);
			num_free = pool->free_blocks;
			mutex_unlock(&pool->mutex);

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

		mutex_unlock(&sLockedPoolsLock);
	}

	return 0;
}


/*! Free all chunks belonging to pool */
static void
free_chunks(locked_pool *pool)
{
	chunk_header *chunk, *next;

	for (chunk = pool->chunks; chunk; chunk = next) {
		int i;
		void *block, *lastBlock;

		next = chunk->next;

		lastBlock = (char *)chunk + pool->header_size +
			(chunk->num_blocks-1) * pool->block_size;

		// don't forget to call remove-hook
		for (i = 0, block = lastBlock; i < pool->num_blocks; ++i, block = (char *)block - pool->block_size) {
			if (pool->remove_hook)
				pool->remove_hook(block, pool->hook_arg);
		}

		delete_area(chunk->area);
	}

	pool->chunks = NULL;
}


/*! Global init, executed when module is loaded */
static status_t
init_locked_pool(void)
{
	status_t status;

	mutex_init(&sLockedPoolsLock, "locked_pool_global_list");

	status = sEnlargerSemaphore = create_sem(0,
		"locked_pool_enlarger");
	if (status < B_OK)
		goto err2;

	sLockedPools = NULL;
	sShuttingDown = false;

	status = sEnlargerThread = spawn_kernel_thread(enlarger_thread,
		"locked_pool_enlarger", B_NORMAL_PRIORITY, NULL);
	if (status < B_OK)
		goto err3;

	resume_thread(sEnlargerThread);
	return B_OK;

err3:
	delete_sem(sEnlargerSemaphore);
err2:
	mutex_destroy(&sLockedPoolsLock);
	return status;
}


/*! Global uninit, executed before module is unloaded */
static status_t
uninit_locked_pool(void)
{
	sShuttingDown = true;

	release_sem(sEnlargerSemaphore);

	wait_for_thread(sEnlargerThread, NULL);

	delete_sem(sEnlargerSemaphore);
	mutex_destroy(&sLockedPoolsLock);

	return B_OK;
}


//	#pragma mark - Module API


/*! Alloc memory from pool */
static void *
pool_alloc(locked_pool *pool)
{
	void *block;

	TRACE(("pool_alloc()\n"));

	mutex_lock(&pool->mutex);

	--pool->free_blocks;

	if (pool->free_blocks > 0) {
		// there are free blocks - grab one

		TRACE(("freeblocks=%d, free_list=%p\n",
			pool->free_blocks, pool->free_list));

		block = pool->free_list;
		pool->free_list = *NEXT_PTR(pool, block);

		TRACE(("new free_list=%p\n", pool->free_list));

		mutex_unlock(&pool->mutex);
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

	TRACE(("%d waiting allocs\n", pool->num_waiting));

	mutex_unlock(&pool->mutex);

	// awake background thread
	release_sem_etc(sEnlargerSemaphore, 1, B_DO_NOT_RESCHEDULE);
	// make samphore up-to-date and wait until a block is available
	acquire_sem(pool->sem);

	mutex_lock(&pool->mutex);

	TRACE(("continuing alloc (%d free blocks)\n", pool->free_blocks));

	block = pool->free_list;
	pool->free_list = *NEXT_PTR(pool, block);

	mutex_unlock(&pool->mutex);
	return block;
}


static void
pool_free(locked_pool *pool, void *block)
{
	TRACE(("pool_free()\n"));

	mutex_lock(&pool->mutex);

	// add to free list
	*NEXT_PTR(pool, block) = pool->free_list;
	pool->free_list = block;

	++pool->free_blocks;

	TRACE(("freeblocks=%d, free_list=%p\n", pool->free_blocks,
		pool->free_list));

	if (pool->num_waiting == 0) {
		// if no one is waiting, this is it
		mutex_unlock(&pool->mutex);
		return;
	}

	// someone is waiting on the semaphore

	TRACE(("%d waiting allocs\n", pool->num_waiting));
	pool->num_waiting--;

	mutex_unlock(&pool->mutex);

	// now it is up-to-date and waiting allocations can be continued
	release_sem(pool->sem);
	return;
}


static locked_pool *
create_pool(int block_size, int alignment, int next_ofs,
	int chunkSize, int max_blocks, int min_free_blocks,
	const char *name, uint32 lock_flags,
	locked_pool_add_hook add_hook,
	locked_pool_remove_hook remove_hook, void *hook_arg)
{
	locked_pool *pool;
	status_t status;

	TRACE(("create_pool()\n"));

	pool = (locked_pool *)malloc(sizeof(*pool));
	if (pool == NULL)
		return NULL;

	memset(pool, 0, sizeof(*pool));

	mutex_init(&pool->mutex, "locked_pool");

	if ((status = pool->sem = create_sem(0, "locked_pool")) < 0)
		goto err1;

	if ((pool->name = strdup(name)) == NULL) {
		status = B_NO_MEMORY;
		goto err3;
	}

	pool->alignment = alignment;

	// take care that there is always enough space to fulfill alignment
	pool->block_size = (block_size + pool->alignment) & ~pool->alignment;

	pool->next_ofs = next_ofs;
	pool->lock_flags = lock_flags;

	pool->header_size = max((sizeof( chunk_header ) + pool->alignment) & ~pool->alignment,
		pool->alignment + 1);

	pool->enlarge_by = (((chunkSize + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1)) - pool->header_size)
		/ pool->block_size;

	pool->max_blocks = max_blocks;
	pool->min_free_blocks = min_free_blocks;
	pool->free_blocks = 0;
	pool->num_blocks = 0;
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
		if ((status = enlarge_pool(pool, min(pool->enlarge_by, pool->max_blocks))) < 0)
			goto err4;
	}

	// add to global list, so enlarger thread takes care of pool
	mutex_lock(&sLockedPoolsLock);
	ADD_DL_LIST_HEAD(pool, sLockedPools, );
	mutex_unlock(&sLockedPoolsLock);

	return pool;

err4:
	free(pool->name);
err3:
	delete_sem(pool->sem);
err1:
	mutex_destroy(&pool->mutex);
	free(pool);
	return NULL;
}


static void
destroy_pool(locked_pool *pool)
{
	TRACE(("destroy_pool()\n"));

	// first, remove from global list, so enlarger thread
	// won't touch this pool anymore
	mutex_lock(&sLockedPoolsLock);
	REMOVE_DL_LIST(pool, sLockedPools, );
	mutex_unlock(&sLockedPoolsLock);

	// then cleanup pool
	free_chunks(pool);

	free(pool->name);
	delete_sem(pool->sem);
	mutex_destroy(&pool->mutex);

	free(pool);
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


module_info *modules[] = {
	&interface.minfo,
	NULL
};
