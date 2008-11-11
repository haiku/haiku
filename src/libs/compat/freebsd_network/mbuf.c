/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 *
 * Some of this code is based on previous work by Marcus Overhagen.
 *
 * `m_defrag' and friends are straight from FreeBSD 6.2.
 */

#include "device.h"

#include <stdint.h>
#include <string.h>
#include <slab/Slab.h>

#include <compat/sys/mbuf.h>
#include <compat/sys/kernel.h>

static object_cache *sMBufCache;
static object_cache *sChunkCache;
static object_cache *sJumbo9ChunkCache;

int max_linkhdr = 16;
int max_protohdr = 40 + 20; /* ip6 + tcp */

/* max_linkhdr + max_protohdr, but that's not allowed by gcc. */
int max_hdr = 16 + 40 + 20;

static int
m_to_oc_flags(int how)
{
	if (how & M_NOWAIT)
		return CACHE_DONT_SLEEP;

	return 0;
}


static int
construct_mbuf(struct mbuf *mb, short type, int flags)
{
	mb->m_next = NULL;
	mb->m_nextpkt = NULL;
	mb->m_len = 0;
	mb->m_flags = flags;
	mb->m_type = type;

	if (flags & M_PKTHDR) {
		mb->m_data = mb->m_pktdat;
		memset(&mb->m_pkthdr, 0, sizeof(mb->m_pkthdr));
		/* SLIST_INIT(&m->m_pkthdr.tags); */
	} else {
		mb->m_data = mb->m_dat;
	}

	return 0;
}


static int
construct_ext_sized_mbuf(struct mbuf *mb, int how, int size)
{
	object_cache *cache;
	int ext_type;
	if (size != MCLBYTES && size != MJUM9BYTES)
		panic("unsupported size");

	if (size == MCLBYTES) {
		cache = sChunkCache;
		ext_type = EXT_CLUSTER;
	} else {
		cache = sJumbo9ChunkCache;
		ext_type = EXT_JUMBO9;
	}
	mb->m_ext.ext_buf = object_cache_alloc(cache, m_to_oc_flags(how));
	if (mb->m_ext.ext_buf == NULL)
		return B_NO_MEMORY;
	
	mb->m_data = mb->m_ext.ext_buf;
	mb->m_flags |= M_EXT;
	/* mb->m_ext.ext_free = NULL; */
	/* mb->m_ext.ext_args = NULL; */
	mb->m_ext.ext_size = size;
	mb->m_ext.ext_type = ext_type;
	/* mb->m_ext.ref_cnt = NULL; */

        return 0;
}


static inline int
construct_ext_mbuf(struct mbuf *mb, int how)
{
	return construct_ext_sized_mbuf(mb, how, MCLBYTES);
}


static int
construct_pkt_mbuf(int how, struct mbuf *mb, short type, int flags)
{
	construct_mbuf(mb, type, flags);
	if (construct_ext_mbuf(mb, how) < 0)
		return -1;
	mb->m_ext.ext_type |= EXT_PACKET;
	return 0;
}


static void
destruct_pkt_mbuf(struct mbuf *mb)
{
	object_cache *cache;
	if (mb->m_ext.ext_type & EXT_CLUSTER)
		cache = sChunkCache;
	else if (mb->m_ext.ext_type & EXT_JUMBO9)
		cache = sJumbo9ChunkCache;
	else
		panic("unknown cache");
	object_cache_free(cache, mb->m_ext.ext_buf);
	mb->m_ext.ext_buf = NULL;
}


struct mbuf *
m_getcl(int how, short type, int flags)
{
	struct mbuf *mb =
		(struct mbuf *)object_cache_alloc(sMBufCache, m_to_oc_flags(how));
	if (mb == NULL)
		return NULL;

	if (construct_pkt_mbuf(how, mb, type, flags) < 0) {
		object_cache_free(sMBufCache, mb);
		return NULL;
	}

	return mb;
}


static struct mbuf *
_m_get(int how, short type, int flags)
{
	struct mbuf *mb =
		(struct mbuf *)object_cache_alloc(sMBufCache, m_to_oc_flags(how));
	if (mb == NULL)
		return NULL;

	construct_mbuf(mb, type, flags);

	return mb;
}


struct mbuf *
m_get(int how, short type)
{
	return _m_get(how, type, 0);
}


struct mbuf *
m_gethdr(int how, short type)
{
	return _m_get(how, type, M_PKTHDR);
}


void
m_clget(struct mbuf *m, int how)
{
	m->m_ext.ext_buf = NULL;
	/* called checks for errors by looking for M_EXT */
	construct_ext_mbuf(m, how);
}


void *
m_cljget(struct mbuf *m, int how, int size)
{
	if (m == NULL)
		panic("m_cljget doesn't support allocate mbuf");
	m->m_ext.ext_buf = NULL;
	construct_ext_sized_mbuf(m, how, size);
	/* shouldn't be used */
	return NULL;
}


void
m_freem(struct mbuf *mb)
{
	while (mb)
		mb = m_free(mb);
}


static void
mb_free_ext(struct mbuf *m)
{
	/*
	if (m->m_ext.ref_count != NULL)
		panic("unsupported");
		*/

	if (m->m_ext.ext_type == EXT_PACKET)
		destruct_pkt_mbuf(m);
	else if (m->m_ext.ext_type == EXT_CLUSTER) {
		object_cache_free(sChunkCache, m->m_ext.ext_buf);
		m->m_ext.ext_buf = NULL;
	} else if (m->m_ext.ext_type == EXT_JUMBO9) {
		object_cache_free(sJumbo9ChunkCache, m->m_ext.ext_buf);
		m->m_ext.ext_buf = NULL;
	} else
		panic("unknown type");

	object_cache_free(sMBufCache, m);
}


struct mbuf *
m_free(struct mbuf *m)
{
	struct mbuf *next = m->m_next;

	if (m->m_flags & M_EXT)
		mb_free_ext(m);
	else
		object_cache_free(sMBufCache, m);

	return next;
}


void
m_extadd(struct mbuf *m, caddr_t buffer, u_int size,
    void (*freeHook)(void *, void *), void *args, int flags, int type)
{
	// TODO: implement?
	panic("m_extadd() called.");
}


status_t
init_mbufs()
{
	sMBufCache = create_object_cache("mbufs", MSIZE, 8, NULL, NULL, NULL);
	if (sMBufCache == NULL)
		goto clean;
	sChunkCache = create_object_cache("mbuf chunks", MCLBYTES, 0, NULL, NULL,
		NULL);
	if (sChunkCache == NULL)
		goto clean;
	sJumbo9ChunkCache = create_object_cache("mbuf jumbo9 chunks", MJUM9BYTES, 0, NULL, NULL,
		NULL);
	if (sJumbo9ChunkCache == NULL)
		goto clean;
	return B_OK;

clean:
	if (sChunkCache != NULL)
		delete_object_cache(sChunkCache);
	if (sMBufCache != NULL)
		delete_object_cache(sMBufCache);
	return B_NO_MEMORY;
}


void
uninit_mbufs()
{
	delete_object_cache(sMBufCache);
	delete_object_cache(sChunkCache);
	delete_object_cache(sJumbo9ChunkCache);
}

