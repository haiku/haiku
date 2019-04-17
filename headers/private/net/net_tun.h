/*
 * Copyright 2008-2019, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef NET_TUN_H
#define NET_TUN_H


#include <sys/socket.h>
#include <net_device.h>


// name of the kernel tun interface
#define NET_TUN_MODULE_NAME "network/devices/tun/v1"


struct tun_device : net_device {
	//queue
	/*
	int		fd;
	uint32	frame_size;*/
};



struct tun_module_info {
	struct net_device_module_info;

	status_t	(*tun_read)(net_device* device, net_buffer* buffer);
	status_t	(*tun_write)(net_device* device, net_buffer** _buffer);
};

#endif	// NET_TUN_H
