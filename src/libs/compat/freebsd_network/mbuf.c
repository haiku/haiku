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


#define MBTOM(how) (how)

static object_cache *sMBufCache;
static object_cache *sChunkCache;

static const int max_protohdr = 40 + 20; /* ip6 + tcp */


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


/*
 * Copy data from an mbuf chain starting "off" bytes from the beginning,
 * continuing for "len" bytes, into the indicated buffer.
 */
void
m_copydata(const struct mbuf *m, int off, int len, caddr_t cp)
{
	u_int count;

	KASSERT(off >= 0, ("m_copydata, negative off %d", off));
	KASSERT(len >= 0, ("m_copydata, negative len %d", len));
	while (off > 0) {
		KASSERT(m != NULL, ("m_copydata, offset > size of mbuf chain"));
		if (off < m->m_len)
			break;
		off -= m->m_len;
		m = m->m_next;
	}
	while (len > 0) {
		KASSERT(m != NULL, ("m_copydata, length > size of mbuf chain"));
		count = min(m->m_len - off, len);
		bcopy(mtod(m, caddr_t) + off, cp, count);
		len -= count;
		cp += count;
		off = 0;
		m = m->m_next;
	}
}


/*
 * Concatenate mbuf chain n to m.
 * Both chains must be of the same type (e.g. MT_DATA).
 * Any m_pkthdr is not updated.
 */
void
m_cat(struct mbuf *m, struct mbuf *n)
{
	while (m->m_next)
		m = m->m_next;
	while (n) {
		if (m->m_flags & M_EXT ||
		    m->m_data + m->m_len + n->m_len >= &m->m_dat[MLEN]) {
			/* just join the two chains */
			m->m_next = n;
			return;
		}
		/* splat the data from one into the other */
		bcopy(mtod(n, caddr_t), mtod(m, caddr_t) + m->m_len,
		    (u_int)n->m_len);
		m->m_len += n->m_len;
		n = m_free(n);
	}
}


u_int
m_length(struct mbuf *m0, struct mbuf **last)
{
	struct mbuf *m;
	u_int len;

	len = 0;
	for (m = m0; m != NULL; m = m->m_next) {
		len += m->m_len;
		if (m->m_next == NULL)
			break;
	}
	if (last != NULL)
		*last = m;
	return (len);
}


u_int
m_fixhdr(struct mbuf *m0)
{
	u_int len;

	len = m_length(m0, NULL);
	m0->m_pkthdr.len = len;
	return (len);
}


static int
m_tag_copy_chain(struct mbuf *to, struct mbuf *from, int how)
{
	return 1;
}


/*
 * Duplicate "from"'s mbuf pkthdr in "to".
 * "from" must have M_PKTHDR set, and "to" must be empty.
 * In particular, this does a deep copy of the packet tags.
 */
static int
m_dup_pkthdr(struct mbuf *to, struct mbuf *from, int how)
{
	MBUF_CHECKSLEEP(how);
	/* to->m_flags = (from->m_flags & M_COPYFLAGS) | (to->m_flags & M_EXT); */
	to->m_flags = (to->m_flags & M_EXT);
	if ((to->m_flags & M_EXT) == 0)
		to->m_data = to->m_pktdat;
	to->m_pkthdr = from->m_pkthdr;
	/* SLIST_INIT(&to->m_pkthdr.tags); */
	return (m_tag_copy_chain(to, from, MBTOM(how)));
}


/*
 * Defragment a mbuf chain, returning the shortest possible
 * chain of mbufs and clusters.  If allocation fails and
 * this cannot be completed, NULL will be returned, but
 * the passed in chain will be unchanged.  Upon success,
 * the original chain will be freed, and the new chain
 * will be returned.
 *
 * If a non-packet header is passed in, the original
 * mbuf (chain?) will be returned unharmed.
 */
struct mbuf *
m_defrag(struct mbuf *m0, int how)
{
	struct mbuf *m_new = NULL, *m_final = NULL;
	int progress = 0, length;

	MBUF_CHECKSLEEP(how);
	if (!(m0->m_flags & M_PKTHDR))
		return (m0);

	m_fixhdr(m0); /* Needed sanity check */

	if (m0->m_pkthdr.len > MHLEN)
		m_final = m_getcl(how, MT_DATA, M_PKTHDR);
	else
		m_final = m_gethdr(how, MT_DATA);

	if (m_final == NULL)
		goto nospace;

	if (m_dup_pkthdr(m_final, m0, how) == 0)
		goto nospace;

	m_new = m_final;

	while (progress < m0->m_pkthdr.len) {
		length = m0->m_pkthdr.len - progress;
		if (length > MCLBYTES)
			length = MCLBYTES;

		if (m_new == NULL) {
			if (length > MLEN)
				m_new = m_getcl(how, MT_DATA, 0);
			else
				m_new = m_get(how, MT_DATA);
			if (m_new == NULL)
				goto nospace;
		}

		m_copydata(m0, progress, length, mtod(m_new, caddr_t));
		progress += length;
		m_new->m_len = length;
		if (m_new != m_final)
			m_cat(m_final, m_new);
		m_new = NULL;
	}

	m_freem(m0);
	m0 = m_final;
	return (m0);
nospace:
	if (m_final)
		m_freem(m_final);
	return (NULL);
}


