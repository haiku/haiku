/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_NET_IF_H_
#define _FBSD_COMPAT_NET_IF_H_


#include <posix/net/if.h>

#include <sys/cdefs.h>
#include <sys/queue.h>
#include <sys/time.h>


#define IF_Kbps(x)				((x) * 1000)
#define IF_Mbps(x)				(IF_Kbps((x) * 1000))
#define IF_Gbps(x)				(IF_Mbps((x) * 1000))

/* Capabilities that interfaces can advertise. */
#define IFCAP_RXCSUM			0x00001	/* can offload checksum on RX */
#define IFCAP_TXCSUM			0x00002	/* can offload checksum on TX */
#define IFCAP_NETCONS			0x00004	/* can be a network console */
#define	IFCAP_VLAN_MTU			0x00008	/* VLAN-compatible MTU */
#define	IFCAP_VLAN_HWTAGGING	0x00010	/* hardware VLAN tag support */
#define	IFCAP_JUMBO_MTU			0x00020	/* 9000 byte MTU supported */
#define	IFCAP_POLLING			0x00040	/* driver supports polling */
#define	IFCAP_VLAN_HWCSUM		0x00080
#define	IFCAP_TSO4				0x00100	/* supports TCP segmentation offload */
#define	IFCAP_TSO6				0x00200	/* can do TCP6 Segmentation Offload */
#define	IFCAP_WOL_UCAST			0x00800	/* wake on any unicast frame */
#define	IFCAP_WOL_MCAST			0x01000	/* wake on any multicast frame */
#define	IFCAP_WOL_MAGIC			0x02000	/* wake on any Magic Packet */
#define	IFCAP_VLAN_HWFILTER		0x10000	/* interface hw can filter vlan tag */
#define	IFCAP_POLLING_NOCOUNT	0x20000
#define	IFCAP_VLAN_HWTSO		0x40000
#define	IFCAP_LINKSTATE			0x80000

#define IFCAP_HWCSUM	(IFCAP_RXCSUM | IFCAP_TXCSUM)
#define IFCAP_TSO		(IFCAP_TSO4 | IFCAP_TSO6)
#define	IFCAP_WOL		(IFCAP_WOL_UCAST | IFCAP_WOL_MCAST | IFCAP_WOL_MAGIC)


#define IFF_DRV_RUNNING		0x00010000
#define IFF_DRV_OACTIVE		0x00020000
#define IFF_LINK0			0x00040000		/* per link layer defined bit */
#define	IFF_LINK1			0x00080000		/* per link layer defined bit */
#define	IFF_LINK2			0x00100000		/* per link layer defined bit */
#define IFF_DEBUG			0x00200000
#define	IFF_MONITOR			0x00400000		/* (n) user-requested monitor mode */

#define LINK_STATE_UNKNOWN	0
#define LINK_STATE_DOWN		1
#define LINK_STATE_UP		2

#define IFQ_MAXLEN			50


/*
 * Structure describing information about an interface
 * which may be of interest to management entities.
 */
struct if_data {
	/* generic interface information */
	u_char	ifi_type;			/* ethernet, tokenring, etc */
	u_char	ifi_physical;		/* e.g., AUI, Thinnet, 10base-T, etc */
	u_char	ifi_addrlen;		/* media address length */
	u_char	ifi_hdrlen;			/* media header length */
	u_char	ifi_link_state;		/* current link state */
	u_char	ifi_recvquota;		/* polling quota for receive intrs */
	u_char	ifi_xmitquota;		/* polling quota for xmit intrs */
	u_char	ifi_datalen;		/* length of this data struct */
	u_long	ifi_mtu;			/* maximum transmission unit */
	u_long	ifi_metric;			/* routing metric (external only) */
	u_long	ifi_baudrate;		/* linespeed */
	/* volatile statistics */
	u_long	ifi_ipackets;		/* packets received on interface */
	u_long	ifi_ierrors;		/* input errors on interface */
	u_long	ifi_opackets;		/* packets sent on interface */
	u_long	ifi_oerrors;		/* output errors on interface */
	u_long	ifi_collisions;		/* collisions on csma interfaces */
	u_long	ifi_ibytes;			/* total number of octets received */
	u_long	ifi_obytes;			/* total number of octets sent */
	u_long	ifi_imcasts;		/* packets received via multicast */
	u_long	ifi_omcasts;		/* packets sent via multicast */
	u_long	ifi_iqdrops;		/* dropped on input, this interface */
	u_long	ifi_noproto;		/* destined for unsupported protocol */
	u_long	ifi_hwassist;		/* HW offload capabilities */
	time_t	ifi_epoch;			/* uptime at attach or stat reset */
#ifdef __alpha__
	u_int	ifi_timepad;		/* time_t is int, not long on alpha */
#endif
	struct	timeval ifi_lastchange;	/* time of last administrative change */
};

struct  ifdrv {
	char			ifd_name[IFNAMSIZ];     /* if name, e.g. "en0" */
	unsigned long	ifd_cmd;
	size_t			ifd_len;
	void*			ifd_data;
};

#ifdef _KERNEL
/* TODO: this should go away soon. */
#	include <net/if_var.h>
#endif


#endif	/* _FBSD_COMPAT_NET_IF_H_ */
