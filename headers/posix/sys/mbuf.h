/* mbuf.h
 * network buffer definitions
 *
 * Based on BSD's mbuf principal.
 */

#include "pools.h"
 
#ifndef OBOS_MBUF_H
#define OBOS_MBUF_H

/* MBuf's are all of a single size, MSIZE bytes. If we have more data
 * than can be fitted we can add a single "MBuf cluster" which is of
 * fixed size MCLBYTES. 
 * If the data to be written is larger than MCLMINBYTES bytes we won't use
 * the storage provided within the MBuf, but rather all data will go 
 * directly into an added cluster.
 */

#define MSIZE				256 /* total size of structure */
#define MCLSHIFT			11
#define MCLBYTES			(1 << MCLSHIFT)

#define MCLMAXBYTES			2048	/* they can't be bigger than 2k */
#define MLEN		(MSIZE - sizeof(struct m_hdr)) /* data length w/o a pkt_hdr */
#define MHLEN		(MLEN - sizeof(struct pkt_hdr)) /* data length w/pkt_hdr */
#define CLLEN		(MCLBYTES - sizeof(struct cl_hdr))

#define MINCLSIZE		(MHLEN + 1)

/* mbuf header structure
 * This appears at teh start of every MBuf
 */
struct m_hdr {
	struct mbuf *m_next;	/* next mbuf in a chain */
	struct mbuf *m_nxtpkt;	/* next chain in a packet/queue */
	char * m_data;		/* pointer to data */
	uint32 m_len;		/* length data in this mbuf NB NOT total length */
	uint16 m_type;		/* what sort of data do we have ? */
	uint16 m_flags;		/* flags as defined below */
	uint16 m_cksum;		/* flags about the checksum on the packet */
};

/* pkt_hdr
 * This is ONLY found in the first MBuf in a chain and is valid
 * ONLY if we have the M_PKTHDR flag is set
 */
/* XXX - we also need to record things like the interface and also maintain
 * a list of the packets associated with this message here.
 */
struct pkt_hdr {
	struct ifnet 	*rcvif;		/* interface we came it on */
	int	len;		/* total length */
	int csum;		/* hardware cksum info... */
};

/* m_ext
 * External storage, i.e. a cluster
 */
struct m_ext {
	char *ext_buf;			/* start of the buffer */
//	void *(ext_free)();		/* free routine (if not standard) */
	uint ext_size;			/* how big is the buffer */
};

/* struct mbuf
 * The actual buffer structure.
 * This is defined as unions as the size is slightly variable
 * depending on the options set.
 */

struct mbuf {
	struct m_hdr	m_hdr;
	union {
		struct {
			struct pkt_hdr	MH_pkthdr;	 
			union {
				struct m_ext MH_ext;
				char MH_databuf[MHLEN];
			} MH_dat;
		} MH;
		char m_databuf[MLEN];
	} um_dat;
};

/* these are simply shortcuts... */
#define m_next		m_hdr.m_next
#define m_len		m_hdr.m_len
#define m_data		m_hdr.m_data
#define m_type		m_hdr.m_type
#define m_flags		m_hdr.m_flags
#define m_cksum		m_hdr.m_cksum
#define m_nextpkt	m_hdr.m_nxtpkt
#define m_ext		um_dat.MH.MH_dat.MH_ext
#define m_pktdat	um_dat.MH.MH_dat.MH_databuf
#define m_pkthdr	um_dat.MH.MH_pkthdr
#define m_dat       um_dat.m_databuf

/* define a little macro that gives us the data pointer for an mbuf */
#define mtod(m, t)		((t)((m)->m_data))
#define dtom(x)         ((struct mbuf *)((int)(x) & ~(MSIZE-1)))

