// ConnectionListenerFactory.cpp

#include <new>

#include <string.h>

#include "ConnectionListenerFactory.h"
#include "InsecureConnectionListener.h"
#include "PortConnectionListener.h"

// constructor
ConnectionListenerFactory::ConnectionListenerFactory()
{
}

// destructor
ConnectionListenerFactory::~ConnectionListenerFactory()
{
}

// CreateConnectionListener
status_t
ConnectionListenerFactory::CreateConnectionListener(const char* type,
	const char* parameters, ConnectionListener** connectionListener)
{
	if (!type)
		return B_BAD_VALUE;
	// create the connection listener
	ConnectionListener* listener = NULL;
	if (strcmp(type, "insecure") == 0)
		listener = new(nothrow) InsecureConnectionListener;
	else if (strcmp(type, "port") == 0)
		listener = new(nothrow) PortConnectionListener;
	else
		return B_BAD_VALUE;
	if (!listener)
		return B_NO_MEMORY;
	// init it
	status_t error = listener->Init(parameters);
	if (error != B_OK) {
		delete listener;
		return error;
	}
	*connectionListener = listener;
	return B_OK;
}

