/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Copyright 2004, Marcus Overhagen. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include "device.h"

#include <stdint.h>
#include <string.h>
#include <slab/Slab.h>

#include <compat/sys/malloc.h>
#include <compat/sys/mbuf.h>
#include <compat/sys/kernel.h>


static object_cache *sMBufCache;
static object_cache *sChunkCache;
static object_cache *sJumbo9ChunkCache;
static object_cache *sJumboPageSizeCache;


int max_linkhdr = 16;
int max_protohdr = 40 + 20; /* ip6 + tcp */

/* max_linkhdr + max_protohdr, but that's not allowed by gcc. */
int max_hdr = 16 + 40 + 20;

/* MHLEN - max_hdr */
int max_datalen = MHLEN - (16 + 40 + 20);


static int
m_to_oc_flags(int how)
{
	if (how & M_NOWAIT)
		return CACHE_DONT_WAIT_FOR_MEMORY;

	return 0;
}


static int
construct_mbuf(struct mbuf *memoryBuffer, short type, int flags)
{
	memoryBuffer->m_next = NULL;
	memoryBuffer->m_nextpkt = NULL;
	memoryBuffer->m_len = 0;
	memoryBuffer->m_flags = flags;
	memoryBuffer->m_type = type;

	if (flags & M_PKTHDR) {
		memoryBuffer->m_data = memoryBuffer->m_pktdat;
		memset(&memoryBuffer->m_pkthdr, 0, sizeof(memoryBuffer->m_pkthdr));
		SLIST_INIT(&memoryBuffer->m_pkthdr.tags);
	} else {
		memoryBuffer->m_data = memoryBuffer->m_dat;
	}

	return 0;
}


static int
construct_ext_sized_mbuf(struct mbuf *memoryBuffer, int how, int size)
{
	object_cache *cache;
	int extType;
	if (size != MCLBYTES && size != MJUM9BYTES && size != MJUMPAGESIZE)
		panic("unsupported size");

	if (size == MCLBYTES) {
		cache = sChunkCache;
		extType = EXT_CLUSTER;
	} else if (size == MJUM9BYTES) {
		cache = sJumbo9ChunkCache;
		extType = EXT_JUMBO9;
	} else {
		cache = sJumboPageSizeCache;
		extType = EXT_JUMBOP;
	}

	memoryBuffer->m_ext.ext_buf = object_cache_alloc(cache, m_to_oc_flags(how));
	if (memoryBuffer->m_ext.ext_buf == NULL)
		return B_NO_MEMORY;

	memoryBuffer->m_data = memoryBuffer->m_ext.ext_buf;
	memoryBuffer->m_flags |= M_EXT;
	memoryBuffer->m_ext.ext_size = size;
	memoryBuffer->m_ext.ext_type = extType;
	memoryBuffer->m_ext.ext_flags = EXT_FLAG_EMBREF;
	memoryBuffer->m_ext.ext_count = 1;

	return 0;
}


static inline int
construct_ext_mbuf(struct mbuf *memoryBuffer, int how)
{
	return construct_ext_sized_mbuf(memoryBuffer, how, MCLBYTES);
}


static int
construct_pkt_mbuf(int how, struct mbuf *memoryBuffer, short type, int flags)
{
	construct_mbuf(memoryBuffer, type, flags);
	if (construct_ext_mbuf(memoryBuffer, how) < 0)
		return -1;
	memoryBuffer->m_ext.ext_type = EXT_CLUSTER;
	return 0;
}


struct mbuf *
m_getcl(int how, short type, int flags)
{
	struct mbuf *memoryBuffer =
		(struct mbuf *)object_cache_alloc(sMBufCache, m_to_oc_flags(how));
	if (memoryBuffer == NULL)
		return NULL;

	if (construct_pkt_mbuf(how, memoryBuffer, type, flags) < 0) {
		object_cache_free(sMBufCache, memoryBuffer, 0);
		return NULL;
	}

	return memoryBuffer;
}


static struct mbuf *
_m_get(int how, short type, int flags)
{
	struct mbuf *memoryBuffer =
		(struct mbuf *)object_cache_alloc(sMBufCache, m_to_oc_flags(how));
	if (memoryBuffer == NULL)
		return NULL;

	construct_mbuf(memoryBuffer, type, flags);

	return memoryBuffer;
}


struct mbuf *
m_get(int how, short type)
{
	return _m_get(how, type, 0);
}


struct mbuf *
m_get2(int size, int how, short type, int flags)
{
	if (size <= MHLEN || (size <= MLEN && (flags & M_PKTHDR) == 0)) {
		size = MCLBYTES;
	} else if (size <= MJUMPAGESIZE) {
		size = MJUMPAGESIZE;
	} else if (size <= MJUM9BYTES) {
		size = MJUM9BYTES;
	} else /* (size > MJUM9BYTES) */ {
		return NULL;
	}

	return m_getjcl(how, type, flags, size);
}


struct mbuf *
m_gethdr(int how, short type)
{
	return _m_get(how, type, M_PKTHDR);
}


