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

class XsiMessageQueue {
public:
	XsiMessageQueue(int flags)
	{
		mutex_init(&fLock, "XsiMessageQueue private mutex");
		SetIpcKey((key_t)-1);
		SetPermissions(flags);
	}

	~XsiMessageQueue()
	{
		mutex_destroy(&fLock);
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

	int ID() const
	{
		return fID;
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

	bool HasMessageQueue()
	{
		if (fMessageQueueId != -1)
			return true;
		return false;
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
	// TODO
	return B_ERROR;
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
