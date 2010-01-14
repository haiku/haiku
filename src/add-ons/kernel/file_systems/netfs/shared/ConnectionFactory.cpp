// ConnectionFactory.cpp

#include <string.h>

#include "ConnectionFactory.h"
#include "InsecureConnection.h"
#include "PortConnection.h"

// constructor
ConnectionFactory::ConnectionFactory()
{
}

// destructor
ConnectionFactory::~ConnectionFactory()
{
}

// CreateConnection
status_t
ConnectionFactory::CreateConnection(const char* type, const char* parameters,
	Connection** _connection)
{
	if (!type)
		return B_BAD_VALUE;
	// create the connection
	Connection* connection = NULL;
	if (strcmp(type, "insecure") == 0)
		connection = new(std::nothrow) InsecureConnection;
	else if (strcmp(type, "port") == 0)
		connection = new(std::nothrow) PortConnection;
	else
		return B_BAD_VALUE;
	if (!connection)
		return B_NO_MEMORY;
	// init it
	status_t error = connection->Init(parameters);
	if (error != B_OK) {
		delete connection;
		return error;
	}
	*_connection = connection;
	return B_OK;
}

