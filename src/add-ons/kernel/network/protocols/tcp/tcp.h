/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Andrew Galante, haiku.galante@gmail.com
 *		Hugo Santos, hugosantos@gmail.com
 */
#ifndef TCP_H
#define TCP_H


#include <net_buffer.h>
#include <net_datalink.h>
#include <net_socket.h>
#include <net_stack.h>

#include <ByteOrder.h>

#include <sys/socket.h>


class EndpointManager;

enum tcp_state {
	// establishing a connection
	CLOSED,
	LISTEN,
	SYNCHRONIZE_SENT,
	SYNCHRONIZE_RECEIVED,
	ESTABLISHED,

	// peer closes the connection
	FINISH_RECEIVED,				// close-wait
	WAIT_FOR_FINISH_ACKNOWLEDGE,	// last-ack

	// we close the connection
	FINISH_SENT,					// fin-wait-1
	FINISH_ACKNOWLEDGED,			// fin-wait-2
	CLOSING,
	TIME_WAIT
};

struct tcp_header {
	uint16	source_port;
	uint16	destination_port;
	uint32	sequence;
	uint32	acknowledge;
	struct {
#if B_HOST_IS_LENDIAN == 1
		uint8	reserved : 4;
		uint8	header_length : 4;
#else
		uint8	header_length : 4;
		uint8	reserved : 4;
#endif
	};
	uint8	flags;
	uint16	advertised_window;
	uint16	checksum;
	uint16	urgent_offset;

	uint32	HeaderLength() const { return (uint32)header_length << 2; }
	uint32	Sequence() const { return B_BENDIAN_TO_HOST_INT32(sequence); }
	uint32	Acknowledge() const { return B_BENDIAN_TO_HOST_INT32(acknowledge); }
	uint16	AdvertisedWindow() const { return B_BENDIAN_TO_HOST_INT16(advertised_window); }
	uint16	Checksum() const { return B_BENDIAN_TO_HOST_INT16(checksum); }
	uint16	UrgentOffset() const { return B_BENDIAN_TO_HOST_INT16(urgent_offset); }
} _PACKED;

class tcp_sequence {
public:
	inline tcp_sequence() {}
	inline tcp_sequence(uint32 sequence)
		: fNumber(sequence)
	{
	}

	inline uint32 Number() const
	{
		return fNumber;
	}

	inline tcp_sequence& operator=(tcp_sequence sequence)
	{
		fNumber = sequence.fNumber;
		return *this;
	}

	inline tcp_sequence& operator+=(tcp_sequence sequence)
	{
		fNumber += sequence.fNumber;
		return *this;
	}

	inline tcp_sequence& operator++()
	{
		fNumber++;
		return *this;
	}

	inline tcp_sequence operator++(int _)
	{
		fNumber++;
		return fNumber - 1;
	}

private:
	uint32	fNumber;
};


// Global tcp_sequence Operators


inline bool
operator>(tcp_sequence a, tcp_sequence b)
{
	return (int32)(a.Number() - b.Number()) > 0;
}


inline bool
operator>=(tcp_sequence a, tcp_sequence b)
{
	return (int32)(a.Number() - b.Number()) >= 0;
}


inline bool
operator<(tcp_sequence a, tcp_sequence b)
{
	return (int32)(a.Number() - b.Number()) < 0;
}


inline bool
operator<=(tcp_sequence a, tcp_sequence b)
{
	return (int32)(a.Number() - b.Number()) <= 0;
}


inline tcp_sequence
operator+(tcp_sequence a, tcp_sequence b)
{
	return a.Number() + b.Number();
}


inline tcp_sequence
operator-(tcp_sequence a, tcp_sequence b)
{
	return a.Number() - b.Number();
}


inline bool
operator!=(tcp_sequence a, tcp_sequence b)
{
	return a.Number() != b.Number();
}


inline bool
operator==(tcp_sequence a, tcp_sequence b)
{
	return a.Number() == b.Number();
}


