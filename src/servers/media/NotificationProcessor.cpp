#include <OS.h>
#include <Message.h>
#include "NotificationProcessor.h"
#include "Queue.h"

#define NOTIFICATION_THREAD_PRIORITY 19

NotificationProcessor::NotificationProcessor()
 :	fNotificationQueue(new Queue),
	fNotificationThreadId(-1)
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
}

void
NotificationProcessor::RequestNotifications(BMessage *msg)
{
}

void
NotificationProcessor::CancelNotifications(BMessage *msg)
{
}

void
NotificationProcessor::SendNotifications(BMessage *msg)
{
	// queue a copy of the message to be processed later
	fNotificationQueue->AddItem(new BMessage(*msg));
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
		BroadcastMessages(msg);
		delete msg;
	}
}

int32
NotificationProcessor::worker_thread(void *arg)
{
	static_cast<NotificationProcessor *>(arg)->WorkerThread();
	return 0;
}
