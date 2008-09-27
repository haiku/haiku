/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */
#ifndef _VNIDPOOL_H
#define _VNIDPOOL_H

#include "googlefs.h"

typedef struct vnidpool {
	lock lock;
	ino_t nextvnid;
	uint32 *bitmap;
	size_t bmsize;
} vnidpool;

status_t vnidpool_alloc(struct vnidpool **pool, size_t size);
status_t vnidpool_free(struct vnidpool *pool);
status_t vnidpool_get(struct vnidpool *pool, ino_t *vnid);
status_t vnidpool_put(struct vnidpool *pool, ino_t vnid);

#endif /* _VNIDPOOL_H */
