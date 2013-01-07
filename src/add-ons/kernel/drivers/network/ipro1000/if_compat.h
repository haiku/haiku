/* Intel PRO/1000 Family Driver
 * Copyright (C) 2004 Marcus Overhagen <marcus@overhagen.de>. All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its 
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies, and that both the
 * copyright notice and this permission notice appear in supporting documentation.
 *
 * Marcus Overhagen makes no representations about the suitability of this software
 * for any purpose. It is provided "as is" without express or implied warranty.
 *
 * MARCUS OVERHAGEN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL MARCUS
 * OVERHAGEN BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef __IF_COMPAT_H
#define __IF_COMPAT_H

#include <OS.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>

#include "if_em_osdep.h" // for TAILQ_...

#define __FreeBSD_version	500001

#define IFF_RUNNING		0x10000
#define IFF_OACTIVE		0x20000

#define IFCAP_HWCSUM			0x0001
#define IFCAP_VLAN_HWTAGGING	0x0002
#define IFCAP_VLAN_MTU			0x0004
#define IFCAP_TXCSUM			0x0010
#define IFCAP_RXCSUM			0x0020

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
#	define IFM_AVALID	0
#	define IFM_FDX		IFM_FULL_DUPLEX
#	define IFM_HDX		IFM_HALF_DUPLEX
#	define IFM_1000_TX	IFM_1000_T
#else
#	define IFM_ACTIVE	0x0001
#	define IFM_FDX		0x0002
#	define IFM_HDX		0x0004
#	define IFM_10_T		0x0008
#	define IFM_100_TX	0x0010
#	define IFM_1000_T	0x0020
#	define IFM_1000_TX	0x0040
#	define IFM_1000_SX	0x0080
#	define IFM_ETHER	0x0100
#	define IFM_AUTO		0x0200
#	define IFM_AVALID	0x0400
#	define IFM_GMASK	(IFM_FDX | IFM_HDX)
#	define IFM_TYPE_MASK (IFM_ETHER)
#	define IFM_SUBTYPE_MASK \
		(IFM_AUTO | IFM_1000_SX | IFM_1000_TX | IFM_1000_T | IFM_100_TX | IFM_10_T)
#	define IFM_TYPE(media)		((media) & IFM_TYPE_MASK)
#	define IFM_SUBTYPE(media)	((media) & IFM_SUBTYPE_MASK)
#endif

#define CSUM_TCP			0x0001 // ifnet::if_hwassist
#define CSUM_UDP			0x0002 // ifnet::if_hwassist
#define CSUM_IP_CHECKED		0x0004
#define CSUM_IP_VALID		0x0008
#define CSUM_DATA_VALID		0x0010
#define CSUM_PSEUDO_HDR		0x0020

#define ETHER_ADDR_LEN          6       /* length of an Ethernet address */
#define ETHER_TYPE_LEN          2       /* length of the Ethernet type field */
#define ETHER_CRC_LEN           4       /* length of the Ethernet CRC */
#define ETHER_HDR_LEN           (ETHER_ADDR_LEN*2+ETHER_TYPE_LEN)
#define ETHER_MIN_LEN           64      /* minimum frame len, including CRC */
#define ETHER_MAX_LEN           1518    /* maximum frame len, including CRC */
#define ETHER_MAX_LEN_JUMBO     9018    /* max jumbo frame len, including CRC */
#define ETHER_VLAN_ENCAP_LEN    4       /* len of 802.1Q VLAN encapsulation */
#define	ETHERMTU				ETHER_MAX_LEN_JUMBO
#define IP_HEADER_SIZE			20
#define OFFSETOF_IPHDR_SUM		10
#define OFFSETOF_TCPHDR_SUM		16
#define OFFSETOF_UDPHDR_SUM		6


// vlan stuff is not supported
#define ETHERTYPE_VLAN					0x8100
#define VLAN_INPUT_TAG(a, b, c, d)
#define VLAN_TAG_VALUE(mtag)			0
#define VLAN_OUTPUT_TAG(ifp, m_head)	0


// BPF listener not supported
#define BPF_MTAP(a, b)


// sysctl stuff is not supported
#define sysctl_ctx_init(a)
#define sysctl_ctx_free(a)
#define SYSCTL_HANDLER_ARGS void
#define SYSCTL_ADD_NODE(a, b, c, d, e, f, g) \
	(struct sysctl_oid *) 1
#define SYSCTL_ADD_PROC(a, b, c, d, e, f, g, h, i, j)

struct sysctl_ctx_list
{
};

struct sysctl_oid
{
};


struct adapter;
struct ifnet;
struct mbuf;


struct ifmedia
{
	uint32 	ifm_media;
};

struct if_queue
{
	volatile struct mbuf *	ifq_head;
	volatile struct mbuf *	ifq_tail;
	int						ifq_maxlen; // ignored
};

typedef void	(*if_start)(struct ifnet *);
typedef int		(*if_ioctl)(struct ifnet *, u_long, caddr_t);
typedef void	(*if_watchdog)(struct ifnet *);
typedef void	(*if_init)(void *);
typedef void	(*if_input)(struct ifnet *, struct mbuf *);

