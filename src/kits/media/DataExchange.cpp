/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <OS.h>
#include <Messenger.h>
#include <string.h>
#include <unistd.h>
#include "debug.h"
#include "PortPool.h"
#include "MediaMisc.h"
#include "DataExchange.h"
#include "ServerInterface.h" // NEW_MEDIA_SERVER_SIGNATURE

#define TIMEOUT 15000000 // 15 seconds timeout!

namespace BPrivate {
namespace media {

team_id team;

namespace dataexchange {

BMessenger *MediaServerMessenger;
static port_id MediaServerPort;
static port_id MediaAddonServerPort;

void find_media_server_port();
void find_media_addon_server_port();

class initit
{
public:
	initit()
	{
		MediaServerMessenger = new BMessenger(NEW_MEDIA_SERVER_SIGNATURE);
		find_media_server_port();
		find_media_addon_server_port();

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


void find_media_server_port()
{
	MediaServerPort = find_port(MEDIA_SERVER_PORT_NAME);
	if (MediaServerPort < 0) {
		ERROR("couldn't find MediaServerPort\n");
		MediaServerPort = BAD_MEDIA_SERVER_PORT; // make this a unique number
	}
}

void find_media_addon_server_port()
{
	MediaAddonServerPort = find_port(MEDIA_ADDON_SERVER_PORT_NAME);
	if (MediaAddonServerPort < 0) {
		ERROR("couldn't find MediaAddonServerPort\n");
		MediaAddonServerPort = BAD_MEDIA_ADDON_SERVER_PORT; // make this a unique number
	}
}


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
	if (rv != B_OK) {
		ERROR("SendToServer: SendMessage failed, error 0x%08lx (%s)\n", rv, strerror(rv));
		DEBUG_ONLY(msg->PrintToStream());
	}
	return rv;
}


status_t 
QueryServer(BMessage &request, BMessage &reply)
{
	status_t status = MediaServerMessenger->SendMessage(&request, &reply, TIMEOUT, TIMEOUT);
	if (status != B_OK) {
		ERROR("QueryServer: SendMessage failed, error 0x%08lx (%s)\n", status, strerror(status));
		DEBUG_ONLY(request.PrintToStream());
		DEBUG_ONLY(reply.PrintToStream);
	}
	return status;
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
	if (rv != B_OK) {
		ERROR("SendToPort: write_port failed, msgcode 0x%lx, port %ld, error %#lx (%s)\n", msgcode, sendport, rv, strerror(rv));
		if (rv == B_BAD_PORT_ID && sendport == MediaServerPort) {
			find_media_server_port();
			sendport = MediaServerPort;
		} else if (rv == B_BAD_PORT_ID && sendport == MediaAddonServerPort) {
			find_media_addon_server_port();
			sendport = MediaAddonServerPort;
		} else {
			return rv;
		}
		rv = write_port_etc(sendport, msgcode, msg, size, B_RELATIVE_TIMEOUT, TIMEOUT);
		if (rv != B_OK) {
			ERROR("SendToPort: retrying write_port failed, msgcode 0x%lx, port %ld, error %#lx (%s)\n", msgcode, sendport, rv, strerror(rv));
			return rv;
		}
	}
	return B_OK;
}


status_t QueryPort(port_id requestport, int32 msgcode, request_data *request, int requestsize, reply_data *reply, int replysize)
{
	status_t rv;
	int32 code;

	request->reply_port = _PortPool->GetPort();

	rv = write_port_etc(requestport, msgcode, request, requestsize, B_RELATIVE_TIMEOUT, TIMEOUT);
	
	if (rv != B_OK) {
		ERROR("QueryPort: write_port failed, msgcode 0x%lx, port %ld, error %#lx (%s)\n", msgcode, requestport, rv, strerror(rv));
		if (rv == B_BAD_PORT_ID && requestport == MediaServerPort) {
			find_media_server_port();
			requestport = MediaServerPort;
		} else if (rv == B_BAD_PORT_ID && requestport == MediaAddonServerPort) {
			find_media_addon_server_port();
			requestport = MediaAddonServerPort;
		} else {
			_PortPool->PutPort(request->reply_port);
			return rv;
		}
		rv = write_port_etc(requestport, msgcode, request, requestsize, B_RELATIVE_TIMEOUT, TIMEOUT);
		if (rv != B_OK) {
			ERROR("QueryPort: retrying write_port failed, msgcode 0x%lx, port %ld, error %#lx (%s)\n", msgcode, requestport, rv, strerror(rv));
			_PortPool->PutPort(request->reply_port);
			return rv;
		}
	}

	rv = read_port_etc(request->reply_port, &code, reply, replysize, B_RELATIVE_TIMEOUT, TIMEOUT);
	_PortPool->PutPort(request->reply_port);

	if (rv < B_OK) {
		ERROR("QueryPort: read_port failed, msgcode 0x%lx, port %ld, error %#lx (%s)\n", msgcode, request->reply_port, rv, strerror(rv));
	}
	
	return (rv < B_OK) ? rv : reply->result;
}

}; // dataexchange
}; // media
}; // BPrivate

