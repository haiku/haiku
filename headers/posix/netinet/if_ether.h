/*
 * Copyright (c) 1982, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)if_ether.h	8.1 (Berkeley) 6/10/93
 */

#ifndef NETINET_IF_ETHER_H
#define NETINET_IF_ETHER_H

#include "net/if_arp.h"

/*
 * Ethernet address - 6 octets
 * this is only used by the ethers(3) functions.
 */
struct ether_addr {
	uint8 ether_addr_octet[6];
};

/*
 * Some Ethernet constants.
 */
#define	ETHER_ADDR_LEN	6	/* Ethernet address length		*/
#define ETHER_TYPE_LEN	2	/* Ethernet type field length		*/
#define ETHER_CRC_LEN	4	/* Ethernet CRC lenght			*/
#define ETHER_HDR_LEN	((ETHER_ADDR_LEN * 2) + ETHER_TYPE_LEN)
#define ETHER_MIN_LEN	64	/* Minimum frame length, CRC included	*/
#define ETHER_MAX_LEN	1518	/* Maximum frame length, CRC included	*/

struct ether_header {
	uint8   ether_dhost[ETHER_ADDR_LEN];
	uint8   ether_shost[ETHER_ADDR_LEN];
	uint16  ether_type;
};

#define	ETHERTYPE_PUP		0x0200	/* PUP protocol */
#define	ETHERTYPE_IP		0x0800	/* IP protocol */
#define	ETHERTYPE_ARP		0x0806	/* address resolution protocol */
#define	ETHERTYPE_REVARP	0x8035	/* reverse addr resolution protocol */
#define	ETHERTYPE_8021Q		0x8100	/* IEEE 802.1Q VLAN tagging */
#define	ETHERTYPE_IPV6		0x86DD	/* IPv6 protocol */
#define	ETHERTYPE_PPPOEDISC	0x8863	/* PPP Over Ethernet Discovery Stage */
#define	ETHERTYPE_PPPOE		0x8864	/* PPP Over Ethernet Session Stage */
#define	ETHERTYPE_LOOPBACK	0x9000	/* used to test interfaces */

#define	ETHER_IS_MULTICAST(addr) (*(addr) & 0x01) /* is address mcast/bcast? */

#define	ETHERMTU	(ETHER_MAX_LEN - ETHER_HDR_LEN - ETHER_CRC_LEN)
#define	ETHERMIN	(ETHER_MIN_LEN - ETHER_HDR_LEN - ETHER_CRC_LEN)


//#ifdef _NETWORK_STACK

/*
 * Macro to map an IP multicast address to an Ethernet multicast address.
 * The high-order 25 bits of the Ethernet address are statically assigned,
 * and the low-order 23 bits are taken from the low end of the IP address.
 */
#define ETHER_MAP_IP_MULTICAST(ipaddr, enaddr)				\
	/* struct in_addr *ipaddr; */					\
	/* u_int8_t enaddr[ETHER_ADDR_LEN]; */				\
{									\
	(enaddr)[0] = 0x01;						\
	(enaddr)[1] = 0x00;						\
	(enaddr)[2] = 0x5e;						\
	(enaddr)[3] = ((uint8 *)ipaddr)[1] & 0x7f;			\
	(enaddr)[4] = ((uint8 *)ipaddr)[2];				\
	(enaddr)[5] = ((uint8 *)ipaddr)[3];				\
}

/*
 * Macro to map an IPv6 multicast address to an Ethernet multicast address.
 * The high-order 16 bits of the Ethernet address are statically assigned,
 * and the low-order 32 bits are taken from the low end of the IPv6 address.
 */
#define ETHER_MAP_IPV6_MULTICAST(ip6addr, enaddr)			\
	/* struct in6_addr *ip6addr; */					\
	/* u_int8_t enaddr[ETHER_ADDR_LEN]; */				\
{									\
	(enaddr)[0] = 0x33;						\
	(enaddr)[1] = 0x33;						\
	(enaddr)[2] = ((u_int8_t *)ip6addr)[12];			\
	(enaddr)[3] = ((u_int8_t *)ip6addr)[13];			\
	(enaddr)[4] = ((u_int8_t *)ip6addr)[14];			\
	(enaddr)[5] = ((u_int8_t *)ip6addr)[15];			\
}
//#endif

/*
 * Structure shared between the ethernet driver modules and
 * the address resolution code.  For example, each ec_softc or il_softc
 * begins with this structure.
 */
struct arpcom {
	struct   ifnet ac_if;                   /* network-visible interface */
	uint8    ac_enaddr[ETHER_ADDR_LEN];     /* ethernet hardware address */
	struct in_addr ac_ipaddr;
	char     ac__pad[2];                    /* pad for some machines */
//	struct ether_multi *ac_multiaddrs;      /* list of ether multicast addrs */
	int	     ac_multicnt;                   /* length of ac_multiaddrs list */
};

struct ether_device {
	struct arpcom sc_ac;
	struct ether_device *next;  /* next ether_device */
};
#define sc_if    sc_ac.ac_if
#define sc_addr  sc_ac.ac_enaddr

struct ether_arp {
	struct arphdr ea_hdr;
	u_char arp_sha[6];
	u_char arp_spa[4];
	u_char arp_tha[6];
	u_char arp_tpa[4];
};
#define	arp_hrd	ea_hdr.ar_hrd
#define	arp_pro	ea_hdr.ar_pro
#define	arp_hln	ea_hdr.ar_hln
#define	arp_pln	ea_hdr.ar_pln
#define	arp_op	ea_hdr.ar_op

struct llinfo_arp {
	struct llinfo_arp *la_next;
	struct llinfo_arp *la_prev;
	struct rtentry *la_rt;
	struct mbuf *la_hold;
	int32 la_asked;
};

#define la_timer la_rt->rt_rmx.rmx_expire

struct sockaddr_inarp {
	uint8          sin_len;
	uint8          sin_family;
	uint16         sin_port;
	struct in_addr sin_addr;
	struct in_addr sin_srcaddr;
	uint16         sin_tos;
	uint16         sin_other;
};
#define SIN_PROXY 1

/*
 * IP and ethernet specific routing flags
 */
#define	RTF_USETRAILERS	  RTF_PROTO1	/* use trailers */
#define	RTF_ANNOUNCE	  RTF_PROTO2	/* announce new arp entry */
#define	RTF_PERMANENT_ARP RTF_PROTO3    /* only manual overwrite of entry */

//#ifdef _NETWORK_STACK

int arpresolve(struct arpcom *ac, struct rtentry *rt, struct mbuf *m,
               struct sockaddr *dst, uint8 *desten);
//void arpwhohas(struct arpcom *ac, struct in_addr *ia);


//#else

char *ether_ntoa (struct ether_addr *);
struct ether_addr *ether_aton (char *);
int ether_ntohost (char *, struct ether_addr *);
int ether_hostton (char *, struct ether_addr *);
int ether_line(char *line, struct ether_addr *e, char *hostname);

//#endif

#endif /* NETINET_IF_ETHER_H */