struct m_pkthdr
{
	int 			len;
	int				csum_flags;
	uint16			csum_data;
	struct ifnet *	rcvif; // set by receive int
};

struct m_ext
{
	void *ext_buf;
};

struct mbuf
{
	struct mbuf *	m_next;		// next buffer in chain (e.g. for jumboframes)
	struct mbuf *	m_nextq;	// next buffer in queue
	int				m_len;
	int 			m_flags;
	void *			m_data;
	struct m_pkthdr	m_pkthdr;
	struct m_ext 	m_ext;
};

#define M_DONTWAIT	0x01
#define M_EXT		0x02
#define	M_PKTHDR	0x04
#define MT_DATA		1
#define MCLBYTES	2048

struct mbuf *m_gethdr(int how, int type);

// Allocate and return a single M_PKTHDR mbuf.  NULL is returned on failure.
#define MGETHDR(mb, how, type) ((mb) = m_gethdr((how), (type)))

void m_clget(struct mbuf * mb, int how);

// Fetch a single mbuf cluster and attach it to an existing mbuf.  If
// successfull, configures the provided mbuf to have mbuf->m_ext.ext_buf
// pointing to the cluster, and sets the M_EXT bit in the mbuf's flags.
// The M_EXT bit is not set on failure
#define MCLGET(mb, how)  m_clget((mb), (how))

struct mbuf *if_dequeue(struct if_queue *queue);

void if_prepend(struct if_queue *queue, struct mbuf *mb);
void if_append(struct if_queue *queue, struct mbuf *mb);

#define IF_DEQUEUE(queue, mb) ((mb) = if_dequeue((queue)))
#define IF_PREPEND(queue, mb) if_prepend((queue), (mb))
#define IF_APPEND(queue, mb) if_append((queue), (mb))

void m_adj(struct mbuf *mp, int bytes);
void m_freem(struct mbuf *mp);

#define mtod(m, t)	((t)((m)->m_data))

void ether_ifattach(struct ifnet *ifp, const uint8 *etheraddr);
void ether_ifdetach(struct ifnet *ifp);

int ether_add_multi(struct ifnet *ifp, const struct sockaddr *address);
int ether_rem_multi(struct ifnet *ifp, const struct sockaddr *address);

TAILQ_HEAD(ifmultihead, ifmultiaddr);

struct ifmultiaddr
{
	TAILQ_ENTRY(ifmultiaddr) ifma_link;
	struct sockaddr *ifma_addr;
	int ifma_refcount;

	/* private */
	struct sockaddr_dl ifma_addr_storage;
};

struct if_data
{
	int ifi_hdrlen;
};

struct ether_vlan_header
{
	uint8  evl_dhost[ETHER_ADDR_LEN];
	uint8  evl_shost[ETHER_ADDR_LEN];
	uint16 evl_encap_proto;
	uint16 evl_tag;
	uint16 evl_proto;
};

struct ifnet 
{
	const char *if_name;

	struct adapter *if_softc;

	int		if_unit;
	int		if_mtu;
	volatile uint32	if_flags;
	uint32	if_capabilities;
	uint32	if_hwassist;
	uint32	if_capenable;
	uint64	if_baudrate;
	int64	if_timer;

	uint64	if_ibytes;
	uint64	if_obytes;
	uint64	if_imcasts;
	uint64	if_collisions;
	uint64	if_ierrors;
	uint64	if_oerrors;
	uint64	if_ipackets;
	uint64	if_opackets;
	
	struct if_queue if_snd; // send queue
	struct if_queue if_rcv; // recveive queue
	
	sem_id if_rcv_sem;

	struct if_data if_data;
	struct ifmultihead if_multiaddrs;
	
	if_start	if_start;
	if_ioctl	if_ioctl;
	if_watchdog	if_watchdog;
	if_init		if_init;
	if_input	if_input;


	void *if_output; // unused
	#define ether_output 0
};

struct arpcom
{
	struct ifnet ac_if;
	uint8 ac_enaddr[ETHER_ADDR_LEN];
};

#define device_get_softc(dev) \
	(dev->adapter ? dev->adapter : (dev->adapter = (struct adapter *)malloc(sizeof(struct adapter), 0, 0)))

#define device_get_unit(dev) \
	dev->devId

enum { // ioctl commands
	SIOCSIFADDR = 0x7000,
	SIOCGIFADDR,
	SIOCSIFMTU,
	SIOCSIFFLAGS,
	SIOCADDMULTI,
	SIOCDELMULTI,
	SIOCSIFMEDIA,
	SIOCGIFMEDIA,
	SIOCSIFCAP,
};

// used for media properties
#define ifmedia_init(a, b, c, d)
#define ifmedia_add(a, b, c, d)
#define ifmedia_set(a, b)
#define ifmedia_ioctl(ifp, ifr, media, command) 0

// called for SIOCxIFADDR (Get/Set Interface Addr)
#define ether_ioctl(ifp, command, data)

#endif // __IF_COMPAT_H
