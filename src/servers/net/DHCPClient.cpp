/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "DHCPClient.h"
#include "NetServer.h"

#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>


// See RFC 2131 for DHCP, see RFC 1533 for BOOTP/DHCP options

#define DHCP_CLIENT_PORT	68
#define DHCP_SERVER_PORT	67

enum message_opcode {
	BOOT_REQUEST = 1,
	BOOT_REPLY
};

enum message_option {
	OPTION_MAGIC = 0x63825363,

	// generic options
	OPTION_PAD = 0,
	OPTION_END = 1,
	OPTION_DATAGRAM_SIZE = 22,
	OPTION_MTU = 26,
	OPTION_BROADCAST_ADDRESS = 28,
	OPTION_NETWORK_TIME_SERVERS = 42,

	// DHCP specific options
	OPTION_REQUEST_IP_ADDRESS = 50,
	OPTION_ADDRESS_LEASE_TIME = 51,
	OPTION_OVERLOAD = 52,
	OPTION_MESSAGE_TYPE = 53,
	OPTION_SERVER_ADDRESS = 54,
	OPTION_REQUEST_PARAMETERS = 55,
	OPTION_ERROR_MESSAGE = 56,
	OPTION_MESSAGE_SIZE = 57,
	OPTION_RENEWAL_TIME = 58,
	OPTION_REBINDING_TIME = 59,
	OPTION_CLASS_IDENTIFIER = 60,
	OPTION_CLIENT_IDENTIFIER = 61,
};

enum message_type {
	DHCP_DISCOVER = 1,
	DHCP_OFFER,
	DHCP_REQUEST,
	DHCP_DECLINE,
	DHCP_ACK,
	DHCP_NAK,
	DHCP_RELEASE,
	DHCP_INFORM
};

struct dhcp_option_cookie {
	dhcp_option_cookie() : state(0), file_has_options(false), server_name_has_options(false) {}

	const uint8* next;
	uint8	state;
	bool	file_has_options;
	bool	server_name_has_options;
};

struct dhcp_message {
	dhcp_message(message_type type);

	uint8		opcode;
	uint8		hardware_type;
	uint8		hardware_address_length;
	uint8		hop_count;
	uint32		transaction_id;
	uint16		seconds_since_boot;
	uint16		flags;
	in_addr_t	client_address;
	in_addr_t	your_address;
	in_addr_t	server_address;
	in_addr_t	gateway_address;
	uint8		mac_address[16];
	uint8		server_name[64];
	uint8		file[128];
	uint32		options_magic;
	uint8		options[1260];

	size_t MinSize() const { return 576; }
	size_t Size() const;

	bool HasOptions() const;
	bool NextOption(dhcp_option_cookie& cookie, message_option& option,
		const uint8*& data, size_t& size) const;
	const uint8* LastOption() const;

	uint8* PutOption(uint8* options, message_option option);
	uint8* PutOption(uint8* options, message_option option, uint8 data);
	uint8* PutOption(uint8* options, message_option option, uint16 data);
	uint8* PutOption(uint8* options, message_option option, uint32 data);
	uint8* PutOption(uint8* options, message_option option, uint8* data, uint32 size);
} _PACKED;


#define ARP_HARDWARE_TYPE_ETHER	1


dhcp_message::dhcp_message(message_type type)
{
	memset(this, 0, sizeof(*this));
	options_magic = htonl(OPTION_MAGIC);

	uint8* next = options;
	next = PutOption(next, OPTION_MESSAGE_TYPE, (uint8)type);
	next = PutOption(next, OPTION_MESSAGE_SIZE, (uint16)sizeof(dhcp_message));
	next = PutOption(next, OPTION_END);
}


bool
dhcp_message::HasOptions() const
{
	return options_magic == htonl(OPTION_MAGIC);
}