/* mbuf flags */
enum {
	M_EXT 		= 0x0001,	/* has extenal storage attached */
	M_PKTHDR	= 0x0002,	/* contains the packet header */
	M_EOR 		= 0x0004,	/* end of record (not used for tcp/udp) */
	M_CLUSTER	= 0x0008,	/* external storage is a cluster */
/*
	M_PROTO		= 0x0010,	 * protocol specific *
*/
	M_BCAST		= 0x0100,	/* rx/tx as a link-level broadcast */
	M_MCAST		= 0x0200,	/* rx/tx as a link-level multicast */
	M_CONF		= 0x0400,	/* packet was encrypted?? */
	M_AUTH		= 0x0800,	/* packet was authenticated */
	M_COMP		= 0x1000,	/* packet was compressed */
	M_ANYCAST6	= 0x4000	/* it was an IPv6 anycast packet */
};
/* the M_COPYFLAGS lists the flags that can be copied when we copy
 * an mbuf with the M_PKTHDR flag set */
#define M_COPYFLAGS	(M_PKTHDR | M_EOR | M_BCAST | M_MCAST)

/* checksum flags */
enum {
	M_IPV4_CSUM_OUT		= 0x0001, /* ipv4 checksum required */
	M_TCPV4_CSUM_OUT	= 0x0002, /* TCP v4 checksum required */
	M_UDPV4_CSUM_OUT	= 0x0004,

	M_IPV4_CSUM_IN_OK	= 0x0008,
	M_IPV4_CSUM_IN_OUT	= 0x0010,

	M_TCP_CSUM_IN_OK	= 0x0020,
	M_TCP_CSUM_IN_BAD	= 0x0040,

	M_UDP_CSUM_IN_OK	= 0x0080,
	M_UDP_CSUM_IN_BAD	= 0x0100
};

/* MBuf types */
enum {
	MT_FREE		= 0,	/* should be on free list */
	MT_DATA		= 1,	/* dynamic data allocation */
	MT_HEADER	= 2,	/* packet header */
	MT_SONAME	= 3,
	MT_SOOPTS	= 4,
	MT_FTABLE	= 5,	/* fragment reassembly header */
	MT_CONTROL	= 6,	/* extra-data protocol message */
	MT_OOBDATA	= 7 	/* out-of-band data */
};

/* length to m_copy to copy all */
#define M_COPYALL       1000000000

/* now some macro's to make life easier... */

/* need to add the macro's here :) */

#define MRESETDATA(m) do { \
	if ((m)->m_flags & M_EXT) \
		(m)->m_data = (m)->m_ext.ext_buf; \
	else if ((m)->m_flags & M_PKTHDR) \
		(m)->m_data = (m)->m_pktdat; \
	else \
		(m)->m_data = (m)->dat; \
	} while (0)
	
/* fill in an mbuf */
#define MGET(n, type) \
	n = (struct mbuf*) pool_get(mbpool); \
	if (n) { \
		(n)->m_type = (type); \
		(n)->m_next = NULL; \
		(n)->m_nextpkt = NULL; \
		(n)->m_data = (n)->m_dat; \
		(n)->m_flags = 0; \
		(n)->m_cksum = 0; \
	}

#define MGETHDR(n, type) \
	n = (struct mbuf*) pool_get(mbpool); \
	if (n) { \
		(n)->m_type = (type); \
		(n)->m_next = NULL; \
		(n)->m_nextpkt = NULL; \
		(n)->m_data = (n)->m_pktdat; \
		(n)->m_flags = M_PKTHDR; \
		(n)->m_cksum = 0; \
	}

#define _MEXTREMOVE(m) do { \
	if ((m)->m_flags & M_CLUSTER) { \
		pool_put(clpool, (m)->m_ext.ext_buf); \
	} \
	(m)->m_flags &= ~(M_CLUSTER|M_EXT); \
	(m)->m_ext.ext_size = 0; \
	} while (0)
	
#define MFREE(m, n) \
	if ((m)->m_flags & M_EXT) { \
		_MEXTREMOVE(m); \
	} \
	(n) = (m)->m_next; \
	pool_put(mbpool, m);

/* set the data pointer to place an object of size len at the end
 * of the mbuf, aligned to long word (16 bits)
 */
#define M_ALIGN(m, len) \
	{ (m)->m_data += (MLEN - (len)) &~ (sizeof(int16) -1);}

#define MH_ALIGN(m, len) \
        { (m)->m_data += (MHLEN - (len)) &~ (sizeof(int16) -1);}

