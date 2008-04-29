/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SIMPLE_NET_BUFFER_H
#define SIMPLE_NET_BUFFER_H

#include <net_buffer.h>


struct simple_net_buffer : net_buffer {
	uint8*				data;

	struct {
		struct sockaddr_storage source;
		struct sockaddr_storage destination;
	} storage;
};


extern net_buffer_module_info gSimpleNetBufferModule;


#endif	// SIMPLE_NET_BUFFER_H
