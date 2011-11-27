/*
 * Copyright 2008-2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Salvatore Benedetto <salvatore.benedetto@gmail.com>
 */

#include <posix/xsi_message_queue.h>

#include <new>

#include <sys/ipc.h>
#include <sys/types.h>

#include <OS.h>

#include <kernel.h>
#include <syscall_restart.h>

#include <util/atomic.h>
#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h>


#define TRACE_XSI_MSG_QUEUE
#ifdef TRACE_XSI_MSG_QUEUE
#	define TRACE(x)			dprintf x
#	define TRACE_ERROR(x)	dprintf x
#else
#	define TRACE(x)			/* nothing */
#	define TRACE_ERROR(x)	dprintf x
#endif

// Queue for holding blocked threads
struct queued_thread : DoublyLinkedListLinkImpl<queued_thread> {
	queued_thread(Thread *_thread, int32 _message_length)
		:
		thread(_thread),
		message_length(_message_length),
		queued(false)
	{
	}

	Thread	*thread;
	int32	message_length;
	bool	queued;
};

typedef DoublyLinkedList<queued_thread> ThreadQueue;


struct queued_message : DoublyLinkedListLinkImpl<queued_message> {
	queued_message(const void *_message, ssize_t _length)
		:
		initOK(false),
		length(_length)
	{
		message = (char *)malloc(sizeof(char) * _length);
		if (message == NULL)
			return;

		if (user_memcpy(&type, _message, sizeof(long)) != B_OK
			|| user_memcpy(message, (void *)((char *)_message + sizeof(long)),
			_length) != B_OK) {
			free(message);
			return;
		}
		initOK = true;
	}

	~queued_message()
	{
		if (initOK)
			free(message);
	}

	ssize_t copy_to_user_buffer(void *_message, ssize_t _length)
	{
		if (_length > length)
			_length = length;

		if (user_memcpy(_message, &type, sizeof(long)) != B_OK
			|| user_memcpy((void *)((char *)_message + sizeof(long)), message,
			_length) != B_OK)
			return B_ERROR;
		return _length;
	}

	bool		initOK;
	ssize_t		length;
	char		*message;
	long		type;
};

typedef DoublyLinkedList<queued_message> MessageQueue;

// Arbitrary limit
#define MAX_BYTES_PER_QUEUE		2048

class XsiMessageQueue {
public:
	XsiMessageQueue(int flags)
		:
		fBytesInQueue(0),
		fThreadsWaitingToReceive(0),
		fThreadsWaitingToSend(0)
	{
		mutex_init(&fLock, "XsiMessageQueue private mutex");
		SetIpcKey((key_t)-1);
		SetPermissions(flags);
		// Initialize all fields to zero
		memset((void *)&fMessageQueue, 0, sizeof(struct msqid_ds));
		fMessageQueue.msg_ctime = (time_t)real_time_clock();
		fMessageQueue.msg_qbytes = MAX_BYTES_PER_QUEUE;
	}

	// Implemented after sXsiMessageCount is declared
	~XsiMessageQueue();

	status_t BlockAndUnlock(Thread *thread, MutexLocker *queueLocker)
	{
		thread_prepare_to_block(thread, B_CAN_INTERRUPT,
				THREAD_BLOCK_TYPE_OTHER, (void*)"xsi message queue");
		// Unlock the queue before blocking
		queueLocker->Unlock();

		InterruptsSpinLocker schedulerLocker(gSchedulerLock);
// TODO: We've got a serious race condition: If BlockAndUnlock() returned due to
// interruption, we will still be queued. A WakeUpThread() at this point will
// call thread_unblock() and might thus screw with our trying to re-lock the
// mutex.
		return thread_block_locked(thread);
	}

	void DoIpcSet(struct msqid_ds *result)
	{
		fMessageQueue.msg_perm.uid = result->msg_perm.uid;
		fMessageQueue.msg_perm.gid = result->msg_perm.gid;
		fMessageQueue.msg_perm.mode = (fMessageQueue.msg_perm.mode & ~0x01ff)
			| (result->msg_perm.mode & 0x01ff);
		fMessageQueue.msg_qbytes = result->msg_qbytes;
		fMessageQueue.msg_ctime = (time_t)real_time_clock();
	}

