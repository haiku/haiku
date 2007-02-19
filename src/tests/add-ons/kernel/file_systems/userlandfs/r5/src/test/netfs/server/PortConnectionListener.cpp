// PortConnectionListener.cpp

#include <stdio.h>

#include "AutoDeleter.h"
#include "PortConnection.h"
#include "PortConnectionListener.h"

using namespace PortConnectionDefs;

// constructor
PortConnectionListener::PortConnectionListener()
	: ConnectionListener(),
	  fPort(-1)
{
}

// destructor
PortConnectionListener::~PortConnectionListener()
{
	if (fPort >= 0)
		delete_port(fPort);
}

// Init
status_t
PortConnectionListener::Init(const char* parameters)
{
	fPort = create_port(5, kPortConnectionPortName);
	if (fPort < 0)
		return fPort;
	return B_OK;
}

// Listen
status_t
PortConnectionListener::Listen(Connection** _connection)
{
	if (!_connection || fPort < 0)
		return B_BAD_VALUE;
	PortChannel* channel = NULL;
	int32 upStreamChannels = 0;
	int32 downStreamChannels = 0;
	do {
		// receive a connect request
		ConnectRequest request;
		ssize_t bytesRead = read_port(fPort, 0, &request,
			sizeof(ConnectRequest));
		if (bytesRead < 0)
			return bytesRead;
		if (bytesRead != sizeof(ConnectRequest))
			continue;
		// check the protocol version
		if (request.protocolVersion != kProtocolVersion)
			continue;
		// check number of up and down stream channels
		upStreamChannels = request.upStreamChannels;
		downStreamChannels = request.downStreamChannels;
		if (upStreamChannels < kMinUpStreamChannels)
			upStreamChannels = kMinUpStreamChannels;
		else if (upStreamChannels > kMaxUpStreamChannels)
			upStreamChannels = kMaxUpStreamChannels;
		if (downStreamChannels < kMinDownStreamChannels)
			downStreamChannels = kMinDownStreamChannels;
		else if (downStreamChannels > kMaxDownStreamChannels)
			downStreamChannels = kMaxDownStreamChannels;
		// create the initial channel
		channel = new(nothrow) PortChannel(&request.channelInfo, true);
		if (!channel)
			return B_NO_MEMORY;
		if (channel->InitCheck() != B_OK) {
			delete channel;
			channel = NULL;
		}
	} while (!channel);
	// create the connection
	PortConnection* connection = new(nothrow) PortConnection;
	if (!connection) {
		delete channel;
		return B_NO_MEMORY;
	}
	status_t error = connection->Init(channel, upStreamChannels,
		downStreamChannels);
	if (error != B_OK) {
		delete connection;
		return error;
	}
	*_connection = connection;
	return B_OK;
}

// StopListening
void
PortConnectionListener::StopListening()
{
	delete_port(fPort);
	fPort = -1;
}

// FinishInitialization
status_t
PortConnectionListener::FinishInitialization(Connection* _connection,
	SecurityContext* securityContext, User** user)
{
	PortConnection* connection = dynamic_cast<PortConnection*>(_connection);
	if (!connection)
		return B_BAD_VALUE;
	*user = NULL;
	return connection->FinishInitialization();
}

