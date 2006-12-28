/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "Services.h"
#include "NetServer.h"
#include "Settings.h"

#include <Autolock.h>

#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <new>
#include <sys/socket.h>
#include <vector>

using namespace std;

struct service_address {
	int		socket;
	int		family;
	int		type;
	int		protocol;
	sockaddr address;
};

typedef vector<service_address> AddressList;

struct service {
	std::string name;
	std::string	launch;
	int		type;
	int		protocol;
	uid_t	user;
	gid_t	group;
	AddressList addresses;

	~service();
	bool operator!=(const struct service& other);
	bool operator==(const struct service& other);
};


int
parse_type(const char* string)
{
	if (!strcasecmp(string, "stream"))
		return SOCK_STREAM;

	return SOCK_DGRAM;
}


int
parse_protocol(const char* string)
{
	struct protoent* proto = getprotobyname(string);
	if (proto == NULL)
		return IPPROTO_TCP;

	return proto->p_proto;
}


int
type_for_protocol(int protocol)
{
	// default determined by protocol
	switch (protocol) {
		case IPPROTO_TCP:
			return SOCK_STREAM;

		case IPPROTO_UDP:
		default:
			return SOCK_DGRAM;
	}
}


//	#pragma mark -


service::~service()
{
	// close all open sockets
	AddressList::const_iterator iterator = addresses.begin();
	for (; iterator != addresses.end(); iterator++) {
		const service_address& address = *iterator;

		close(address.socket);
	}
}


bool
service::operator!=(const struct service& other)
{
	return !(*this == other);
}


bool
service::operator==(const struct service& other)
{
	if (name != other.name
		|| launch != other.launch
		|| type != other.type
		|| protocol != other.protocol)
		return false;

	// TODO: compare addresses!
	return true;
}


//	#pragma mark -


Services::Services(const BMessage& services)
	:
	fListener(-1),
	fUpdate(0),
	fMaxSocket(0)
{
	_Update(services);

	// setup pipe to communicate with the listener thread - as the listener
	// blocks on select(), we need a mechanism to interrupt it
	if (pipe(&fReadPipe) < 0) {
		fReadPipe = -1;
		return;
	}

	fListener = spawn_thread(_Listener, "services listener", B_NORMAL_PRIORITY, this);
	if (fListener >= B_OK)
		resume_thread(fListener);

	FD_ZERO(&fSet);
	FD_SET(fReadPipe, &fSet);

	_UpdateMaxSocket(fWritePipe);
	fMinSocket = fMaxSocket;
}


Services::~Services()
{
	wait_for_thread(fListener, NULL);
}


status_t
Services::InitCheck() const
{
	return fListener >= B_OK ? B_OK : fListener;
}


void
Services::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgUpdateServices:
			_Update(*message);
			break;

		default:
			BHandler::MessageReceived(message);
	}
}


void
Services::_NotifyListener(bool quit)
{
	write(fWritePipe, quit ? "q" : "u", 1);
}


void
Services::_UpdateMaxSocket(int socket)
{
	if (socket >= fMaxSocket)
		fMaxSocket = socket + 1;
}


status_t
Services::_StartService(struct service& service)
{
	// create socket

	bool failed = false;
	AddressList::iterator iterator = service.addresses.begin();
	for (; iterator != service.addresses.end(); iterator++) {
		service_address& address = *iterator;

		address.socket = socket(address.family, address.type, address.protocol);
		if (address.socket < 0) {
			failed = true;
			break;
		}

		if (bind(address.socket, &address.address, address.address.sa_len) < 0) {
			failed = true;
			break;
		}

		fSocketMap[address.socket] = &service;
		_UpdateMaxSocket(address.socket);
		FD_SET(address.socket, &fSet);
	}

	if (failed) {
		// open sockets will be closed when the service is deleted
		return errno;
	}

	// add service to maps

	fNameMap[service.name] = &service;

	iterator = service.addresses.begin();
	for (; iterator != service.addresses.end(); iterator++) {
		const service_address& address = *iterator;

		fSocketMap[address.socket] = &service;
	}

	_NotifyListener();
printf("Starting service '%s'\n", service.name.c_str());
	return B_OK;
}


status_t
Services::_StopService(struct service& service)
{
	// remove service from maps
	{
		ServiceNameMap::iterator iterator = fNameMap.find(service.name);
		if (iterator != fNameMap.end())
			fNameMap.erase(iterator);
	}
	{
		AddressList::const_iterator iterator = service.addresses.begin();
		for (; iterator != service.addresses.end(); iterator++) {
			const service_address& address = *iterator;

			ServiceSocketMap::iterator socketIterator = fSocketMap.find(address.socket);
			if (socketIterator != fSocketMap.end())
				fSocketMap.erase(socketIterator);

			close(address.socket);
			FD_CLR(address.socket, &fSet);
		}
	}

	delete &service;
	return B_OK;
}