	void Deque(queued_thread *queueEntry, bool waitForMessage)
	{
		if (queueEntry->queued) {
			if (waitForMessage) {
				fWaitingToReceive.Remove(queueEntry);
				fThreadsWaitingToReceive--;
			} else {
				fWaitingToSend.Remove(queueEntry);
				fThreadsWaitingToSend--;
			}
		}
	}

	void Enqueue(queued_thread *queueEntry, bool waitForMessage)
	{
		if (waitForMessage) {
			fWaitingToReceive.Add(queueEntry);
			fThreadsWaitingToReceive++;
		} else {
			fWaitingToSend.Add(queueEntry);
			fThreadsWaitingToSend++;
		}
		queueEntry->queued = true;
	}

	struct msqid_ds &GetMessageQueue()
	{
		return fMessageQueue;
	}

	bool HasPermission() const
	{
		if ((fMessageQueue.msg_perm.mode & S_IWOTH) != 0)
			return true;

		uid_t uid = geteuid();
		if (uid == 0 || (uid == fMessageQueue.msg_perm.uid
			&& (fMessageQueue.msg_perm.mode & S_IWUSR) != 0))
			return true;

		gid_t gid = getegid();
		if (gid == fMessageQueue.msg_perm.gid
			&& (fMessageQueue.msg_perm.mode & S_IWGRP) != 0)
			return true;

		return false;
	}

	bool HasReadPermission() const
	{
		// TODO: fix this
		return HasPermission();
	}

	int ID() const
	{
		return fID;
	}

	// Implemented after sXsiMessageCount is declared
	bool Insert(queued_message *message);

	key_t IpcKey() const
	{
		return fMessageQueue.msg_perm.key;
	}

	mutex &Lock()
	{
		return fLock;
	}

	msglen_t MaxBytes() const
	{
		return fMessageQueue.msg_qbytes;
	}

	// Implemented after sXsiMessageCount is declared
	queued_message *Remove(long typeRequested);

	uint32 SequenceNumber() const
	{
		return fSequenceNumber;
	}

	// Implemented after sMessageQueueHashTable is declared
	void SetID();

	void SetIpcKey(key_t key)
	{
		fMessageQueue.msg_perm.key = key;
	}

	void SetPermissions(int flags)
	{
		fMessageQueue.msg_perm.uid = fMessageQueue.msg_perm.cuid = geteuid();
		fMessageQueue.msg_perm.gid = fMessageQueue.msg_perm.cgid = getegid();
		fMessageQueue.msg_perm.mode = (flags & 0x01ff);
	}

	void WakeUpThread(bool waitForMessage)
	{
		InterruptsSpinLocker schedulerLocker(gSchedulerLock);

		if (waitForMessage) {
			// Wake up all waiting thread for a message
			// TODO: this can cause starvation for any
			// very-unlucky-and-slow thread
			while (queued_thread *entry = fWaitingToReceive.RemoveHead()) {
				entry->queued = false;
				fThreadsWaitingToReceive--;
				thread_unblock_locked(entry->thread, 0);
			}
		} else {
			// Wake up only one thread waiting to send
			if (queued_thread *entry = fWaitingToSend.RemoveHead()) {
				entry->queued = false;
				fThreadsWaitingToSend--;
				thread_unblock_locked(entry->thread, 0);
			}
		}
	}

	XsiMessageQueue*& Link()
	{
		return fLink;
	}

private:
	msglen_t			fBytesInQueue;
	int					fID;
	mutex				fLock;
	MessageQueue		fMessage;
	struct msqid_ds		fMessageQueue;
	uint32				fSequenceNumber;
	uint32				fThreadsWaitingToReceive;
	uint32				fThreadsWaitingToSend;

	ThreadQueue			fWaitingToReceive;
	ThreadQueue			fWaitingToSend;

	XsiMessageQueue*	fLink;
};


// Xsi message queue hash table
struct MessageQueueHashTableDefinition {
	typedef int					KeyType;
	typedef XsiMessageQueue		ValueType;

	size_t HashKey (const int key) const
	{
		return (size_t)key;
	}

