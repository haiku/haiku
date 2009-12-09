/*
 * Copyright 2002-2007, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <DataExchange.h>

#include <string.h>
#include <unistd.h>

#include <Messenger.h>
#include <OS.h>

#include "debug.h"
#include "PortPool.h"
#include "MediaMisc.h"
#include "ServerInterface.h"


#define TIMEOUT 15000000 // 15 seconds timeout!


namespace BPrivate {
namespace media {
namespace dataexchange {


BMessenger* gMediaServerMessenger;
static port_id sMediaServerPort;
static port_id sMediaAddonServerPort;

static void find_media_server_port();
static void find_media_addon_server_port();

static BMessenger*
GetMediaServerMessenger()
{
	static BMessenger* messenger = new BMessenger(B_MEDIA_SERVER_SIGNATURE);
	return gMediaServerMessenger = messenger;
}

class initit {
public:
	initit()
	{
		gMediaServerMessenger = 0;
		find_media_server_port();
		find_media_addon_server_port();
	}

	~initit()
	{
		delete gMediaServerMessenger;
	}
};
initit _initit;


static void
find_media_server_port()
{
	sMediaServerPort = find_port(MEDIA_SERVER_PORT_NAME);
	if (sMediaServerPort < 0) {
		ERROR("couldn't find sMediaServerPort\n");
		sMediaServerPort = BAD_MEDIA_SERVER_PORT; // make this a unique number
	}
}


static void
find_media_addon_server_port()
{
	sMediaAddonServerPort = find_port(MEDIA_ADDON_SERVER_PORT_NAME);
	if (sMediaAddonServerPort < 0) {
		ERROR("couldn't find sMediaAddonServerPort\n");
		sMediaAddonServerPort = BAD_MEDIA_ADDON_SERVER_PORT; // make this a unique number
	}
}


status_t
request_data::SendReply(status_t result, reply_data *reply,
	size_t replySize) const
{
	reply->result = result;
	// we cheat and use the (command_data *) version of SendToPort
	return SendToPort(reply_port, 0, reinterpret_cast<command_data *>(reply),
		replySize);
}


//! BMessage based data exchange with the media_server
status_t
SendToServer(BMessage* msg)
{
	status_t status = GetMediaServerMessenger()->SendMessage(msg,
		static_cast<BHandler*>(NULL), TIMEOUT);
	if (status != B_OK) {
		ERROR("SendToServer: SendMessage failed: %s\n", strerror(status));
		DEBUG_ONLY(msg->PrintToStream());
	}
	return status;
}


status_t
QueryServer(BMessage& request, BMessage& reply)
{
	status_t status = GetMediaServerMessenger()->SendMessage(&request, &reply,
		TIMEOUT, TIMEOUT);
	if (status != B_OK) {
		ERROR("QueryServer: SendMessage failed: %s\n", strerror(status));
		DEBUG_ONLY(request.PrintToStream());
		DEBUG_ONLY(reply.PrintToStream());
	}
	return status;
}


//! Raw data based data exchange with the media_server
status_t
SendToServer(int32 msgCode, command_data* msg, size_t size)
{
	return SendToPort(sMediaServerPort, msgCode, msg, size);
}

status_t
QueryServer(int32 msgCode, request_data* request, size_t requestSize,
	reply_data* reply, size_t replySize)
{
	return QueryPort(sMediaServerPort, msgCode, request, requestSize, reply,
		replySize);
}


//! Raw data based data exchange with the media_addon_server
status_t
SendToAddOnServer(int32 msgCode, command_data* msg, size_t size)
{
	return SendToPort(sMediaAddonServerPort, msgCode, msg, size);
}


status_t
QueryAddOnServer(int32 msgCode, request_data* request, size_t requestSize,
	reply_data* reply, size_t replySize)
{
	return QueryPort(sMediaAddonServerPort, msgCode, request, requestSize,
		reply, replySize);
}


//! Raw data based data exchange with the media_server
status_t
SendToPort(port_id sendPort, int32 msgCode, command_data* msg, size_t size)
{
	status_t status = write_port_etc(sendPort, msgCode, msg, size,
		B_RELATIVE_TIMEOUT, TIMEOUT);
	if (status != B_OK) {
		ERROR("SendToPort: write_port failed, msgcode 0x%lx, port %ld: %s\n",
			msgCode, sendPort, strerror(status));
		if (status == B_BAD_PORT_ID && sendPort == sMediaServerPort) {
			find_media_server_port();
			sendPort = sMediaServerPort;
		} else if (status == B_BAD_PORT_ID
			&& sendPort == sMediaAddonServerPort) {
			find_media_addon_server_port();
			sendPort = sMediaAddonServerPort;
		} else
			return status;

		status = write_port_etc(sendPort, msgCode, msg, size,
			B_RELATIVE_TIMEOUT, TIMEOUT);
		if (status != B_OK) {
			ERROR("SendToPort: retrying write_port failed, msgCode 0x%lx, "
				"port %ld: %s\n", msgCode, sendPort, strerror(status));
			return status;
		}
	}
	return B_OK;
}


status_t
QueryPort(port_id requestPort, int32 msgCode, request_data* request,
	size_t requestSize, reply_data* reply, size_t replySize)
{

	request->reply_port = gPortPool->GetPort();

	status_t status = write_port_etc(requestPort, msgCode, request, requestSize,
		B_RELATIVE_TIMEOUT, TIMEOUT);
	if (status != B_OK) {
		ERROR("QueryPort: write_port failed, msgcode 0x%lx, port %ld: %s\n",
			msgCode, requestPort, strerror(status));

		if (status == B_BAD_PORT_ID && requestPort == sMediaServerPort) {
			find_media_server_port();
			requestPort = sMediaServerPort;
		} else if (status == B_BAD_PORT_ID
			&& requestPort == sMediaAddonServerPort) {
			find_media_addon_server_port();
			requestPort = sMediaAddonServerPort;
		} else {
			gPortPool->PutPort(request->reply_port);
			return status;
		}

		status = write_port_etc(requestPort, msgCode, request, requestSize,
			B_RELATIVE_TIMEOUT, TIMEOUT);
		if (status != B_OK) {
			ERROR("QueryPort: retrying write_port failed, msgcode 0x%lx, port "
				"%ld: %s\n", msgCode, requestPort, strerror(status));
			gPortPool->PutPort(request->reply_port);
			return status;
		}
	}

	int32 code;
	status = read_port_etc(request->reply_port, &code, reply, replySize,
		B_RELATIVE_TIMEOUT, TIMEOUT);

	gPortPool->PutPort(request->reply_port);

	if (status < B_OK) {
		ERROR("QueryPort: read_port failed, msgcode 0x%lx, port %ld: %s\n",
			msgCode, request->reply_port, strerror(status));
	}

	return status < B_OK ? status : reply->result;
}


}	// dataexchange
}	// media
}	// BPrivate
