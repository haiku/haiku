#ifndef _FBSD_COMPAT_SYS_MBUF_H_
#define _FBSD_COMPAT_SYS_MBUF_H_

#include <sys/malloc.h>
#include <sys/param.h>

#define MLEN		(MSIZE - sizeof(struct m_hdr))
#define MHLEN		(MSIZE - sizeof(struct pkthdr))

#ifdef _KERNEL

struct m_hdr {
	struct mbuf *	mh_next;
	struct mbuf *	mh_nextpkt;
	void *			mh_data;
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
	void *			ext_buf;
	unsigned int	ext_size;
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

#define CSUM_IP			0x0001
#define CSUM_TCP		0x0002
#define CSUM_UDP		0x0004
#define CSUM_IP_CHECKED	0x0100
#define CSUM_IP_VALID	0x0200
#define CSUM_DATA_VALID	0x0400
#define CSUM_PSEUDO_HDR	0x0800
#define CSUM_DELAY_DATA	(CSUM_TCP | CSUM_UDP)

struct mbuf *m_getcl(int how, short type, int flags);
void m_freem(struct mbuf *mbuf);
struct mbuf *m_defrag(struct mbuf *m, int);

#define mtod(m, type)	(type)((m)->m_data)

#endif

#endif
