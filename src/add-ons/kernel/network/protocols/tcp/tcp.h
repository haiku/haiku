/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Galante, haiku.galante@gmail.com
 */

#include <ByteOrder.h>

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
	uint32 options;
};

/* TCP flag constants */
#define TCP_FLG_CWR 0x80 /* Congestion Window Reduced */
#define TCP_FLG_ECN 0x40 /* Explicit Congestion Notification echo */
#define TCP_FLG_URG 0x20 /* URGent */
#define TCP_FLG_ACK 0x10 /* ACKnowledge */
#define TCP_FLG_PUS 0x08 /* PUSh */
#define TCP_FLG_RST 0x04 /* ReSeT */
#define TCP_FLG_SYN 0x02 /* SYNchronize */
#define TCP_FLG_FIN 0x01 /* FINish */
