/*
 * Copyright 2006-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "AutoconfigLooper.h"

#include <errno.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_types.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/sockio.h>

#include <net_notifications.h>

#include "DHCPClient.h"
#include "NetServer.h"


static const uint32 kMsgReadyToRun = 'rdyr';


AutoconfigLooper::AutoconfigLooper(BMessenger target, const char* device)
	: BLooper(device),
	fTarget(target),
	fDevice(device),
	fCurrentClient(NULL)
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
AutoconfigLooper::_Configure()
{
	ifreq request;
	if (!prepare_request(request, fDevice.String()))
		return;

	// set IFF_CONFIGURING flag on interface

	int socket = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (socket < 0)
		return;

	if (ioctl(socket, SIOCGIFFLAGS, &request, sizeof(struct ifreq)) == 0) {
		request.ifr_flags |= IFF_CONFIGURING;
		ioctl(socket, SIOCSIFFLAGS, &request, sizeof(struct ifreq));
	}

	// remove current handler

	_RemoveClient();

	// start with DHCP

	fCurrentClient = new DHCPClient(fTarget, fDevice.String());
	AddHandler(fCurrentClient);

	if (fCurrentClient->Initialize() == B_OK) {
		close(socket);
		return;
	}

	_RemoveClient();

	puts("DHCP failed miserably!");

	// DHCP obviously didn't work out, take some default values for now
	// TODO: have a look at zeroconf
	// TODO: this could also be done add-on based

	if (ioctl(socket, SIOCGIFFLAGS, &request, sizeof(struct ifreq)) == 0
		&& (request.ifr_flags & IFF_CONFIGURING) == 0) {
		// Someone else configured the interface in the mean time
		close(socket);
		return;
	}

	close(socket);

	BMessage interface(kMsgConfigureInterface);
	interface.AddString("device", fDevice.String());
	interface.AddBool("auto", true);

	uint8 mac[6];
	uint8 last = 56;
	if (get_mac_address(fDevice.String(), mac) == B_OK) {
		// choose IP address depending on the MAC address, if available
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
	address.AddString("family", "inet");
	address.AddString("address", string);
	interface.AddMessage("address", &address);

	fTarget.SendMessage(&interface);
}


void
AutoconfigLooper::_ReadyToRun()
{
	start_watching_network(B_WATCH_NETWORK_LINK_CHANGES, this);
	_Configure();
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

			if ((media & IFM_ACTIVE) != 0) {
				// Reconfigure the interface when we have a link again
				_Configure();
			}
			break;

		default:
			BLooper::MessageReceived(message);
			break;
	}
}