	size_t Hash(XsiMessageQueue *variable) const
	{
		return (size_t)variable->ID();
	}

	bool Compare(const int key, XsiMessageQueue *variable) const
	{
		return (int)key == (int)variable->ID();
	}

	XsiMessageQueue*& GetLink(XsiMessageQueue *variable) const
	{
		return variable->Link();
	}
};


// IPC class
class Ipc {
public:
	Ipc(key_t key)
		: fKey(key),
		fMessageQueueId(-1)
	{
	}

	key_t Key() const
	{
		return fKey;
	}

	int MessageQueueID() const
	{
		return fMessageQueueId;
	}

	void SetMessageQueueID(XsiMessageQueue *messageQueue)
	{
		fMessageQueueId = messageQueue->ID();
	}

	Ipc*& Link()
	{
		return fLink;
	}

private:
	key_t				fKey;
	int					fMessageQueueId;
	Ipc*				fLink;
};


struct IpcHashTableDefinition {
	typedef key_t	KeyType;
	typedef Ipc		ValueType;

	size_t HashKey (const key_t key) const
	{
		return (size_t)(key);
	}

	size_t Hash(Ipc *variable) const
	{
		return (size_t)HashKey(variable->Key());
	}

	bool Compare(const key_t key, Ipc *variable) const
	{
		return (key_t)key == (key_t)variable->Key();
	}

	Ipc*& GetLink(Ipc *variable) const
	{
		return variable->Link();
	}
};

// Arbitrary limits
#define MAX_XSI_MESSAGE			4096
#define MAX_XSI_MESSAGE_QUEUE	1024
static BOpenHashTable<IpcHashTableDefinition> sIpcHashTable;
static BOpenHashTable<MessageQueueHashTableDefinition> sMessageQueueHashTable;

static mutex sIpcLock;
static mutex sXsiMessageQueueLock;

static uint32 sGlobalSequenceNumber = 1;
static vint32 sXsiMessageCount = 0;
static vint32 sXsiMessageQueueCount = 0;


//	#pragma mark -


XsiMessageQueue::~XsiMessageQueue()
{
	mutex_destroy(&fLock);

	// Wake up any threads still waiting
	if (fThreadsWaitingToSend || fThreadsWaitingToReceive) {
		InterruptsSpinLocker schedulerLocker(gSchedulerLock);

		while (queued_thread *entry = fWaitingToReceive.RemoveHead()) {
			entry->queued = false;
			thread_unblock_locked(entry->thread, EIDRM);
		}
		while (queued_thread *entry = fWaitingToSend.RemoveHead()) {
			entry->queued = false;
			thread_unblock_locked(entry->thread, EIDRM);
		}
	}

	// Free up any remaining messages
	if (fMessageQueue.msg_qnum) {
		while (queued_message *message = fMessage.RemoveHead()) {
			atomic_add(&sXsiMessageCount, -1);
			delete message;
		}
	}
}


bool
XsiMessageQueue::Insert(queued_message *message)
{
	// The only situation that would make us (potentially) wait
	// is that we exceed with bytes or with the total number of messages
	if (fBytesInQueue + message->length > fMessageQueue.msg_qbytes)
		return true;

	while (true) {
		int32 oldCount = atomic_get(&sXsiMessageCount);
		if (oldCount >= MAX_XSI_MESSAGE)
			return true;
		// If another thread updates the counter we keep
		// iterating
		if (atomic_test_and_set(&sXsiMessageCount, oldCount + 1, oldCount)
			== oldCount)
			break;
	}

	fMessage.Add(message);
	fMessageQueue.msg_qnum++;
	fMessageQueue.msg_lspid = getpid();
	fMessageQueue.msg_stime = real_time_clock();
	fBytesInQueue += message->length;
	if (fThreadsWaitingToReceive)
		WakeUpThread(true /* WaitForMessage */);
	return false;
}


