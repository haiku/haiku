/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_NET_IF_H_
#define _FBSD_COMPAT_NET_IF_H_


#include <posix/net/if.h>

#include <sys/time.h>


#define IF_Kbps(x)		((x) * 1000)
#define IF_Mbps(x)		(IF_Kbps((x) * 1000))
#define IF_Gbps(x)		(IF_Mbps((x) * 1000))


/* Capabilities that interfaces can advertise. */
#define IFCAP_RXCSUM		0x0001  /* can offload checksum on RX */
#define IFCAP_TXCSUM		0x0002  /* can offload checksum on TX */
#define IFCAP_NETCONS		0x0004  /* can be a network console */
#define	IFCAP_VLAN_MTU		0x0008	/* VLAN-compatible MTU */
#define	IFCAP_VLAN_HWTAGGING	0x0010	/* hardware VLAN tag support */
#define	IFCAP_JUMBO_MTU		0x0020	/* 9000 byte MTU supported */
#define	IFCAP_POLLING		0x0040	/* driver supports polling */
#define	IFCAP_VLAN_HWCSUM	0x0080
#define	IFCAP_TSO4			0x0100	/* supports TCP segmentation offload */
#define	IFCAP_VLAN_HWFILTER	0x10000 /* interface hw can filter vlan tag */

#define IFCAP_HWCSUM		(IFCAP_RXCSUM | IFCAP_TXCSUM)


#define IFF_DRV_RUNNING		0x10000
#define IFF_DRV_OACTIVE		0x20000
#define IFF_LINK0			0x40000
#define IFF_DEBUG			0x80000


#define LINK_STATE_UNKNOWN	0
#define LINK_STATE_DOWN		1
#define LINK_STATE_UP		2


#define IFQ_MAXLEN			50


struct ifmediareq {
	char	ifm_name[IFNAMSIZ];	/* if name, e.g. "en0" */
	int	ifm_current;		/* current media options */
	int	ifm_mask;		/* don't care mask */
	int	ifm_status;		/* media status */
	int	ifm_active;		/* active options */
	int	ifm_count;		/* # entries in ifm_ulist array */
	int	*ifm_ulist;		/* media words */
};

/*
 * Structure describing information about an interface
 * which may be of interest to management entities.
 */
struct if_data {
	/* generic interface information */
	u_char	ifi_type;		/* ethernet, tokenring, etc */
	u_char	ifi_physical;		/* e.g., AUI, Thinnet, 10base-T, etc */
	u_char	ifi_addrlen;		/* media address length */
	u_char	ifi_hdrlen;		/* media header length */
	u_char	ifi_link_state;		/* current link state */
	u_char	ifi_recvquota;		/* polling quota for receive intrs */
	u_char	ifi_xmitquota;		/* polling quota for xmit intrs */
	u_char	ifi_datalen;		/* length of this data struct */
	u_long	ifi_mtu;		/* maximum transmission unit */
	u_long	ifi_metric;		/* routing metric (external only) */
	u_long	ifi_baudrate;		/* linespeed */
	/* volatile statistics */
	u_long	ifi_ipackets;		/* packets received on interface */
	u_long	ifi_ierrors;		/* input errors on interface */
	u_long	ifi_opackets;		/* packets sent on interface */
	u_long	ifi_oerrors;		/* output errors on interface */
	u_long	ifi_collisions;		/* collisions on csma interfaces */
	u_long	ifi_ibytes;		/* total number of octets received */
	u_long	ifi_obytes;		/* total number of octets sent */
	u_long	ifi_imcasts;		/* packets received via multicast */
	u_long	ifi_omcasts;		/* packets sent via multicast */
	u_long	ifi_iqdrops;		/* dropped on input, this interface */
	u_long	ifi_noproto;		/* destined for unsupported protocol */
	u_long	ifi_hwassist;		/* HW offload capabilities */
	time_t	ifi_epoch;		/* uptime at attach or stat reset */
#ifdef __alpha__
	u_int	ifi_timepad;		/* time_t is int, not long on alpha */
#endif
	struct	timeval ifi_lastchange;	/* time of last administrative change */
};

#endif
