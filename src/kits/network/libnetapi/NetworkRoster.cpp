/*
 * Copyright 2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <NetworkRoster.h>

#include <errno.h>
#include <sys/sockio.h>

#include <NetworkInterface.h>

#include <net_notifications.h>
#include <AutoDeleter.h>


// TODO: using AF_INET for the socket isn't really a smart idea, as one
// could completely remove IPv4 support from the stack easily.
// Since in the stack, device_interfaces are pretty much interfaces now, we
// could get rid of them more or less, and make AF_LINK provide the same
// information as AF_INET for the interface functions mostly.


BNetworkRoster BNetworkRoster::sDefault;


/*static*/ BNetworkRoster&
BNetworkRoster::Default()
{
	return sDefault;
}


size_t
BNetworkRoster::CountInterfaces() const
{
	int socket = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (socket < 0)
		return 0;

	FileDescriptorCloser closer(socket);

	ifconf config;
	config.ifc_len = sizeof(config.ifc_value);
	if (ioctl(socket, SIOCGIFCOUNT, &config, sizeof(struct ifconf)) != 0)
		return 0;

	return (size_t)config.ifc_value;
}


status_t
BNetworkRoster::GetNextInterface(uint32* cookie,
	BNetworkInterface& interface) const
{
	// TODO: think about caching the interfaces!

	if (cookie == NULL)
		return B_BAD_VALUE;

	// get a list of all interfaces

	int socket = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (socket < 0)
		return errno;

	FileDescriptorCloser closer(socket);

	ifconf config;
	config.ifc_len = sizeof(config.ifc_value);
	if (ioctl(socket, SIOCGIFCOUNT, &config, sizeof(struct ifconf)) < 0)
		return errno;

	size_t count = (size_t)config.ifc_value;
	if (count == 0)
		return B_BAD_VALUE;

	char* buffer = (char*)malloc(count * sizeof(struct ifreq));
	if (buffer == NULL)
		return B_NO_MEMORY;

	MemoryDeleter deleter(buffer);

	config.ifc_len = count * sizeof(struct ifreq);
	config.ifc_buf = buffer;
	if (ioctl(socket, SIOCGIFCONF, &config, sizeof(struct ifconf)) < 0)
		return errno;

	ifreq* interfaces = (ifreq*)buffer;
	ifreq* end = (ifreq*)(buffer + config.ifc_len);

	for (uint32 i = 0; interfaces < end; i++) {
		interface.SetTo(interfaces[0].ifr_name);
		if (i == *cookie) {
			(*cookie)++;
			return B_OK;
		}

		interfaces = (ifreq*)((uint8*)interfaces
			+ _SIZEOF_ADDR_IFREQ(interfaces[0]));
	}

	return B_BAD_VALUE;
}


status_t
BNetworkRoster::AddInterface(const char* name)
{
	int socket = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (socket < 0)
		return errno;

	ifaliasreq request;
	memset(&request, 0, sizeof(ifaliasreq));
	strlcpy(request.ifra_name, name, IF_NAMESIZE);

	if (ioctl(socket, SIOCAIFADDR, &request, sizeof(request)) != 0)
		return errno;

	return B_OK;
}


status_t
BNetworkRoster::AddInterface(const BNetworkInterface& interface)
{
	return AddInterface(interface.Name());
}


status_t
BNetworkRoster::RemoveInterface(const char* name)
{
	int socket = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (socket < 0)
		return errno;

	ifreq request;
	strlcpy(request.ifr_name, name, IF_NAMESIZE);

	request.ifr_addr.sa_family = AF_UNSPEC;

	if (ioctl(socket, SIOCDIFADDR, &request, sizeof(request)) != 0)
		return errno;

	return B_OK;
}


status_t
BNetworkRoster::RemoveInterface(const BNetworkInterface& interface)
{
	return RemoveInterface(interface.Name());
}


status_t
BNetworkRoster::StartWatching(const BMessenger& target, uint32 eventMask)
{
	return start_watching_network(eventMask, target);
}


void
BNetworkRoster::StopWatching(const BMessenger& target)
{
	stop_watching_network(target);
}


// #pragma mark - private


BNetworkRoster::BNetworkRoster()
{
}


BNetworkRoster::~BNetworkRoster()
{
}