queued_message*
XsiMessageQueue::Remove(long typeRequested)
{
	queued_message *message = NULL;
	if (typeRequested < 0) {
		// Return first message of the lowest type
		// that is less than or equal to the absolute
		// value of type requested.
		MessageQueue::Iterator iterator = fMessage.GetIterator();
		while (iterator.HasNext()) {
			queued_message *current = iterator.Next();
			if (current->type <= -typeRequested) {
				message = iterator.Remove();
				break;
			}
		}
	} else if (typeRequested == 0) {
		// Return the first message on the queue
		message = fMessage.RemoveHead();
	} else {
		// Return the first message of type requested
		MessageQueue::Iterator iterator = fMessage.GetIterator();
		while (iterator.HasNext()) {
			queued_message *current = iterator.Next();
			if (current->type == typeRequested) {
				message = iterator.Remove();
				break;
			}
		}
	}

	if (message == NULL)
		return NULL;

	fMessageQueue.msg_qnum--;
	fMessageQueue.msg_lrpid = getpid();
	fMessageQueue.msg_rtime = real_time_clock();
	fBytesInQueue -= message->length;
	atomic_add(&sXsiMessageCount, -1);
	if (fThreadsWaitingToSend)
		WakeUpThread(false /* WaitForMessage */);
	return message;
}


void
XsiMessageQueue::SetID()
{
	fID = real_time_clock();
	// The lock is held before calling us
	while (true) {
		if (sMessageQueueHashTable.Lookup(fID) == NULL)
			break;
		fID++;
	}
	sGlobalSequenceNumber = (sGlobalSequenceNumber + 1) % UINT_MAX;
	fSequenceNumber = sGlobalSequenceNumber;
}


//	#pragma mark - Kernel exported API


void
xsi_msg_init()
{
	// Initialize hash tables
	status_t status = sIpcHashTable.Init();
	if (status != B_OK)
		panic("xsi_msg_init() failed to initialize ipc hash table\n");
	status =  sMessageQueueHashTable.Init();
	if (status != B_OK)
		panic("xsi_msg_init() failed to initialize message queue hash table\n");

	mutex_init(&sIpcLock, "global POSIX message queue IPC table");
	mutex_init(&sXsiMessageQueueLock, "global POSIX xsi message queue table");
}


//	#pragma mark - Syscalls


int
_user_xsi_msgctl(int messageQueueID, int command, struct msqid_ds *buffer)
{
	TRACE(("xsi_msgctl: messageQueueID = %d, command = %d\n", messageQueueID, command));
	MutexLocker ipcHashLocker(sIpcLock);
	MutexLocker messageQueueHashLocker(sXsiMessageQueueLock);
	XsiMessageQueue *messageQueue = sMessageQueueHashTable.Lookup(messageQueueID);
	if (messageQueue == NULL) {
		TRACE_ERROR(("xsi_msgctl: message queue id %d not valid\n", messageQueueID));
		return EINVAL;
	}
	if (!IS_USER_ADDRESS(buffer)) {
		TRACE_ERROR(("xsi_msgctl: buffer address is not valid\n"));
		return B_BAD_ADDRESS;
	}

	// Lock the message queue itself and release both the ipc hash table lock
	// and the message queue hash table lock _only_ if the command it's not
	// IPC_RMID, this prevents undesidered situation from happening while
	// (hopefully) improving the concurrency.
	MutexLocker messageQueueLocker;
	if (command != IPC_RMID) {
		messageQueueLocker.SetTo(&messageQueue->Lock(), false);
		messageQueueHashLocker.Unlock();
		ipcHashLocker.Unlock();
	} else
		// Since we are going to delete the message queue object
		// along with its mutex, we can't use a MutexLocker object,
		// as the mutex itself won't exist on function exit
		mutex_lock(&messageQueue->Lock());

	switch (command) {
		case IPC_STAT: {
			if (!messageQueue->HasReadPermission()) {
				TRACE_ERROR(("xsi_msgctl: calling process has not read "
					"permission on message queue %d, key %d\n", messageQueueID,
					(int)messageQueue->IpcKey()));
				return EACCES;
			}
			struct msqid_ds msg = messageQueue->GetMessageQueue();
			if (user_memcpy(buffer, &msg, sizeof(struct msqid_ds)) < B_OK) {
				TRACE_ERROR(("xsi_msgctl: user_memcpy failed\n"));
				return B_BAD_ADDRESS;
			}
			break;
		}

		case IPC_SET: {
			if (!messageQueue->HasPermission()) {
				TRACE_ERROR(("xsi_msgctl: calling process has not permission "
					"on message queue %d, key %d\n", messageQueueID,
					(int)messageQueue->IpcKey()));
				return EPERM;
			}
			struct msqid_ds msg;
			if (user_memcpy(&msg, buffer, sizeof(struct msqid_ds)) < B_OK) {
				TRACE_ERROR(("xsi_msgctl: user_memcpy failed\n"));
				return B_BAD_ADDRESS;
			}
			if (msg.msg_qbytes > messageQueue->MaxBytes() && getuid() != 0) {
				TRACE_ERROR(("xsi_msgctl: user does not have permission to "
					"increase the maximum number of bytes allowed on queue\n"));
				return EPERM;
			}
			if (msg.msg_qbytes == 0) {
				TRACE_ERROR(("xsi_msgctl: can't set msg_qbytes to 0!\n"));
				return EINVAL;
			}

			messageQueue->DoIpcSet(&msg);
			break;
		}

		case IPC_RMID: {
			// If this was the command, we are still holding the message
			// queue hash table lock along with the ipc one, but not the
			// message queue lock itself. This prevents other process
			// to try and acquire a destroyed mutex
			if (!messageQueue->HasPermission()) {
				TRACE_ERROR(("xsi_msgctl: calling process has not permission "
					"on message queue %d, key %d\n", messageQueueID,
					(int)messageQueue->IpcKey()));
				return EPERM;
			}
			key_t key = messageQueue->IpcKey();
			Ipc *ipcKey = NULL;
			if (key != -1) {
				ipcKey = sIpcHashTable.Lookup(key);
				sIpcHashTable.Remove(ipcKey);
			}
			sMessageQueueHashTable.Remove(messageQueue);
			// Wake up of any threads waiting on this
			// queue happens in destructor
			if (key != -1)
				delete ipcKey;
			atomic_add(&sXsiMessageQueueCount, -1);

			delete messageQueue;
			break;
		}

		default:
			TRACE_ERROR(("xsi_semctl: command %d not valid\n", command));
			return EINVAL;
	}

	return B_OK;
}


