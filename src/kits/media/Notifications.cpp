/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
/* This is a interface class for media kit notifications.
 * It is private to the media kit which uses it to pass
 * notifications up to the media_server which will broadcast
 * them.
 */
/* XXX The BeBook, MediaDefs.h and the BeOS R5 do list
 * XXX different strings for the message data fields of
 * XXX the notification messages.
 */
#include <Messenger.h>
#include <MediaNode.h>
#include "debug.h"
#include "DataExchange.h"
#include "Notifications.h"

namespace BPrivate {
namespace media {

extern team_id team;

namespace notifications {

status_t
Register(const BMessenger &notifyHandler, const media_node &node, int32 notification)
{
	CALLED();
	BMessage msg(MEDIA_SERVER_REQUEST_NOTIFICATIONS);
	msg.AddInt32(NOTIFICATION_PARAM_WHAT, notification);
	msg.AddInt32(NOTIFICATION_PARAM_TEAM, team);
	msg.AddMessenger(NOTIFICATION_PARAM_MESSENGER, notifyHandler);
	msg.AddData("node", B_RAW_TYPE, &node, sizeof(node));
	return BPrivate::media::dataexchange::SendToServer(&msg);
}


status_t
Unregister(const BMessenger &notifyHandler, const media_node &node, int32 notification)
{
	CALLED();
	BMessage msg(MEDIA_SERVER_CANCEL_NOTIFICATIONS);
	msg.AddInt32(NOTIFICATION_PARAM_WHAT, notification);
	msg.AddInt32(NOTIFICATION_PARAM_TEAM, team);
	msg.AddMessenger(NOTIFICATION_PARAM_MESSENGER, notifyHandler);
	msg.AddData("node", B_RAW_TYPE, &node, sizeof(node));
	return BPrivate::media::dataexchange::SendToServer(&msg);
}


status_t
ReportError(const media_node &node, BMediaNode::node_error what, const BMessage * info)
{
	/* Transmits the error code specified by whichError to anyone that's receiving notifications from
	 * this node. If info isn't NULL, it's used as a model message for the error notification message.
	 * The message field "be:node_id" will contain the node ID.
	 */
	CALLED();
	BMessage msg;
	if (info)
		msg = *info;
	msg.what = MEDIA_SERVER_SEND_NOTIFICATIONS;
	msg.AddInt32(NOTIFICATION_PARAM_WHAT, what);
	msg.AddInt32("be:node_id", node.node);
	msg.AddData("node", B_RAW_TYPE, &node, sizeof(node));
	return BPrivate::media::dataexchange::SendToServer(&msg);
}


void
NodesCreated(const media_node_id *ids, int32 count)
{
	CALLED();
	BMessage msg(MEDIA_SERVER_SEND_NOTIFICATIONS);
	msg.AddInt32(NOTIFICATION_PARAM_WHAT, B_MEDIA_NODE_CREATED);
	for (int32 i = 0; i < count; i++) {
		msg.AddInt32("media_node_id", ids[i]);
	}
	BPrivate::media::dataexchange::SendToServer(&msg);
}


void
NodesDeleted(const media_node_id *ids, int32 count)
{
	CALLED();
	BMessage msg(MEDIA_SERVER_SEND_NOTIFICATIONS);
	msg.AddInt32(NOTIFICATION_PARAM_WHAT, B_MEDIA_NODE_DELETED);
	for (int32 i = 0; i < count; i++) {
		msg.AddInt32("media_node_id", ids[i]);
	}
	BPrivate::media::dataexchange::SendToServer(&msg);
}


void
ConnectionMade(const media_input &input, const media_output &output, const media_format &format)
{
	CALLED();
	BMessage msg(MEDIA_SERVER_SEND_NOTIFICATIONS);
	msg.AddInt32(NOTIFICATION_PARAM_WHAT, B_MEDIA_CONNECTION_MADE);
	msg.AddData("input", B_RAW_TYPE, &input, sizeof(input));
	msg.AddData("output", B_RAW_TYPE, &output, sizeof(output));
	msg.AddData("format", B_RAW_TYPE, &format, sizeof(format));
	BPrivate::media::dataexchange::SendToServer(&msg);
}


void
ConnectionBroken(const media_source &source, const media_destination &destination)
{
	CALLED();
	BMessage msg(MEDIA_SERVER_SEND_NOTIFICATIONS);
	msg.AddInt32(NOTIFICATION_PARAM_WHAT, B_MEDIA_CONNECTION_BROKEN);
	msg.AddData("source", B_RAW_TYPE, &source, sizeof(source));
	msg.AddData("destination", B_RAW_TYPE, &destination, sizeof(destination));
	BPrivate::media::dataexchange::SendToServer(&msg);
}


void
BuffersCreated(area_info *areas, int32 count)
{
	CALLED();
	BMessage msg(MEDIA_SERVER_SEND_NOTIFICATIONS);
	msg.AddInt32(NOTIFICATION_PARAM_WHAT, B_MEDIA_BUFFER_CREATED);
	for (int32 i = 0; i < count; i++) {
		msg.AddData("clone_info", B_RAW_TYPE, &areas[i], sizeof(area_info));
	}
	BPrivate::media::dataexchange::SendToServer(&msg);
}


void
BuffersDeleted(const media_buffer_id *ids, int32 count)
{
	CALLED();
	BMessage msg(MEDIA_SERVER_SEND_NOTIFICATIONS);
	msg.AddInt32(NOTIFICATION_PARAM_WHAT, B_MEDIA_BUFFER_DELETED);
	for (int32 i = 0; i < count; i++) {
		msg.AddInt32("media_buffer_id", ids[i]);
	}
	BPrivate::media::dataexchange::SendToServer(&msg);
}


void
FormatChanged(const media_source &source, const media_destination &destination, const media_format &format)
{
	CALLED();
	BMessage msg(MEDIA_SERVER_SEND_NOTIFICATIONS);
	msg.AddInt32(NOTIFICATION_PARAM_WHAT, B_MEDIA_FORMAT_CHANGED);
	msg.AddData("source", B_RAW_TYPE, &source, sizeof(source));
	msg.AddData("destination", B_RAW_TYPE, &destination, sizeof(destination));
	msg.AddData("format", B_RAW_TYPE, &format, sizeof(format));
	BPrivate::media::dataexchange::SendToServer(&msg);
}


status_t
ParameterChanged(const media_node &node, int32 parameterid)
{
	CALLED();
	BMessage msg(MEDIA_SERVER_SEND_NOTIFICATIONS);
	msg.AddInt32(NOTIFICATION_PARAM_WHAT, B_MEDIA_PARAMETER_CHANGED);
	msg.AddData("node", B_RAW_TYPE, &node, sizeof(node));
	msg.AddInt32("parameter", parameterid);
	return BPrivate::media::dataexchange::SendToServer(&msg);
}


void
WebChanged(const media_node &node)
{
	CALLED();
	BMessage msg(MEDIA_SERVER_SEND_NOTIFICATIONS);
	msg.AddInt32(NOTIFICATION_PARAM_WHAT, B_MEDIA_WEB_CHANGED);
	msg.AddData("node", B_RAW_TYPE, &node, sizeof(node));
	BPrivate::media::dataexchange::SendToServer(&msg);
}


status_t
NewParameterValue(const media_node &node, int32 parameterid, bigtime_t when, const void *param, size_t paramsize)
{
	CALLED();
	BMessage msg(MEDIA_SERVER_SEND_NOTIFICATIONS);
	msg.AddInt32(NOTIFICATION_PARAM_WHAT, B_MEDIA_NEW_PARAMETER_VALUE);
	msg.AddData("node", B_RAW_TYPE, &node, sizeof(node));
	msg.AddInt32("parameter", parameterid);
	msg.AddInt64("when", when);
	msg.AddData("value", B_RAW_TYPE, param, paramsize);
	return BPrivate::media::dataexchange::SendToServer(&msg);
}


void
FlavorsChanged(media_addon_id addonid, int32 newcount, int32 gonecount)
{
	CALLED();
	BMessage msg(MEDIA_SERVER_SEND_NOTIFICATIONS);
	msg.AddInt32(NOTIFICATION_PARAM_WHAT, B_MEDIA_FLAVORS_CHANGED);
	msg.AddInt32("be:addon_id", addonid);
	msg.AddInt32("be:new_count", newcount);
	msg.AddInt32("be:gone_count", gonecount);
	BPrivate::media::dataexchange::SendToServer(&msg);
}


void
NodeStopped(const media_node &node, bigtime_t when)
{
	CALLED();
	BMessage msg(MEDIA_SERVER_SEND_NOTIFICATIONS);
	msg.AddInt32(NOTIFICATION_PARAM_WHAT, B_MEDIA_NODE_STOPPED);
	msg.AddData("node", B_RAW_TYPE, &node, sizeof(node));
	msg.AddInt64("when", when);
	BPrivate::media::dataexchange::SendToServer(&msg);
}


// XXX missing: B_MEDIA_TRANSPORT_STATE		/* "state", "location", "realtime" */
// XXX missing: B_MEDIA_DEFAULT_CHANGED		/* "default", "node"  */


bool
IsValidNotificationRequest(bool node_specific, int32 notification)
{
	switch (notification) {
		// valid for normal and node specific watching
		case B_MEDIA_WILDCARD:
		case B_MEDIA_NODE_CREATED:
		case B_MEDIA_NODE_DELETED:
		case B_MEDIA_CONNECTION_MADE:
		case B_MEDIA_CONNECTION_BROKEN:
		case B_MEDIA_BUFFER_CREATED:
		case B_MEDIA_BUFFER_DELETED:
		case B_MEDIA_TRANSPORT_STATE:
		case B_MEDIA_DEFAULT_CHANGED:
		case B_MEDIA_FLAVORS_CHANGED:
			return true;
			
		// only valid for node specific watching
		case B_MEDIA_PARAMETER_CHANGED:
		case B_MEDIA_FORMAT_CHANGED:
		case B_MEDIA_WEB_CHANGED:
		case B_MEDIA_NEW_PARAMETER_VALUE:
		case B_MEDIA_NODE_STOPPED:
			return node_specific;
		
		// everything else is invalid
		default:
			return false;
	}
}

}; // namespace notifications
}; // namespace media
}; // namespace BPrivate
