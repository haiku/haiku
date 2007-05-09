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

int max_protohdr = 40 + 20; /* ip6 + tcp */


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
construct_ext_mbuf(struct mbuf *mb, int how)
{
	mb->m_ext.ext_buf = object_cache_alloc(sChunkCache, m_to_oc_flags(how));
	if (mb->m_ext.ext_buf == NULL)
		return B_NO_MEMORY;

	mb->m_data = mb->m_ext.ext_buf;
	mb->m_flags |= M_EXT;
	/* mb->m_ext.ext_free = NULL; */
	/* mb->m_ext.ext_args = NULL; */
	mb->m_ext.ext_size = MCLBYTES;
	mb->m_ext.ext_type = EXT_CLUSTER;
	/* mb->m_ext.ref_cnt = NULL; */

	return 0;
}


static int
construct_pkt_mbuf(int how, struct mbuf *mb, short type, int flags)
{
	construct_mbuf(mb, type, flags);
	if (construct_ext_mbuf(mb, how) < 0)
		return -1;
	mb->m_ext.ext_type = EXT_PACKET;
	return 0;
}


static void
destruct_pkt_mbuf(struct mbuf *mb)
{
	object_cache_free(sChunkCache, mb->m_ext.ext_buf);
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


status_t
init_mbufs()
{
	sMBufCache = create_object_cache("mbufs", MSIZE, 8, NULL, NULL, NULL);
	if (sMBufCache == NULL)
		return B_NO_MEMORY;

	sChunkCache = create_object_cache("mbuf chunks", MCLBYTES, 0, NULL, NULL,
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

