/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <MediaNode.h>
#include "TList.h"

class Queue;

struct Notification
{
	BMessenger messenger;
	media_node node;
	int32 what;
	team_id team;
};

class NotificationManager
{
public:
	NotificationManager();
	~NotificationManager();
	
	void Dump();
	
	void EnqueueMessage(BMessage *msg);

	void CleanupTeam(team_id team);

private:
	void RequestNotifications(BMessage *msg);
	void CancelNotifications(BMessage *msg);
	void SendNotifications(BMessage *msg);

	void WorkerThread();
	static int32 worker_thread(void *arg);

private:
	Queue *		fNotificationQueue;
	thread_id	fNotificationThreadId;
	BLocker	*	fLocker;
	List<Notification> *fNotificationList;
};
