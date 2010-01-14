// SendReceiveRequest.h

#ifndef NET_FS_SEND_RECEIVE_REQUEST_H
#define NET_FS_SEND_RECEIVE_REQUEST_H

#include "RequestChannel.h"
#include "RequestConnection.h"


// error code when disconnected
enum {
	ERROR_NOT_CONNECTED = ENOTCONN
};

// SendRequest
template<typename Reply>
static
status_t
SendRequest(RequestConnection* connection, Request* request,
	Reply** _reply)
{
	Request* reply;
	status_t error = connection->SendRequest(request, &reply);
	if (error != B_OK)
		return error;
	*_reply = dynamic_cast<Reply*>(reply);
	if (!*_reply) {
		delete reply;
		return B_BAD_DATA;
	}
	return B_OK;
}

// ReceiveRequest
template<typename SpecificRequest>
static
status_t
ReceiveRequest(RequestChannel* channel, SpecificRequest** _request)
{
	Request* request;
	status_t error = channel->ReceiveRequest(&request);
	if (error != B_OK)
		return error;
	*_request = dynamic_cast<SpecificRequest*>(request);
	if (!*_request) {
		delete request;
		return B_BAD_DATA;
	}
	return B_OK;
}


#endif	// NET_FS_SEND_RECEIVE_REQUEST_H
