/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _NOTIFICATION_MANAGER_H
#define _NOTIFICATION_MANAGER_H

namespace BPrivate {
namespace media {


class NotificationManager
{
public:
	enum notification_type_mask
	{
		notification_no_mask			= 0,
		notification_node_created 		= (1 <<  0),
		notification_node_deleted 		= (1 <<  1),
		notification_connection_made 	= (1 <<  2),
		notification_connection_broken	= (1 <<  3),
		notification_buffer_created		= (1 <<  4),
		notification_buffer_deleted		= (1 <<  5),
		notification_transport_state	= (1 <<  6),
		notification_parameter_changed	= (1 <<  7),
		notification_format_change		= (1 <<  8),
		notification_web_changed 		= (1 <<  9),
		notification_default_changed	= (1 << 10),
		notification_new_parameter_value= (1 << 11),
		notification_node_stopped		= (1 << 12),
		notification_flavors_changed	= (1 << 13),
		notification_error				= (1 << 31), // XXX, um well... always allow error notifications? Handled in the server
		notification_basic				= (notification_node_created | notification_node_deleted | notification_connection_made | notification_connection_broken),
		notification_wildcard			= (0xffffffff - notification_error) // exclude error, it is handled in the server
	};

public:
	NotificationManager();
	~NotificationManager();
		
	status_t Register(const BMessenger &notifyHandler, const media_node &node, notification_type_mask mask);
	status_t Unregister(const BMessenger &notifyHandler, const media_node &node, notification_type_mask mask);

	status_t ReportError(const media_node &node, BMediaNode::node_error what, const BMessage * info);
	
	void NodesCreated(const media_node_id *ids, int32 count);
	void NodesDeleted(const media_node_id *ids, int32 count);
	void ConnectionMade(const media_input &input, const media_output &output, const media_format &format);
	void ConnectionBroken(const media_source &source, const media_destination &destination);
	void BuffersCreated(); // XXX fix
	void BuffersDeleted(const media_buffer_id *ids, int32 count);
	// XXX header file lists: B_MEDIA_TRANSPORT_STATE,		/* "state", "location", "realtime" */
	void FormatChanged(const media_source &source, const media_destination &destination, const media_format &format);
	status_t ParameterChanged(const media_node &node, int32 parameterid);
	void WebChanged(const media_node &node); // XXX fix
	// XXX header file lists: B_MEDIA_DEFAULT_CHANGED /* "default", "node" -- handled by BMediaRoster */
	status_t NewParameterValue(const media_node &node, int32 parameterid, bigtime_t when, const void *param, size_t paramsize);
	void FlavorsChanged(media_addon_id addonis, int32 newcount, int32 gonecount);
	void NodeStopped(const media_node &node, bigtime_t when); // XXX fix
	
	static bool IsValidNotificationType(int32 notificationType);
	static notification_type_mask NotificationType2Mask(int32 notificationType);
private:
	status_t SendMessageToMediaServer(BMessage *msg);

private:
	BMessenger *fMessenger;
};

}; // namespace media
}; // namespace BPrivate

#define NOTIFICATION_PARAM_WHAT "be:media:internal:what"
#define NOTIFICATION_PARAM_MASK "be:media:internal:mask"

extern BPrivate::media::NotificationManager *_NotificationManager;

#endif // _NOTIFICATION_MANAGER_H
