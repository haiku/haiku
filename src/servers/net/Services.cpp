/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "Services.h"
#include "Settings.h"

#include <Autolock.h>

#include <new>
#include <sys/socket.h>
#include <vector>

using namespace std;

struct service_address {
	int		socket;
	int		family;
	uint16	port;
	sockaddr address;
};

typedef vector<service_address> AddressList;

struct service {
	std::string name;
	std::string	launch;
	int		type;
	int		protocol;
	uint16	port;
	uid_t	user;
	gid_t	group;
	AddressList addresses;

	~service();
	bool operator!=(const struct service& other);
	bool operator==(const struct service& other);
};


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
		|| protocol != other.protocol
		|| port != other.port)
		return false;

	// TODO: compare addresses!
	return true;
}


//	#pragma mark -


Services::Services(const BMessage& services)
	:
	fUpdate(0)
{
	_Update(services);

	fListener = spawn_thread(_Listener, "services listener", B_NORMAL_PRIORITY, this);
	if (fListener >= B_OK)
		resume_thread(fListener);
	
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
		case kMsgServiceSettingsUpdated:
			_Update(*message);
			break;

		default:
			BHandler::MessageReceived(message);
	}
}


status_t
Services::_StartService(struct service& service)
{
	// add service to maps

	fNameMap[service.name] = &service;

	AddressList::const_iterator iterator = service.addresses.begin();
	for (; iterator != service.addresses.end(); iterator++) {
		const service_address& address = *iterator;

		fSocketMap[address.socket] = &service;
	}

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

	// TODO: fill in other fields
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


/*static*/ status_t
Services::_Listener(void* self)
{
	return B_OK;
}

