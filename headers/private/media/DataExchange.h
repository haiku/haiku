/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _DATA_EXCHANGE_H
#define _DATA_EXCHANGE_H

#include <MediaDefs.h>
#include <MediaNode.h>
#include <MediaAddOn.h>
#include <Entry.h>

namespace BPrivate {
namespace media {
namespace dataexchange {

struct reply_data;
struct request_data;

// BMessage based data exchange with the media_server
status_t SendToServer(BMessage *msg);
status_t QueryServer(BMessage *request, BMessage *reply);

// BMessage based data exchange with the media_addon_server
status_t SendToAddonServer(BMessage *msg);

// Raw data based data exchange with the media_server
status_t SendToServer(int32 msgcode, void *msg, int size);
status_t QueryServer(int32 msgcode, const request_data *request, int requestsize, reply_data *reply, int replysize);

// Raw data based data exchange with the media_addon_server
status_t SendToAddonServer(int32 msgcode, void *msg, int size);
status_t QueryAddonServer(int32 msgcode, const request_data *request, int requestsize, reply_data *reply, int replysize);

// Raw data based data exchange with the media_server
status_t SendToPort(port_id sendport, int32 msgcode, void *msg, int size);
status_t QueryPort(port_id requestport, int32 msgcode, const request_data *request, int requestsize, reply_data *reply, int replysize);

// The base struct used for all raw requests
struct request_data
{
	port_id reply_port;

	void SendReply(reply_data *reply, int replysize) const;
};

// The base struct used for all raw replys
struct reply_data
{
	status_t result;
};


}; // dataexchange
}; // media
}; // BPrivate

using namespace BPrivate::media::dataexchange;

enum {

	// BMediaRoster notification service
	MEDIA_SERVER_REQUEST_NOTIFICATIONS = 1000,
	MEDIA_SERVER_CANCEL_NOTIFICATIONS,
	MEDIA_SERVER_SEND_NOTIFICATIONS

};

struct request_addonserver_instantiate_dormant_node : public request_data
{
	dormant_node_info info;
};

struct reply_addonserver_instantiate_dormant_node : public reply_data
{
	media_node node;
};


#endif // _DATA_EXCHANGE_H
