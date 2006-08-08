/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _NET_IF_H
#define _NET_IF_H


#include <net/route.h>
#include <sys/socket.h>


#define IF_NAMESIZE	32

/* BSD specific/proprietary part */

#define IFNAMSIZ IF_NAMESIZE

struct ifreq_parameter {
	char		base_name[IF_NAMESIZE];
	char		device[128];
	uint8_t		sub_type;
};

struct ifreq_stream_stats {
	uint32_t	packets;
	uint32_t	errors;
	uint64_t	bytes;
	uint32_t	multicast_packets;
	uint32_t	dropped;
};

struct ifreq_stats {
	struct ifreq_stream_stats receive;
	struct ifreq_stream_stats send;
	uint32_t	collisions;
};

struct ifreq {
	char		ifr_name[IF_NAMESIZE];
	union {
		struct sockaddr ifr_addr;
		struct sockaddr ifr_dstaddr;
		struct sockaddr ifr_broadaddr;
		struct sockaddr ifr_mask;
		struct ifreq_parameter ifr_parameter;
		struct ifreq_stats ifr_stats;
		struct route_entry ifr_route;
		int		ifr_flags;
		int		ifr_index;
		int		ifr_metric;
		int		ifr_mtu;
		int		ifr_media;
		int		ifr_type;
	};
};

/* interface flags */
#define IFF_UP			0x0001
#define IFF_BROADCAST	0x0002	/* valid broadcast address */
#define IFF_LOOPBACK	0x0008
#define IFF_POINTOPOINT	0x0010	/* point-to-point link */
#define IFF_NOARP		0x0040	/* no address resolution */
#define IFF_AUTOUP		0x0080	/* auto dial */
#define IFF_PROMISC		0x0100	/* receive all packets */
#define IFF_ALLMULTI	0x0200	/* receive all multicast packets */
#define IFF_SIMPLEX		0x0800	/* doesn't receive own transmissions */
#define IFF_MULTICAST	0x8000	/* supports multicast */

struct ifconf {
	int			ifc_len;	/* size of buffer */
	union {
		void	*ifc_buf;
		struct ifreq *ifc_req;
		int		ifc_value;
	};
};

/* POSIX definitions follow */

struct if_nameindex {
	unsigned	if_index;	/* positive interface index */
	char		*if_name;	/* interface name, ie. "loopback" */
};


#ifdef __cplusplus
extern "C" {
#endif

unsigned if_nametoindex(const char *name);
char *if_indextoname(unsigned index, char *nameBuffer);
struct if_nameindex *if_nameindex(void);
void if_freenameindex(struct if_nameindex *interfaceArray);

#ifdef __cplusplus
}
#endif

#endif	/* _NET_IF_H */
