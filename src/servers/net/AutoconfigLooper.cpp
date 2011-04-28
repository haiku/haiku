/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
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

#include <NetworkInterface.h>
#include <NetworkNotifications.h>

#include "DHCPClient.h"
#include "NetServer.h"


static const uint32 kMsgReadyToRun = 'rdyr';


AutoconfigLooper::AutoconfigLooper(BMessenger target, const char* device)
	: BLooper(device),
	fTarget(target),
	fDevice(device),
	fCurrentClient(NULL)
{
	memset(fCurrentMac, 0, sizeof(fCurrentMac));
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


#ifdef INET6
static in6_addr
BuildIPv6LinkLocalAddress(uint8 mac[6])
{	
	in6_addr result;

	result.s6_addr[0] = 0xfe;
	result.s6_addr[1] = 0x80;
	result.s6_addr[2] = 0;
	result.s6_addr[3] = 0;
	result.s6_addr[4] = 0;
	result.s6_addr[5] = 0;
	result.s6_addr[6] = 0;
	result.s6_addr[7] = 0;

	result.s6_addr[8] = mac[0] ^ 0x02;
	result.s6_addr[9] = mac[1];
	result.s6_addr[10] = mac[2];
	result.s6_addr[11] = 0xff;
	result.s6_addr[12] = 0xfe;
	result.s6_addr[13] = mac[3];
	result.s6_addr[14] = mac[4];
	result.s6_addr[15] = mac[5];

	return result;
}


void
AutoconfigLooper::_ConfigureIPv6LinkLocal(bool add)
{
	// do not touch the loopback device
	if (!strncmp(fDevice.String(), "loop", 4))
		return;

	ifreq request;
	if (!prepare_request(request, fDevice.String()))
		return;

	int socket = ::socket(AF_INET6, SOCK_DGRAM, 0);
	if (socket < 0)
		return;

	// set IFF_CONFIGURING flag on interface
	if (ioctl(socket, SIOCGIFFLAGS, &request, sizeof(struct ifreq)) == 0) {
		request.ifr_flags |= IFF_CONFIGURING;
		ioctl(socket, SIOCSIFFLAGS, &request, sizeof(struct ifreq));
	}

	uint8 mac[6];
	memcpy(mac, fCurrentMac, 6);
	if (add == true) {
		if (get_mac_address(fDevice.String(), mac) != B_OK)
			add = false;
	}

	if (add == true) {
		in6_addr inetAddress = BuildIPv6LinkLocalAddress(mac);
		if (_AddIPv6LinkLocal(socket, inetAddress) != true)
			add = false;

		// save the MAC address for later usage
		memcpy(fCurrentMac, mac, 6);
	}

	if (add == false) {
		static const uint8 zeroMac[6] = {0};
		if (memcmp(fCurrentMac, zeroMac, 6)) {
			in6_addr inetAddress = BuildIPv6LinkLocalAddress(fCurrentMac);
			_RemoveIPv6LinkLocal(socket, inetAddress);
			// reset the stored MAC address
			memcpy(fCurrentMac, zeroMac, 6);
		}
	}

	if (ioctl(socket, SIOCGIFFLAGS, &request, sizeof(struct ifreq)) == 0
		&& (request.ifr_flags & IFF_CONFIGURING) == 0) {
		// Someone else configured the interface in the mean time
		close(socket);
		return;
	}

	close(socket);
}


bool
AutoconfigLooper::_AddIPv6LinkLocal(int socket, const in6_addr &address)
{
	struct ifreq request;
	if (!prepare_request(request, fDevice.String()))
		return false;

	ifaliasreq aliasRequest;
	memset(&aliasRequest, 0, sizeof(ifaliasreq));
	strlcpy(aliasRequest.ifra_name, fDevice.String(), IF_NAMESIZE);
	aliasRequest.ifra_addr.ss_len = sizeof(sockaddr_in6);
	aliasRequest.ifra_addr.ss_family = AF_INET6;

	if (ioctl(socket, SIOCAIFADDR, &aliasRequest, sizeof(ifaliasreq)) < 0) {
		if (errno != B_NAME_IN_USE)
			return false;
	}

	sockaddr_in6* socketAddress = (sockaddr_in6*)&request.ifr_addr;
	socketAddress->sin6_len = sizeof(sockaddr_in6);
	socketAddress->sin6_family = AF_INET6;

	// address
	memcpy(&socketAddress->sin6_addr, &address, sizeof(in6_addr)); 
	if (ioctl(socket, SIOCSIFADDR, &request, sizeof(struct ifreq)) < 0)
		return false;

	// mask (/64)
	memset(socketAddress->sin6_addr.s6_addr, 0xff, 8);
	memset(socketAddress->sin6_addr.s6_addr + 8, 0, 8);

	if (ioctl(socket, SIOCSIFNETMASK, &request, sizeof(struct ifreq)) < 0)
		return false;

	return true;
}


void
AutoconfigLooper::_RemoveIPv6LinkLocal(int socket, const in6_addr &address)
{
	struct ifreq request;
	if (!prepare_request(request, fDevice.String()))
		return;

	sockaddr_in6* socketAddress = (sockaddr_in6*)&request.ifr_addr;
	socketAddress->sin6_len = sizeof(sockaddr_in6);
	socketAddress->sin6_family = AF_INET6;

	// address
	memcpy(&socketAddress->sin6_addr, &address, sizeof(in6_addr)); 
	if (ioctl(socket, SIOCDIFADDR, &request, sizeof(struct ifreq)) < 0)
		return;
}
#endif // INET6


void
AutoconfigLooper::_ReadyToRun()
{
	start_watching_network(B_WATCH_NETWORK_LINK_CHANGES, this);
	_ConfigureIPv4();
#ifdef INET6
	_ConfigureIPv6LinkLocal(true);
#endif
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
				_ConfigureIPv4();
			}
#ifdef INET6
			_ConfigureIPv6LinkLocal((media & IFM_ACTIVE) != 0);
#endif
			break;

		default:
			BLooper::MessageReceived(message);
			break;
	}
}

