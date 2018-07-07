/*
 * Copyright 2009, Colin Günther, coling@gmx.de.
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_SYS_MBUF_H_
#define _FBSD_COMPAT_SYS_MBUF_H_


#include <sys/queue.h>
#include <sys/systm.h>
#include <vm/uma.h>


#define MHSIZE		__offsetof(struct mbuf, m_dat)
#define MPKTHSIZE	__offsetof(struct mbuf, m_pktdat)
#define MLEN		((int)(MSIZE - MHSIZE))
#define MHLEN		((int)(MSIZE - MPKTHSIZE))

#define MINCLSIZE	(MHLEN + 1)

#define MBTOM(how)	(how)
#define M_DONTWAIT	M_NOWAIT
#define M_TRYWAIT	M_WAITOK
#define M_WAIT		M_WAITOK

#define MT_DATA		1

/*
 * mbuf flags of global significance and layer crossing.
 * Those of only protocol/layer specific significance are to be mapped
 * to M_PROTO[1-12] and cleared at layer handoff boundaries.
 * NB: Limited to the lower 24 bits.
 */
#define	M_EXT		0x00000001 /* has associated external storage */
#define	M_PKTHDR	0x00000002 /* start of record */
#define	M_EOR		0x00000004 /* end of record */
#define	M_RDONLY	0x00000008 /* associated data is marked read-only */
#define	M_BCAST		0x00000010 /* send/received as link-level broadcast */
#define	M_MCAST		0x00000020 /* send/received as link-level multicast */
#define	M_PROMISC	0x00000040 /* packet was not for us */
#define	M_VLANTAG	0x00000080 /* ether_vtag is valid */
#define	M_UNUSED_8	0x00000100 /* --available-- */
#define	M_NOFREE	0x00000200 /* do not free mbuf, embedded in cluster */

#define	M_PROTO1	0x00001000 /* protocol-specific */
#define	M_PROTO2	0x00002000 /* protocol-specific */
#define	M_PROTO3	0x00004000 /* protocol-specific */
#define	M_PROTO4	0x00008000 /* protocol-specific */
#define	M_PROTO5	0x00010000 /* protocol-specific */
#define	M_PROTO6	0x00020000 /* protocol-specific */
#define	M_PROTO7	0x00040000 /* protocol-specific */
#define	M_PROTO8	0x00080000 /* protocol-specific */
#define	M_PROTO9	0x00100000 /* protocol-specific */
#define	M_PROTO10	0x00200000 /* protocol-specific */
#define	M_PROTO11	0x00400000 /* protocol-specific */
#define	M_PROTO12	0x00800000 /* protocol-specific */

#define	M_PROTOFLAGS \
	(M_PROTO1|M_PROTO2|M_PROTO3|M_PROTO4|M_PROTO5|M_PROTO6|M_PROTO7|M_PROTO8|\
	 M_PROTO9|M_PROTO10|M_PROTO11|M_PROTO12)
	// Flags to purge when crossing layers.

#define M_COPYFLAGS \
	(M_PKTHDR|M_EOR|M_RDONLY|M_BCAST|M_MCAST|M_PROMISC|M_VLANTAG| \
	 M_PROTOFLAGS)
	// Flags preserved when copying m_pkthdr

#define M_MOVE_PKTHDR(to, from)	m_move_pkthdr((to), (from))
#define MGET(m, how, type)		((m) = m_get((how), (type)))
#define MGETHDR(m, how, type)	((m) = m_gethdr((how), (type)))
#define MCLGET(m, how)			m_clget((m), (how))
#define m_getm(m, len, how, type)					\
    m_getm2((m), (len), (how), (type), M_PKTHDR)

#define mtod(m, type)	((type)((m)->m_data))

// Check if the supplied mbuf has a packet header, or else panic.
#define M_ASSERTPKTHDR(m) KASSERT(m != NULL && m->m_flags & M_PKTHDR, \
	("%s: no mbuf packet header!", __func__))

#define MBUF_CHECKSLEEP(how) do { } while (0)

