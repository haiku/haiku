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
#include <String.h>


class BMessageRunner;

class DHCPClient : public BHandler {
	public:
		DHCPClient(const char* device);
		virtual ~DHCPClient();

		status_t InitCheck();

		virtual void MessageReceived(BMessage* message);

	private:
		BString			fDevice;
		BMessageRunner*	fRunner;
		uint32			fTransactionID;
		status_t		fStatus;
};

#endif	// DHCP_CLIENT_H
