/* Intel PRO/1000 Family Driver
 * Copyright (C) 2004 Marcus Overhagen <marcus@overhagen.de>. All rights reserved.
 */
#ifndef __MEMPOOL_H
#define __MEMPOOL_H

int mempool_init(int count);
void mempool_exit(void);

void *mbuf_pool_get(void);
void *chunk_pool_get(void);

void chunk_pool_put(void *p);
void mbuf_pool_put(void *p);

#endif
