#include <OS.h>
#include <Locker.h>
#include <Message.h>
#include <Messenger.h>
#include <MediaNode.h>
#include <Debug.h>
#include "NotificationManager.h"
#include "NotificationProcessor.h"
#include "ServerInterface.h"
#include "DataExchange.h"
#include "Queue.h"

#define NOTIFICATION_THREAD_PRIORITY 19

struct RegisteredHandler
{
	BMessenger messenger;
	media_node_id nodeid;
	int32 mask;
	team_id team;
};

NotificationProcessor::NotificationProcessor()
 :	fNotificationQueue(new Queue),
	fNotificationThreadId(-1),
	fLocker(new BLocker)
{
	fNotificationThreadId = spawn_thread(NotificationProcessor::worker_thread, "notification broadcast", NOTIFICATION_THREAD_PRIORITY, this);
	resume_thread(fNotificationThreadId);
}

NotificationProcessor::~NotificationProcessor()
{
	// properly terminate the queue and wait until the worker thread has finished
	status_t dummy;
	fNotificationQueue->Terminate();
	wait_for_thread(fNotificationThreadId, &dummy);
	delete fNotificationQueue;
	delete fLocker;
}

void
NotificationProcessor::EnqueueMessage(BMessage *msg)
{
	// queue a copy of the message to be processed later
	fNotificationQueue->AddItem(new BMessage(*msg));
}

void
NotificationProcessor::RequestNotifications(BMessage *msg)
{
	BMessenger messenger;
	const media_node *node;
	ssize_t nodesize;
	team_id team;
	int32 mask;

	msg->FindMessenger(NOTIFICATION_PARAM_MESSENGER, &messenger);
	msg->FindInt32(NOTIFICATION_PARAM_TEAM, &team);
	msg->FindInt32(NOTIFICATION_PARAM_MASK, &mask);
	msg->FindData("node", B_RAW_TYPE, reinterpret_cast<const void **>(&node), &nodesize);
	ASSERT(nodesize == sizeof(node));

}

void
NotificationProcessor::CancelNotifications(BMessage *msg)
{
	BMessenger messenger;
	const media_node *node;
	ssize_t nodesize;
	team_id team;
	int32 mask;

	msg->FindMessenger(NOTIFICATION_PARAM_MESSENGER, &messenger);
	msg->FindInt32(NOTIFICATION_PARAM_TEAM, &team);
	msg->FindInt32(NOTIFICATION_PARAM_MASK, &mask);
	msg->FindData("node", B_RAW_TYPE, reinterpret_cast<const void **>(&node), &nodesize);
	ASSERT(nodesize == sizeof(node));

}

void
NotificationProcessor::SendNotifications(BMessage *msg)
{
}
	
void
NotificationProcessor::CleanupTeam(team_id team)
{
}

void
NotificationProcessor::BroadcastMessages(BMessage *msg)
{
}

void
NotificationProcessor::WorkerThread()
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
NotificationProcessor::worker_thread(void *arg)
{
	static_cast<NotificationProcessor *>(arg)->WorkerThread();
	return 0;
}
