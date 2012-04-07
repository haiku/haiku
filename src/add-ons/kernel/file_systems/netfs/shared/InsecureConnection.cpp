// InsecureConnection.cpp

// TODO: Asynchronous connecting on client side?

#include <new>

#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <ByteOrder.h>

#include "Compatibility.h"
#include "DebugSupport.h"
#include "InsecureChannel.h"
#include "InsecureConnection.h"
#include "NetAddress.h"
#include "NetFSDefs.h"

namespace InsecureConnectionDefs {

const int32 kProtocolVersion = 1;

const bigtime_t kAcceptingTimeout = 10000000;	// 10 s

// number of client up/down stream channels
const int32 kMinUpStreamChannels		= 1;
const int32 kMaxUpStreamChannels		= 10;
const int32 kDefaultUpStreamChannels	= 5;
const int32 kMinDownStreamChannels		= 1;
const int32 kMaxDownStreamChannels		= 5;
const int32 kDefaultDownStreamChannels	= 1;

} // namespace InsecureConnectionDefs

using namespace InsecureConnectionDefs;

// SocketCloser
struct SocketCloser {
	SocketCloser(int fd) : fFD(fd) {}
	~SocketCloser()
	{
		if (fFD >= 0)
			closesocket(fFD);
	}

	int Detach()
	{
		int fd = fFD;
		fFD = -1;
		return fd;
	}

private:
	int	fFD;
};


// #pragma mark -
// #pragma mark ----- InsecureConnection -----

// constructor
InsecureConnection::InsecureConnection()
{
}

// destructor
InsecureConnection::~InsecureConnection()
{
}

// Init (server side)
status_t
InsecureConnection::Init(int fd)
{
	status_t error = AbstractConnection::Init();
	if (error != B_OK) {
		closesocket(fd);
		return error;
	}
	// create the initial channel
	Channel* channel = new(std::nothrow) InsecureChannel(fd);
	if (!channel) {
		closesocket(fd);
		return B_NO_MEMORY;
	}
	// add it
	error = AddDownStreamChannel(channel);
	if (error != B_OK) {
		delete channel;
		return error;
	}
	return B_OK;
}

// Init (client side)
status_t
InsecureConnection::Init(const char* parameters)
{
PRINT(("InsecureConnection::Init\n"));
	if (!parameters)
		return B_BAD_VALUE;
	status_t error = AbstractConnection::Init();
	if (error != B_OK)
		return error;
	// parse the parameters to get a server name and a port we shall connect to
	// parameter format is "<server>[:port] [ <up> [ <down> ] ]"
	char server[256];
	uint16 port = kDefaultInsecureConnectionPort;
	int upStreamChannels = kDefaultUpStreamChannels;
	int downStreamChannels = kDefaultDownStreamChannels;
	if (strchr(parameters, ':')) {
		int result = sscanf(parameters, "%255[^:]:%hu %d %d", server, &port,
			&upStreamChannels, &downStreamChannels);
		if (result < 2)
			return B_BAD_VALUE;
	} else {
		int result = sscanf(parameters, "%255[^:] %d %d", server,
			&upStreamChannels, &downStreamChannels);
		if (result < 1)
			return B_BAD_VALUE;
	}
	// resolve server address
	NetAddress netAddress;
	error = NetAddressResolver().GetHostAddress(server, &netAddress);
	if (error != B_OK)
		return error;
	in_addr serverAddr = netAddress.GetAddress().sin_addr;
	// open the initial channel
	Channel* channel;
	error = _OpenClientChannel(serverAddr, port, &channel);
	if (error != B_OK)
		return error;
	error = AddUpStreamChannel(channel);
	if (error != B_OK) {
		delete channel;
		return error;
	}
	// send the server a connect request
	ConnectRequest request;
	request.protocolVersion = B_HOST_TO_BENDIAN_INT32(kProtocolVersion);
	request.serverAddress = serverAddr.s_addr;
	request.upStreamChannels = B_HOST_TO_BENDIAN_INT32(upStreamChannels);
	request.downStreamChannels = B_HOST_TO_BENDIAN_INT32(downStreamChannels);
	error = channel->Send(&request, sizeof(ConnectRequest));
	if (error != B_OK)
		return error;
	// get the server reply
	ConnectReply reply;
	error = channel->Receive(&reply, sizeof(ConnectReply));
	if (error != B_OK)
		return error;
	error = B_BENDIAN_TO_HOST_INT32(reply.error);
	if (error != B_OK)
		return error;
	upStreamChannels = B_BENDIAN_TO_HOST_INT32(reply.upStreamChannels);
	downStreamChannels = B_BENDIAN_TO_HOST_INT32(reply.downStreamChannels);
	port = B_BENDIAN_TO_HOST_INT16(reply.port);
	// open the remaining channels
	int32 allChannels = upStreamChannels + downStreamChannels;
	for (int32 i = 1; i < allChannels; i++) {
		PRINT("  creating channel %ld\n", i);
		// open the channel
		error = _OpenClientChannel(serverAddr, port, &channel);
		if (error != B_OK)
			RETURN_ERROR(error);
		// add it
		if (i < upStreamChannels)
			error = AddUpStreamChannel(channel);
		else
			error = AddDownStreamChannel(channel);
		if (error != B_OK) {
			delete channel;
			return error;
		}
	}
	return B_OK;
}

