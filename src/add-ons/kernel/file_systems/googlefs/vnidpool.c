/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */

#include <malloc.h>
#include <string.h>
#include <KernelExport.h>
#include "vnidpool.h"

/* primary type for the bitmap */
#define BMT uint32

#define BM_BYTE(p, vnid) (p->bitmap[(vnid % p->bmsize) / sizeof(BMT)])
#define BM_ISSET(p, vnid) (BM_BYTE(p, vnid) & (1 << (p->nextvnid % sizeof(BMT))))
#define BM_SET(p, vnid) (BM_BYTE(p, vnid) |= (1 << (p->nextvnid % sizeof(BMT))))
#define BM_UNSET(p, vnid) (BM_BYTE(p, vnid) &= ~(1 << (p->nextvnid % sizeof(BMT))))

status_t vnidpool_alloc(struct vnidpool **pool, size_t size)
{
	struct vnidpool *p;
	if (size < 2)
		return EINVAL;
	if (!pool)
		return EINVAL;
	size = (size + sizeof(BMT) - 1) / sizeof(BMT);
	size *= sizeof(BMT);
	p = malloc(sizeof(struct vnidpool) + size / sizeof(BMT));
	if (!p)
		return B_NO_MEMORY;
	new_lock(&p->lock, "vnidpool lock");
	p->nextvnid = 1LL;
	p->bitmap = (BMT *)(p + 1);
	p->bmsize = size;
	memset(p->bitmap, 0, size / sizeof(BMT));
	dprintf("vnidpool_alloc: pool @ %p, bitmap @ %p, size %ld\n", p, p->bitmap, p->bmsize);
	*pool = p;
	return B_OK;
}

status_t vnidpool_free(struct vnidpool *pool) {
	unsigned int i;
	dprintf("vnidpool_free: pool @ %p\n", pool);
	if (!pool)
		return EINVAL;
	if (LOCK(&pool->lock) < B_OK)
		return B_ERROR;
	/* make sure no vnid is left in use */
	for (i = 0; i < (pool->bmsize % sizeof(BMT)); i++) {
		if (pool->bitmap[i])
			dprintf("WARNING: vnidpool_free called with vnids still in use!!!\n");
			//panic("vnidpool_free: vnids still in use");
	}
	free_lock(&pool->lock);
	free(pool);
	return B_OK;
}

status_t vnidpool_get(struct vnidpool *pool, ino_t *vnid)
{
	status_t err = B_ERROR;
	uint32 i;
	if (!pool)
		return EINVAL;
	if (LOCK(&pool->lock) < B_OK)
		return B_ERROR;
	for (i = 0; BM_ISSET(pool, pool->nextvnid) && i < pool->bmsize; pool->nextvnid++, i++) {
		/* avoid 0 as vnid */
		if (!pool->nextvnid)
			continue;
	}
	if (BM_ISSET(pool, pool->nextvnid))
		err = ENOBUFS;
	else {
		BM_SET(pool, pool->nextvnid);
		*vnid = pool->nextvnid++;
		err = B_OK;
	}
	UNLOCK(&pool->lock);
	return err;
}

status_t vnidpool_put(struct vnidpool *pool, ino_t vnid)
{
	status_t err = B_ERROR;
	if (!pool)
		return EINVAL;
	if (LOCK(&pool->lock) < B_OK)
		return B_ERROR;
	if (!BM_ISSET(pool, vnid))
		err = EINVAL;
	else {
		BM_UNSET(pool, vnid);
		err = B_OK;
	}
	UNLOCK(&pool->lock);
	return err;
}