void
m_adj(struct mbuf *mp, int req_len)
{
	int len = req_len;
	struct mbuf *m;
	int count;

	if ((m = mp) == NULL)
		return;
	if (len >= 0) {
		/*
		 * Trim from head.
		 */
		while (m != NULL && len > 0) {
			if (m->m_len <= len) {
				len -= m->m_len;
				m->m_len = 0;
				m = m->m_next;
			} else {
				m->m_len -= len;
				m->m_data += len;
				len = 0;
			}
		}
		m = mp;
		if (mp->m_flags & M_PKTHDR)
			m->m_pkthdr.len -= (req_len - len);
	} else {
		/*
		 * Trim from tail.  Scan the mbuf chain,
		 * calculating its length and finding the last mbuf.
		 * If the adjustment only affects this mbuf, then just
		 * adjust and return.  Otherwise, rescan and truncate
		 * after the remaining size.
		 */
		len = -len;
		count = 0;
		for (;;) {
			count += m->m_len;
			if (m->m_next == (struct mbuf *)0)
				break;
			m = m->m_next;
		}
		if (m->m_len >= len) {
			m->m_len -= len;
			if (mp->m_flags & M_PKTHDR)
				mp->m_pkthdr.len -= len;
			return;
		}
		count -= len;
		if (count < 0)
			count = 0;
		/*
		 * Correct length for chain is "count".
		 * Find the mbuf with last data, adjust its length,
		 * and toss data from remaining mbufs on chain.
		 */
		m = mp;
		if (m->m_flags & M_PKTHDR)
			m->m_pkthdr.len = count;
		for (; m; m = m->m_next) {
			if (m->m_len >= count) {
				m->m_len = count;
				if (m->m_next != NULL) {
					m_freem(m->m_next);
					m->m_next = NULL;
				}
				break;
			}
			count -= m->m_len;
		}
	}
}


/*
 * Rearange an mbuf chain so that len bytes are contiguous
 * and in the data area of an mbuf (so that mtod and dtom
 * will work for a structure of size len).  Returns the resulting
 * mbuf chain on success, frees it and returns null on failure.
 * If there is room, it will add up to max_protohdr-len extra bytes to the
 * contiguous region in an attempt to avoid being called next time.
 */
struct mbuf *
m_pullup(struct mbuf *n, int len)
{
	struct mbuf *m;
	int count;
	int space;

	/*
	 * If first mbuf has no cluster, and has room for len bytes
	 * without shifting current data, pullup into it,
	 * otherwise allocate a new mbuf to prepend to the chain.
	 */
	if ((n->m_flags & M_EXT) == 0 &&
	    n->m_data + len < &n->m_dat[MLEN] && n->m_next) {
		if (n->m_len >= len)
			return (n);
		m = n;
		n = n->m_next;
		len -= m->m_len;
	} else {
		if (len > MHLEN)
			goto bad;
		MGET(m, M_DONTWAIT, n->m_type);
		if (m == NULL)
			goto bad;
		m->m_len = 0;
		if (n->m_flags & M_PKTHDR)
			M_MOVE_PKTHDR(m, n);
	}
	space = &m->m_dat[MLEN] - (m->m_data + m->m_len);
	do {
		count = min(min(max(len, max_protohdr), space), n->m_len);
		bcopy(mtod(n, caddr_t), mtod(m, caddr_t) + m->m_len,
		  (u_int)count);
		len -= count;
		m->m_len += count;
		n->m_len -= count;
		space -= count;
		if (n->m_len)
			n->m_data += count;
		else
			n = m_free(n);
	} while (len > 0 && n);
	if (len > 0) {
		(void) m_free(m);
		goto bad;
	}
	m->m_next = n;
	return (m);
bad:
	m_freem(n);
	return (NULL);
}


/*
 * Lesser-used path for M_PREPEND:
 * allocate new mbuf to prepend to chain,
 * copy junk along.
 */
struct mbuf *
m_prepend(struct mbuf *m, int len, int how)
{
	struct mbuf *mn;

	if (m->m_flags & M_PKTHDR)
		MGETHDR(mn, how, m->m_type);
	else
		MGET(mn, how, m->m_type);
	if (mn == NULL) {
		m_freem(m);
		return (NULL);
	}
	if (m->m_flags & M_PKTHDR)
		M_MOVE_PKTHDR(mn, m);
	mn->m_next = m;
	m = mn;
	if (len < MHLEN)
		MH_ALIGN(m, len);
	m->m_len = len;
	return (m);
}


/*
 * "Move" mbuf pkthdr from "from" to "to".
 * "from" must have M_PKTHDR set, and "to" must be empty.
 */
void
m_move_pkthdr(struct mbuf *to, struct mbuf *from)
{
#ifdef MAC
	/*
	 * XXXMAC: It could be this should also occur for non-MAC?
	 */
	if (to->m_flags & M_PKTHDR)
		m_tag_delete_chain(to, NULL);
#endif
	/* to->m_flags = (from->m_flags & M_COPYFLAGS) | (to->m_flags & M_EXT); */
	/* we don't have M_COPYFLAGS -hugo */
	to->m_flags = to->m_flags & M_EXT;
	if ((to->m_flags & M_EXT) == 0)
		to->m_data = to->m_pktdat;
	to->m_pkthdr = from->m_pkthdr;		/* especially tags */
	/* we don't have tags -hugo */
#if 0
	SLIST_INIT(&from->m_pkthdr.tags);	/* purge tags from src */
#endif
	from->m_flags &= ~M_PKTHDR;
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

