/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Galante, haiku.galante@gmail.com
 */
#ifndef TCP_H
#define TCP_H


#include <net_buffer.h>
#include <net_datalink.h>
#include <net_stack.h>

#include <util/khash.h>

#include <sys/socket.h>


typedef enum {
	// establishing a connection
	CLOSED,
	LISTEN,
	SYNCHRONIZE_SENT,
	SYNCHRONIZE_RECEIVED,
	ESTABLISHED,

	// peer closes the connection
	FINISH_RECEIVED,
	WAIT_FOR_FINISH_ACKNOWLEDGE,

	// we close the connection
	FINISH_SENT,
	FINISH_ACKNOWLEDGED,
	CLOSING,
	TIME_WAIT
} tcp_state;

struct tcp_header {
	uint16 source_port;
	uint16 destination_port;
	uint32 sequence_num;
	uint32 acknowledge_num;
	struct {
#if B_HOST_IS_LENDIAN == 1
		uint8 reserved : 4;
		uint8 header_length : 4;
#else
		uint8 header_length : 4;
		uint8 reserved : 4;
#endif
	};
	uint8  flags;
	uint16 advertised_window;
	uint16 checksum;
	uint16 urgent_ptr;
};

// TCP flag constants
#define TCP_FLAG_FINISH			0x01
#define TCP_FLAG_SYNCHRONIZE	0x02
#define TCP_FLAG_RESET			0x04
#define TCP_FLAG_PUSH			0x08
#define TCP_FLAG_ACKNOWLEDGE	0x10
#define TCP_FLAG_URGENT			0x20
#define TCP_FLAG_ECN			0x40 // Explicit Congestion Notification echo
#define TCP_FLAG_CWR			0x80 // Congestion Window Reduced

struct tcp_connection_key {
	const sockaddr	*local;
	const sockaddr	*peer;
};


extern net_domain *gDomain;
extern net_address_module_info *gAddressModule;
extern net_buffer_module_info *gBufferModule;
extern net_datalink_module_info *gDatalinkModule;
extern net_stack_module_info *gStackModule;
extern hash_table *gConnectionHash;
extern benaphore gConnectionLock;


status_t add_tcp_header(net_buffer *buffer, uint16 flags, uint32 sequence,
	uint32 ack, uint16 advertisedWindow);

#endif	// TCP_H
