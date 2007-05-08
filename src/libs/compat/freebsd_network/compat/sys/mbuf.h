#ifndef _FBSD_COMPAT_SYS_MBUF_H_
#define _FBSD_COMPAT_SYS_MBUF_H_

#include <sys/malloc.h>
#include <sys/param.h>

#define MLEN		((int)(MSIZE - sizeof(struct m_hdr)))
#define MHLEN		((int)(MSIZE - sizeof(struct pkthdr)))

#define MINCLSIZE	(MHLEN + 1)

#ifdef _KERNEL

struct m_hdr {
	struct mbuf *	mh_next;
	struct mbuf *	mh_nextpkt;
	caddr_t			mh_data;
	int				mh_len;
	int				mh_flags;
	short			mh_type;
};

struct pkthdr {
	struct ifnet *	rcvif;
	int				len;
	int				csum_flags;
	uint16_t		csum_data;
};

struct m_ext {
	caddr_t			ext_buf;
	unsigned int	ext_size;
	int				ext_type;
};

struct mbuf {
	struct m_hdr m_hdr;
	union {
		struct {
			struct pkthdr	MH_pkthdr;
			union {
				struct m_ext	MH_ext;
				char			MH_databuf[MHLEN];
			} MH_dat;
		} MH;
		char M_databuf[MLEN];
	} M_dat;
};

#define m_next      m_hdr.mh_next
#define m_len       m_hdr.mh_len
#define m_data      m_hdr.mh_data
#define m_type      m_hdr.mh_type
#define m_flags     m_hdr.mh_flags
#define m_nextpkt   m_hdr.mh_nextpkt
#define m_act       m_nextpkt
#define m_pkthdr    M_dat.MH.MH_pkthdr
#define m_ext       M_dat.MH.MH_dat.MH_ext
#define m_pktdat    M_dat.MH.MH_dat.MH_databuf
#define m_dat       M_dat.M_databuf


#define M_DONTWAIT		M_NOWAIT
#define M_TRYWAIT		M_WAITOK
#define M_WAIT			M_WAITOK

#define MT_DATA			1

#define M_EXT			0x0001
#define M_PKTHDR		0x0002

#define EXT_CLUSTER		1
#define EXT_PACKET		3

#define M_BCAST			0x0200
#define M_MCAST			0x0400

#define CSUM_IP			0x0001
#define CSUM_TCP		0x0002
#define CSUM_UDP		0x0004
#define CSUM_IP_CHECKED	0x0100
#define CSUM_IP_VALID	0x0200
#define CSUM_DATA_VALID	0x0400
#define CSUM_PSEUDO_HDR	0x0800
#define CSUM_DELAY_DATA	(CSUM_TCP | CSUM_UDP)

#define	M_MOVE_PKTHDR(to, from)	m_move_pkthdr((to), (from))
#define MGET(m, how, type)		((m) = m_get((how), (type)))
#define MGETHDR(m, how, type)	((m) = m_gethdr((how), (type)))
#define MCLGET(m, how)			m_clget((m), (how))

struct mbuf *m_getcl(int how, short type, int flags);
void m_freem(struct mbuf *mbuf);
struct mbuf *m_free(struct mbuf *m);
struct mbuf *m_defrag(struct mbuf *m, int);
void m_adj(struct mbuf *m, int);
struct mbuf *m_pullup(struct mbuf *m, int len);
struct mbuf *m_prepend(struct mbuf *m, int, int);
void m_move_pkthdr(struct mbuf *, struct mbuf *);

u_int m_length(struct mbuf *m, struct mbuf **last);
u_int m_fixhdr(struct mbuf *m);
void m_cat(struct mbuf *m, struct mbuf *n);
void m_copydata(const struct mbuf *m, int off, int len, caddr_t cp);

struct mbuf *m_get(int how, short type);
struct mbuf *m_gethdr(int how, short type);
void m_clget(struct mbuf *m, int how);

#define mtod(m, type)	(type)((m)->m_data)

#define m_tag_delete(mb, tag) \
	panic("m_tag_delete unsupported.");

/* Check if the supplied mbuf has a packet header, or else panic. */
#define	M_ASSERTPKTHDR(m)						\
	KASSERT(m != NULL && m->m_flags & M_PKTHDR,			\
	    ("%s: no mbuf packet header!", __func__))

#define MBUF_CHECKSLEEP(how) do { } while (0)


/*
 * Set the m_data pointer of a newly-allocated mbuf (m_get/MGET) to place
 * an object of the specified size at the end of the mbuf, longword aligned.
 */
#define	M_ALIGN(m, len) do {						\
	(m)->m_data += (MLEN - (len)) & ~(sizeof(long) - 1);		\
} while (0)

/*
 * As above, for mbufs allocated with m_gethdr/MGETHDR
 * or initialized by M_COPY_PKTHDR.
 */
#define	MH_ALIGN(m, len) do {						\
	(m)->m_data += (MHLEN - (len)) & ~(sizeof(long) - 1);		\
} while (0)


/*
#define	MEXT_IS_REF(m)	(((m)->m_ext.ref_cnt != NULL)			\
    && (*((m)->m_ext.ref_cnt) > 1))
 */
#define MEXT_IS_REF(m)	0

/*
 * Evaluate TRUE if it's safe to write to the mbuf m's data region (this
 * can be both the local data payload, or an external buffer area,
 * depending on whether M_EXT is set).
 */

/*
#define	M_WRITABLE(m)	(!((m)->m_flags & M_RDONLY) && (!((m)->m_flags  \
			    & M_EXT) || !MEXT_IS_REF(m)))
 */
#define M_WRITABLE(m)	(!((m)->m_flags & M_EXT) || !MEXT_IS_REF(m))

/*
 * Compute the amount of space available
 * before the current start of data in an mbuf.
 *
 * The M_WRITABLE() is a temporary, conservative safety measure: the burden
 * of checking writability of the mbuf data area rests solely with the caller.
 */
#define	M_LEADINGSPACE(m)						\
	((m)->m_flags & M_EXT ?						\
	    (M_WRITABLE(m) ? (m)->m_data - (m)->m_ext.ext_buf : 0):	\
	    (m)->m_flags & M_PKTHDR ? (m)->m_data - (m)->m_pktdat :	\
	    (m)->m_data - (m)->m_dat)

/*
 * Arrange to prepend space of size plen to mbuf m.
 * If a new mbuf must be allocated, how specifies whether to wait.
 * If the allocation fails, the original mbuf chain is freed and m is
 * set to NULL.
 */
#define	M_PREPEND(m, plen, how) do {					\
	struct mbuf **_mmp = &(m);					\
	struct mbuf *_mm = *_mmp;					\
	int _mplen = (plen);						\
	int __mhow = (how);						\
									\
	MBUF_CHECKSLEEP(how);						\
	if (M_LEADINGSPACE(_mm) >= _mplen) {				\
		_mm->m_data -= _mplen;					\
		_mm->m_len += _mplen;					\
	} else								\
		_mm = m_prepend(_mm, _mplen, __mhow);			\
	if (_mm != NULL && _mm->m_flags & M_PKTHDR)			\
		_mm->m_pkthdr.len += _mplen;				\
	*_mmp = _mm;							\
} while (0)

#endif

#endif
