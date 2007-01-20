#ifndef _VNIDPOOL_H
#define _VNIDPOOL_H

#include "googlefs.h"

typedef struct vnidpool {
	lock lock;
	vnode_id nextvnid;
	uint32 *bitmap;
	size_t bmsize;
} vnidpool;

status_t vnidpool_alloc(struct vnidpool **pool, size_t size);
status_t vnidpool_free(struct vnidpool *pool);
status_t vnidpool_get(struct vnidpool *pool, vnode_id *vnid);
status_t vnidpool_put(struct vnidpool *pool, vnode_id vnid);

#endif /* _VNIDPOOL_H */