#define M_MOVE_HDR(to, from) { \
	(to)->m_pkthdr = (from)->m_pkthdr; \
	(from)->m_flags &= ~M_PKTHDR; \
	}

#define M_MOVE_PKTHDR(to, from) { \
	(to)->m_flags = (from)->m_flags & M_COPYFLAGS; \
	M_MOVE_HDR((to), (from)); \
	(to)->m_data = (to)->m_pktdat; \
	}

#define MCLGET(m) do { \
	(m)->m_ext.ext_buf = pool_get(clpool); \
	if ((m)->m_ext.ext_buf != NULL) { \
		(m)->m_data = (m)->m_ext.ext_buf; \
		(m)->m_flags |= M_EXT|M_CLUSTER; \
		(m)->m_ext.ext_size = MCLBYTES; \
	} \
	} while (0)

#define M_LEADINGSPACE(m) \
        ((m)->m_flags & M_EXT ? (m)->m_data - (m)->m_ext.ext_buf : \
         (m)->m_flags & M_PKTHDR ? (m)->m_data - (m)->m_pktdat : \
         (m)->m_data - (m)->m_dat)

#define M_PREPEND(m, plen) { \
        if (M_LEADINGSPACE(m) >= (plen)) { \
                (m)->m_data -= (plen); \
                (m)->m_len += (plen); \
        } else \
                (m) = m_prepend((m), (plen)); \
        if ((m) && (m)->m_flags & M_PKTHDR) \
                (m)->m_pkthdr.len += (plen); \
}

/*
 * Duplicate just m_pkthdr from from to to.
 */
#define M_DUP_HDR(to, from) { \
        (to)->m_pkthdr = (from)->m_pkthdr; \
}

/*
 * Duplicate mbuf pkthdr from from to to.
 * from must have M_PKTHDR set, and to must be empty.
 */
#define M_DUP_PKTHDR(to, from) { \
        (to)->m_flags = (from)->m_flags & M_COPYFLAGS; \
        M_DUP_HDR((to), (from)); \
        (to)->m_data = (to)->m_pktdat; \
}

#define M_TRAILINGSPACE(m) \
        ((m)->m_flags & M_EXT ? (m)->m_ext.ext_buf + (m)->m_ext.ext_size - \
            ((m)->m_data + (m)->m_len) : \
            &(m)->m_dat[MLEN] - ((m)->m_data + (m)->m_len))

#define M_IS_CLUSTER(m)	((m)->m_flags & M_EXT)

#define M_DATASTART(m)	\
	(M_IS_CLUSTER(m) ? (m)->m_ext.ext_buf : \
	    (m)->m_flags & M_PKTHDR ? (m)->m_pktdat : (m)->m_dat)

#define M_DATASIZE(m)	\
	(M_IS_CLUSTER(m) ? (m)->m_ext.ext_size : \
	    (m)->m_flags & M_PKTHDR ? MHLEN: MLEN)

/* Functions! */
void mbinit(void);
struct mbuf *m_get(int type);

struct mbuf *m_gethdr(int type);
struct mbuf *m_getclr(int type);

struct mbuf *m_prepend(struct mbuf *m, int len);
struct mbuf *m_devget(char *buf, int totlen, int off0,
                        struct ifnet *ifp,
                        void (*copy)(const void *, void *, size_t));
struct mbuf *m_pullup(struct mbuf *, int len);
void m_copyback(struct mbuf *m0, int off, int len, caddr_t cp);

struct mbuf *m_free(struct mbuf *mfree);
void m_freem(struct mbuf *m);

void m_reserve(struct mbuf *mp, int len);
void m_cat(struct mbuf *m, struct mbuf *n);
void m_adj(struct mbuf *mp, int req_len);
void m_copydata(struct mbuf *m, int off, int len, caddr_t cp);
struct mbuf *m_copym(struct mbuf *m, int off0, int len);

/* debug functions */
void dump_freelist(void);

/* Stats structure */
/* XXX - add me! */


struct pool_ctl *mbpool;
struct pool_ctl *clpool;

int     max_hdr;		/* largest link+protocol header */
int	max_linkhdr;		/* largest link level header */
int 	max_protohdr;		/* largest protocol header */

#endif /* OBOS_MBUF_H */
