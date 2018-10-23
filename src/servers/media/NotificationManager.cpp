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
 *
 */

#include "NotificationManager.h"

#include <Autolock.h>
#include <Locker.h>
#include <Message.h>
#include <OS.h>

#include "DataExchange.h"
#include "MediaDebug.h"
#include "media_server.h"
#include "NodeManager.h"
#include "Notifications.h"


#define NOTIFICATION_THREAD_PRIORITY 19
#define TIMEOUT 100000


NotificationManager::NotificationManager()
	:
	fNotificationThreadId(-1),
	fLocker("notification manager locker")
{
	fNotificationThreadId = spawn_thread(NotificationManager::worker_thread,
		"notification broadcast", NOTIFICATION_THREAD_PRIORITY, this);
	resume_thread(fNotificationThreadId);
}


NotificationManager::~NotificationManager()
{
	// properly terminate the queue and wait until the worker thread has finished
	fNotificationQueue.Terminate();

	status_t dummy;
	wait_for_thread(fNotificationThreadId, &dummy);
}


void
NotificationManager::EnqueueMessage(BMessage *msg)
{
	// queue a copy of the message to be processed later
	fNotificationQueue.AddItem(new BMessage(*msg));
}


void
NotificationManager::RequestNotifications(BMessage *msg)
{
	BMessenger messenger;
	const media_node *node;
	ssize_t nodeSize;
	team_id team;
	int32 what;

	msg->FindMessenger(NOTIFICATION_PARAM_MESSENGER, &messenger);
	msg->FindInt32(NOTIFICATION_PARAM_TEAM, &team);
	msg->FindInt32(NOTIFICATION_PARAM_WHAT, &what);
	msg->FindData("node", B_RAW_TYPE, reinterpret_cast<const void **>(&node),
		&nodeSize);
	ASSERT(nodeSize == sizeof(media_node));

	Notification n;
	n.messenger = messenger;
	n.node = *node;
	n.what = what;
	n.team = team;

	TRACE("NotificationManager::RequestNotifications node %ld, team %ld, "
		"what %#lx\n",node->node, team, what);

	fLocker.Lock();
	fNotificationList.Insert(n);
	fLocker.Unlock();

	// send the initial B_MEDIA_NODE_CREATED containing all existing live nodes
	BMessage initmsg(B_MEDIA_NODE_CREATED);
	if (gNodeManager->GetLiveNodes(&initmsg) == B_OK)
		messenger.SendMessage(&initmsg, static_cast<BHandler *>(NULL), TIMEOUT);
}


void
NotificationManager::CancelNotifications(BMessage *msg)
{
	BMessenger messenger;
	const media_node *node;
	ssize_t nodeSize;
	team_id team;
	int32 what;

	msg->FindMessenger(NOTIFICATION_PARAM_MESSENGER, &messenger);
	msg->FindInt32(NOTIFICATION_PARAM_TEAM, &team);
	msg->FindInt32(NOTIFICATION_PARAM_WHAT, &what);
	msg->FindData("node", B_RAW_TYPE, reinterpret_cast<const void **>(&node),
		&nodeSize);
	ASSERT(nodeSize == sizeof(media_node));

	TRACE("NotificationManager::CancelNotifications node %ld, team %ld, what "
		"%#lx\n", node->node, team, what);

	/* if 		what == B_MEDIA_WILDCARD && node == media_node::null
	 *		=> delete all notifications for the matching team & messenger 
	 * else if 	what != B_MEDIA_WILDCARD && node == media_node::null
	 *		=> delete all notifications for the matching what & team & messenger 
	 * else if 	what == B_MEDIA_WILDCARD && node != media_node::null
	 *		=> delete all notifications for the matching team & messenger & node
	 * else if 	what != B_MEDIA_WILDCARD && node != media_node::null
	 *		=> delete all notifications for the matching what & team & messenger
	 *				& node
	 */
	 
	BAutolock _(fLocker);

	Notification *n;
	for (fNotificationList.Rewind(); fNotificationList.GetNext(&n); ) {
		bool remove;
		if (what == B_MEDIA_WILDCARD && *node == media_node::null
			&& team == n->team && messenger == n->messenger)
			remove = true;
		else if (what != B_MEDIA_WILDCARD && *node == media_node::null
			&& what == n->what && team == n->team && messenger == n->messenger)
			remove = true;
		else if (what == B_MEDIA_WILDCARD && *node != media_node::null
			&& team == n->team && messenger == n->messenger && n->node == *node)
			remove = true;
		else if (what != B_MEDIA_WILDCARD && *node != media_node::null
			&& what == n->what && team == n->team && messenger == n->messenger
			&& n->node == *node)
			remove = true;
		else
			remove = false;

		if (remove) {
			if (!fNotificationList.RemoveCurrent()) {
				ASSERT(false);
			}
		}
	}
}