status_t
Services::_ToService(const BMessage& message, struct service*& service)
{
	// get mandatory fields
	const char* name;
	const char* launch;
	if (message.FindString("name", &name) != B_OK
		|| message.FindString("launch", &launch) != B_OK)
		return B_BAD_VALUE;

	service = new (std::nothrow) ::service;
	if (service == NULL)
		return B_NO_MEMORY;

	service->name = name;
	service->launch = launch;

	// TODO: user/group is currently ignored!

	// default family/port/protocol/type for all addresses
	const char* string;
	int32 serviceFamilyIndex;
	int32 serviceFamily = -1;
	if (message.FindString("family", &string) == B_OK) {
		if (get_family_index(string, serviceFamilyIndex))
			serviceFamily = family_at_index(serviceFamilyIndex);
	}

	int32 serviceProtocol;
	if (message.FindString("protocol", &string) == B_OK)
		serviceProtocol = parse_protocol(string);
	else {
		string = "tcp";
			// we set 'string' here for an eventual call to getservbyname() below
		serviceProtocol = IPPROTO_TCP;
	}

	int32 servicePort;
	if (message.FindInt32("port", &servicePort) != B_OK) {
		struct servent* servent = getservbyname(name, string);
		if (servent != NULL)
			servicePort = servent->s_port;
		else
			servicePort = -1;
	}

	int32 serviceType = -1;
	if (message.FindString("type", &string) == B_OK) {
		serviceType = parse_type(string);
	} else {
		serviceType = type_for_protocol(serviceProtocol);
	}

	BMessage address;
	int32 i = 0;
	for (; message.FindMessage("address", i, &address) == B_OK; i++) {
		// TODO: dump problems in the settings to syslog
		service_address serviceAddress;
		if (address.FindString("family", &string) != B_OK)
			continue;

		int32 familyIndex;
		if (!get_family_index(string, familyIndex))
			continue;

		serviceAddress.family = family_at_index(familyIndex);

		if (address.FindString("protocol", &string) == B_OK)
			serviceAddress.protocol = parse_protocol(string);
		else
			serviceAddress.protocol = serviceProtocol;

		if (message.FindString("type", &string) == B_OK)
			serviceAddress.type = parse_type(string);
		else if (serviceAddress.protocol != serviceProtocol)
			serviceAddress.type = type_for_protocol(serviceAddress.protocol);
		else
			serviceAddress.type = serviceType;

		if (address.FindString("address", &string) == B_OK) {
			if (!parse_address(familyIndex, string, serviceAddress.address))
				continue;
		} else
			set_any_address(familyIndex, serviceAddress.address);

		int32 port;
		if (address.FindInt32("port", &port) != B_OK)
			port = servicePort;

		set_port(familyIndex, serviceAddress.address, port);
		serviceAddress.socket = -1;

		service->addresses.push_back(serviceAddress);
	}

	if (i == 0 && (serviceFamily < 0 || servicePort < 0)) {
		// no address specified
		delete service;
		return B_BAD_VALUE;
	}

	if (i == 0) {
		// no address specified, but family/port were given; add empty address
		service_address serviceAddress;
		serviceAddress.family = serviceFamily;
		serviceAddress.type = serviceType;
		serviceAddress.protocol = serviceProtocol;

		set_any_address(serviceFamilyIndex, serviceAddress.address);
		set_port(serviceFamilyIndex, serviceAddress.address, servicePort);
		serviceAddress.socket = -1;

		service->addresses.push_back(serviceAddress);
	}

	return B_OK;
}


void
Services::_Update(const BMessage& services)
{
	BAutolock locker(fLock);
	fUpdate++;

	BMessage message;
	for (int32 index = 0; services.FindMessage("service", index,
			&message) == B_OK; index++) {
		const char* name;
		if (message.FindString("name", &name) != B_OK)
			continue;

		struct service* service;
		if (_ToService(message, service) != B_OK)
			continue;

		ServiceNameMap::iterator iterator = fNameMap.find(name);
		if (iterator == fNameMap.end()) {
			// this service does not exist yet, start it
			_StartService(*service);
		} else {
			// this service does already exist - check for any changes

			if (service != iterator->second) {
				_StopService(*iterator->second);
				_StartService(*service);
			}
		}
	}
}


status_t
Services::_Listener()
{
	while (true) {
		fLock.Lock();
		fd_set set = fSet;
		fLock.Unlock();

		if (select(fMaxSocket, &set, NULL, NULL, NULL) < 0) {
			// sleep a bit before trying again
			snooze(1000000LL);
		}

printf("select returned!\n");
		if (FD_ISSET(fReadPipe, &set)) {
			char command;
			if (read(fReadPipe, &command, 1) == 1 && command == 'q')
				break;
		}

		BAutolock locker(fLock);

		for (int i = fMinSocket; i < fMaxSocket; i++) {
			if (!FD_ISSET(i, &set))
				continue;

			ServiceSocketMap::iterator iterator = fSocketMap.find(i);
			if (iterator == fSocketMap.end())
				continue;

			// launch this service's handler

			struct service* service = iterator->second;
			printf("LAUNCH: %s\n", service->launch.c_str());
		}
	}
	return B_OK;
}


/*static*/ status_t
Services::_Listener(void* _self)
{
	Services* self = (Services*)_self;
	return self->_Listener();
}
