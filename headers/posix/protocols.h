/* protocols.h
 * The various protocols we're likely to come across as
 * they're identified in the type fields of packets...
 */

#include "netinet/in.h"

#ifndef OBOS_PROTOCOLS_H
#define OBOS_PROTOCOLS_H

/* define some protocol numbers unique to ethernet */
enum {
	ETHER_IPV4        = 0x0800,
	ETHER_ARP         = 0x0806,
	ETHER_RARP        = 0x8035,	 
	ETHER_ATALK       = 0x809b, /* Appletalk */
	ETHER_SNMP        = 0x814c, /* SNMP */
	ETHER_IPV6        = 0x86dd, /* IPv6 */
	ETHER_PPPOE_DISC  = 0x8863, /* PPPoE Discovery */
	ETHER_PPPOE_SESS  = 0x8864  /* PPPoE Session */
};

/* these are used when assigning slots in the protocol table, so they
 * should tie in with IP numbers wherever possible, with other 
 * protocols fitting in.
 * These are used in the module definitions.
 */
enum {
	NS_IPV4	= IPPROTO_IP,
	NS_ICMP	= IPPROTO_ICMP,
	NS_IGMP	= IPPROTO_IGMP,
	NS_TCP 	= IPPROTO_TCP,
	NS_UDP 	= IPPROTO_UDP,
	NS_ETHER=200,
	NS_IPV6,
	NS_ATALK,
	NS_ARP,
	NS_RARP,
	NS_PPPOE,
	NS_SERIAL,
	NS_LOOP,
	NS_SOCKET,
	NS_ROUTE
};


#endif /* OBOS_PROTOCOLS_H */

