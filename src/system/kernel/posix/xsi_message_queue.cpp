/*
 * Copyright 2008, Haiku Inc. All rights reserved.
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


//#define TRACE_XSI_MSG_QUEUE
#ifdef TRACE_XSI_MSG_QUEUE
#	define TRACE(x)			dprintf x
#	define TRACE_ERROR(x)	dprintf x
#else
#	define TRACE(x)			/* nothing */
#	define TRACE_ERROR(x)	dprintf x
#endif

// Queue for holding blocked threads
struct queued_thread : DoublyLinkedListLinkImpl<queued_thread> {
	queued_thread(struct thread *_thread, int32 _message_length)
		:
		thread(_thread),
		message_length(_message_length),
		queued(false)
	{
	}

	struct thread	*thread;
	int32			message_length;
	bool			queued;
};

typedef DoublyLinkedList<queued_thread> ThreadQueue;


struct queued_message : DoublyLinkedListLinkImpl<queued_message> {
	queued_message(long type, char *_message, ssize_t length)
		:
		init(false),
		length(length),
		type(type)
	{
		message = (char *)malloc(sizeof(char) * length);
		if (message)
			init = true;
		memcpy(_message, message, length);
	}

	~queued_message()
	{
		if (init)
			free(message);
	}

	bool		init;
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
	{
		mutex_init(&fLock, "XsiMessageQueue private mutex");
		SetIpcKey((key_t)-1);
		SetPermissions(flags);
		// Initialize all fields to zero
		memset((void *)&fMessageQueue, 0, sizeof(struct msqid_ds));
		fMessageQueue.msg_ctime = (time_t)real_time_clock();
		fMessageQueue.msg_qbytes = MAX_BYTES_PER_QUEUE;
	}

	~XsiMessageQueue()
	{
		mutex_destroy(&fLock);
		UnsetID();
		// TODO: free up all messages
	}

	void DoIpcSet(struct msqid_ds *result)
	{
		fMessageQueue.msg_perm.uid = result->msg_perm.uid;
		fMessageQueue.msg_perm.gid = result->msg_perm.gid;
		fMessageQueue.msg_perm.mode = (fMessageQueue.msg_perm.mode & ~0x01ff)
			| (result->msg_perm.mode & 0x01ff);
		fMessageQueue.msg_ctime = (time_t)real_time_clock();
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

	struct msqid_ds &GetMessageQueue()
	{
		return fMessageQueue;
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

	// Implemented after sMessageQueueHashTable is declared
	void UnsetID();

	HashTableLink<XsiMessageQueue>* Link()
	{
		return &fLink;
	}

private:
	int					fID;
	mutex				fLock;
	MessageQueue		fMessage;
	struct msqid_ds		fMessageQueue;
	ThreadQueue			fThreadWaitingToSend;
	ThreadQueue			fThreadWaitingToReceive;

	::HashTableLink<XsiMessageQueue> fLink;
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

	HashTableLink<XsiMessageQueue>* GetLink(XsiMessageQueue *variable) const
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

	HashTableLink<Ipc>* Link()
	{
		return &fLink;
	}

private:
	key_t				fKey;
	int					fMessageQueueId;
	HashTableLink<Ipc>	fLink;
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

	HashTableLink<Ipc>* GetLink(Ipc *variable) const
	{
		return variable->Link();
	}
};

// Arbitrary limit
#define MAX_XSI_MESSAGE_QUEUE	2048
static OpenHashTable<IpcHashTableDefinition> sIpcHashTable;
static OpenHashTable<MessageQueueHashTableDefinition> sMessageQueueHashTable;

static mutex sIpcLock;
static mutex sXsiMessageQueueLock;

static vint32 sNextAvailableID = 1;
static vint32 sXsiMessageQueueCount = 0;


//	#pragma mark -

void
XsiMessageQueue::SetID()
{
	// The lock is held before calling us
	while (true) {
		if (sMessageQueueHashTable.Lookup(sNextAvailableID) == NULL)
			break;
		sNextAvailableID++;
	}
	fID = sNextAvailableID++;
}

void
XsiMessageQueue::UnsetID()
{
	sNextAvailableID = fID;
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


int _user_xsi_msgctl(int messageQueueID, int command, struct msqid_ds *buffer)
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
	}

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


int _user_xsi_msgget(key_t key, int flags)
{
	TRACE(("xsi_msgget: key = %d, flags = %d\n", (nt)key, flags));
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
			sIpcHashTable.Lookup(key);
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


ssize_t _user_xsi_msgrcv(int messageQueueID, void *messagePointer,
	size_t messageSize, long messageType, int messageFlags)
{
	// TODO
	return B_ERROR;
}


int _user_xsi_msgsnd(int messageQueueID, const void *messagePointer,
	size_t messageSize, int messageFlags)
{
	// TODO
	return B_ERROR;
}
