// InsecureConnectionListener.cpp

#include <new>

#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "Compatibility.h"
#include "DebugSupport.h"
#include "InsecureChannel.h"
#include "InsecureConnection.h"
#include "InsecureConnectionListener.h"
#include "NetFSDefs.h"
#include "Utils.h"

using namespace InsecureConnectionDefs;

// constructor
InsecureConnectionListener::InsecureConnectionListener()
	: ConnectionListener(),
	  fSocket(-1)
{
}

// destructor
InsecureConnectionListener::~InsecureConnectionListener()
{
	safe_closesocket(fSocket);
}

// Init
status_t
InsecureConnectionListener::Init(const char* parameters)
{
	// parse the parameters
	// parameter format is "[port]"
	uint16 port = kDefaultInsecureConnectionPort;
	if (parameters && strlen(parameters) > 0) {
		int result = sscanf(parameters, "%hu", &port);
		if (result < 1)
			return B_BAD_VALUE;
	}
	// create a listener socket
	fSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (fSocket < 0)
		return errno;
	// bind it to the port
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;
	if (bind(fSocket, (sockaddr*)&addr, sizeof(addr)) < 0)
		return errno;
	// start listening
	if (listen(fSocket, 5) < 0)
		return errno;
	return B_OK;
}

// Listen
status_t
InsecureConnectionListener::Listen(Connection** _connection)
{
	if (!_connection || fSocket < 0)
		return B_BAD_VALUE;
	// accept a connection
	int fd = -1;
	do {
		fd = accept(fSocket, NULL, 0);
		if (fd < 0) {
			status_t error = errno;
			if (error != B_INTERRUPTED)
				return error;
		}
	} while (fd < 0);
	// create a connection
	InsecureConnection* connection = new(std::nothrow) InsecureConnection;
	if (!connection) {
		closesocket(fd);
		return B_NO_MEMORY;
	}
	status_t error = connection->Init(fd);
	if (error != B_OK) {
		delete connection;
		return error;
	}
	*_connection = connection;
	return B_OK;
}

// StopListening
void
InsecureConnectionListener::StopListening()
{
	safe_closesocket(fSocket);
}

// FinishInitialization
status_t
InsecureConnectionListener::FinishInitialization(Connection* _connection,
	SecurityContext* securityContext, User** user)
{
	InsecureConnection* connection
		= dynamic_cast<InsecureConnection*>(_connection);
	if (!connection)
		return B_BAD_VALUE;
	*user = NULL;
	return connection->FinishInitialization();
}