// TCP flag constants
#define TCP_FLAG_FINISH					0x01
#define TCP_FLAG_SYNCHRONIZE			0x02
#define TCP_FLAG_RESET					0x04
#define TCP_FLAG_PUSH					0x08
#define TCP_FLAG_ACKNOWLEDGE			0x10
#define TCP_FLAG_URGENT					0x20
#define TCP_FLAG_CONGESTION_NOTIFICATION_ECHO	0x40
#define TCP_FLAG_CONGESTION_WINDOW_REDUCED		0x80

#define TCP_CONNECTION_TIMEOUT			75000000	// 75 secs
#define TCP_DELAYED_ACKNOWLEDGE_TIMEOUT	100000		// 100 msecs
#define TCP_DEFAULT_MAX_SEGMENT_SIZE	536
#define TCP_MAX_WINDOW					65535
#define TCP_MAX_SEGMENT_LIFETIME		60000000	// 60 secs
#define TCP_PERSIST_TIMEOUT				1000000		// 1 sec

// Initial estimate for packet round trip time (RTT)
#define TCP_INITIAL_RTT					2000000		// 2 secs
// Minimum retransmit timeout (consider delayed ack)
#define TCP_MIN_RETRANSMIT_TIMEOUT		200000		// 200 msecs
// Maximum retransmit timeout (per RFC6298)
#define TCP_MAX_RETRANSMIT_TIMEOUT		60000000	// 60 secs
// New value for timeout in case of lost SYN (RFC 6298)
#define TCP_SYN_RETRANSMIT_TIMEOUT 		3000000		// 3 secs

struct tcp_sack {
	uint32 left_edge;
	uint32 right_edge;
} _PACKED;

struct tcp_option {
	uint8	kind;
	uint8	length;
	union {
		uint8		window_shift;
		uint16		max_segment_size;
		struct {
			uint32	value;
			uint32	reply;
		}			timestamp;
		tcp_sack	sack[0];
	};
} _PACKED;

enum tcp_option_kind {
	TCP_OPTION_END				= 0,
	TCP_OPTION_NOP				= 1,
	TCP_OPTION_MAX_SEGMENT_SIZE	= 2,
	TCP_OPTION_WINDOW_SHIFT		= 3,
	TCP_OPTION_SACK_PERMITTED	= 4,
	TCP_OPTION_SACK				= 5,
	TCP_OPTION_TIMESTAMP		= 8,
};

#define TCP_MAX_WINDOW_SHIFT	14

enum {
	TCP_HAS_WINDOW_SCALE	= 1 << 0,
	TCP_HAS_TIMESTAMPS		= 1 << 1,
	TCP_SACK_PERMITTED		= 1 << 2,
};

struct tcp_segment_header {
	tcp_segment_header(uint8 _flags)
		:
		flags(_flags),
		window_shift(0),
		max_segment_size(0),
		sack_count(0),
		options(0)
	{}

	uint32	sequence;
	uint32	acknowledge;
	uint16	advertised_window;
	uint16	urgent_offset;
	uint8	flags;
	uint8	window_shift;
	uint16	max_segment_size;

	uint32	timestamp_value;
	uint32	timestamp_reply;

	tcp_sack	*sacks;
	int			sack_count;

	uint32	options;

	bool AcknowledgeOnly() const
	{
		return (flags & (TCP_FLAG_SYNCHRONIZE | TCP_FLAG_FINISH | TCP_FLAG_RESET
			| TCP_FLAG_URGENT | TCP_FLAG_ACKNOWLEDGE)) == TCP_FLAG_ACKNOWLEDGE;
	}
};

enum tcp_segment_action {
	KEEP					= 0x00,
	DROP					= 0x01,
	RESET					= 0x02,
	ACKNOWLEDGE				= 0x04,
	IMMEDIATE_ACKNOWLEDGE	= 0x08,
	DELETED_ENDPOINT		= 0x10,
};


extern net_buffer_module_info* gBufferModule;
extern net_datalink_module_info* gDatalinkModule;
extern net_socket_module_info* gSocketModule;
extern net_stack_module_info* gStackModule;


EndpointManager* get_endpoint_manager(net_domain* domain);
void put_endpoint_manager(EndpointManager* manager);

status_t add_tcp_header(net_address_module_info* addressModule,
	tcp_segment_header& segment, net_buffer* buffer);
size_t tcp_options_length(tcp_segment_header& segment);

const char* name_for_state(tcp_state state);

#endif	// TCP_H