int
_user_xsi_msgget(key_t key, int flags)
{
	TRACE(("xsi_msgget: key = %d, flags = %d\n", (int)key, flags));
	XsiMessageQueue *messageQueue = NULL;
	Ipc *ipcKey = NULL;
	// Default assumptions
	bool isPrivate = true;
	bool create = true;

	if (key != IPC_PRIVATE) {
		isPrivate = false;
		// Check if key already exist, if it does it already has a message
		// queue associated with it
		ipcKey = sIpcHashTable.Lookup(key);
		if (ipcKey == NULL) {
			if (!(flags & IPC_CREAT)) {
				TRACE_ERROR(("xsi_msgget: key %d does not exist, but the "
					"caller did not ask for creation\n", (int)key));
				return ENOENT;
			}
			ipcKey = new(std::nothrow) Ipc(key);
			if (ipcKey == NULL) {
				TRACE_ERROR(("xsi_msgget: failed to create new Ipc object "
					"for key %d\n", (int)key));
				return ENOMEM;
			}
			sIpcHashTable.Insert(ipcKey);
		} else {
			// The IPC key exist and it already has a message queue
			if ((flags & IPC_CREAT) && (flags & IPC_EXCL)) {
				TRACE_ERROR(("xsi_msgget: key %d already exist\n", (int)key));
				return EEXIST;
			}
			int messageQueueID = ipcKey->MessageQueueID();

			MutexLocker _(sXsiMessageQueueLock);
			messageQueue = sMessageQueueHashTable.Lookup(messageQueueID);
			if (!messageQueue->HasPermission()) {
				TRACE_ERROR(("xsi_msgget: calling process has not permission "
					"on message queue %d, key %d\n", messageQueue->ID(),
					(int)key));
				return EACCES;
			}
			create = false;
		}
	}

	if (create) {
		// Create a new message queue for this key
		if (sXsiMessageQueueCount >= MAX_XSI_MESSAGE_QUEUE) {
			TRACE_ERROR(("xsi_msgget: reached limit of maximun number of "
				"message queues\n"));
			return ENOSPC;
		}

		messageQueue = new(std::nothrow) XsiMessageQueue(flags);
		if (messageQueue == NULL) {
			TRACE_ERROR(("xsi_msgget: failed to allocate new xsi "
				"message queue\n"));
			return ENOMEM;
		}
		atomic_add(&sXsiMessageQueueCount, 1);

		MutexLocker _(sXsiMessageQueueLock);
		messageQueue->SetID();
		if (isPrivate)
			messageQueue->SetIpcKey((key_t)-1);
		else {
			messageQueue->SetIpcKey(key);
			ipcKey->SetMessageQueueID(messageQueue);
		}
		sMessageQueueHashTable.Insert(messageQueue);
	}

	return messageQueue->ID();
}


