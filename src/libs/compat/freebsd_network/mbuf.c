/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 *
 * Some of this code is based on previous work by Marcus Overhagen.
 */

#include <stdint.h>
#include <slab/Slab.h>

#include <compat/sys/mbuf.h>


status_t init_mbufs(void);
void uninit_mbufs(void);


#define CHUNK_SIZE	2048

static object_cache *sMBufCache;
static object_cache *sChunkCache;


void
m_freem(struct mbuf *mp)
{
	struct mbuf *next = mp;

	do {
		mp = next;
		next = next->m_next;

		if (mp->m_flags & M_EXT)
			object_cache_free(sChunkCache, mp->m_ext.ext_buf);
		object_cache_free(sMBufCache, mp);
	} while (next);
}


status_t
init_mbufs()
{
	sMBufCache = create_object_cache("mbufs", sizeof(struct mbuf), 8, NULL,
		NULL, NULL);
	if (sMBufCache == NULL)
		return B_NO_MEMORY;

	sChunkCache = create_object_cache("mbuf chunks", CHUNK_SIZE, 0, NULL, NULL,
		NULL);
	if (sChunkCache == NULL) {
		delete_object_cache(sMBufCache);
		return B_NO_MEMORY;
	}

	return B_OK;
}


void
uninit_mbufs()
{
	delete_object_cache(sMBufCache);
	delete_object_cache(sChunkCache);
}

