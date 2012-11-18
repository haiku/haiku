/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "Services.h"
#include "NetServer.h"
#include "Settings.h"

#include <Autolock.h>
#include <NetworkAddress.h>

#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <new>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <vector>

using namespace std;

struct service_address {
	struct service* owner;
	int		socket;
	int		family;
	int		type;
	int		protocol;
	BNetworkAddress address;

	bool operator==(const struct service_address& other) const;
};

typedef std::vector<service_address> AddressList;
typedef std::vector<std::string> StringList;

struct service {
	std::string	name;
	StringList	arguments;
	uid_t		user;
	gid_t		group;
	AddressList	addresses;
	uint32		update;
	bool		stand_alone;
	pid_t		process;

	~service();
	bool operator!=(const struct service& other) const;
	bool operator==(const struct service& other) const;
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


bool
service_address::operator==(const struct service_address& other) const
{
	return family == other.family
		&& type == other.type
		&& protocol == other.protocol
		&& address == other.address;
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
service::operator!=(const struct service& other) const
{
	return !(*this == other);
}


bool
service::operator==(const struct service& other) const
{
	if (name != other.name
		|| arguments.size() != other.arguments.size()
		|| addresses.size() != other.addresses.size()
		|| stand_alone != other.stand_alone)
		return false;

	// compare arguments

	for(size_t i = 0; i < arguments.size(); i++) {
		if (arguments[i] != other.arguments[i])
			return false;
	}

	// compare addresses

	AddressList::const_iterator iterator = addresses.begin();
	for (; iterator != addresses.end(); iterator++) {
		const service_address& address = *iterator;

		// find address in other addresses

		AddressList::const_iterator otherIterator = other.addresses.begin();
		for (; otherIterator != other.addresses.end(); otherIterator++) {
			if (address == *otherIterator)
				break;
		}

		if (otherIterator == other.addresses.end())
			return false;
	}

	return true;
}


//	#pragma mark -


Services::Services(const BMessage& services)
	:
	fListener(-1),
	fUpdate(0),
	fMaxSocket(0)
{
	// setup pipe to communicate with the listener thread - as the listener
	// blocks on select(), we need a mechanism to interrupt it
	if (pipe(&fReadPipe) < 0) {
		fReadPipe = -1;
		return;
	}

	fcntl(fReadPipe, F_SETFD, FD_CLOEXEC);
	fcntl(fWritePipe, F_SETFD, FD_CLOEXEC);

	FD_ZERO(&fSet);
	FD_SET(fReadPipe, &fSet);

	fMinSocket = fWritePipe + 1;
	fMaxSocket = fWritePipe + 1;

	_Update(services);

	fListener = spawn_thread(_Listener, "services listener", B_NORMAL_PRIORITY,
		this);
	if (fListener >= B_OK)
		resume_thread(fListener);
}


Services::~Services()
{
	wait_for_thread(fListener, NULL);

	close(fReadPipe);
	close(fWritePipe);

	// stop all services

	while (!fNameMap.empty()) {
		_StopService(*fNameMap.begin()->second);
	}
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
Services::_UpdateMinMaxSocket(int socket)
{
	if (socket >= fMaxSocket)
		fMaxSocket = socket + 1;
	if (socket < fMinSocket)
		fMinSocket = socket;
}


status_t
Services::_StartService(struct service& service)
{
	if (service.stand_alone && service.process == -1) {
		status_t status = _LaunchService(service, -1);
		if (status == B_OK) {
			// add service
			fNameMap[service.name] = &service;
			service.update = fUpdate;
		}
		return status;
	}

	// create socket

	bool failed = false;
	AddressList::iterator iterator = service.addresses.begin();
	for (; iterator != service.addresses.end(); iterator++) {
		service_address& address = *iterator;

		address.socket = socket(address.family, address.type, address.protocol);
		if (address.socket < 0
			|| bind(address.socket, address.address, address.address.Length())
					< 0
			|| fcntl(address.socket, F_SETFD, FD_CLOEXEC) < 0) {
			failed = true;
			break;
		}

		if (address.type == SOCK_STREAM && listen(address.socket, 50) < 0) {
			failed = true;
			break;
		}
	}

	if (failed) {
		// open sockets will be closed when the service is deleted
		return errno;
	}

	// add service to maps and activate it

	fNameMap[service.name] = &service;
	service.update = fUpdate;

	iterator = service.addresses.begin();
	for (; iterator != service.addresses.end(); iterator++) {
		service_address& address = *iterator;

		fSocketMap[address.socket] = &address;
		_UpdateMinMaxSocket(address.socket);
		FD_SET(address.socket, &fSet);
	}

	_NotifyListener();
	printf("Starting service '%s'\n", service.name.c_str());
	return B_OK;
}


status_t
Services::_StopService(struct service& service)
{
	printf("Stop service '%s'\n", service.name.c_str());

	// remove service from maps
	{
		ServiceNameMap::iterator iterator = fNameMap.find(service.name);
		if (iterator != fNameMap.end())
			fNameMap.erase(iterator);
	}

	if (!service.stand_alone) {
		AddressList::const_iterator iterator = service.addresses.begin();
		for (; iterator != service.addresses.end(); iterator++) {
			const service_address& address = *iterator;

			ServiceSocketMap::iterator socketIterator
				= fSocketMap.find(address.socket);
			if (socketIterator != fSocketMap.end())
				fSocketMap.erase(socketIterator);

			close(address.socket);
			FD_CLR(address.socket, &fSet);
		}
	}

	// Shutdown the running server, if any
	if (service.process != -1) {
		printf("  Sending SIGTERM to process %" B_PRId32 "\n", service.process);
		kill(-service.process, SIGTERM);
	}

	delete &service;
	return B_OK;
}


status_t
Services::_ToService(const BMessage& message, struct service*& service)
{
	// get mandatory fields
	const char* name;
	if (message.FindString("name", &name) != B_OK
		|| !message.HasString("launch"))
		return B_BAD_VALUE;

	service = new (std::nothrow) ::service;
	if (service == NULL)
		return B_NO_MEMORY;

	service->name = name;

	const char* argument;
	for (int i = 0; message.FindString("launch", i, &argument) == B_OK; i++) {
		service->arguments.push_back(argument);
	}

	service->stand_alone = false;
	service->process = -1;

	// TODO: user/group is currently ignored!

	// Default family/port/protocol/type for all addresses

	// we default to inet/tcp/port-from-service-name if nothing is specified
	const char* string;
	if (message.FindString("family", &string) != B_OK)
		string = "inet";

	int32 serviceFamily = get_address_family(string);
	if (serviceFamily == AF_UNSPEC)
		serviceFamily = AF_INET;

	int32 serviceProtocol;
	if (message.FindString("protocol", &string) == B_OK)
		serviceProtocol = parse_protocol(string);
	else {
		string = "tcp";
			// we set 'string' here for an eventual call to getservbyname()
			// below
		serviceProtocol = IPPROTO_TCP;
	}

	int32 servicePort;
	if (message.FindInt32("port", &servicePort) != B_OK) {
		struct servent* servent = getservbyname(name, string);
		if (servent != NULL)
			servicePort = ntohs(servent->s_port);
		else
			servicePort = -1;
	}

	int32 serviceType = -1;
	if (message.FindString("type", &string) == B_OK) {
		serviceType = parse_type(string);
	} else {
		serviceType = type_for_protocol(serviceProtocol);
	}

	bool standAlone = false;
	if (message.FindBool("stand_alone", &standAlone) == B_OK)
		service->stand_alone = standAlone;

	BMessage address;
	int32 i = 0;
	for (; message.FindMessage("address", i, &address) == B_OK; i++) {
		// TODO: dump problems in the settings to syslog
		service_address serviceAddress;
		if (address.FindString("family", &string) != B_OK)
			continue;

		serviceAddress.family = get_address_family(string);
		if (serviceAddress.family == AF_UNSPEC)
			continue;

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
			if (!parse_address(serviceFamily, string, serviceAddress.address))
				continue;
		} else
			serviceAddress.address.SetToWildcard(serviceFamily);

		int32 port;
		if (address.FindInt32("port", &port) != B_OK)
			port = servicePort;

		serviceAddress.address.SetPort(port);
		serviceAddress.socket = -1;

		serviceAddress.owner = service;
		service->addresses.push_back(serviceAddress);
	}

	if (i == 0 && (serviceFamily < 0 || servicePort < 0)) {
		// no address specified
		printf("service %s has no address specified\n", name);
		delete service;
		return B_BAD_VALUE;
	}

	if (i == 0) {
		// no address specified, but family/port were given; add empty address
		service_address serviceAddress;
		serviceAddress.family = serviceFamily;
		serviceAddress.type = serviceType;
		serviceAddress.protocol = serviceProtocol;
		serviceAddress.address.SetToWildcard(serviceFamily, servicePort);

		serviceAddress.socket = -1;

		serviceAddress.owner = service;
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
			printf("New service %s\n", service->name.c_str());
			_StartService(*service);
		} else {
			// this service does already exist - check for any changes

			if (*service != *iterator->second) {
				printf("Restart service %s\n", service->name.c_str());
				_StopService(*iterator->second);
				_StartService(*service);
			} else
				iterator->second->update = fUpdate;
		}
	}

	// stop all services that are not part of the update message

	ServiceNameMap::iterator iterator = fNameMap.begin();
	while (iterator != fNameMap.end()) {
		struct service* service = iterator->second;
		iterator++;

		if (service->update != fUpdate) {
			// this service has to be removed
			_StopService(*service);
		}
	}
}


status_t
Services::_LaunchService(struct service& service, int socket)
{
	printf("Launch service: %s\n", service.arguments[0].c_str());

	if (socket != -1 && fcntl(socket, F_SETFD, 0) < 0) {
		// could not clear FD_CLOEXEC on socket
		return errno;
	}

	pid_t child = fork();
	if (child == 0) {
		setsid();
			// make sure we're in our own session, and don't accidently quit
			// the net_server

		if (socket != -1) {
			// We're the child, replace standard input/output
			dup2(socket, STDIN_FILENO);
			dup2(socket, STDOUT_FILENO);
			dup2(socket, STDERR_FILENO);
			close(socket);
		}

		// build argument array

		const char** args = (const char**)malloc(
			(service.arguments.size() + 1) * sizeof(char*));
		if (args == NULL)
			exit(1);

		for (size_t i = 0; i < service.arguments.size(); i++) {
			args[i] = service.arguments[i].c_str();
		}
		args[service.arguments.size()] = NULL;

		if (execv(service.arguments[0].c_str(), (char* const*)args) < 0)
			exit(1);

		// we'll never trespass here
	} else {
		// the server does not need the socket anymore
		if (socket != -1)
			close(socket);

		if (service.stand_alone)
			service.process = child;
	}

	// TODO: make sure child started successfully...
	return B_OK;
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

			struct service_address& address = *iterator->second;
			int socket;

			if (address.type == SOCK_STREAM) {
				// accept incoming connection
				int value = 1;
				ioctl(i, FIONBIO, &value);
					// make sure we don't wait for the connection

				socket = accept(address.socket, NULL, NULL);

				value = 0;
				ioctl(i, FIONBIO, &value);

				if (socket < 0)
					continue;
			} else
				socket = address.socket;

			// launch this service's handler

			_LaunchService(*address.owner, socket);
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