ssize_t
_user_xsi_msgrcv(int messageQueueID, void *messagePointer,
	size_t messageSize, long messageType, int messageFlags)
{
	TRACE(("xsi_msgrcv: messageQueueID = %d, messageSize = %ld\n",
		messageQueueID, messageSize));
	MutexLocker messageQueueHashLocker(sXsiMessageQueueLock);
	XsiMessageQueue *messageQueue = sMessageQueueHashTable.Lookup(messageQueueID);
	if (messageQueue == NULL) {
		TRACE_ERROR(("xsi_msgrcv: message queue id %d not valid\n",
			messageQueueID));
		return EINVAL;
	}
	MutexLocker messageQueueLocker(messageQueue->Lock());
	messageQueueHashLocker.Unlock();

	if (messageSize > MAX_BYTES_PER_QUEUE) {
		TRACE_ERROR(("xsi_msgrcv: message size is out of range\n"));
		return EINVAL;
	}
	if (!messageQueue->HasPermission()) {
		TRACE_ERROR(("xsi_msgrcv: calling process has not permission "
			"on message queue id %d, key %d\n", messageQueueID,
			(int)messageQueue->IpcKey()));
		return EACCES;
	}
	if (!IS_USER_ADDRESS(messagePointer)) {
		TRACE_ERROR(("xsi_msgrcv: message address is not valid\n"));
		return B_BAD_ADDRESS;
	}

	queued_message *message = NULL;
	while (true) {
		message = messageQueue->Remove(messageType);

		if (message == NULL && !(messageFlags & IPC_NOWAIT)) {
			// We are going to sleep
			Thread *thread = thread_get_current_thread();
			queued_thread queueEntry(thread, messageSize);
			messageQueue->Enqueue(&queueEntry, /* waitForMessage */ true);

			uint32 sequenceNumber = messageQueue->SequenceNumber();

			TRACE(("xsi_msgrcv: thread %d going to sleep\n", (int)thread->id));
			status_t result
				= messageQueue->BlockAndUnlock(thread, &messageQueueLocker);
			TRACE(("xsi_msgrcv: thread %d back to life\n", (int)thread->id));

			messageQueueHashLocker.Lock();
			messageQueue = sMessageQueueHashTable.Lookup(messageQueueID);
			if (result == EIDRM || messageQueue == NULL || (messageQueue != NULL
				&& sequenceNumber != messageQueue->SequenceNumber())) {
				TRACE_ERROR(("xsi_msgrcv: message queue id %d (sequence = %ld) "
					"got destroyed\n", messageQueueID, sequenceNumber));
				return EIDRM;
			} else if (result == B_INTERRUPTED) {
				TRACE_ERROR(("xsi_msgrcv: thread %d got interrupted while "
					"waiting on message queue %d\n",(int)thread->id,
					messageQueueID));
				messageQueue->Deque(&queueEntry, /* waitForMessage */ true);
				return EINTR;
			} else {
				messageQueueLocker.Lock();
				messageQueueHashLocker.Unlock();
			}
		} else if (message == NULL) {
			// There is not message of type requested and
			// we can't wait
			return ENOMSG;
		} else {
			// Message received correctly (so far)
			if ((ssize_t)messageSize < message->length
				&& !(messageFlags & MSG_NOERROR)) {
				TRACE_ERROR(("xsi_msgrcv: message too big!\n"));
				// Put the message back inside. Since we hold the
				// queue message lock, not one else could have filled
				// up the queue meanwhile
				messageQueue->Insert(message);
				return E2BIG;
			}

			ssize_t result
				= message->copy_to_user_buffer(messagePointer, messageSize);
			if (result < 0) {
				messageQueue->Insert(message);
				return B_BAD_ADDRESS;
			}

			delete message;
			TRACE(("xsi_msgrcv: message received correctly\n"));
			return result;
		}
	}

	return B_OK;
}


