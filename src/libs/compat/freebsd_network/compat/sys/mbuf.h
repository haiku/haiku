#ifndef _FBSD_COMPAT_SYS_MBUF_H_
#define _FBSD_COMPAT_SYS_MBUF_H_

#include <sys/malloc.h>

struct m_pkthdr {
	int				len;
	int				csum_flags;
	uint16_t		csum_data;
	struct ifnet *	rcvif;
};

struct m_ext {
	void *			ext_buf;
	unsigned int	ext_size;
};

struct mbuf {
	struct mbuf *	m_next;
	struct mbuf *	m_nextpkt;
	int				m_len;
	int				m_flags;
	void *			m_data;

	struct m_pkthdr	m_pkthdr;
	struct m_ext	m_ext;
};

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
