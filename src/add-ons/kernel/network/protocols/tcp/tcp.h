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
#include <net_socket.h>
#include <net_stack.h>

#include <util/khash.h>

#include <ByteOrder.h>

#include <sys/socket.h>


class TCPConnection;

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
	uint32 sequence;
	uint32 acknowledge;
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
	uint16 urgent_offset;

	uint32 HeaderLength() const { return (uint32)header_length << 2; }
	uint32 Sequence() const { return ntohl(sequence); }
	uint32 Acknowledge() const { return ntohl(acknowledge); }
	uint16 AdvertisedWindow() const { return ntohs(advertised_window); }
	uint16 Checksum() const { return ntohs(checksum); }
	uint16 UrgentOffset() const { return ntohs(urgent_offset); }
} _PACKED;

class tcp_sequence {
	public:
		tcp_sequence() {}
		tcp_sequence(uint32 sequence) : number(sequence) {}

		operator uint32() const { return number; }

		void operator=(uint32 sequence) { number = sequence; }
		bool operator>(uint32 sequence) const { return (int32)(number - sequence) > 0; }
		bool operator>=(uint32 sequence) const { return (int32)(number - sequence) >= 0; }
		bool operator<(uint32 sequence) const { return (int32)(number - sequence) < 0; }
		bool operator<=(uint32 sequence) const { return (int32)(number - sequence) <= 0; }

		uint32& operator+=(uint32 sequence) { return number += sequence; }
		uint32& operator++() { return ++number; }
		uint32 operator++(int _) { return number++; }

	private:
		uint32 number;
};

// TCP flag constants
#define TCP_FLAG_FINISH			0x01
#define TCP_FLAG_SYNCHRONIZE	0x02
#define TCP_FLAG_RESET			0x04
#define TCP_FLAG_PUSH			0x08
#define TCP_FLAG_ACKNOWLEDGE	0x10
#define TCP_FLAG_URGENT			0x20
#define TCP_FLAG_CONGESTION_NOTIFICATION_ECHO	0x40
#define TCP_FLAG_CONGESTION_WINDOW_REDUCED		0x80

#define TCP_CONNECTION_TIMEOUT	75000000	// 75 secs

struct tcp_option {
	uint8	kind;
	uint8	length;
	union {
		uint8	window_shift;
		uint16	max_segment_size;
		uint32	timestamp;
	};
	uint32	timestamp_reply;
} _PACKED;

enum tcp_option_kind {
	TCP_OPTION_END	= 0,
	TCP_OPTION_NOP	= 1,
	TCP_OPTION_MAX_SEGMENT_SIZE = 2,
	TCP_OPTION_WINDOW_SHIFT = 3,
	TCP_OPTION_TIMESTAMP = 8,
};

#define TCP_MAX_WINDOW_SHIFT	14

struct tcp_segment_header {
	tcp_segment_header() : window_shift(0), max_segment_size(0) {}
		// constructor zeros options

	uint32	sequence;
	uint32	acknowledge;
	uint16	advertised_window;
	uint16	urgent_offset;
	uint8	flags;
	uint8	window_shift;
	uint16	max_segment_size;

	bool AcknowledgeOnly() const
	{
		return (flags & (TCP_FLAG_SYNCHRONIZE | TCP_FLAG_FINISH | TCP_FLAG_RESET
			| TCP_FLAG_URGENT | TCP_FLAG_ACKNOWLEDGE)) == TCP_FLAG_ACKNOWLEDGE;
	}
};

enum tcp_segment_action {
	KEEP		= 0x00,
	DROP		= 0x01,
	RESET		= 0x02,
	ACKNOWLEDGE	= 0x04,
	IMMEDIATE_ACKNOWLEDGE = 0x08,
};

struct tcp_connection_key {
	const sockaddr	*local;
	const sockaddr	*peer;
};


extern net_domain *gDomain;
extern net_address_module_info *gAddressModule;
extern net_buffer_module_info *gBufferModule;
extern net_datalink_module_info *gDatalinkModule;
extern net_socket_module_info *gSocketModule;
extern net_stack_module_info *gStackModule;
//extern hash_table *gConnectionHash;
//extern benaphore gConnectionLock;


status_t add_tcp_header(tcp_segment_header &segment, net_buffer *buffer);
status_t remove_connection(TCPConnection *connection);
status_t insert_connection(TCPConnection *connection);

#endif	// TCP_H
