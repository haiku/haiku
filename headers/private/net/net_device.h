/*
 * Copyright 2006-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef NET_DEVICE_H
#define NET_DEVICE_H


#include <module.h>

#include <net/if.h>


struct net_hardware_address {
	uint8	data[64];
	uint8	length;
};

struct net_device {
	struct net_device_module_info *module;

	char	name[IF_NAMESIZE];
	uint32	index;
	uint32	flags;		// IFF_LOOPBACK, ...
	uint32	type;		// IFT_ETHER, ...
	size_t	mtu;
	uint32	media;
	uint64	link_speed;
	uint32	link_quality;
	size_t	header_length;

	struct net_hardware_address address;

	ifreq_stats stats;
};

struct net_device_module_info {
	struct module_info info;

	status_t	(*init_device)(const char *name, struct net_device **_device);
	status_t	(*uninit_device)(struct net_device *device);

	status_t	(*up)(struct net_device *device);
	void		(*down)(struct net_device *device);

	status_t	(*control)(struct net_device *device, int32 op,
					void *argument, size_t length);

	status_t	(*send_data)(struct net_device *device,
					struct net_buffer *buffer);
	status_t	(*receive_data)(struct net_device *device,
					struct net_buffer **_buffer);

	status_t	(*set_mtu)(struct net_device *device, size_t mtu);
	status_t	(*set_promiscuous)(struct net_device *device, bool promiscuous);
	status_t	(*set_media)(struct net_device *device, uint32 media);

	status_t	(*get_multicast_addrs)(struct net_device *device,
					net_hardware_address **addressArray, uint32 count);
	status_t	(*set_multicast_addrs)(struct net_device *device, 
					const net_hardware_address **addressArray, uint32 count);
};

#endif	// NET_DEVICE_H