struct mbuf *
m_getjcl(int how, short type, int flags, int size)
{
	struct mbuf *memoryBuffer =
		(struct mbuf *)object_cache_alloc(sMBufCache, m_to_oc_flags(how));
	if (memoryBuffer == NULL)
		return NULL;
	construct_mbuf(memoryBuffer, type, flags);
	if (construct_ext_sized_mbuf(memoryBuffer, how, size) < 0) {
		object_cache_free(sMBufCache, memoryBuffer, 0);
		return NULL;
	}
	return memoryBuffer;
}


int
m_clget(struct mbuf *memoryBuffer, int how)
{
	memoryBuffer->m_ext.ext_buf = NULL;
	/* called checks for errors by looking for M_EXT */
	construct_ext_mbuf(memoryBuffer, how);
	return memoryBuffer->m_flags & M_EXT;
}


void *
m_cljget(struct mbuf *memoryBuffer, int how, int size)
{
	if (memoryBuffer == NULL)
		panic("m_cljget doesn't support allocate mbuf");
	memoryBuffer->m_ext.ext_buf = NULL;
	construct_ext_sized_mbuf(memoryBuffer, how, size);
	/* shouldn't be used */
	return NULL;
}


void
m_freem(struct mbuf *memoryBuffer)
{
	while (memoryBuffer)
		memoryBuffer = m_free(memoryBuffer);
}


static void
mb_free_ext(struct mbuf *memoryBuffer)
{
	object_cache *cache = NULL;
	volatile u_int *refcnt;
	struct mbuf *mref;
	int freembuf;

	KASSERT(memoryBuffer->m_flags & M_EXT, ("%s: M_EXT not set on %p",
		__func__, memoryBuffer));

	/* See if this is the mbuf that holds the embedded refcount. */
	if (memoryBuffer->m_ext.ext_flags & EXT_FLAG_EMBREF) {
		refcnt = &memoryBuffer->m_ext.ext_count;
		mref = memoryBuffer;
	} else {
		KASSERT(memoryBuffer->m_ext.ext_cnt != NULL,
			("%s: no refcounting pointer on %p", __func__, memoryBuffer));
		refcnt = memoryBuffer->m_ext.ext_cnt;
		mref = __containerof(refcnt, struct mbuf, m_ext.ext_count);
	}

	/*
	 * Check if the header is embedded in the cluster.  It is
	 * important that we can't touch any of the mbuf fields
	 * after we have freed the external storage, since mbuf
	 * could have been embedded in it.  For now, the mbufs
	 * embedded into the cluster are always of type EXT_EXTREF,
	 * and for this type we won't free the mref.
	 */
	if (memoryBuffer->m_flags & M_NOFREE) {
		freembuf = 0;
		KASSERT(memoryBuffer->m_ext.ext_type == EXT_EXTREF,
			("%s: no-free mbuf %p has wrong type", __func__, memoryBuffer));
	} else
		freembuf = 1;

	/* Free attached storage only if this mbuf is the only reference to it. */
	if (!(*refcnt == 1 || atomic_add(refcnt, -1) == 1)
			&& !(freembuf && memoryBuffer != mref))
		return;

	if (memoryBuffer->m_ext.ext_type == EXT_CLUSTER)
		cache = sChunkCache;
	else if (memoryBuffer->m_ext.ext_type == EXT_JUMBO9)
		cache = sJumbo9ChunkCache;
	else if (memoryBuffer->m_ext.ext_type == EXT_JUMBOP)
		cache = sJumboPageSizeCache;
	else
		panic("unknown type");

	object_cache_free(cache, memoryBuffer->m_ext.ext_buf, 0);
	memoryBuffer->m_ext.ext_buf = NULL;
	object_cache_free(sMBufCache, memoryBuffer, 0);
}


struct mbuf *
m_free(struct mbuf *memoryBuffer)
{
	struct mbuf *next = memoryBuffer->m_next;

	if (memoryBuffer->m_flags & M_EXT)
		mb_free_ext(memoryBuffer);
	else
		object_cache_free(sMBufCache, memoryBuffer, 0);

	return next;
}


void
m_extadd(struct mbuf *memoryBuffer, caddr_t buffer, u_int size,
    void (*freeHook)(void *, void *), void *arg1, void *arg2, int flags, int type)
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
	sJumbo9ChunkCache = create_object_cache("mbuf jumbo9 chunks", MJUM9BYTES, 0,
		NULL, NULL, NULL);
	if (sJumbo9ChunkCache == NULL)
		goto clean;
	sJumboPageSizeCache = create_object_cache("mbuf jumbo page size chunks",
		MJUMPAGESIZE, 0, NULL, NULL, NULL);
	if (sJumboPageSizeCache == NULL)
		goto clean;
	return B_OK;

clean:
	if (sJumbo9ChunkCache != NULL)
		delete_object_cache(sJumbo9ChunkCache);
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
	delete_object_cache(sJumboPageSizeCache);
}