#define MTAG_PERSISTENT	0x800

#define	M_COPYALL	1000000000
	// Length to m_copy to copy all.

#define EXT_CLUSTER		1		// 2048 bytes
#define EXT_JUMBOP		4		// Page size
#define EXT_JUMBO9		5		// 9 * 1024 bytes
#define EXT_NET_DRV		100		// custom ext_buf provided by net driver

#define EXT_EXTREF		255		// has externally maintained ext_cnt ptr

/*
 * Flags for external mbuf buffer types.
 * NB: limited to the lower 24 bits.
 */
#define EXT_FLAG_EMBREF		0x000001	/* embedded ext_count */
#define EXT_FLAG_EXTREF		0x000002	/* external ext_cnt, notyet */

#define CSUM_IP			0x0001
#define CSUM_TCP		0x0002
#define CSUM_UDP		0x0004
#define CSUM_TSO		0x0020
#define CSUM_IP_CHECKED	0x0100
#define CSUM_IP_VALID	0x0200
#define CSUM_DATA_VALID	0x0400
#define CSUM_PSEUDO_HDR	0x0800
#define CSUM_DELAY_DATA	(CSUM_TCP | CSUM_UDP)

#define MEXTADD(m, buf, size, free, arg1, arg2, flags, type) \
	m_extadd((m), (caddr_t)(buf), (size), (free),(arg1),(arg2),(flags), (type))


extern int max_linkhdr;
extern int max_protohdr;
extern int max_hdr;
extern int max_datalen;		// MHLEN - max_hdr


struct pkthdr {
	struct ifnet*					rcvif;
	int								len;
	int								csum_flags;
	int								csum_data;
	uint16_t						tso_segsz;
	uint16_t						ether_vtag;
	SLIST_HEAD(packet_tags, m_tag)	tags;
};

struct m_tag {
	SLIST_ENTRY(m_tag)	m_tag_link;		// List of packet tags
	u_int16_t			m_tag_id;		// Tag ID
	u_int16_t			m_tag_len;		// Length of data
	u_int32_t			m_tag_cookie;	// ABI/Module ID
	void				(*m_tag_free)(struct m_tag*);
};

struct m_ext {
	union {
		volatile u_int	 ext_count;	/* value of ref count info */
		volatile u_int	*ext_cnt;	/* pointer to ref count info */
	};
	caddr_t		 ext_buf;	 /* start of buffer */
	uint32_t	 ext_size;	 /* size of buffer, for ext_free */
	uint32_t	 ext_type:8, /* type of external storage */
			 ext_flags:24;	 /* external storage mbuf flags */
};

struct mbuf {
	union {	/* next buffer in chain */
		struct mbuf		*m_next;
		SLIST_ENTRY(mbuf)	m_slist;
		STAILQ_ENTRY(mbuf)	m_stailq;
	};
	union {	/* next chain in queue/record */
		struct mbuf		*m_nextpkt;
		SLIST_ENTRY(mbuf)	m_slistpkt;
		STAILQ_ENTRY(mbuf)	m_stailqpkt;
	};
	caddr_t		 m_data;	/* location of data */
	int32_t		 m_len;		/* amount of data in this mbuf */
	uint32_t	 m_type:8,	/* type of data in this mbuf */
			 m_flags:24;
#if !defined(__LP64__)
	uint32_t	 m_pad;		/* pad for 64bit alignment */
#endif

	union {
		struct {
			struct pkthdr	MH_pkthdr;
			union {
				struct m_ext	MH_ext;
				char			MH_databuf[0];
			} MH_dat;
		} MH;
		char M_databuf[0];
	} M_dat;
};

/* The reason we use these really nasty macros, instead of naming the
 * structs and unions properly like FreeBSD does ... is because of a
 * GCC2 compiler bug. Specifically a parser bug: adding -O0 has no
 * effect on this problem. */
