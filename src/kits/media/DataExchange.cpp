/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <OS.h>
#include <Messenger.h>
#include <string.h>
#include "debug.h"
#include "PortPool.h"
#include "DataExchange.h"
#include "ServerInterface.h" // NEW_MEDIA_SERVER_SIGNATURE

#define TIMEOUT 2000000

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
		
		thread_info info; 
		get_thread_info(find_thread(NULL), &info); 
		team = info.team; 
	}
	~initit()
	{
		delete MediaServerMessenger;
	}
};
initit _initit;


status_t
request_data::SendReply(status_t result, reply_data *reply, int replysize) const
{
	reply->result = result;
	// we cheat and use the (command_data *) version of SendToPort
	return SendToPort(reply_port, 0, reinterpret_cast<command_data *>(reply), replysize);
}


// BMessage based data exchange with the media_server
status_t SendToServer(BMessage *msg)
{
	status_t rv;
	rv = MediaServerMessenger->SendMessage(msg, static_cast<BHandler *>(NULL), TIMEOUT);
	if (rv != B_OK)
		FATAL("SendToServer: SendMessage failed\n");
	return rv;
}


status_t QueryServer(BMessage *request, BMessage *reply)
{
	status_t rv;
	rv = MediaServerMessenger->SendMessage(request, reply, TIMEOUT, TIMEOUT);
	if (rv != B_OK)
		FATAL("QueryServer: SendMessage failed\n");
	return rv;
}


// Raw data based data exchange with the media_server
status_t SendToServer(int32 msgcode, command_data *msg, int size)
{
	return SendToPort(MediaServerPort, msgcode, msg, size);
}

status_t QueryServer(int32 msgcode, request_data *request, int requestsize, reply_data *reply, int replysize)
{
	return QueryPort(MediaServerPort, msgcode, request, requestsize, reply, replysize);
}


// Raw data based data exchange with the media_addon_server
status_t SendToAddonServer(int32 msgcode, command_data *msg, int size)
{
	return SendToPort(MediaAddonServerPort, msgcode, msg, size);
}


status_t QueryAddonServer(int32 msgcode, request_data *request, int requestsize, reply_data *reply, int replysize)
{
	return QueryPort(MediaAddonServerPort, msgcode, request, requestsize, reply, replysize);
}


// Raw data based data exchange with the media_server
status_t SendToPort(port_id sendport, int32 msgcode, command_data *msg, int size)
{
	status_t rv;
	rv = write_port_etc(sendport, msgcode, msg, size, B_RELATIVE_TIMEOUT, TIMEOUT);
	if (rv != B_OK)
		FATAL("SendToPort: write_port failed, port %ld, error %#lx (%s)\n", sendport, rv, strerror(rv));
	return B_OK;
}


status_t QueryPort(port_id requestport, int32 msgcode, request_data *request, int requestsize, reply_data *reply, int replysize)
{
	status_t rv;
	int32 code;

	request->reply_port = _PortPool->GetPort();

	rv = write_port_etc(requestport, msgcode, request, requestsize, B_RELATIVE_TIMEOUT, TIMEOUT);
	if (rv != B_OK) {
		FATAL("QueryPort: write_port failed, port %ld, error %#lx (%s)\n", requestport, rv, strerror(rv));
		_PortPool->PutPort(request->reply_port);
		return rv;
	}

	rv = read_port_etc(request->reply_port, &code, reply, replysize, B_RELATIVE_TIMEOUT, TIMEOUT);
	_PortPool->PutPort(request->reply_port);

	if (rv < B_OK)
		FATAL("QueryPort: read_port failed, port %ld, error %#lx (%s)\n", request->reply_port, rv, strerror(rv));
	
	return (rv < B_OK) ? rv : reply->result;
}

}; // dataexchange
}; // media
}; // BPrivate

