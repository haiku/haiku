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
NotificationManager::Register(const BMessenger &notifyHandler, const media_node &node, int32 notificationType)
{
	CALLED();
	

	BMessage msg(MEDIA_SERVER_REQUEST_NOTIFICATIONS);
	return SendMessageToMediaServer(&msg);
}


status_t
NotificationManager::Unregister(const BMessenger &notifyHandler, const media_node &node, int32 notificationType)
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
	msg.what = what;
	
	return SendMessageToMediaServer(&msg);
}


void
NotificationManager::NodesCreated(const media_node_id *ids, int32 count)
{
	CALLED();
	BMessage msg(B_MEDIA_NODE_CREATED);

	SendMessageToMediaServer(&msg);
}


void
NotificationManager::NodesDeleted(const media_node_id *ids, int32 count)
{
	CALLED();
	BMessage msg(B_MEDIA_NODE_DELETED);

	SendMessageToMediaServer(&msg);
}


void
NotificationManager::ConnectionMade(const media_input &input, const media_output &output, const media_format &format)
{
	CALLED();
	BMessage msg(B_MEDIA_CONNECTION_MADE);

	SendMessageToMediaServer(&msg);
}


void
NotificationManager::ConnectionBroken(const media_source &source, const media_destination &destination)
{
	CALLED();
	BMessage msg(B_MEDIA_CONNECTION_BROKEN);

	SendMessageToMediaServer(&msg);
}


void
NotificationManager::BuffersCreated() // XXX fix
{
	CALLED();
	BMessage msg(B_MEDIA_BUFFER_CREATED);

	SendMessageToMediaServer(&msg);
}


void
NotificationManager::BuffersDeleted(const media_buffer_id *ids, int32 count)
{
	CALLED();
	BMessage msg(B_MEDIA_BUFFER_DELETED);

	SendMessageToMediaServer(&msg);
}


void
NotificationManager::FormatChanged(const media_source &source, const media_destination &destination, const media_format &format)
{
	CALLED();
	BMessage msg(B_MEDIA_FORMAT_CHANGED);

	SendMessageToMediaServer(&msg);
}


status_t
NotificationManager::ParameterChanged(const media_node &node, int32 parameterid)
{
	CALLED();
	BMessage msg(B_MEDIA_PARAMETER_CHANGED);

	return B_OK;
}


void
NotificationManager::WebChanged(const media_node &node) // XXX fix
{
	CALLED();
	BMessage msg(B_MEDIA_WEB_CHANGED);

}


status_t
NotificationManager::NewParameterValue(const media_node &node, int32 parameterid, bigtime_t when, const void *param, size_t paramsize)
{
	CALLED();
	BMessage msg(B_MEDIA_NEW_PARAMETER_VALUE);

	return SendMessageToMediaServer(&msg);
}


void
NotificationManager::FlavorsChanged(media_addon_id addonis, int32 newcount, int32 gonecount)
{
	CALLED();
	BMessage msg(B_MEDIA_FLAVORS_CHANGED);

	SendMessageToMediaServer(&msg);
}

void
NotificationManager::NodeStopped(const media_node &node, bigtime_t when) // XXX fix
{
	CALLED();
	BMessage msg(B_MEDIA_NODE_STOPPED);

	SendMessageToMediaServer(&msg);
}

status_t
NotificationManager::SendMessageToMediaServer(BMessage *msg)
{
	#define TIMEOUT	1000000
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

}; // namespace media
}; // namespace BPrivate
