/* pools.h
 * simple fixed size block allocator
 */

#ifndef OBOS_POOLS_H
#define OBOS_POOLS_H


#include <kernel/OS.h>

#include "lock.h"


typedef struct pool_ctl	pool_ctl;

struct pool_mem {
	struct pool_mem *next;
	area_id	aid;
	char *base_addr;
	size_t mem_size;
	char *ptr;
	size_t avail;
	benaphore lock;
};

struct free_blk {
	char *next;
};

#define POOL_USES_BENAPHORES 0
#define POOL_DEBUG_NAME_SZ	32

struct pool_ctl {
	struct pool_mem *list;
	char *freelist;
	size_t alloc_size;
	size_t block_size;
#if POOL_USES_BENAPHORES
	benaphore lock;
#else
	rw_lock lock;
#endif
	int debug;
	char name[POOL_DEBUG_NAME_SZ];
};

status_t pool_init(pool_ctl **p, size_t sz);
char *pool_get(pool_ctl *p);
void pool_put(pool_ctl *p, void *ptr);
void pool_destroy(pool_ctl *p);
void pool_debug(struct pool_ctl *p, char *name);

void pool_debug_walk(pool_ctl *p);

#endif	/* OBOS_POOLS_H */
