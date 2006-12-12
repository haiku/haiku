/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef DHCP_CLIENT_H
#define DHCP_CLIENT_H


#include <Handler.h>
#include <Messenger.h>
#include <String.h>

#include <netinet/in.h>

class BMessageRunner;
class dhcp_message;


class DHCPClient : public BHandler {
	public:
		DHCPClient(BMessenger target, const char* device);
		virtual ~DHCPClient();

		status_t InitCheck();

		virtual void MessageReceived(BMessage* message);

	private:
		void _ParseOptions(dhcp_message& message, BMessage& address);
		void _PrepareMessage(dhcp_message& message);
		status_t _SendMessage(int socket, dhcp_message& message, sockaddr_in& address) const;
		void _ResetTimeout(int socket);
		bool _TimeoutShift(int socket);
		BString _ToString(const uint8* data) const;
		BString _ToString(in_addr_t address) const;

		BString			fDevice;
		BMessage		fConfiguration;
		BMessageRunner*	fRunner;
		uint8			fMAC[6];
		uint32			fTransactionID;
		in_addr_t		fAssignedAddress;
		sockaddr_in		fServer;
		time_t			fTimeout;
		uint32			fTries;
		status_t		fStatus;
};

#endif	// DHCP_CLIENT_H
