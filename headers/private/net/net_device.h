/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef NET_DEVICE_H
#define NET_DEVICE_H


#include <net/if.h>

#include <module.h>


typedef struct net_buffer net_buffer;


struct net_hardware_address {
	uint8	data[64];
	uint8	length;
};

typedef struct net_device {
	struct net_device_module_info* module;

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

	struct ifreq_stats stats;
} net_device;


struct net_device_module_info {
	struct module_info info;

	status_t	(*init_device)(const char* name, net_device** _device);
	status_t	(*uninit_device)(net_device* device);

	status_t	(*up)(net_device* device);
	void		(*down)(net_device* device);

	status_t	(*control)(net_device* device, int32 op, void* argument,
					size_t length);

	status_t	(*send_data)(net_device* device, net_buffer* buffer);
	status_t	(*receive_data)(net_device* device, net_buffer** _buffer);

	status_t	(*set_mtu)(net_device* device, size_t mtu);
	status_t	(*set_promiscuous)(net_device* device, bool promiscuous);
	status_t	(*set_media)(net_device* device, uint32 media);

	status_t	(*add_multicast)(net_device* device,
					const struct sockaddr* address);
	status_t	(*remove_multicast)(net_device* device,
					const struct sockaddr* address);
};


#endif	// NET_DEVICE_H
