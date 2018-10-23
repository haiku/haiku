/*
 * Copyright (c) 2002, 2003 Marcus Overhagen <Marcus@Overhagen.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files or portions
 * thereof (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice
 *    in the  binary, as well as this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided with
 *    the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


/*!	This is a interface class for media kit notifications.
	It is private to the media kit which uses it to pass notifications up to
	the media_server which will broadcast them.
*/

// TODO: The BeBook, MediaDefs.h and the BeOS R5 do list
// different strings for the message data fields of
// the notification messages.


#include "Notifications.h"

#include <Messenger.h>
#include <MediaNode.h>

#include <AppMisc.h>

#include "MediaDebug.h"
#include "DataExchange.h"


namespace BPrivate {
namespace media {
namespace notifications {


status_t
Register(const BMessenger& notifyHandler, const media_node& node,
	int32 notification)
{
	CALLED();

	if (notification == B_MEDIA_SERVER_STARTED
		|| notification == B_MEDIA_SERVER_QUIT) {
		BMessage msg(MEDIA_ROSTER_REQUEST_NOTIFICATIONS);
		msg.AddInt32(NOTIFICATION_PARAM_WHAT, notification);
		msg.AddMessenger(NOTIFICATION_PARAM_MESSENGER, notifyHandler);
		return BPrivate::media::dataexchange::SendToRoster(&msg);
	}

	BMessage msg(MEDIA_SERVER_REQUEST_NOTIFICATIONS);
	msg.AddInt32(NOTIFICATION_PARAM_WHAT, notification);
	msg.AddInt32(NOTIFICATION_PARAM_TEAM, BPrivate::current_team());
	msg.AddMessenger(NOTIFICATION_PARAM_MESSENGER, notifyHandler);
	msg.AddData("node", B_RAW_TYPE, &node, sizeof(node));

	return BPrivate::media::dataexchange::SendToServer(&msg);
}


status_t
Unregister(const BMessenger& notifyHandler, const media_node& node,
	int32 notification)
{
	CALLED();

	if (notification == B_MEDIA_SERVER_STARTED
		|| notification == B_MEDIA_SERVER_QUIT) {
		BMessage msg(MEDIA_ROSTER_CANCEL_NOTIFICATIONS);
		msg.AddInt32(NOTIFICATION_PARAM_WHAT, notification);
		msg.AddMessenger(NOTIFICATION_PARAM_MESSENGER, notifyHandler);
		return BPrivate::media::dataexchange::SendToRoster(&msg);
	}

	BMessage msg(MEDIA_SERVER_CANCEL_NOTIFICATIONS);
	msg.AddInt32(NOTIFICATION_PARAM_WHAT, notification);
	msg.AddInt32(NOTIFICATION_PARAM_TEAM, BPrivate::current_team());
	msg.AddMessenger(NOTIFICATION_PARAM_MESSENGER, notifyHandler);
	msg.AddData("node", B_RAW_TYPE, &node, sizeof(node));

	return BPrivate::media::dataexchange::SendToServer(&msg);
}


/*!	Transmits the error code specified by \a what to anyone who's receiving
	notifications from this node. If \a info isn't \c NULL, it's used as a
	model message for the error notification message.
	The message field "be:node_id" will contain the node ID.
*/
status_t
ReportError(const media_node& node, BMediaNode::node_error what,
	const BMessage* info)
{
	CALLED();
	BMessage msg;
	if (info != NULL)
		msg = *info;

	msg.what = MEDIA_SERVER_SEND_NOTIFICATIONS;
	msg.AddInt32(NOTIFICATION_PARAM_WHAT, what);
	msg.AddInt32("be:node_id", node.node);
	msg.AddData("node", B_RAW_TYPE, &node, sizeof(node));

	return BPrivate::media::dataexchange::SendToServer(&msg);
}


void
NodesCreated(const media_node_id* ids, int32 count)
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
NodesDeleted(const media_node_id* ids, int32 count)
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
ConnectionMade(const media_input& input, const media_output& output,
	const media_format& format)
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
ConnectionBroken(const media_source& source,
	const media_destination& destination)
{
	CALLED();
	BMessage msg(MEDIA_SERVER_SEND_NOTIFICATIONS);
	msg.AddInt32(NOTIFICATION_PARAM_WHAT, B_MEDIA_CONNECTION_BROKEN);
	msg.AddData("source", B_RAW_TYPE, &source, sizeof(source));
	msg.AddData("destination", B_RAW_TYPE, &destination, sizeof(destination));

	BPrivate::media::dataexchange::SendToServer(&msg);
}


void
BuffersCreated(area_info* areas, int32 count)
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
BuffersDeleted(const media_buffer_id* ids, int32 count)
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
FormatChanged(const media_source& source, const media_destination& destination,
	const media_format& format)
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
ParameterChanged(const media_node& node, int32 parameterID)
{
	CALLED();
	BMessage msg(MEDIA_SERVER_SEND_NOTIFICATIONS);
	msg.AddInt32(NOTIFICATION_PARAM_WHAT, B_MEDIA_PARAMETER_CHANGED);
	msg.AddData("node", B_RAW_TYPE, &node, sizeof(node));
	msg.AddInt32("parameter", parameterID);

	return BPrivate::media::dataexchange::SendToServer(&msg);
}


void
WebChanged(const media_node& node)
{
	CALLED();
	BMessage msg(MEDIA_SERVER_SEND_NOTIFICATIONS);
	msg.AddInt32(NOTIFICATION_PARAM_WHAT, B_MEDIA_WEB_CHANGED);
	msg.AddData("node", B_RAW_TYPE, &node, sizeof(node));

	BPrivate::media::dataexchange::SendToServer(&msg);
}


status_t
NewParameterValue(const media_node& node, int32 parameterID, bigtime_t when,
	const void* param, size_t paramsize)
{
	CALLED();
	BMessage msg(MEDIA_SERVER_SEND_NOTIFICATIONS);
	msg.AddInt32(NOTIFICATION_PARAM_WHAT, B_MEDIA_NEW_PARAMETER_VALUE);
	msg.AddData("node", B_RAW_TYPE, &node, sizeof(node));
	msg.AddInt32("parameter", parameterID);
	msg.AddInt64("when", when);
	msg.AddData("value", B_RAW_TYPE, param, paramsize);

	return BPrivate::media::dataexchange::SendToServer(&msg);
}


void
FlavorsChanged(media_addon_id addOnID, int32 newCount, int32 goneCount)
{
	CALLED();
	BMessage msg(MEDIA_SERVER_SEND_NOTIFICATIONS);
	msg.AddInt32(NOTIFICATION_PARAM_WHAT, B_MEDIA_FLAVORS_CHANGED);
	msg.AddInt32("be:addon_id", addOnID);
	msg.AddInt32("be:new_count", newCount);
	msg.AddInt32("be:gone_count", goneCount);

	BPrivate::media::dataexchange::SendToServer(&msg);
}


void
NodeStopped(const media_node& node, bigtime_t when)
{
	CALLED();
	BMessage msg(MEDIA_SERVER_SEND_NOTIFICATIONS);
	msg.AddInt32(NOTIFICATION_PARAM_WHAT, B_MEDIA_NODE_STOPPED);
	msg.AddData("node", B_RAW_TYPE, &node, sizeof(node));
	msg.AddInt64("when", when);

	BPrivate::media::dataexchange::SendToServer(&msg);
}


// TODO: missing: B_MEDIA_TRANSPORT_STATE: "state", "location", "realtime"
// TODO: missing: B_MEDIA_DEFAULT_CHANGED: "default", "node"


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

		// invalid if we watch for a specific node
		case B_MEDIA_SERVER_STARTED:
		case B_MEDIA_SERVER_QUIT:
			return !node_specific;

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

}	// namespace notifications
}	// namespace media
}	// namespace BPrivate