// FinishInitialization
status_t
InsecureConnection::FinishInitialization()
{
PRINT(("InsecureConnection::FinishInitialization()\n"));
	// get the down stream channel
	InsecureChannel* channel
		= dynamic_cast<InsecureChannel*>(DownStreamChannelAt(0));
	if (!channel)
		return B_BAD_VALUE;
	// receive the connect request
	ConnectRequest request;
	status_t error = channel->Receive(&request, sizeof(ConnectRequest));
	if (error != B_OK)
		return error;
	// check the protocol version
	int32 protocolVersion = B_BENDIAN_TO_HOST_INT32(request.protocolVersion);
	if (protocolVersion != kProtocolVersion) {
		_SendErrorReply(channel, B_ERROR);
		return B_ERROR;
	}
	// get our address (we need it for binding)
	in_addr serverAddr;
	serverAddr.s_addr = request.serverAddress;
	// check number of up and down stream channels
	int32 upStreamChannels = B_BENDIAN_TO_HOST_INT32(request.upStreamChannels);
	int32 downStreamChannels = B_BENDIAN_TO_HOST_INT32(
		request.downStreamChannels);
	if (upStreamChannels < kMinUpStreamChannels)
		upStreamChannels = kMinUpStreamChannels;
	else if (upStreamChannels > kMaxUpStreamChannels)
		upStreamChannels = kMaxUpStreamChannels;
	if (downStreamChannels < kMinDownStreamChannels)
		downStreamChannels = kMinDownStreamChannels;
	else if (downStreamChannels > kMaxDownStreamChannels)
		downStreamChannels = kMaxDownStreamChannels;
	// due to a bug on BONE we have a maximum of 2 working connections
	// accepted on one listener socket.
	NetAddress peerAddress;
	if (channel->GetPeerAddress(&peerAddress) == B_OK
		&& peerAddress.IsLocal()) {
		upStreamChannels = 1;
		if (downStreamChannels > 2)
			downStreamChannels = 2;
	}
	int32 allChannels = upStreamChannels + downStreamChannels;
	// create a listener socket
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		error = errno;
		_SendErrorReply(channel, error);
		return error;
	}
	SocketCloser _(fd);
	// bind it to some port
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = 0;
	addr.sin_addr = serverAddr;
	if (bind(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
		error = errno;
		_SendErrorReply(channel, error);
		return error;
	}
	// get the port
	socklen_t addrSize = sizeof(addr);
	if (getsockname(fd, (sockaddr*)&addr, &addrSize) < 0) {
		error = errno;
		_SendErrorReply(channel, error);
		return error;
	}
	// set socket to non-blocking
	int dontBlock = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_NONBLOCK, &dontBlock, sizeof(int)) < 0) {
		error = errno;
		_SendErrorReply(channel, error);
		return error;
	}
	// start listening
	if (listen(fd, allChannels - 1) < 0) {
		error = errno;
		_SendErrorReply(channel, error);
		return error;
	}
	// send the reply
	ConnectReply reply;
	reply.error = B_HOST_TO_BENDIAN_INT32(B_OK);
	reply.upStreamChannels = B_HOST_TO_BENDIAN_INT32(upStreamChannels);
	reply.downStreamChannels = B_HOST_TO_BENDIAN_INT32(downStreamChannels);
	reply.port = addr.sin_port;
	error = channel->Send(&reply, sizeof(ConnectReply));
	if (error != B_OK)
		return error;
	// start accepting
	bigtime_t startAccepting = system_time();
	for (int32 i = 1; i < allChannels; ) {
		// accept a connection
		int channelFD = accept(fd, NULL, 0);
		if (channelFD < 0) {
			error = errno;
			if (error == B_INTERRUPTED) {
				error = B_OK;
				continue;
			}
			if (error == B_WOULD_BLOCK) {
				bigtime_t now = system_time();
				if (now - startAccepting > kAcceptingTimeout)
					RETURN_ERROR(B_TIMED_OUT);
				snooze(10000);
				continue;
			}
			RETURN_ERROR(error);
		}
		PRINT("  accepting channel %ld\n", i);
		// create a channel
		channel = new(std::nothrow) InsecureChannel(channelFD);
		if (!channel) {
			closesocket(channelFD);
			return B_NO_MEMORY;
		}
		// add it
		if (i < upStreamChannels)	// inverse, since we are on server side
			error = AddDownStreamChannel(channel);
		else
			error = AddUpStreamChannel(channel);
		if (error != B_OK) {
			delete channel;
			return error;
		}
		i++;
		startAccepting = system_time();
	}
	return B_OK;
}

// _OpenClientChannel
status_t
InsecureConnection::_OpenClientChannel(in_addr serverAddr, uint16 port,
	Channel** _channel)
{
	// create a socket
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		return errno;
	// connect
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr = serverAddr;
	if (connect(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
		status_t error = errno;
		closesocket(fd);
		RETURN_ERROR(error);
	}
	// create the channel
	Channel* channel = new(std::nothrow) InsecureChannel(fd);
	if (!channel) {
		closesocket(fd);
		return B_NO_MEMORY;
	}
	*_channel = channel;
	return B_OK;
}

// _SendErrorReply
status_t
InsecureConnection::_SendErrorReply(Channel* channel, status_t error)
{
	ConnectReply reply;
	reply.error = B_HOST_TO_BENDIAN_INT32(error);
	return channel->Send(&reply, sizeof(ConnectReply));
}

