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
	NotificationManager();
	~NotificationManager();
		
	status_t Register(const BMessenger &notifyHandler, const media_node &node, int32 notificationType);
	status_t Unregister(const BMessenger &notifyHandler, const media_node &node, int32 notificationType);

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
private:
	status_t SendMessageToMediaServer(BMessage *msg);

private:
	BMessenger *fMessenger;
};

}; // namespace media
}; // namespace BPrivate

extern BPrivate::media::NotificationManager *_NotificationManager;

#endif // _NOTIFICATION_MANAGER_H
