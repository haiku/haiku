/*
 * Copyright 2006-2012, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Alexander von Gluck, kallisti5@unixzen.com
 */


#include "AutoconfigLooper.h"

#include <errno.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_types.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/sockio.h>

#include <NetworkInterface.h>
#include <NetworkNotifications.h>

#include "DHCPClient.h"
#include "NetServer.h"


static const uint32 kMsgReadyToRun = 'rdyr';


AutoconfigLooper::AutoconfigLooper(BMessenger target, const char* device)
	: BLooper(device),
	fTarget(target),
	fDevice(device),
	fCurrentClient(NULL),
	fLastMediaStatus(0)
{
	BMessage ready(kMsgReadyToRun);
	PostMessage(&ready);
}


AutoconfigLooper::~AutoconfigLooper()
{
}


void
AutoconfigLooper::_RemoveClient()
{
	if (fCurrentClient == NULL)
		return;

	RemoveHandler(fCurrentClient);
	delete fCurrentClient;
	fCurrentClient = NULL;
}


void
AutoconfigLooper::_ConfigureIPv4()
{
	// start with DHCP

	if (fCurrentClient == NULL) {
		fCurrentClient = new DHCPClient(fTarget, fDevice.String());
		AddHandler(fCurrentClient);
	}

	// set IFF_CONFIGURING flag on interface

	BNetworkInterface interface(fDevice.String());
	int32 flags = interface.Flags() & ~IFF_AUTO_CONFIGURED;
	interface.SetFlags(flags | IFF_CONFIGURING);

	if (fCurrentClient->Initialize() == B_OK)
		return;

	_RemoveClient();

	puts("DHCP failed miserably!");

	// DHCP obviously didn't work out, take some default values for now
	// TODO: have a look at zeroconf
	// TODO: this could also be done add-on based

	if ((interface.Flags() & IFF_CONFIGURING) == 0) {
		// Someone else configured the interface in the mean time
		return;
	}

	BMessage message(kMsgConfigureInterface);
	message.AddString("device", fDevice.String());
	message.AddBool("auto_configured", true);

	BNetworkAddress link;
	uint8 last = 56;
	if (interface.GetHardwareAddress(link) == B_OK) {
		// choose IP address depending on the MAC address, if available
		uint8* mac = link.LinkLevelAddress();
		last = mac[0] ^ mac[1] ^ mac[2] ^ mac[3] ^ mac[4] ^ mac[5];
		if (last > 253)
			last = 253;
		else if (last == 0)
			last = 1;
	}

	// IANA defined the default autoconfig network (for when a DHCP request
	// fails for some reason) as being 169.254.0.0/255.255.0.0. We are only
	// generating the last octet but we could also use the 2 last octets if
	// wanted.
	char string[64];
	snprintf(string, sizeof(string), "169.254.0.%u", last);

	BMessage address;
	address.AddInt32("family", AF_INET);
	address.AddString("address", string);
	message.AddMessage("address", &address);

	fTarget.SendMessage(&message);
}


void
AutoconfigLooper::_ReadyToRun()
{
	start_watching_network(B_WATCH_NETWORK_LINK_CHANGES, this);
	_ConfigureIPv4();
	//_ConfigureIPv6();	// TODO: router advertisement and dhcpv6
}


void
AutoconfigLooper::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgReadyToRun:
			_ReadyToRun();
			break;

		case B_NETWORK_MONITOR:
			const char* device;
			int32 opcode;
			int32 media;
			if (message->FindInt32("opcode", &opcode) != B_OK
				|| opcode != B_NETWORK_DEVICE_LINK_CHANGED
				|| message->FindString("device", &device) != B_OK
				|| fDevice != device
				|| message->FindInt32("media", &media) != B_OK)
				break;

			if ((fLastMediaStatus & IFM_ACTIVE) == 0
				&& (media & IFM_ACTIVE) != 0) {
				// Reconfigure the interface when we have a link again
				_ConfigureIPv4();
				//_ConfigureIPv6();	// TODO: router advertisement and dhcpv6
			}
			fLastMediaStatus = media;
			break;

		default:
			BLooper::MessageReceived(message);
			break;
	}
}

