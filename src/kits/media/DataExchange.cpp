/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <OS.h>
#include <Messenger.h>
#include "debug.h"
#include "PortPool.h"
#include "DataExchange.h"
#include "ServerInterface.h" // NEW_MEDIA_SERVER_SIGNATURE

#define TIMEOUT 100000

namespace BPrivate {
namespace media {

team_id team;

namespace dataexchange {

BMessenger *MediaServerMessenger;
static port_id MediaServerPort;
static port_id MediaAddonServerPort;

class initit
{
public:
	initit()
	{
		MediaServerMessenger = new BMessenger(NEW_MEDIA_SERVER_SIGNATURE);
		MediaServerPort = find_port("media_server port");
		MediaAddonServerPort = find_port("media_addon_server port");
	}
	~initit()
	{
		delete MediaServerMessenger;
	}
};
initit _initit;


void
request_data::SendReply(reply_data *reply, int replysize) const
{
	SendToPort(reply_port, 0, reply, replysize);
}


// BMessage based data exchange with the media_server
status_t SendToServer(BMessage *msg)
{
	status_t rv;
	rv = MediaServerMessenger->SendMessage(msg, static_cast<BHandler *>(NULL), TIMEOUT);
	if (rv != B_OK)
		TRACE("SendToServer: SendMessage failed\n");
	return rv;
}


status_t QueryServer(BMessage *request, BMessage *reply)
{
	status_t rv;
	rv = MediaServerMessenger->SendMessage(request, reply, TIMEOUT, TIMEOUT);
	if (rv != B_OK)
		TRACE("QueryServer: SendMessage failed\n");
	return rv;
}


// Raw data based data exchange with the media_server
status_t SendToServer(int32 msgcode, void *msg, int size)
{
	return SendToPort(MediaServerPort, msgcode, msg, size);
}

status_t QueryServer(int32 msgcode, request_data *request, int requestsize, reply_data *reply, int replysize)
{
	return QueryPort(MediaServerPort, msgcode, request, requestsize, reply, replysize);
}


// Raw data based data exchange with the media_addon_server
status_t SendToAddonServer(int32 msgcode, void *msg, int size)
{
	return SendToPort(MediaAddonServerPort, msgcode, msg, size);
}


status_t QueryAddonServer(int32 msgcode, request_data *request, int requestsize, reply_data *reply, int replysize)
{
	return QueryPort(MediaAddonServerPort, msgcode, request, requestsize, reply, replysize);
}


// Raw data based data exchange with the media_server
status_t SendToPort(port_id sendport, int32 msgcode, void *msg, int size)
{
	status_t rv;
	rv = write_port(sendport, msgcode, msg, size);
	if (rv != B_OK)
		TRACE("SendToPort: write_port failed\n");
	return B_OK;
}


status_t QueryPort(port_id requestport, int32 msgcode, request_data *request, int requestsize, reply_data *reply, int replysize)
{
	status_t rv;
	int32 code;

	request->reply_port = _PortPool->GetPort();

	rv = write_port(requestport, msgcode, request, requestsize);
	if (rv != B_OK) {
		TRACE("QueryPort: write_port failed\n");
		_PortPool->PutPort(request->reply_port);
		return rv;
	}

	rv = read_port(request->reply_port, &code, reply, replysize);
	_PortPool->PutPort(request->reply_port);

	if (rv < B_OK)
		TRACE("QueryPort: read_port failed\n");
	
	return (rv < B_OK) ? rv : reply->result;
}

}; // dataexchange
}; // media
}; // BPrivate

