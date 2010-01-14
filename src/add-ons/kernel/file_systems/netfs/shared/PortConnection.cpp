// PortConnection.cpp

#include <stdio.h>

#include "AutoDeleter.h"
#include "PortConnection.h"

namespace PortConnectionDefs {

const int32 kProtocolVersion = 1;

const char* kPortConnectionPortName = "NetFS port connection port";

// number of client up/down stream channels
const int32 kMinUpStreamChannels		= 1;
const int32 kMaxUpStreamChannels		= 10;
const int32 kDefaultUpStreamChannels	= 5;
const int32 kMinDownStreamChannels		= 1;
const int32 kMaxDownStreamChannels		= 5;
const int32 kDefaultDownStreamChannels	= 1;

} // namespace PortConnectionDefs

using namespace PortConnectionDefs;

// #pragma mark -
// #pragma mark ----- PortConnection -----

// constructor
PortConnection::PortConnection()
	: AbstractConnection(),
	  fUpStreamChannels(0),
	  fDownStreamChannels(0)
{
}

// destructor
PortConnection::~PortConnection()
{
}

// Init (server side)
status_t
PortConnection::Init(Channel* channel, int32 upStreamChannels,
	int32 downStreamChannels)
{
	ObjectDeleter<Channel> deleter(channel);
	status_t error = AbstractConnection::Init();
	if (error != B_OK)
		return error;
	// add the channel
	error = AddDownStreamChannel(channel);
	if (error != B_OK)
		return error;
	deleter.Detach();
	fUpStreamChannels = upStreamChannels;
	fDownStreamChannels = downStreamChannels;
	return B_OK;
}

// Init (client side)
status_t
PortConnection::Init(const char* parameters)
{
	status_t error = AbstractConnection::Init();
	if (error != B_OK)
		return error;
	// parse the parameters
	// parameter format is "[ <up> [ <down> ] ]"
	int upStreamChannels = kDefaultUpStreamChannels;
	int downStreamChannels = kDefaultDownStreamChannels;
	if (parameters)
		sscanf(parameters, "%d %d", &upStreamChannels, &downStreamChannels);
	// get the server port
	port_id serverPort = find_port(kPortConnectionPortName);
	if (serverPort < 0)
		return serverPort;
	// create a client channel
	PortChannel* channel;
	error = _CreateChannel(&channel);
	if (error != B_OK)
		return error;
	// add it as up stream channel
	error = AddUpStreamChannel(channel);
	if (error != B_OK) {
		delete channel;
		return error;
	}
	// send the server a connect request
	ConnectRequest request;
	request.protocolVersion = kProtocolVersion;
	request.upStreamChannels = upStreamChannels;
	request.downStreamChannels = downStreamChannels;
	channel->GetInfo(&request.channelInfo);
	error = write_port(serverPort, 0, &request, sizeof(ConnectRequest));
	if (error != B_OK)
		return error;
	// get the server reply
	ConnectReply reply;
	error = channel->Receive(&reply, sizeof(ConnectReply));
	if (error != B_OK)
		return error;
	error = reply.error;
	if (error != B_OK)
		return error;
	upStreamChannels = reply.upStreamChannels;
	downStreamChannels = reply.downStreamChannels;
	int32 allChannels = upStreamChannels + downStreamChannels;
	// create the channels
	PortChannel::Info* infos = new(std::nothrow)
		PortChannel::Info[allChannels - 1];
	if (!infos)
		return B_NO_MEMORY;
	ArrayDeleter<PortChannel::Info> _(infos);
	for (int32 i = 1; i < allChannels; i++) {
		// create a channel
		PortChannel* otherChannel;
		error = _CreateChannel(&otherChannel);
		if (error != B_OK)
			return error;
		// add the channel
		if (i < upStreamChannels)
			error = AddUpStreamChannel(otherChannel);
		else
			error = AddDownStreamChannel(otherChannel);
		if (error != B_OK) {
			delete otherChannel;
			return error;
		}
		// add the info
		otherChannel->GetInfo(infos + i - 1);
	}
	// send the infos to the server
	error = channel->Send(infos, sizeof(PortChannel::Info) * (allChannels - 1));
	if (error != B_OK)
		return error;
	return B_OK;
}

// FinishInitialization
status_t
PortConnection::FinishInitialization()
{
	// get the down stream channel
	Channel* channel = DownStreamChannelAt(0);
	if (!channel)
		return B_BAD_VALUE;
	// send the connect reply
	ConnectReply reply;
	reply.error = B_OK;
	reply.upStreamChannels = fUpStreamChannels;
	reply.downStreamChannels = fDownStreamChannels;
	status_t error = channel->Send(&reply, sizeof(ConnectReply));
	if (error != B_OK)
		return error;
	// receive the channel infos
	int32 allChannels = fUpStreamChannels + fDownStreamChannels;
	PortChannel::Info* infos = new(std::nothrow)
		PortChannel::Info[allChannels - 1];
	if (!infos)
		return B_NO_MEMORY;
	ArrayDeleter<PortChannel::Info> _(infos);
	error = channel->Receive(infos,
		sizeof(PortChannel::Info) * (allChannels - 1));
	if (error != B_OK)
		return error;
	// create the channels
	for (int32 i = 1; i < allChannels; i++) {
		// create a channel
		PortChannel* otherChannel;
		error = _CreateChannel(&otherChannel, infos + i - 1, true);
		if (error != B_OK)
			return error;
		// add the channel
		if (i < fUpStreamChannels)	// inverse, since we're on server side
			error = AddDownStreamChannel(otherChannel);
		else
			error = AddUpStreamChannel(otherChannel);
		if (error != B_OK) {
			delete otherChannel;
			return error;
		}
	}
	return B_OK;
}

// _CreateChannel
status_t
PortConnection::_CreateChannel(PortChannel** _channel, PortChannel::Info* info,
	bool inverse)
{
	PortChannel* channel = (info ? new(std::nothrow) PortChannel(info, inverse)
								 : new(std::nothrow) PortChannel);
	if (!channel)
		return B_NO_MEMORY;
	status_t error = channel->InitCheck();
	if (error != B_OK) {
		delete channel;
		return error;
	}
	*_channel = channel;
	return B_OK;
}

