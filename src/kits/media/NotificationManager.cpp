/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
/* This is a interface class for media kit notifications.
 * It is private to the media kit which uses it to pass
 * notifications up to the media_server which will broadcast
 * them.
 */
#include <Messenger.h>
#include <MediaNode.h>
#include "debug.h"
#include "PortPool.h"
#include "ServerInterface.h"
#include "NotificationManager.h"

static BPrivate::media::NotificationManager manager;
BPrivate::media::NotificationManager *_NotificationManager = &manager;

namespace BPrivate {
namespace media {

NotificationManager::NotificationManager()
 :	fMessenger(new BMessenger())
{
	CALLED();
}


NotificationManager::~NotificationManager()
{
	CALLED();
	delete fMessenger;
}


status_t
NotificationManager::Register(const BMessenger &notifyHandler, const media_node &node, notification_type_mask mask)
{
	CALLED();
	BMessage msg(MEDIA_SERVER_REQUEST_NOTIFICATIONS);

	return SendMessageToMediaServer(&msg);
}


status_t
NotificationManager::Unregister(const BMessenger &notifyHandler, const media_node &node, notification_type_mask mask)
{
	CALLED();
	BMessage msg(MEDIA_SERVER_CANCEL_NOTIFICATIONS);

	return SendMessageToMediaServer(&msg);
}


status_t
NotificationManager::ReportError(const media_node &node, BMediaNode::node_error what, const BMessage * info)
{
	/* Transmits the error code specified by whichError to anyone that's receiving notifications from
	 * this node. If info isn't NULL, it's used as a model message for the error notification message.
	 * The message field "be:node_id" will contain the node ID.
	 */
	CALLED();
	// sanity check the what value
	switch (what) {
		case BMediaNode::B_NODE_FAILED_START:
		case BMediaNode::B_NODE_FAILED_STOP:
		case BMediaNode::B_NODE_FAILED_SEEK:
		case BMediaNode::B_NODE_FAILED_SET_RUN_MODE:
		case BMediaNode::B_NODE_FAILED_TIME_WARP:
		case BMediaNode::B_NODE_FAILED_PREROLL:
		case BMediaNode::B_NODE_FAILED_SET_TIME_SOURCE_FOR:
		case BMediaNode::B_NODE_IN_DISTRESS:
			break;
		default:
			TRACE("BMediaNode::ReportError (NotificationManager::ReportError): invalid what!\n");
			return B_BAD_VALUE;
	}
	BMessage msg;
	if (info)
		msg = *info;
	msg.what = MEDIA_SERVER_SEND_NOTIFICATIONS;
	msg.AddInt32(NOTIFICATION_PARAM_WHAT, what);
	msg.AddInt32(NOTIFICATION_PARAM_MASK, notification_error);
	return SendMessageToMediaServer(&msg);
}


void
NotificationManager::NodesCreated(const media_node_id *ids, int32 count)
{
	CALLED();
	BMessage msg(MEDIA_SERVER_SEND_NOTIFICATIONS);
	msg.AddInt32(NOTIFICATION_PARAM_WHAT, B_MEDIA_NODE_CREATED);
	msg.AddInt32(NOTIFICATION_PARAM_MASK, NotificationType2Mask(B_MEDIA_NODE_CREATED));

	SendMessageToMediaServer(&msg);
}


void
NotificationManager::NodesDeleted(const media_node_id *ids, int32 count)
{
	CALLED();
	BMessage msg(MEDIA_SERVER_SEND_NOTIFICATIONS);
	msg.AddInt32(NOTIFICATION_PARAM_WHAT, B_MEDIA_NODE_DELETED);
	msg.AddInt32(NOTIFICATION_PARAM_MASK, NotificationType2Mask(B_MEDIA_NODE_DELETED));

	SendMessageToMediaServer(&msg);
}


void
NotificationManager::ConnectionMade(const media_input &input, const media_output &output, const media_format &format)
{
	CALLED();
	BMessage msg(MEDIA_SERVER_SEND_NOTIFICATIONS);
	msg.AddInt32(NOTIFICATION_PARAM_WHAT, B_MEDIA_CONNECTION_MADE);
	msg.AddInt32(NOTIFICATION_PARAM_MASK, NotificationType2Mask(B_MEDIA_CONNECTION_MADE));

	SendMessageToMediaServer(&msg);
}


void
NotificationManager::ConnectionBroken(const media_source &source, const media_destination &destination)
{
	CALLED();
	BMessage msg(MEDIA_SERVER_SEND_NOTIFICATIONS);
	msg.AddInt32(NOTIFICATION_PARAM_WHAT, B_MEDIA_CONNECTION_BROKEN);
	msg.AddInt32(NOTIFICATION_PARAM_MASK, NotificationType2Mask(B_MEDIA_CONNECTION_BROKEN));

	SendMessageToMediaServer(&msg);
}


void
NotificationManager::BuffersCreated() // XXX fix
{
	CALLED();
	BMessage msg(MEDIA_SERVER_SEND_NOTIFICATIONS);
	msg.AddInt32(NOTIFICATION_PARAM_WHAT, B_MEDIA_BUFFER_CREATED);
	msg.AddInt32(NOTIFICATION_PARAM_MASK, NotificationType2Mask(B_MEDIA_BUFFER_CREATED));

	SendMessageToMediaServer(&msg);
}


void
NotificationManager::BuffersDeleted(const media_buffer_id *ids, int32 count)
{
	CALLED();
	BMessage msg(MEDIA_SERVER_SEND_NOTIFICATIONS);
	msg.AddInt32(NOTIFICATION_PARAM_WHAT, B_MEDIA_BUFFER_DELETED);
	msg.AddInt32(NOTIFICATION_PARAM_MASK, NotificationType2Mask(B_MEDIA_BUFFER_DELETED));

	SendMessageToMediaServer(&msg);
}


void
NotificationManager::FormatChanged(const media_source &source, const media_destination &destination, const media_format &format)
{
	CALLED();
	BMessage msg(MEDIA_SERVER_SEND_NOTIFICATIONS);
	msg.AddInt32(NOTIFICATION_PARAM_WHAT, B_MEDIA_FORMAT_CHANGED);
	msg.AddInt32(NOTIFICATION_PARAM_MASK, NotificationType2Mask(B_MEDIA_FORMAT_CHANGED));

	SendMessageToMediaServer(&msg);
}


status_t
NotificationManager::ParameterChanged(const media_node &node, int32 parameterid)
{
	CALLED();
	BMessage msg(MEDIA_SERVER_SEND_NOTIFICATIONS);
	msg.AddInt32(NOTIFICATION_PARAM_WHAT, B_MEDIA_PARAMETER_CHANGED);
	msg.AddInt32(NOTIFICATION_PARAM_MASK, NotificationType2Mask(B_MEDIA_PARAMETER_CHANGED));

	return SendMessageToMediaServer(&msg);
}


void
NotificationManager::WebChanged(const media_node &node) // XXX fix
{
	CALLED();
	BMessage msg(MEDIA_SERVER_SEND_NOTIFICATIONS);
	msg.AddInt32(NOTIFICATION_PARAM_WHAT, B_MEDIA_WEB_CHANGED);
	msg.AddInt32(NOTIFICATION_PARAM_MASK, NotificationType2Mask(B_MEDIA_WEB_CHANGED));

	SendMessageToMediaServer(&msg);
}


status_t
NotificationManager::NewParameterValue(const media_node &node, int32 parameterid, bigtime_t when, const void *param, size_t paramsize)
{
	CALLED();
	BMessage msg(MEDIA_SERVER_SEND_NOTIFICATIONS);
	msg.AddInt32(NOTIFICATION_PARAM_WHAT, B_MEDIA_NEW_PARAMETER_VALUE);
	msg.AddInt32(NOTIFICATION_PARAM_MASK, NotificationType2Mask(B_MEDIA_NEW_PARAMETER_VALUE));

	return SendMessageToMediaServer(&msg);
}


void
NotificationManager::FlavorsChanged(media_addon_id addonis, int32 newcount, int32 gonecount)
{
	CALLED();
	BMessage msg(MEDIA_SERVER_SEND_NOTIFICATIONS);
	msg.AddInt32(NOTIFICATION_PARAM_WHAT, B_MEDIA_FLAVORS_CHANGED);
	msg.AddInt32(NOTIFICATION_PARAM_MASK, NotificationType2Mask(B_MEDIA_FLAVORS_CHANGED));

	SendMessageToMediaServer(&msg);
}

void
NotificationManager::NodeStopped(const media_node &node, bigtime_t when) // XXX fix
{
	CALLED();
	BMessage msg(MEDIA_SERVER_SEND_NOTIFICATIONS);
	msg.AddInt32(NOTIFICATION_PARAM_WHAT, B_MEDIA_NODE_STOPPED);
	msg.AddInt32(NOTIFICATION_PARAM_MASK, NotificationType2Mask(B_MEDIA_NODE_STOPPED));

	SendMessageToMediaServer(&msg);
}

status_t
NotificationManager::SendMessageToMediaServer(BMessage *msg)
{
	#define TIMEOUT	100000
	BMessage reply;
	status_t rv;
	if (!fMessenger->IsValid()) {
		TRACE("NotificationManager::SendMessageToMediaServer: setting new messenger target\n");
		*fMessenger = BMessenger(NEW_MEDIA_SERVER_SIGNATURE);
	}
	rv = fMessenger->SendMessage(msg, &reply, TIMEOUT);
	if (rv != B_OK) {
		TRACE("NotificationManager::SendMessageToMediaServer: failed to send message\n");
	} else {
		rv = reply.what;
	}
	return rv;
}

bool
NotificationManager::IsValidNotificationType(int32 notificationType)
{
	switch (notificationType) {
		case BMediaNode::B_NODE_FAILED_START:
		case BMediaNode::B_NODE_FAILED_STOP:
		case BMediaNode::B_NODE_FAILED_SEEK:
		case BMediaNode::B_NODE_FAILED_SET_RUN_MODE:
		case BMediaNode::B_NODE_FAILED_TIME_WARP:
		case BMediaNode::B_NODE_FAILED_PREROLL:
		case BMediaNode::B_NODE_FAILED_SET_TIME_SOURCE_FOR:
		case BMediaNode::B_NODE_IN_DISTRESS:
			debugger("NotificationManager::IsValidNotificationType, encountered BMediaNode Error otification\n");
			return false; // XXX we consider registering for this notifications as impossible, you always get them anyway
		case B_MEDIA_WILDCARD:
		case B_MEDIA_NODE_CREATED:
		case B_MEDIA_NODE_DELETED:
		case B_MEDIA_CONNECTION_MADE:
		case B_MEDIA_CONNECTION_BROKEN:
		case B_MEDIA_BUFFER_CREATED:
		case B_MEDIA_BUFFER_DELETED:
		case B_MEDIA_TRANSPORT_STATE:
		case B_MEDIA_PARAMETER_CHANGED:
		case B_MEDIA_FORMAT_CHANGED:
		case B_MEDIA_WEB_CHANGED:
		case B_MEDIA_DEFAULT_CHANGED:
		case B_MEDIA_NEW_PARAMETER_VALUE:
		case B_MEDIA_NODE_STOPPED:
		case B_MEDIA_FLAVORS_CHANGED:
			return true;
		default:
			return false;
	}
}


NotificationManager::notification_type_mask
NotificationManager::NotificationType2Mask(int32 notificationType)
{
	switch (notificationType) {
		case BMediaNode::B_NODE_FAILED_START:
		case BMediaNode::B_NODE_FAILED_STOP:
		case BMediaNode::B_NODE_FAILED_SEEK:
		case BMediaNode::B_NODE_FAILED_SET_RUN_MODE:
		case BMediaNode::B_NODE_FAILED_TIME_WARP:
		case BMediaNode::B_NODE_FAILED_PREROLL:
		case BMediaNode::B_NODE_FAILED_SET_TIME_SOURCE_FOR:
		case BMediaNode::B_NODE_IN_DISTRESS:
			return notification_error;
		case B_MEDIA_WILDCARD:
			return notification_wildcard;
		case B_MEDIA_NODE_CREATED:
			return notification_node_created;
		case B_MEDIA_NODE_DELETED:
			return notification_node_deleted;
		case B_MEDIA_CONNECTION_MADE:
			return notification_connection_made;
		case B_MEDIA_CONNECTION_BROKEN:
			return notification_connection_broken;
		case B_MEDIA_BUFFER_CREATED:
			return notification_buffer_created;
		case B_MEDIA_BUFFER_DELETED:
			return notification_buffer_deleted;
		case B_MEDIA_TRANSPORT_STATE:
			return notification_transport_state;
		case B_MEDIA_PARAMETER_CHANGED:
			return notification_parameter_changed;
		case B_MEDIA_FORMAT_CHANGED:
			return notification_format_change;
		case B_MEDIA_WEB_CHANGED:
			return notification_web_changed;
		case B_MEDIA_DEFAULT_CHANGED:
			return notification_default_changed;
		case B_MEDIA_NEW_PARAMETER_VALUE:
			return notification_new_parameter_value;
		case B_MEDIA_NODE_STOPPED:
			return notification_node_stopped;
		case B_MEDIA_FLAVORS_CHANGED:
			return notification_flavors_changed;
		default:
			return notification_no_mask;
	}
}

}; // namespace media
}; // namespace BPrivate
