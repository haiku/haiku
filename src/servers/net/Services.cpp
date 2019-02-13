/*
 * Copyright 2006-2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "Services.h"

#include <new>

#include <errno.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <Autolock.h>
#include <NetworkAddress.h>
#include <NetworkSettings.h>

#include <NetServer.h>


using namespace std;
using namespace BNetworkKit;


struct service_connection {
	struct service* owner;
	int		socket;
	BNetworkServiceAddressSettings address;

	//service_connection() : owner(NULL), socket(-1) {}

	int Family() const { return address.Family(); }
	int Protocol() const { return address.Protocol(); }
	int Type() const { return address.Type(); }
	const BNetworkAddress& Address() const { return address.Address(); }

	bool operator==(const struct service_connection& other) const;
};

typedef std::vector<service_connection> ConnectionList;
typedef std::vector<std::string> StringList;

struct service {
	std::string		name;
	StringList		arguments;
	uid_t			user;
	gid_t			group;
	ConnectionList	connections;
	uint32			update;
	bool			stand_alone;
	pid_t			process;

	~service();
	bool operator!=(const struct service& other) const;
	bool operator==(const struct service& other) const;
};


//	#pragma mark -


bool
service_connection::operator==(const struct service_connection& other) const
{
	return address == other.address;
}


//	#pragma mark -


service::~service()
{
	// close all open sockets
	ConnectionList::const_iterator iterator = connections.begin();
	for (; iterator != connections.end(); iterator++) {
		const service_connection& connection = *iterator;

		close(connection.socket);
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
		|| connections.size() != other.connections.size()
		|| stand_alone != other.stand_alone)
		return false;

	// Compare arguments

	for(size_t i = 0; i < arguments.size(); i++) {
		if (arguments[i] != other.arguments[i])
			return false;
	}

	// Compare connections

	ConnectionList::const_iterator iterator = connections.begin();
	for (; iterator != connections.end(); iterator++) {
		const service_connection& connection = *iterator;

		// Find address in other addresses

		ConnectionList::const_iterator otherIterator
			= other.connections.begin();
		for (; otherIterator != other.connections.end(); otherIterator++) {
			if (connection == *otherIterator)
				break;
		}

		if (otherIterator == other.connections.end())
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
		_StopService(fNameMap.begin()->second);
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

		case kMsgIsServiceRunning:
		{
			const char* name = message->GetString("name");
			if (name == NULL)
				break;

			BMessage reply(B_REPLY);
			reply.AddString("name", name);
			reply.AddBool("running", fNameMap.find(name) != fNameMap.end());
			message->SendReply(&reply);
			break;
		}

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
	ConnectionList::iterator iterator = service.connections.begin();
	for (; iterator != service.connections.end(); iterator++) {
		service_connection& connection = *iterator;

		connection.socket = socket(connection.Family(),
			connection.Type(), connection.Protocol());
		if (connection.socket < 0
			|| bind(connection.socket, connection.Address(),
				connection.Address().Length()) < 0
			|| fcntl(connection.socket, F_SETFD, FD_CLOEXEC) < 0) {
			failed = true;
			break;
		}

		if (connection.Type() == SOCK_STREAM
			&& listen(connection.socket, 50) < 0) {
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

	iterator = service.connections.begin();
	for (; iterator != service.connections.end(); iterator++) {
		service_connection& connection = *iterator;

		fSocketMap[connection.socket] = &connection;
		_UpdateMinMaxSocket(connection.socket);
		FD_SET(connection.socket, &fSet);
	}

	_NotifyListener();
	printf("Starting service '%s'\n", service.name.c_str());
	return B_OK;
}


status_t
Services::_StopService(struct service* service)
{
	printf("Stop service '%s'\n", service->name.c_str());

	// remove service from maps
	{
		ServiceNameMap::iterator iterator = fNameMap.find(service->name);
		if (iterator != fNameMap.end())
			fNameMap.erase(iterator);
	}

	if (!service->stand_alone) {
		ConnectionList::const_iterator iterator = service->connections.begin();
		for (; iterator != service->connections.end(); iterator++) {
			const service_connection& connection = *iterator;

			ServiceSocketMap::iterator socketIterator
				= fSocketMap.find(connection.socket);
			if (socketIterator != fSocketMap.end())
				fSocketMap.erase(socketIterator);

			close(connection.socket);
			FD_CLR(connection.socket, &fSet);
		}
	}

	// Shutdown the running server, if any
	if (service->process != -1) {
		printf("  Sending SIGTERM to process %" B_PRId32 "\n", service->process);
		kill(-service->process, SIGTERM);
	}

	delete service;
	return B_OK;
}


status_t
Services::_ToService(const BMessage& message, struct service*& service)
{
	BNetworkServiceSettings settings(message);
	status_t status = settings.InitCheck();
	if (status != B_OK)
		return status;
	if (!settings.IsEnabled())
		return B_NAME_NOT_FOUND;

	service = new (std::nothrow) ::service;
	if (service == NULL)
		return B_NO_MEMORY;

	service->name = settings.Name();
	service->stand_alone = settings.IsStandAlone();
	service->process = -1;

	// Copy launch arguments
	for (int32 i = 0; i < settings.CountArguments(); i++)
		service->arguments.push_back(settings.ArgumentAt(i));

	// Copy addresses to listen to
	for (int32 i = 0; i < settings.CountAddresses(); i++) {
		const BNetworkServiceAddressSettings& address = settings.AddressAt(i);
		service_connection connection;
		connection.owner = service;
		connection.socket = -1;
		connection.address = address;

		service->connections.push_back(connection);
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
		struct service* service;
		if (_ToService(message, service) != B_OK)
			continue;

		ServiceNameMap::iterator iterator = fNameMap.find(service->name);
		if (iterator == fNameMap.end()) {
			// this service does not exist yet, start it
			printf("New service %s\n", service->name.c_str());
			_StartService(*service);
		} else {
			// this service does already exist - check for any changes

			if (*service != *iterator->second) {
				printf("Restart service %s\n", service->name.c_str());
				_StopService(iterator->second);
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
			_StopService(service);
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

		if (execv(service.arguments[0].c_str(), (char* const*)args) < 0) {
			free(args);
			exit(1);
		}

		// we'll never trespass here
	} else {
		// the server does not need the socket anymore
		if (socket != -1)
			close(socket);

		if (child < 0) {
			fprintf(stderr, "Could not start service %s\n",
				service.name.c_str());
		} else if (service.stand_alone)
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

			struct service_connection& connection = *iterator->second;
			int socket;

			if (connection.Type() == SOCK_STREAM) {
				// accept incoming connection
				int value = 1;
				ioctl(i, FIONBIO, &value, sizeof(value));
					// make sure we don't wait for the connection

				socket = accept(connection.socket, NULL, NULL);

				value = 0;
				ioctl(i, FIONBIO, &value, sizeof(value));

				if (socket < 0)
					continue;
			} else
				socket = connection.socket;

			// launch this service's handler

			_LaunchService(*connection.owner, socket);
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