#define m_pkthdr	M_dat.MH.MH_pkthdr
#define m_ext		M_dat.MH.MH_dat.MH_ext
#define m_pktdat	M_dat.MH.MH_dat.MH_databuf
#define m_dat		M_dat.M_databuf

void			m_catpkt(struct mbuf *m, struct mbuf *n);
void			m_adj(struct mbuf*, int);
int				m_append(struct mbuf*, int, c_caddr_t);
void			m_cat(struct mbuf*, struct mbuf*);
int				m_clget(struct mbuf*, int);
void*			m_cljget(struct mbuf*, int, int);
struct mbuf*	m_collapse(struct mbuf*, int, int);
void			m_copyback(struct mbuf *m0, int off, int len, c_caddr_t cp);
void			m_copydata(const struct mbuf*, int, int, caddr_t);
struct mbuf*	m_copypacket(struct mbuf*, int);
struct mbuf *	m_copym(struct mbuf *m, int off0, int len, int wait);
struct mbuf*	m_defrag(struct mbuf*, int);
struct mbuf*	m_devget(char*, int, int, struct ifnet*,
	void(*) (char*, caddr_t, u_int));

struct mbuf*	m_dup(const struct mbuf *m, int how);
int				m_dup_pkthdr(struct mbuf *to, const struct mbuf *from, int how);

void			m_demote_pkthdr(struct mbuf *m);
void			m_demote(struct mbuf *m0, int all, int flags);

void			m_extadd(struct mbuf*, caddr_t, u_int, void(*) (void*, void*),
	void*, void*, int, int);

u_int			m_fixhdr(struct mbuf*);
struct mbuf*	m_free(struct mbuf*);
void			m_freem(struct mbuf*);
struct mbuf*	m_get(int, short);
struct mbuf*	m_get2(int size, int how, short type, int flags);
struct mbuf *	m_getm2(struct mbuf *m, int len, int how, short type, int flags);
struct mbuf*	m_gethdr(int, short);
struct mbuf*	m_getjcl(int, short, int, int);
u_int			m_length(struct mbuf*, struct mbuf**);
struct mbuf*	m_getcl(int, short, int);
void			m_move_pkthdr(struct mbuf*, struct mbuf*);
struct mbuf*	m_prepend(struct mbuf*, int, int);
struct mbuf*	m_pulldown(struct mbuf*, int, int, int*);
struct mbuf*	m_pullup(struct mbuf*, int);
struct mbuf*	m_split(struct mbuf*, int, int);
struct mbuf*	m_unshare(struct mbuf*, int);

struct m_tag*	m_tag_alloc(u_int32_t, int, int, int);
void			m_tag_delete(struct mbuf*, struct m_tag*);
void			m_tag_delete_chain(struct mbuf*, struct m_tag*);
void			m_tag_free_default(struct m_tag*);
struct m_tag*	m_tag_locate(struct mbuf*, u_int32_t, int, struct m_tag*);
struct m_tag*	m_tag_copy(struct m_tag*, int);
int				m_tag_copy_chain(struct mbuf *to, const struct mbuf *from, int how);
void			m_tag_delete_nonpersistent(struct mbuf*);


static inline void
m_tag_setup(struct m_tag* tagPointer, u_int32_t cookie, int type, int length)
{
	tagPointer->m_tag_id = type;
	tagPointer->m_tag_len = length;
	tagPointer->m_tag_cookie = cookie;
}


static inline void
m_tag_free(struct m_tag* tag)
{
	(*tag->m_tag_free)(tag);
}


static inline void
m_tag_prepend(struct mbuf* memoryBuffer, struct m_tag* tag)
{
	SLIST_INSERT_HEAD(&memoryBuffer->m_pkthdr.tags, tag, m_tag_link);
}


static inline void
m_tag_unlink(struct mbuf* memoryBuffer, struct m_tag* tag)
{
	SLIST_REMOVE(&memoryBuffer->m_pkthdr.tags, tag, m_tag, m_tag_link);
}


#include <sys/mbuf-fbsd.h>

#endif	/* _FBSD_COMPAT_SYS_MBUF_H_ */
