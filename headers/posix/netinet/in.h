/* in.h */
#ifndef _NETINET_IN_H_
#define _NETINET_IN_H_

#include <ByteOrder.h> /* for htonl */

#ifdef _NETWORK_STACK
#include "net_misc.h"
#endif
#include "net/if.h"

/* XXX - This really doesn't belong in here... */
/* XXX - move these to sys/param.h */
typedef uint32	in_addr_t; 
typedef uint16  in_port_t;

/* Protocol definitions - add to as required... */

enum {
	IPPROTO_IP      =   0,	/* 0, IPv4 */
	IPPROTO_ICMP    =   1,	/* 1, ICMP (v4) */
	IPPROTO_IGMP    =   2,	/* 2, IGMP (group management) */
	IPPROTO_TCP	    =   6,	/* 6, tcp */
	IPPROTO_UDP	    =  17,	/* 17, UDP */
	IPPROTO_IPV6    =  41,   /* 41, IPv6 in IPv6 */
	IPPROTO_ROUTING =  43,	/* 43, Routing */
	IPPROTO_ICMPV6  =  58,	/* 58, IPv6 ICMP */
	IPPROTO_ETHERIP =  97,	/* 97, Ethernet in IPv4 */
	IPPROTO_RAW	    = 255    /* 255 */
};

#define IPPROTO_MAX	256

/* Port numbers...
 * < IPPORT_RESERVED are privileged and should be
 * accessible only by root
 * > IPPORT_USERRESERVED are reserved for servers, though
 * not requiring privileged status
 */

#define IPPORT_RESERVED         1024
#define IPPORT_USERRESERVED     49151

/* This is an IPv4 address structure. Why is it a structure?
 * Historical reasons.
 */ 
struct in_addr {
	in_addr_t s_addr;
};

/*
 * IP Version 4 socket address.
 */
struct sockaddr_in {
	uint8		sin_len;
	uint8		sin_family;
	uint16		sin_port;
	struct in_addr 	sin_addr;
	int8		sin_zero[8];
};
/* the address is therefore at sin_addr.s_addr */

/*
 * Options for use with [gs]etsockopt at the IP level.
 * First word of comment is data type; bool is stored in int.
 */
#define IP_OPTIONS               1   /* buf/ip_opts; set/get IP options */
#define IP_HDRINCL               2   /* int; header is included with data */
#define IP_TOS                   3   /* int; IP type of service and preced. */
#define IP_TTL                   4   /* int; IP time to live */
#define IP_RECVOPTS              5   /* bool; receive all IP opts w/dgram */
#define IP_RECVRETOPTS           6   /* bool; receive IP opts for response */
#define IP_RECVDSTADDR           7   /* bool; receive IP dst addr w/dgram */
#define IP_RETOPTS               8   /* ip_opts; set/get IP options */
#define IP_MULTICAST_IF          9   /* in_addr; set/get IP multicast i/f  */
#define IP_MULTICAST_TTL        10   /* u_char; set/get IP multicast ttl */
#define IP_MULTICAST_LOOP       11   /* u_char; set/get IP multicast loopback */
#define IP_ADD_MEMBERSHIP       12   /* ip_mreq; add an IP group membership */
#define IP_DROP_MEMBERSHIP      13   /* ip_mreq; drop an IP group membership */ 

#ifdef _NETWORK_STACK
#define __IPADDR(x)     ((uint32) htonl((uint32)(x)))
#else
#define __IPADDR(x)     ((uint32)(x))
#endif

#define INADDR_ANY              __IPADDR(0x00000000)
#define INADDR_LOOPBACK         __IPADDR(0x7f000001)
#define INADDR_BROADCAST        __IPADDR(0xffffffff) /* must be masked */

#define INADDR_UNSPEC_GROUP     __IPADDR(0xe0000000)    /* 224.0.0.0 */
#define INADDR_ALLHOSTS_GROUP   __IPADDR(0xe0000001)    /* 224.0.0.1 */
#define INADDR_ALLROUTERS_GROUP __IPADDR(0xe0000002)    /* 224.0.0.2 */
#define INADDR_MAX_LOCAL_GROUP  __IPADDR(0xe00000ff)    /* 224.0.0.255 */

#define IN_LOOPBACKNET          127                     /* official! */

#ifndef _NETWORK_STACK
#define INADDR_NONE             __IPADDR(0xffffffff)
#endif

#define IN_CLASSA(i)            (((uint32)(i) & __IPADDR(0x80000000)) == \
                                 __IPADDR(0x00000000))
#define IN_CLASSA_NET           __IPADDR(0xff000000)
#define IN_CLASSA_NSHIFT        24
#define IN_CLASSA_HOST          __IPADDR(0x00ffffff)
#define IN_CLASSA_MAX           128

#define IN_CLASSB(i)            (((uint32)(i) & __IPADDR(0xc0000000)) == \
                                 __IPADDR(0x80000000))
#define IN_CLASSB_NET           __IPADDR(0xffff0000)
#define IN_CLASSB_NSHIFT        16
#define IN_CLASSB_HOST          __IPADDR(0x0000ffff)
#define IN_CLASSB_MAX           65536

#define IN_CLASSC(i)            (((uint32)(i) & __IPADDR(0xe0000000)) == \
                                 __IPADDR(0xc0000000))
#define IN_CLASSC_NET           __IPADDR(0xffffff00)
#define IN_CLASSC_NSHIFT        8
#define IN_CLASSC_HOST          __IPADDR(0x000000ff)

#define IN_CLASSD(i)            (((uint32)(i) & __IPADDR(0xf0000000)) == \
                                 __IPADDR(0xe0000000))
/* These ones aren't really net and host fields, but routing needn't know. */
#define IN_CLASSD_NET           __IPADDR(0xf0000000)
#define IN_CLASSD_NSHIFT        28
#define IN_CLASSD_HOST          __IPADDR(0x0fffffff)

#define IN_MULTICAST(i)	        IN_CLASSD(i)

#define IN_EXPERIMENTAL(i)      (((uint32)(i) & 0xf0000000) == 0xf0000000)
#define IN_BADCLASS(i)          (((uint32)(i) & 0xf0000000) == 0xf0000000)

#define IP_MAX_MEMBERSHIPS      20

/* some helpful macro's :) */
#define in_hosteq(s,t)  ((s).s_addr == (t).s_addr)
#define in_nullhost(x)  ((x).s_addr == INADDR_ANY)
#define satosin(sa)     ((struct sockaddr_in *)(sa))
#define sintosa(sin)    ((struct sockaddr *)(sin))

/* Prototypes... */
int  in_broadcast  (struct in_addr, struct ifnet *); 
int  in_canforward (struct in_addr);
int  in_localaddr  (struct in_addr);
void in_socktrim   (struct sockaddr_in*);

#endif /* NETINET_IN_H */
