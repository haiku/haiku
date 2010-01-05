/*
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _DATA_EXCHANGE_H
#define _DATA_EXCHANGE_H


#include <ServerInterface.h>


namespace BPrivate {
namespace media {
namespace dataexchange {


void InitDataExchange();

// BMessage based data exchange with the media_server
status_t SendToServer(BMessage* msg);
status_t QueryServer(BMessage& request, BMessage& reply);

// Raw data based data exchange with the media_server
status_t SendToServer(int32 msgCode, command_data* msg, size_t size);
status_t QueryServer(int32 msgCode, request_data* request, size_t requestSize,
	reply_data* reply, size_t replySize);

// Raw data based data exchange with the media_addon_server
status_t SendToAddOnServer(int32 msgCode, command_data *msg, size_t size);
status_t QueryAddOnServer(int32 msgCode, request_data* request,
	size_t requestSize, reply_data* reply, size_t replySize);

// Raw data based data exchange with any (media node control-) port
status_t SendToPort(port_id sendPort, int32 msgCode, command_data* msg,
	size_t size);
status_t QueryPort(port_id requestPort, int32 msgCode, request_data* request,
	size_t requestSize, reply_data* reply, size_t replySize);


}	// namespace dataexchange
}	// namespace media
}	// namespace BPrivate


using namespace BPrivate::media::dataexchange;


#endif // _DATA_EXCHANGE_H