bool
dhcp_message::NextOption(dhcp_option_cookie& cookie,
	message_option& option, const uint8*& data, size_t& size) const
{
	if (cookie.state == 0) {
		if (!HasOptions())
			return false;

		cookie.state++;
		cookie.next = options;
	}

	uint32 bytesLeft = 0;

	switch (cookie.state) {
		case 1:
			// options from "options"
			bytesLeft = sizeof(options) + cookie.next - options;
			break;

		case 2:
			// options from "file"
			bytesLeft = sizeof(options) + cookie.next - options;
			break;

		case 3:
			// options from "server_name"
			bytesLeft = sizeof(options) + cookie.next - options;
			break;
	}

	while (true) {
		if (bytesLeft == 0) {
			// TODO: suppport OPTION_OVERLOAD!
			cookie.state = 4;
			return false;
		}

		option = (message_option)cookie.next[0];
		if (option == OPTION_END) {
			cookie.state = 4;
			return false;
		} else if (option == OPTION_PAD) {
			bytesLeft--;
			cookie.next++;
			continue;
		}

		size = cookie.next[1];
		data = &cookie.next[2];
		cookie.next += 2 + size;

		if (option == OPTION_OVERLOAD) {
			cookie.file_has_options = data[0] & 1;
			cookie.server_name_has_options = data[0] & 2;
			continue;
		}

		return true;
	}
}


const uint8*
dhcp_message::LastOption() const
{
	dhcp_option_cookie cookie;
	message_option option;
	const uint8 *data;
	size_t size;
	while (NextOption(cookie, option, data, size)) {
		// iterate through all options
	}

	return cookie.next;
}


size_t
dhcp_message::Size() const
{
	const uint8* last = LastOption();
	return sizeof(dhcp_message) - sizeof(options) + last + 1 - options;
}


uint8*
dhcp_message::PutOption(uint8* options, message_option option)
{
	options[0] = option;
	return options + 1;
}


uint8*
dhcp_message::PutOption(uint8* options, message_option option, uint8 data)
{
	return PutOption(options, option, &data, 1);
}


uint8*
dhcp_message::PutOption(uint8* options, message_option option, uint16 data)
{
	data = htons(data);
	return PutOption(options, option, (uint8*)&data, sizeof(data));
}


uint8*
dhcp_message::PutOption(uint8* options, message_option option, uint32 data)
{
	data = htonl(data);
	return PutOption(options, option, (uint8*)&data, sizeof(data));
}


uint8*
dhcp_message::PutOption(uint8* options, message_option option, uint8* data, uint32 size)
{
	options[0] = option;
	options[1] = size;
	memcpy(&options[2], data, size);

	return options + 2 + size;
}


//	#pragma mark -


DHCPClient::DHCPClient(const char* device)
	: BHandler("dhcp"),
	fDevice(device)
{
	fTransactionID = system_time();

	dhcp_message message(DHCP_DISCOVER);
	message.opcode = BOOT_REQUEST;
	message.hardware_type = ARP_HARDWARE_TYPE_ETHER;
	message.hardware_address_length = 6;
	message.transaction_id = htonl(fTransactionID);
	message.seconds_since_boot = htons(max_c(system_time() / 1000000LL, 65535));
	fStatus = get_mac_address(device, message.mac_address);
	if (fStatus < B_OK)
		return;

	int socket = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (socket < 0) {
		fStatus = errno;
		return;
	}

	sockaddr_in local;
	memset(&local, 0, sizeof(struct sockaddr_in));
	local.sin_family = AF_INET;
	local.sin_len = sizeof(struct sockaddr_in);
	local.sin_port = htons(DHCP_CLIENT_PORT);
	local.sin_addr.s_addr = INADDR_ANY;

	if (bind(socket, (struct sockaddr *)&local, sizeof(local)) < 0) {
		fStatus = errno;
		close(socket);
		return;
	}

	sockaddr_in broadcast;
	memset(&broadcast, 0, sizeof(struct sockaddr_in));
	broadcast.sin_family = AF_INET;
	broadcast.sin_len = sizeof(struct sockaddr_in);
	broadcast.sin_port = htons(DHCP_SERVER_PORT);
	broadcast.sin_addr.s_addr = INADDR_BROADCAST;

	int option = 1;
	setsockopt(socket, SOL_SOCKET, SO_BROADCAST, &option, sizeof(option));

printf("DHCP message size %lu\n", message.Size());

	ssize_t bytesSent = sendto(socket, &message, message.MinSize(), MSG_BCAST, 
		(struct sockaddr*)&broadcast, sizeof(broadcast));
	if (bytesSent < 0) {
		fStatus = errno;
		close(socket);
		return;
	}

	close(socket);
	fStatus = B_ERROR;
}


DHCPClient::~DHCPClient()
{
}


status_t
DHCPClient::InitCheck()
{
printf("DHCP for %s, status: %s\n", fDevice.String(), strerror(fStatus));
	return fStatus;
}


void
DHCPClient::MessageReceived(BMessage* message)
{
	BHandler::MessageReceived(message);
}

