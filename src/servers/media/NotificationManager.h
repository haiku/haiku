
class Queue;

class NotificationManager
{
public:
	NotificationManager();
	~NotificationManager();
	
	void EnqueueMessage(BMessage *msg);

	void CleanupTeam(team_id team);

private:
	void RequestNotifications(BMessage *msg);
	void CancelNotifications(BMessage *msg);
	void SendNotifications(BMessage *msg);

	void BroadcastMessages(BMessage *msg);
	void WorkerThread();
	static int32 worker_thread(void *arg);

private:
	Queue *		fNotificationQueue;
	thread_id	fNotificationThreadId;
	BLocker	*	fLocker;
};
