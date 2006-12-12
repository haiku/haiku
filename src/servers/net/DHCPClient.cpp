/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "DHCPClient.h"
#include "NetServer.h"

#include <Message.h>

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>


// See RFC 2131 for DHCP, see RFC 1533 for BOOTP/DHCP options

#define DHCP_CLIENT_PORT	68
#define DHCP_SERVER_PORT	67

#define DEFAULT_TIMEOUT		2	// secs
#define MAX_TIMEOUT			15	// secs

enum message_opcode {
	BOOT_REQUEST = 1,
	BOOT_REPLY
};

enum message_option {
	OPTION_MAGIC = 0x63825363,

	// generic options
	OPTION_PAD = 0,
	OPTION_END = 255,
	OPTION_SUBNET_MASK = 1,
	OPTION_ROUTER_ADDRESS = 3,
	OPTION_DOMAIN_NAME_SERVER = 6,
	OPTION_HOST_NAME = 12,
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
	DHCP_NONE = 0,
	DHCP_DISCOVER,
	DHCP_OFFER,
	DHCP_REQUEST,
	DHCP_DECLINE,
	DHCP_ACK,
	DHCP_NACK,
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
	message_type Type() const;
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


message_type
dhcp_message::Type() const
{
	dhcp_option_cookie cookie;
	message_option option;
	const uint8 *data;
	size_t size;
	while (NextOption(cookie, option, data, size)) {
		// iterate through all options
		if (option == OPTION_MESSAGE_TYPE)
			return (message_type)data[0];
	}

	return DHCP_NONE;
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


DHCPClient::DHCPClient(BMessenger target, const char* device)
	: BHandler("dhcp"),
	fDevice(device),
	fConfiguration(kMsgConfigureInterface)
{
	fTransactionID = system_time();

	fStatus = get_mac_address(device, fMAC);
	if (fStatus < B_OK)
		return;

	dhcp_message discover(DHCP_DISCOVER);
	_PrepareMessage(discover);

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

	memset(&fServer, 0, sizeof(struct sockaddr_in));
	fServer.sin_family = AF_INET;
	fServer.sin_len = sizeof(struct sockaddr_in);
	fServer.sin_port = htons(DHCP_SERVER_PORT);

	sockaddr_in broadcast;
	memset(&broadcast, 0, sizeof(struct sockaddr_in));
	broadcast.sin_family = AF_INET;
	broadcast.sin_len = sizeof(struct sockaddr_in);
	broadcast.sin_port = htons(DHCP_SERVER_PORT);
	broadcast.sin_addr.s_addr = INADDR_BROADCAST;

	int option = 1;
	setsockopt(socket, SOL_SOCKET, SO_BROADCAST, &option, sizeof(option));

	_ResetTimeout(socket);

	fStatus = _SendMessage(socket, discover, broadcast);
	if (fStatus < B_OK) {
		close(socket);
		return;
	}

	// receive loop until we've got an offer and acknowledged it

	enum {
		INIT,
		REQUESTING,
		ACKNOWLEDGED,
	} state = INIT;

	dhcp_message request(DHCP_REQUEST);
		// will be filled when handling a DHCP_OFFER

	while (state != ACKNOWLEDGED) {
		char buffer[2048];
		ssize_t bytesReceived = recvfrom(socket, buffer, sizeof(buffer),
			0, NULL, NULL);
		if (bytesReceived == B_TIMED_OUT) {
			// depending on the state, we'll just try again
			if (!_TimeoutShift(socket)) {
				fStatus = B_TIMED_OUT;
				break;
			}

			if (state == INIT)
				fStatus = _SendMessage(socket, discover, broadcast);
			else if (state == REQUESTING) {
				if (fTries > 0) {
					// The server not only didn't acknowledge the address, it did nothing.
					// So we just ignore this little annoyance and continue with life.
					fStatus = B_OK;
					break;
				}
				fStatus = _SendMessage(socket, request, broadcast);
			}

			if (fStatus < B_OK)
				break;
		} else if (bytesReceived < B_OK)
			break;

		dhcp_message *message = (dhcp_message *)buffer;
		if (message->transaction_id != htonl(fTransactionID)
			|| !message->HasOptions()
			|| memcmp(message->mac_address, discover.mac_address,
				discover.hardware_address_length)) {
			// this message is not for us
			continue;
		}

		switch (message->Type()) {
			case DHCP_NONE:
			default:
				// ignore this message
				break;

			case DHCP_OFFER:
			{
				// first offer wins
				if (state != INIT)
					break;

				// collect interface options

				fAssignedAddress = message->your_address;

				fConfiguration.MakeEmpty();
				fConfiguration.AddString("device", fDevice.String());

				BMessage address;
				address.AddString("family", "inet");
				address.AddString("address", _ToString(fAssignedAddress));
				_ParseOptions(*message, address);

				fConfiguration.AddMessage("address", &address);

				// request configuration from the server

				_ResetTimeout(socket);
				state = REQUESTING;
				_PrepareMessage(request);

				fStatus = _SendMessage(socket, request, broadcast);
					// we're sending a broadcast so that all potential offers get an answer
				break;
			}

			case DHCP_ACK:
			{
				if (state != REQUESTING)
					continue;

				// our address request has been acknowledged
				state = ACKNOWLEDGED;

				// configure interface
				BMessage reply;
				target.SendMessage(&fConfiguration, &reply);

				if (reply.FindInt32("status", &fStatus) != B_OK)
					fStatus = B_OK;
				break;
			}

			case DHCP_NACK:
				if (state != REQUESTING)
					continue;

				// try again
				fStatus = _SendMessage(socket, discover, broadcast);
				if (fStatus == B_OK)
					state = INIT;
				break;
		}
	}

	close(socket);
}


DHCPClient::~DHCPClient()
{
	if (fStatus != B_OK)
		return;

	int socket = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (socket < 0)
		return;

	// release lease

	dhcp_message release(DHCP_RELEASE);
	_PrepareMessage(release);

	_SendMessage(socket, release, fServer);
	close(socket);
}


void
DHCPClient::_ParseOptions(dhcp_message& message, BMessage& address)
{
	dhcp_option_cookie cookie;
	message_option option;
	const uint8 *data;
	size_t size;
	while (message.NextOption(cookie, option, data, size)) {
		// iterate through all options
		switch (option) {
			case OPTION_ROUTER_ADDRESS:
				address.AddString("gateway", _ToString(data));
				break;
			case OPTION_SUBNET_MASK:
				address.AddString("mask", _ToString(data));
				break;
			case OPTION_DOMAIN_NAME_SERVER:
				// TODO: for now, write it out to /etc/resolv.conf
				for (uint32 i = 0; i < size / 4; i++) {
					printf("DNS: %s\n", _ToString(&data[i*4]).String());
				}
				break;
			case OPTION_SERVER_ADDRESS:
				fServer.sin_addr.s_addr = *(in_addr_t*)data;
				break;
			case OPTION_ADDRESS_LEASE_TIME:
				printf("lease time of %lu seconds\n", htonl(*(uint32*)data));
				break;

			case OPTION_HOST_NAME:
				char name[256];
				memcpy(name, data, size);
				name[size] = '\0';
				printf("DHCP host name: \"%s\"\n", name);
				break;

			case OPTION_MESSAGE_TYPE:
				break;

			default:
				printf("unknown option %lu\n", (uint32)option);
				break;
		}
	}
}


void
DHCPClient::_PrepareMessage(dhcp_message& message)
{
	message.opcode = BOOT_REQUEST;
	message.hardware_type = ARP_HARDWARE_TYPE_ETHER;
	message.hardware_address_length = 6;
	message.transaction_id = htonl(fTransactionID);
	message.seconds_since_boot = htons(max_c(system_time() / 1000000LL, 65535));
	memcpy(message.mac_address, fMAC, 6);

	switch (message.Type()) {
		case DHCP_REQUEST:
		case DHCP_RELEASE:
		{
			// add server identifier option
			uint8* next = message.options;
			next = message.PutOption(next, OPTION_MESSAGE_TYPE, (uint8)DHCP_REQUEST);
			next = message.PutOption(next, OPTION_MESSAGE_SIZE,
				(uint16)sizeof(dhcp_message));
			next = message.PutOption(next, OPTION_SERVER_ADDRESS,
				(uint32)fServer.sin_addr.s_addr);
			next = message.PutOption(next, OPTION_REQUEST_IP_ADDRESS, fAssignedAddress);
			next = message.PutOption(next, OPTION_END);
			break;
		}

		default:
			// the default options are fine
			break;
	}
}


void
DHCPClient::_ResetTimeout(int socket)
{
	fTimeout = DEFAULT_TIMEOUT;
	fTries = 0;

	struct timeval value;
	value.tv_sec = fTimeout;
	value.tv_usec = 0;
	setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &value, sizeof(value));
}


bool
DHCPClient::_TimeoutShift(int socket)
{
	fTimeout += fTimeout;
	if (fTimeout > MAX_TIMEOUT) {
		fTimeout = DEFAULT_TIMEOUT;

		if (++fTries > 2)
			return false;
	}
printf("DHCP timeout shift: %lu secs (try %lu)\n", fTimeout, fTries);

	struct timeval value;
	value.tv_sec = fTimeout;
	value.tv_usec = 0;
	setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &value, sizeof(value));

	return true;
}


BString
DHCPClient::_ToString(const uint8* data) const
{
	BString target = inet_ntoa(*(in_addr*)data);
	return target;
}


BString
DHCPClient::_ToString(in_addr_t address) const
{
	BString target = inet_ntoa(*(in_addr*)&address);
	return target;
}


status_t
DHCPClient::_SendMessage(int socket, dhcp_message& message, sockaddr_in& address) const
{
	ssize_t bytesSent = sendto(socket, &message, message.Size(),
		address.sin_addr.s_addr == INADDR_BROADCAST ? MSG_BCAST : 0,
		(struct sockaddr*)&address, sizeof(sockaddr_in));
	if (bytesSent < 0)
		return errno;

	return B_OK;
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

