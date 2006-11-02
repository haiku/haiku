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
	CLOSED,
	LISTEN,
	SYN_SENT,
	SYN_RCVD,
	ESTABLISHED,
	CLOSE_WAIT,
	LAST_ACK,
	FIN_WAIT1,
	FIN_WAIT2,
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
#define TCP_FLG_CWR 0x80 // Congestion Window Reduced
#define TCP_FLG_ECN 0x40 // Explicit Congestion Notification echo
#define TCP_FLG_URG 0x20 // URGent
#define TCP_FLG_ACK 0x10 // ACKnowledge
#define TCP_FLG_PUS 0x08 // PUSh
#define TCP_FLG_RST 0x04 // ReSeT
#define TCP_FLG_SYN 0x02 // SYNchronize
#define TCP_FLG_FIN 0x01 // FINish

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

#endif TCP_H