void
NotificationManager::SendNotifications(BMessage *msg)
{
	const media_source *source;
	const media_destination *destination;
	const media_node *node;
	ssize_t size;
	int32 what;

	msg->FindInt32(NOTIFICATION_PARAM_WHAT, &what);
	msg->RemoveName(NOTIFICATION_PARAM_WHAT);
	msg->what = what;

	TRACE("NotificationManager::SendNotifications what %#lx\n", what);

	BAutolock _(fLocker);

	Notification *n;
	for (fNotificationList.Rewind(); fNotificationList.GetNext(&n); ) {
		if (n->what != B_MEDIA_WILDCARD && n->what != what)
			continue;
		
		switch (what) {
			case B_MEDIA_NODE_CREATED:
			case B_MEDIA_NODE_DELETED:
			case B_MEDIA_CONNECTION_MADE:
			case B_MEDIA_CONNECTION_BROKEN:
			case B_MEDIA_BUFFER_CREATED:
			case B_MEDIA_BUFFER_DELETED:
			case B_MEDIA_TRANSPORT_STATE:
			case B_MEDIA_DEFAULT_CHANGED:
			case B_MEDIA_FLAVORS_CHANGED:
				if (n->node != media_node::null)
					continue;
				break;

			case B_MEDIA_NEW_PARAMETER_VALUE:
			case B_MEDIA_PARAMETER_CHANGED:
			case B_MEDIA_NODE_STOPPED:
			case B_MEDIA_WEB_CHANGED:
				msg->FindData("node", B_RAW_TYPE,
					reinterpret_cast<const void **>(&node), &size);
				ASSERT(size == sizeof(media_node));
				if (n->node != *node)
					continue;
				break;

			case B_MEDIA_FORMAT_CHANGED:
				msg->FindData("source", B_RAW_TYPE,
					reinterpret_cast<const void **>(&source), &size);
				ASSERT(size == sizeof(media_source));
				msg->FindData("destination", B_RAW_TYPE,
					reinterpret_cast<const void **>(&destination), &size);
				ASSERT(size == sizeof(media_destination));

				if (n->node.port != source->port
					&& n->node.port != destination->port)
					continue;
				break;
		}

		TRACE("NotificationManager::SendNotifications sending\n");
		n->messenger.SendMessage(msg, static_cast<BHandler *>(NULL), TIMEOUT);
	}
}
	

void
NotificationManager::CleanupTeam(team_id team)
{
	TRACE("NotificationManager::CleanupTeam team %ld\n", team);
	BAutolock _(fLocker);

	int debugCount = 0;
	Notification *n;
	for (fNotificationList.Rewind(); fNotificationList.GetNext(&n); ) {
		if (n->team == team) {
			if (fNotificationList.RemoveCurrent()) {
				debugCount++;
			} else {
				ASSERT(false);
			}
		}
	}

	if (debugCount != 0) {
		ERROR("NotificationManager::CleanupTeam: removed  %d notifications for "
			"team %" B_PRId32 "\n", debugCount, team);
	}
}


void
NotificationManager::WorkerThread()
{
	while (BMessage *msg
			= static_cast<BMessage *>(fNotificationQueue.RemoveItem())) {
		switch (msg->what) {
			case MEDIA_SERVER_REQUEST_NOTIFICATIONS:
				RequestNotifications(msg);
				break;
			case MEDIA_SERVER_CANCEL_NOTIFICATIONS:
				CancelNotifications(msg);
				break;
			case MEDIA_SERVER_SEND_NOTIFICATIONS:
				SendNotifications(msg);
				break;
			default:
				debugger("bad notification message\n");
		}
		delete msg;
	}
}


int32
NotificationManager::worker_thread(void *arg)
{
	static_cast<NotificationManager *>(arg)->WorkerThread();
	return 0;
}


void
NotificationManager::Dump()
{
	BAutolock lock(fLocker);

	printf("\n");
	printf("NotificationManager: list of subscribers follows:\n");
	Notification *n;	
	for (fNotificationList.Rewind(); fNotificationList.GetNext(&n); ) {
		printf(" team %" B_PRId32 ", what %#08" B_PRIx32 ", node-id %" B_PRId32
			", node-port %" B_PRId32 ", messenger %svalid\n", n->team, n->what,
			 n->node.node, n->node.port, n->messenger.IsValid() ? "" : "NOT ");
	}
	printf("NotificationManager: list end\n");
}
