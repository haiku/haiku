/*
 * Copyright 2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <NetworkInterface.h>

#include <errno.h>
#include <net/if.h>
#include <sys/sockio.h>

#include <AutoDeleter.h>
#include <Messenger.h>
#include <NetServer.h>


static int
family_from_interface_address(const BNetworkInterfaceAddress& address)
{
	if (address.Address().Family() != AF_UNSPEC)
		return address.Address().Family();
	if (address.Mask().Family() != AF_UNSPEC)
		return address.Mask().Family();
	if (address.Destination().Family() != AF_UNSPEC)
		return address.Destination().Family();

	return AF_INET;
}


static int
family_from_route(const route_entry& route)
{
	if (route.destination != NULL && route.destination->sa_family != AF_UNSPEC)
		return route.destination->sa_family;
	if (route.mask != NULL && route.mask->sa_family != AF_UNSPEC)
		return route.mask->sa_family;
	if (route.gateway != NULL && route.gateway->sa_family != AF_UNSPEC)
		return route.gateway->sa_family;
	if (route.source != NULL && route.source->sa_family != AF_UNSPEC)
		return route.source->sa_family;

	return AF_UNSPEC;
}


static status_t
do_ifaliasreq(const char* name, int32 option, BNetworkInterfaceAddress& address,
	bool readBack = false)
{
	int family = AF_INET;
	if (!readBack)
		family = family_from_interface_address(address);

	int socket = ::socket(family, SOCK_DGRAM, 0);
	if (socket < 0)
		return errno;

	FileDescriptorCloser closer(socket);

	ifaliasreq request;
	strlcpy(request.ifra_name, name, IF_NAMESIZE);
	request.ifra_index = address.Index();
	request.ifra_flags = address.Flags();

	memcpy(&request.ifra_addr, &address.Address().SockAddr(),
		address.Address().Length());
	memcpy(&request.ifra_mask, &address.Mask().SockAddr(),
		address.Mask().Length());
	memcpy(&request.ifra_broadaddr, &address.Broadcast().SockAddr(),
		address.Broadcast().Length());

	if (ioctl(socket, option, &request, sizeof(struct ifaliasreq)) < 0)
		return errno;

	if (readBack) {
		address.SetFlags(request.ifra_flags);
		address.Address().SetTo(request.ifra_addr);
		address.Mask().SetTo(request.ifra_mask);
		address.Broadcast().SetTo(request.ifra_broadaddr);
	}

	return B_OK;
}


static status_t
do_ifaliasreq(const char* name, int32 option,
	const BNetworkInterfaceAddress& address)
{
	return do_ifaliasreq(name, option,
		const_cast<BNetworkInterfaceAddress&>(address));
}


template<typename T> status_t
do_request(int family, T& request, const char* name, int option)
{
	int socket = ::socket(family, SOCK_DGRAM, 0);
	if (socket < 0)
		return errno;

	FileDescriptorCloser closer(socket);

	strlcpy(((struct ifreq&)request).ifr_name, name, IF_NAMESIZE);

	if (ioctl(socket, option, &request, sizeof(T)) < 0)
		return errno;

	return B_OK;
}


// #pragma mark -


BNetworkInterfaceAddress::BNetworkInterfaceAddress()
	:
	fIndex(-1),
	fFlags(0)
{
}


BNetworkInterfaceAddress::~BNetworkInterfaceAddress()
{
}


status_t
BNetworkInterfaceAddress::SetTo(const BNetworkInterface& interface, int32 index)
{
	fIndex = index;
	return do_ifaliasreq(interface.Name(), B_SOCKET_GET_ALIAS, *this, true);
}


void
BNetworkInterfaceAddress::SetAddress(const BNetworkAddress& address)
{
	fAddress = address;
}


void
BNetworkInterfaceAddress::SetMask(const BNetworkAddress& mask)
{
	fMask = mask;
}


void
BNetworkInterfaceAddress::SetBroadcast(const BNetworkAddress& broadcast)
{
	fBroadcast = broadcast;
}


void
BNetworkInterfaceAddress::SetDestination(const BNetworkAddress& destination)
{
	fBroadcast = destination;
}


void
BNetworkInterfaceAddress::SetFlags(uint32 flags)
{
	fFlags = flags;
}


// #pragma mark -


BNetworkInterface::BNetworkInterface()
{
	Unset();
}


BNetworkInterface::BNetworkInterface(const char* name)
{
	SetTo(name);
}


BNetworkInterface::BNetworkInterface(uint32 index)
{
	SetTo(index);
}


BNetworkInterface::~BNetworkInterface()
{
}


void
BNetworkInterface::Unset()
{
	fName[0] = '\0';
}


void
BNetworkInterface::SetTo(const char* name)
{
	strlcpy(fName, name, IF_NAMESIZE);
}


status_t
BNetworkInterface::SetTo(uint32 index)
{
	ifreq request;
	request.ifr_index = index;

	status_t status = do_request(AF_INET, request, "", SIOCGIFNAME);
	if (status != B_OK)
		return status;

	strlcpy(fName, request.ifr_name, IF_NAMESIZE);
	return B_OK;
}


bool
BNetworkInterface::Exists() const
{
	ifreq request;
	return do_request(AF_INET, request, Name(), SIOCGIFINDEX) == B_OK;
}


const char*
BNetworkInterface::Name() const
{
	return fName;
}


uint32
BNetworkInterface::Index() const
{
	ifreq request;
	if (do_request(AF_INET, request, Name(), SIOCGIFINDEX) != B_OK)
		return 0;

	return request.ifr_index;
}


uint32
BNetworkInterface::Flags() const
{
	ifreq request;
	if (do_request(AF_INET, request, Name(), SIOCGIFFLAGS) != B_OK)
		return 0;

	return request.ifr_flags;
}


uint32
BNetworkInterface::MTU() const
{
	ifreq request;
	if (do_request(AF_INET, request, Name(), SIOCGIFMTU) != B_OK)
		return 0;

	return request.ifr_mtu;
}


int32
BNetworkInterface::Media() const
{
	ifmediareq request;
	request.ifm_count = 0;
	request.ifm_ulist = NULL;

	if (do_request(AF_INET, request, Name(), SIOCGIFMEDIA) != B_OK)
		return -1;

	return request.ifm_current;
}


uint32
BNetworkInterface::Metric() const
{
	ifreq request;
	if (do_request(AF_INET, request, Name(), SIOCGIFMETRIC) != B_OK)
		return 0;

	return request.ifr_metric;
}


uint32
BNetworkInterface::Type() const
{
	ifreq request;
	if (do_request(AF_INET, request, Name(), SIOCGIFTYPE) != B_OK)
		return 0;

	return request.ifr_type;
}


status_t
BNetworkInterface::GetStats(ifreq_stats& stats)
{
	ifreq request;
	status_t status = do_request(AF_INET, request, Name(), SIOCGIFSTATS);
	if (status != B_OK)
		return status;

	memcpy(&stats, &request.ifr_stats, sizeof(ifreq_stats));
	return B_OK;
}


bool
BNetworkInterface::HasLink() const
{
	return (Flags() & IFF_LINK) != 0;
}


status_t
BNetworkInterface::SetFlags(uint32 flags)
{
	ifreq request;
	request.ifr_flags = flags;
	return do_request(AF_INET, request, Name(), SIOCSIFFLAGS);
}


status_t
BNetworkInterface::SetMTU(uint32 mtu)
{
	ifreq request;
	request.ifr_mtu = mtu;
	return do_request(AF_INET, request, Name(), SIOCSIFMTU);
}


status_t
BNetworkInterface::SetMedia(int32 media)
{
	ifreq request;
	request.ifr_media = media;
	return do_request(AF_INET, request, Name(), SIOCSIFMEDIA);
}


status_t
BNetworkInterface::SetMetric(uint32 metric)
{
	ifreq request;
	request.ifr_metric = metric;
	return do_request(AF_INET, request, Name(), SIOCSIFMETRIC);
}


int32
BNetworkInterface::CountAddresses() const
{
	ifreq request;
	if (do_request(AF_INET, request, Name(), B_SOCKET_COUNT_ALIASES) != B_OK)
		return 0;

	return request.ifr_count;
}


status_t
BNetworkInterface::GetAddressAt(int32 index, BNetworkInterfaceAddress& address)
{
	return address.SetTo(*this, index);
}


int32
BNetworkInterface::FindAddress(const BNetworkAddress& address)
{
	int socket = ::socket(address.Family(), SOCK_DGRAM, 0);
	if (socket < 0)
		return -1;

	FileDescriptorCloser closer(socket);

	ifaliasreq request;
	memset(&request, 0, sizeof(ifaliasreq));

	strlcpy(request.ifra_name, Name(), IF_NAMESIZE);
	request.ifra_index = -1;
	memcpy(&request.ifra_addr, &address.SockAddr(), address.Length());

	if (ioctl(socket, B_SOCKET_GET_ALIAS, &request, sizeof(struct ifaliasreq))
			< 0)
		return -1;

	return request.ifra_index;
}


int32
BNetworkInterface::FindFirstAddress(int family)
{
	int socket = ::socket(family, SOCK_DGRAM, 0);
	if (socket < 0)
		return -1;

	FileDescriptorCloser closer(socket);

	ifaliasreq request;
	memset(&request, 0, sizeof(ifaliasreq));

	strlcpy(request.ifra_name, Name(), IF_NAMESIZE);
	request.ifra_index = -1;
	request.ifra_addr.ss_family = AF_UNSPEC;

	if (ioctl(socket, B_SOCKET_GET_ALIAS, &request, sizeof(struct ifaliasreq))
			< 0)
		return -1;

	return request.ifra_index;
}


status_t
BNetworkInterface::AddAddress(const BNetworkInterfaceAddress& address)
{
	return do_ifaliasreq(Name(), B_SOCKET_ADD_ALIAS, address);
}


status_t
BNetworkInterface::AddAddress(const BNetworkAddress& local)
{
	BNetworkInterfaceAddress address;
	address.SetAddress(local);

	return do_ifaliasreq(Name(), B_SOCKET_ADD_ALIAS, address);
}


status_t
BNetworkInterface::SetAddress(const BNetworkInterfaceAddress& address)
{
	return do_ifaliasreq(Name(), B_SOCKET_SET_ALIAS, address);
}


status_t
BNetworkInterface::RemoveAddress(const BNetworkInterfaceAddress& address)
{
	ifreq request;
	memcpy(&request.ifr_addr, &address.Address().SockAddr(),
		address.Address().Length());

	return do_request(family_from_interface_address(address), request, Name(),
		B_SOCKET_REMOVE_ALIAS);
}


status_t
BNetworkInterface::RemoveAddress(const BNetworkAddress& address)
{
	ifreq request;
	memcpy(&request.ifr_addr, &address.SockAddr(), address.Length());

	return do_request(address.Family(), request, Name(), B_SOCKET_REMOVE_ALIAS);
}


status_t
BNetworkInterface::RemoveAddressAt(int32 index)
{
	BNetworkInterfaceAddress address;
	status_t status = GetAddressAt(index, address);
	if (status != B_OK)
		return status;

	return RemoveAddress(address);
}


status_t
BNetworkInterface::GetHardwareAddress(BNetworkAddress& address)
{
	int socket = ::socket(AF_LINK, SOCK_DGRAM, 0);
	if (socket < 0)
		return errno;

	FileDescriptorCloser closer(socket);

	ifreq request;
	strlcpy(request.ifr_name, Name(), IF_NAMESIZE);

	if (ioctl(socket, SIOCGIFADDR, &request, sizeof(struct ifreq)) < 0)
		return errno;

	address.SetTo(request.ifr_addr);
	return B_OK;
}


status_t
BNetworkInterface::AddRoute(const route_entry& route)
{
	int family = family_from_route(route);
	if (family == AF_UNSPEC)
		return B_BAD_VALUE;

	ifreq request;
	request.ifr_route = route;
	return do_request(family, request, Name(), SIOCADDRT);
}


status_t
BNetworkInterface::AddDefaultRoute(const BNetworkAddress& gateway)
{
	route_entry route;
	memset(&route, 0, sizeof(route_entry));
	route.flags = RTF_STATIC | RTF_DEFAULT | RTF_GATEWAY;
	route.gateway = const_cast<sockaddr*>(&gateway.SockAddr());

	return AddRoute(route);
}


status_t
BNetworkInterface::RemoveRoute(const route_entry& route)
{
	int family = family_from_route(route);
	if (family == AF_UNSPEC)
		return B_BAD_VALUE;

	return RemoveRoute(family, route);
}


status_t
BNetworkInterface::RemoveRoute(int family, const route_entry& route)
{
	ifreq request;
	request.ifr_route = route;
	return do_request(family, request, Name(), SIOCDELRT);
}


status_t
BNetworkInterface::RemoveDefaultRoute(int family)
{
	route_entry route;
	memset(&route, 0, sizeof(route_entry));
	route.flags = RTF_STATIC | RTF_DEFAULT;

	return RemoveRoute(family, route);
}


status_t
BNetworkInterface::AutoConfigure(int family)
{
	BMessage message(kMsgConfigureInterface);
	message.AddString("device", Name());

	BMessage address;
	address.AddInt32("family", family);
	address.AddBool("auto_config", true);
	message.AddMessage("address", &address);

	BMessenger networkServer(kNetServerSignature);
	BMessage reply;
	status_t status = networkServer.SendMessage(&message, &reply);
	if (status == B_OK)
		reply.FindInt32("status", &status);

	return status;
}
