#include <OS.h>
#include <Locker.h>
#include <Message.h>
#include <Messenger.h>
#include <MediaNode.h>
#include <Debug.h>
#include "DataExchange.h"
#include "Notifications.h"
#include "NotificationManager.h"
#include "Queue.h"

#define NOTIFICATION_THREAD_PRIORITY 19

struct RegisteredHandler
{
	BMessenger messenger;
	media_node_id nodeid;
	int32 mask;
	team_id team;
};

NotificationManager::NotificationManager()
 :	fNotificationQueue(new Queue),
	fNotificationThreadId(-1),
	fLocker(new BLocker)
{
	fNotificationThreadId = spawn_thread(NotificationManager::worker_thread, "notification broadcast", NOTIFICATION_THREAD_PRIORITY, this);
	resume_thread(fNotificationThreadId);
}

NotificationManager::~NotificationManager()
{
	// properly terminate the queue and wait until the worker thread has finished
	status_t dummy;
	fNotificationQueue->Terminate();
	wait_for_thread(fNotificationThreadId, &dummy);
	delete fNotificationQueue;
	delete fLocker;
}

void
NotificationManager::EnqueueMessage(BMessage *msg)
{
	// queue a copy of the message to be processed later
	fNotificationQueue->AddItem(new BMessage(*msg));
}

void
NotificationManager::RequestNotifications(BMessage *msg)
{
	BMessenger messenger;
	const media_node *node;
	ssize_t nodesize;
	team_id team;
	int32 what;

	msg->FindMessenger(NOTIFICATION_PARAM_MESSENGER, &messenger);
	msg->FindInt32(NOTIFICATION_PARAM_TEAM, &team);
	msg->FindInt32(NOTIFICATION_PARAM_WHAT, &what);
	msg->FindData("node", B_RAW_TYPE, reinterpret_cast<const void **>(&node), &nodesize);
	ASSERT(nodesize == sizeof(node));

}

void
NotificationManager::CancelNotifications(BMessage *msg)
{
	BMessenger messenger;
	const media_node *node;
	ssize_t nodesize;
	team_id team;
	int32 what;

	msg->FindMessenger(NOTIFICATION_PARAM_MESSENGER, &messenger);
	msg->FindInt32(NOTIFICATION_PARAM_TEAM, &team);
	msg->FindInt32(NOTIFICATION_PARAM_WHAT, &what);
	msg->FindData("node", B_RAW_TYPE, reinterpret_cast<const void **>(&node), &nodesize);
	ASSERT(nodesize == sizeof(node));

}

void
NotificationManager::SendNotifications(BMessage *msg)
{
}
	
void
NotificationManager::CleanupTeam(team_id team)
{
}

void
NotificationManager::BroadcastMessages(BMessage *msg)
{
}

void
NotificationManager::WorkerThread()
{
	BMessage *msg;
	while (NULL != (msg = static_cast<BMessage *>(fNotificationQueue->RemoveItem()))) {
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
