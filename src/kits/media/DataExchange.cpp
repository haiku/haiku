/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <OS.h>
#include "DataExchange.h"

namespace BPrivate {
namespace media {

team_id team;

namespace dataexchange {

void
request_data::SendReply(reply_data *reply, int replysize) const
{
}


// BMessage based data exchange with the media_server
status_t SendToServer(BMessage *msg)
{
	return B_OK;
}


status_t QueryServer(BMessage *request, BMessage *reply)
{
	return B_OK;
}


// BMessage based data exchange with the media_addon_server
status_t SendToAddonServer(BMessage *msg)
{
	return B_OK;
}


// Raw data based data exchange with the media_server
status_t SendToServer(int32 msgcode, void *msg, int size)
{
	return B_OK;
}

status_t QueryServer(int32 msgcode, const request_data *request, int requestsize, reply_data *reply, int replysize)
{
	return B_OK;
}


// Raw data based data exchange with the media_addon_server
status_t SendToAddonServer(int32 msgcode, void *msg, int size)
{
	return B_OK;
}


status_t QueryAddonServer(int32 msgcode, const request_data *request, int requestsize, reply_data *reply, int replysize)
{
	return B_OK;
}


// Raw data based data exchange with the media_server
status_t SendToPort(port_id sendport, int32 msgcode, void *msg, int size)
{
	return B_OK;
}


status_t QueryPort(port_id requestport, int32 msgcode, const request_data *request, int requestsize, reply_data *reply, int replysize)
{
	return B_OK;
}

}; // dataexchange
}; // media
}; // BPrivate