int
_user_xsi_msgsnd(int messageQueueID, const void *messagePointer,
	size_t messageSize, int messageFlags)
{
	TRACE(("xsi_msgsnd: messageQueueID = %d, messageSize = %ld\n",
		messageQueueID, messageSize));
	MutexLocker messageQueueHashLocker(sXsiMessageQueueLock);
	XsiMessageQueue *messageQueue = sMessageQueueHashTable.Lookup(messageQueueID);
	if (messageQueue == NULL) {
		TRACE_ERROR(("xsi_msgsnd: message queue id %d not valid\n",
			messageQueueID));
		return EINVAL;
	}
	MutexLocker messageQueueLocker(messageQueue->Lock());
	messageQueueHashLocker.Unlock();

	if (messageSize > MAX_BYTES_PER_QUEUE) {
		TRACE_ERROR(("xsi_msgsnd: message size is out of range\n"));
		return EINVAL;
	}
	if (!messageQueue->HasPermission()) {
		TRACE_ERROR(("xsi_msgsnd: calling process has not permission "
			"on message queue id %d, key %d\n", messageQueueID,
			(int)messageQueue->IpcKey()));
		return EACCES;
	}
	if (!IS_USER_ADDRESS(messagePointer)) {
		TRACE_ERROR(("xsi_msgsnd: message address is not valid\n"));
		return B_BAD_ADDRESS;
	}

	queued_message *message
		= new(std::nothrow) queued_message(messagePointer, messageSize);
	if (message == NULL || message->initOK != true) {
		TRACE_ERROR(("xsi_msgsnd: failed to create new message to queue\n"));
		delete message;
		return ENOMEM;
	}

	bool notSent = true;
	status_t result = B_OK;
	while (notSent) {
		bool goToSleep = messageQueue->Insert(message);

		if (goToSleep && !(messageFlags & IPC_NOWAIT)) {
			// We are going to sleep
			Thread *thread = thread_get_current_thread();
			queued_thread queueEntry(thread, messageSize);
			messageQueue->Enqueue(&queueEntry, /* waitForMessage */ false);

			uint32 sequenceNumber = messageQueue->SequenceNumber();

			TRACE(("xsi_msgsnd: thread %d going to sleep\n", (int)thread->id));
			result = messageQueue->BlockAndUnlock(thread, &messageQueueLocker);
			TRACE(("xsi_msgsnd: thread %d back to life\n", (int)thread->id));

			messageQueueHashLocker.Lock();
			messageQueue = sMessageQueueHashTable.Lookup(messageQueueID);
			if (result == EIDRM || messageQueue == NULL || (messageQueue != NULL
				&& sequenceNumber != messageQueue->SequenceNumber())) {
				TRACE_ERROR(("xsi_msgsnd: message queue id %d (sequence = %ld) "
					"got destroyed\n", messageQueueID, sequenceNumber));
				delete message;
				notSent = false;
				result = EIDRM;
			} else if (result == B_INTERRUPTED) {
				TRACE_ERROR(("xsi_msgsnd: thread %d got interrupted while "
					"waiting on message queue %d\n",(int)thread->id,
					messageQueueID));
				messageQueue->Deque(&queueEntry, /* waitForMessage */ false);
				delete message;
				notSent = false;
				result = EINTR;
			} else {
				messageQueueLocker.Lock();
				messageQueueHashLocker.Unlock();
			}
		} else if (goToSleep) {
			// We did not send the message and we can't wait
			delete message;
			notSent = false;
			result = EAGAIN;
		} else {
			// Message delivered correctly
			TRACE(("xsi_msgsnd: message sent correctly\n"));
			notSent = false;
		}
	}

	return result;
}
