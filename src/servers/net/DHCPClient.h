/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef DHCP_CLIENT_H
#define DHCP_CLIENT_H


#include "AutoconfigClient.h"

#include <netinet/in.h>

#include <NetworkAddress.h>


class BMessageRunner;
class dhcp_message;


enum dhcp_state {
	INIT,
	SELECTING,
	INIT_REBOOT,
	REBOOTING,
	REQUESTING,
	BOUND,
	RENEWING,
	REBINDING,
};


class DHCPClient : public AutoconfigClient {
public:
								DHCPClient(BMessenger target,
									const char* device);
	virtual						~DHCPClient();

	virtual	status_t			Initialize();

	virtual	void				MessageReceived(BMessage* message);

private:
			status_t			_Negotiate(dhcp_state state);
			void				_ParseOptions(dhcp_message& message,
									BMessage& address,
									BMessage& resolverConfiguration);
			void				_PrepareMessage(dhcp_message& message,
									dhcp_state state);
			status_t			_SendMessage(int socket, dhcp_message& message,
									const BNetworkAddress& address) const;
			dhcp_state			_CurrentState() const;
			void				_ResetTimeout(int socket, time_t& timeout,
									uint32& tries);
			bool				_TimeoutShift(int socket, time_t& timeout,
									uint32& tries);
			void				_RestartLease(bigtime_t lease);

	static	BString				_AddressToString(const uint8* data);
	static 	BString				_AddressToString(in_addr_t address);

private:
			BMessage			fConfiguration;
			BMessage			fResolverConfiguration;
			BMessageRunner*		fRunner;
			uint8				fMAC[6];
			BString				fHostName;
			uint32				fTransactionID;
			in_addr_t			fAssignedAddress;
			BNetworkAddress		fServer;
			bigtime_t			fStartTime;
			bigtime_t			fRenewalTime;
			bigtime_t			fRebindingTime;
			bigtime_t			fLeaseTime;
			status_t			fStatus;
};

#endif	// DHCP_CLIENT_H
