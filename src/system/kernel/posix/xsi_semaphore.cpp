/*
 * Copyright 2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Salvatore Benedetto <salvatore.benedetto@gmail.com>
 */

#include <posix/xsi_semaphore.h>

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
#include <util/Vector.h>


//#define TRACE_XSI_SEM
#ifdef TRACE_XSI_SEM
#	define TRACE(x)			dprintf x
#	define TRACE_ERROR(x)	dprintf x
#else
#	define TRACE(x)			/* nothing */
#	define TRACE_ERROR(x)	dprintf x
#endif

// Queue for holding blocked threads
struct queued_thread : DoublyLinkedListLinkImpl<queued_thread> {
	queued_thread(struct thread *thread, int32 count)
		:
		thread(thread),
		count(count),
		queued(false)
	{
	}

	struct thread	*thread;
	int32			count;
	bool			queued;
};

typedef DoublyLinkedList<queued_thread> ThreadQueue;

class XsiSemaphoreSet;

struct sem_undo : DoublyLinkedListLinkImpl<sem_undo> {
	sem_undo(XsiSemaphoreSet *semaphoreSet, struct team *team, int16 *undoValues)
		:
		semaphore_set(semaphoreSet),
		team(team),
		undo_values(undoValues)
	{
	}

	DoublyLinkedListLink<sem_undo>		team_link;
	XsiSemaphoreSet						*semaphore_set;
	struct team							*team;
	int16								*undo_values;
};

typedef DoublyLinkedList<sem_undo> UndoList;
typedef DoublyLinkedList<sem_undo,
	DoublyLinkedListMemberGetLink<sem_undo, &sem_undo::team_link> > TeamList;

struct xsi_sem_context {
	xsi_sem_context()
	{
		mutex_init(&lock, "Private team undo_list lock");
	}

	~xsi_sem_context()
	{
		mutex_destroy(&lock);
	}

	TeamList	undo_list;
	mutex		lock;
};

// Xsi semaphore definition
class XsiSemaphore {
public:
	XsiSemaphore()
		: fLastPidOperation(0),
		fThreadsWaitingToIncrease(0),
		fThreadsWaitingToBeZero(0),
		fValue(0)
	{
	}

	~XsiSemaphore()
	{
		// For some reason the semaphore is getting destroyed.
		// Wake up any remaing awaiting threads
		InterruptsSpinLocker _(gThreadSpinlock);
		while (queued_thread *entry = fWaitingToIncreaseQueue.RemoveHead()) {
			entry->queued = false;
			thread_unblock_locked(entry->thread, EIDRM);
		}
		while (queued_thread *entry = fWaitingToBeZeroQueue.RemoveHead()) {
			entry->queued = false;
			thread_unblock_locked(entry->thread, EIDRM);
		}
		// No need to remove any sem_undo request still
		// hanging. When the process exit and doesn't found
		// the semaphore set, it'll just ignore the sem_undo
		// request. That's better than iterating trough the
		// whole sUndoList. Beside we don't know our semaphore
		// number nor our semaphore set id.
	}

	// We return true in case the operation causes the
	// caller to wait, so it can undo all the operations
	// previously done
	bool Add(short value)
	{
		if ((int)(fValue + value) < 0) {
			TRACE(("XsiSemaphore::Add: potentially going to sleep\n"));
			return true;
		} else {
			fValue += value;
			if (fValue == 0 && fThreadsWaitingToBeZero > 0)
				WakeUpThread(true);
			else if (fValue > 0 && fThreadsWaitingToIncrease > 0)
				WakeUpThread(false);
			return false;
		}
	}

	pid_t LastPid() const
	{
		return fLastPidOperation;
	}

	void Revert(short value)
	{
		fValue -= value;
		if (fValue == 0 && fThreadsWaitingToBeZero > 0)
			WakeUpThread(true);
		else if (fValue > 0 && fThreadsWaitingToIncrease > 0)
			WakeUpThread(false);
	}

	void SetPid(pid_t pid)
	{
		fLastPidOperation = pid;
	}

	void SetValue(ushort value)
	{
		fValue = value;
	}

	ushort ThreadsWaitingToIncrease() const
	{
		return fThreadsWaitingToIncrease;
	}

	ushort ThreadsWaitingToBeZero() const
	{
		return fThreadsWaitingToBeZero;
	}

	ushort Value() const
	{
		return fValue;
	}

	status_t Wait(int32 count, bool waitForZero)
	{
		TRACE(("XsiSemaphore::Wait: going to sleep\n"));
		// enqueue the thread in the appropriate
		// queue and get ready to wait
		struct thread *thread = thread_get_current_thread();
		queued_thread queueEntry(thread, count);
		if (waitForZero) {
			fWaitingToBeZeroQueue.Add(&queueEntry);
			fThreadsWaitingToBeZero++;
		} else {
			fWaitingToIncreaseQueue.Add(&queueEntry);
			fThreadsWaitingToIncrease++;
		}
		queueEntry.queued = true;

		thread_prepare_to_block(thread, B_CAN_INTERRUPT,
			THREAD_BLOCK_TYPE_OTHER, (void*)"xsi semaphore");

		InterruptsSpinLocker _(gThreadSpinlock);
		status_t result = thread_block_locked(thread);

		if (queueEntry.queued) {
			// If we are still queued, we failed to acquire
			// the semaphore for some reason
			if (waitForZero) {
				fWaitingToBeZeroQueue.Remove(&queueEntry);
				fThreadsWaitingToBeZero--;
			} else {
				fWaitingToIncreaseQueue.Remove(&queueEntry);
				fThreadsWaitingToIncrease--;
			}
		}
		return result;
	}

	void WakeUpThread(bool waitingForZero)
	{
		InterruptsSpinLocker _(gThreadSpinlock);
		if (waitingForZero) {
			// Wake up all threads waiting on zero
			while (queued_thread *entry = fWaitingToBeZeroQueue.RemoveHead()) {
				entry->queued = false;
				fThreadsWaitingToBeZero--;
				thread_unblock_locked(entry->thread, 0);
			}
		} else {
			// Wake up all threads even though they might go back to sleep
			while (queued_thread *entry = fWaitingToIncreaseQueue.RemoveHead()) {
				entry->queued = false;
				fThreadsWaitingToIncrease--;
				thread_unblock_locked(entry->thread, 0);
			}
		}
	}

private:
	pid_t				fLastPidOperation;				// sempid
	ushort				fThreadsWaitingToIncrease;		// semncnt
	ushort				fThreadsWaitingToBeZero;		// semzcnt
	ushort				fValue;							// semval

	ThreadQueue			fWaitingToIncreaseQueue;
	ThreadQueue			fWaitingToBeZeroQueue;
};

#define MAX_XSI_SEMS_PER_TEAM	128

// Xsi semaphore set definition (semid_ds)
class XsiSemaphoreSet {
public:
	XsiSemaphoreSet(int numberOfSemaphores, int flags)
		: fInitOK(false),
		fLastSemctlTime((time_t)real_time_clock()),
		fLastSemopTime(0),
		fNumberOfSemaphores(numberOfSemaphores),
		fSemaphores(0)
	{
		mutex_init(&fLock, "XsiSemaphoreSet private mutex");
		SetIpcKey((key_t)-1);
		SetPermissions(flags);
		fSemaphores = new(std::nothrow) XsiSemaphore[numberOfSemaphores];
		if (fSemaphores == NULL) {
			TRACE_ERROR(("XsiSemaphoreSet::XsiSemaphore(): failed to allocate "
				"XsiSemaphore object\n"));
		} else
			fInitOK = true;
	}

	~XsiSemaphoreSet()
	{
		TRACE(("XsiSemaphoreSet::~XsiSemaphoreSet(): removing semaphore "
			"set %d\n", fID));
		mutex_destroy(&fLock);
		UnsetID();
		delete[] fSemaphores;
	}

	void ClearUndo(unsigned short semaphoreNumber)
	{
		struct team *team = thread_get_current_thread()->team;
		UndoList::Iterator iterator = fUndoList.GetIterator();
		while (iterator.HasNext()) {
			struct sem_undo *current = iterator.Next();
			if (current->team == team) {
				TRACE(("XsiSemaphoreSet::ClearUndo: teamID = %d, "
					"semaphoreSetID = %d, semaphoreNumber = %d\n",
					fID, semaphoreNumber, (int)team->id));
				MutexLocker _(team->xsi_sem_context->lock);
				current->undo_values[semaphoreNumber] = 0;
				return;
			}
		}
	}

	void ClearUndos()
	{
		// Clear all undo_values (POSIX semadj equivalent)
		// of the calling team. This happens only on semctl SETALL.
		struct team *team = thread_get_current_thread()->team;
		DoublyLinkedList<sem_undo>::Iterator iterator = fUndoList.GetIterator();
		while (iterator.HasNext()) {
			struct sem_undo *current = iterator.Next();
			if (current->team == team) {
				TRACE(("XsiSemaphoreSet::ClearUndos: teamID = %d, "
					"semaphoreSetID = %d\n", (int)team->id, fID));
				MutexLocker _(team->xsi_sem_context->lock);
				memset(current->undo_values, 0,
					sizeof(int16) * fNumberOfSemaphores);
				return;
			}
		}
	}

	void DoIpcSet(struct semid_ds *result)
	{
		fPermissions.uid = result->sem_perm.uid;
		fPermissions.gid = result->sem_perm.gid;
		fPermissions.mode = (fPermissions.mode & ~0x01ff)
			| (result->sem_perm.mode & 0x01ff);
	}

	bool HasPermission() const
	{
		if ((fPermissions.mode & S_IWOTH) != 0)
			return true;

		uid_t uid = geteuid();
		if (uid == 0 || (uid == fPermissions.uid
			&& (fPermissions.mode & S_IWUSR) != 0))
			return true;

		gid_t gid = getegid();
		if (gid == fPermissions.gid && (fPermissions.mode & S_IWGRP) != 0)
			return true;

		return false;
	}

	bool HasReadPermission() const
	{
		// TODO: fix this
		return HasPermission();
	}

	int	ID() const
	{
		return fID;
	}

	bool InitOK()
	{
		return fInitOK;
	}

	key_t IpcKey() const
	{
		return fPermissions.key;
	}

	struct ipc_perm IpcPermission() const
	{
		return fPermissions;
	}

	time_t LastSemctlTime() const
	{
		return fLastSemctlTime;
	}

	time_t LastSemopTime() const
	{
		return fLastSemopTime;
	}

	mutex &Lock()
	{
		return fLock;
	}

	ushort NumberOfSemaphores() const
	{
		return fNumberOfSemaphores;
	}

	// Record the sem_undo operation into our private fUndoList and
	// the team undo_list. The only limit here is the memory needed
	// for creating a new sem_undo structure.
	int RecordUndo(short semaphoreNumber, short value)
	{
		// Look if there is already a record from the team caller
		// for the same semaphore set
		bool notFound = true;
		struct team *team = thread_get_current_thread()->team;
		DoublyLinkedList<sem_undo>::Iterator iterator = fUndoList.GetIterator();
		while (iterator.HasNext()) {
			struct sem_undo *current = iterator.Next();
			if (current->team == team) {
				// Update its undo value
				MutexLocker _(team->xsi_sem_context->lock);
				int newValue = current->undo_values[semaphoreNumber] + value;
				if (newValue > USHRT_MAX || newValue < -USHRT_MAX) {
					TRACE_ERROR(("XsiSemaphoreSet::RecordUndo: newValue %d "
						"out of range\n", newValue));
					return ERANGE;
				}
				current->undo_values[semaphoreNumber] = newValue;
				notFound = false;
				TRACE(("XsiSemaphoreSet::RecordUndo: found record. Team = %d, "
					"semaphoreSetID = %d, semaphoreNumber = %d, value = %d\n",
					(int)team->id, fID, semaphoreNumber,
					current->undo_values[semaphoreNumber]));
				break;
			}
		}

		if (notFound) {
			// First sem_undo request from this team for this
			// semaphore set
			int16 *undoValues
				= (int16 *)malloc(sizeof(int16) * fNumberOfSemaphores);
			if (undoValues == NULL)
				return B_NO_MEMORY;
			struct sem_undo *request
				= new(std::nothrow) sem_undo(this, team, undoValues);
			if (request == NULL) {
				free(undoValues);
				return B_NO_MEMORY;
			}
			memset(request->undo_values, 0, sizeof(int16) * fNumberOfSemaphores);
			request->undo_values[semaphoreNumber] = value;

			// Check if it's the very first sem_undo request for this team
			xsi_sem_context *context = atomic_pointer_get(&team->xsi_sem_context);
			if (context == NULL) {
				// Create the context
				context = new(std::nothrow) xsi_sem_context;
				if (context == NULL) {
					free(request->undo_values);
					delete request;
					return B_NO_MEMORY;
				}
				// Since we don't hold any global lock, someone
				// else could have been quicker than us, so we have
				// to delete the one we just created and use the one
				// in place.
				if (atomic_pointer_test_and_set(&team->xsi_sem_context, context,
					(xsi_sem_context *)NULL) != NULL)
					delete context;
			}

			// Add the request to both XsiSemaphoreSet and team list
			fUndoList.Add(request);
			MutexLocker _(team->xsi_sem_context->lock);
			team->xsi_sem_context->undo_list.Add(request);
			TRACE(("XsiSemaphoreSet::RecordUndo: new record added. Team = %d, "
				"semaphoreSetID = %d, semaphoreNumber = %d, value = %d\n",
				(int)team->id, fID, semaphoreNumber, value));
		}
		return B_OK;
	}

	void RevertUndo(short semaphoreNumber, short value)
	{
		// This can be called only when RecordUndo fails.
		struct team *team = thread_get_current_thread()->team;
		DoublyLinkedList<sem_undo>::Iterator iterator = fUndoList.GetIterator();
		while (iterator.HasNext()) {
			struct sem_undo *current = iterator.Next();
			if (current->team == team) {
				MutexLocker _(team->xsi_sem_context->lock);
				fSemaphores[semaphoreNumber].Revert(value);
				break;
			}
		}
	}

	XsiSemaphore* Semaphore(int nth) const
	{
		return &fSemaphores[nth];
	}

	// Implemented after sSemaphoreHashTable is declared
	void SetID();

	void SetIpcKey(key_t key)
	{
		fPermissions.key = key;
	}

	void SetLastSemctlTime()
	{
		fLastSemctlTime = real_time_clock();
	}

	void SetLastSemopTime()
	{
		fLastSemopTime = real_time_clock();
	}

	void SetPermissions(int flags)
	{
		fPermissions.uid = fPermissions.cuid = geteuid();
		fPermissions.gid = fPermissions.cgid = getegid();
		fPermissions.mode = (flags & 0x01ff);
	}

	UndoList &GetUndoList()
	{
		return fUndoList;
	}

	// Implemented after sSemaphoreHashTable is declared
	void UnsetID();

	HashTableLink<XsiSemaphoreSet>* Link()
	{
		return &fLink;
	}

private:
	bool						fInitOK;
	int							fID;					// semaphore set id
	time_t						fLastSemctlTime;		// sem_ctime
	time_t						fLastSemopTime;			// sem_otime
	ushort						fNumberOfSemaphores;	// sem_nsems
	struct ipc_perm				fPermissions;			// sem_perm
	XsiSemaphore				*fSemaphores;			// array of semaphores
	UndoList					fUndoList;				// undo list requests
	mutex 						fLock;					// private lock

	::HashTableLink<XsiSemaphoreSet> fLink;
};

// Xsi semaphore set hash table
struct SemaphoreHashTableDefinition {
	typedef int					KeyType;
	typedef XsiSemaphoreSet		ValueType;

	size_t HashKey (const int key) const
	{
		return (size_t)key;
	}

	size_t Hash(XsiSemaphoreSet *variable) const
	{
		return (size_t)variable->ID();
	}

	bool Compare(const int key, XsiSemaphoreSet *variable) const
	{
		return (int)key == (int)variable->ID();
	}

	HashTableLink<XsiSemaphoreSet>* GetLink(XsiSemaphoreSet *variable) const
	{
		return variable->Link();
	}
};


// IPC class
class Ipc {
public:
	Ipc(key_t key)
		: fKey(key),
		fSemaphoreSetId(-1)
	{
	}

	key_t Key() const
	{
		return fKey;
	}

	int SemaphoreSetID() const
	{
		return fSemaphoreSetId;
	}

	void SetSemaphoreSetID(XsiSemaphoreSet *semaphoreSet)
	{
		fSemaphoreSetId = semaphoreSet->ID();
	}

	bool HasSemaphoreSet()
	{
		if (fSemaphoreSetId != -1)
			return true;
		return false;
	}

	HashTableLink<Ipc>* Link()
	{
		return &fLink;
	}

private:
	key_t				fKey;
	int					fSemaphoreSetId;
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
#define MAX_XSI_SEMAPHORE		512
static OpenHashTable<IpcHashTableDefinition> sIpcHashTable;
static OpenHashTable<SemaphoreHashTableDefinition> sSemaphoreHashTable;

static mutex sIpcLock;
static mutex sXsiSemaphoreSetLock;

static vint32 sNextAvailableID = 1;
static vint32 sXsiSemaphoreCount = 0;


//	#pragma mark -


void
XsiSemaphoreSet::SetID()
{
	// The lock is held before calling us
	while (true) {
		if (sSemaphoreHashTable.Lookup(sNextAvailableID) == NULL)
			break;
		sNextAvailableID++;
	}
	fID = sNextAvailableID++;
}


void
XsiSemaphoreSet::UnsetID()
{
	sNextAvailableID = fID;
}


//	#pragma mark - Kernel exported API


void
xsi_ipc_init()
{
	// Initialize hash tables
	status_t status = sIpcHashTable.Init();
	if (status != B_OK)
		panic("xsi_ipc_init() failed to initialized ipc hash table\n");
	status =  sSemaphoreHashTable.Init();
	if (status != B_OK)
		panic("xsi_ipc_init() failed to initialized semaphore hash table\n");

	mutex_init(&sIpcLock, "global Posix IPC table");
	mutex_init(&sXsiSemaphoreSetLock, "global Posix xsi sem table");
}


/*!	Function called on team exit to process any sem_undo requests */
void
xsi_sem_undo(struct team *team)
{
	if (team->xsi_sem_context == NULL)
		return;

	// By acquiring first the semaphore hash table lock
	// we make sure the semaphore set in our sem_undo
	// list won't get removed by IPC_RMID call
	MutexLocker _(sXsiSemaphoreSetLock);

	// Process all sem_undo request in the team sem undo list
	// if any
	TeamList::Iterator iterator
		= team->xsi_sem_context->undo_list.GetIterator();
	while (iterator.HasNext()) {
		struct sem_undo *current = iterator.Next();
		XsiSemaphoreSet *semaphoreSet = current->semaphore_set;
		// Acquire the set lock in order to prevent race
		// condition with RecordUndo
		MutexLocker setLocker(semaphoreSet->Lock());
		MutexLocker _(team->xsi_sem_context->lock);
		// Revert the changes done by this process
		for (int i = 0; i < semaphoreSet->NumberOfSemaphores(); i++)
			if (current->undo_values[i] != 0) {
				TRACE(("xsi_sem_undo: TeamID = %d, SemaphoreSetID = %d, "
					"SemaphoreNumber = %d, undo value = %d\n", (int)team->id,
					semaphoreSet->ID(), i, (int)current->undo_values[i]));
				semaphoreSet->Semaphore(i)->Revert(current->undo_values[i]);
			}

		// Remove and free the sem_undo structure from both lists
		iterator.Remove();
		semaphoreSet->GetUndoList().Remove(current);
		delete current;
	}
	delete team->xsi_sem_context;
	team->xsi_sem_context = NULL;
}


//	#pragma mark - Syscalls


int
_user_xsi_semget(key_t key, int numberOfSemaphores, int flags)
{
	TRACE(("xsi_semget: key = %d, numberOfSemaphores = %d, flags = %d\n",
		(int)key, numberOfSemaphores, flags));
	XsiSemaphoreSet *semaphoreSet = NULL;
	Ipc *ipcKey = NULL;
	// Default assumptions
	bool isPrivate = true;
	bool create = true;

	MutexLocker _(sIpcLock);
	if (key != IPC_PRIVATE) {
		isPrivate = false;
		// Check if key already has a semaphore associated with it
		ipcKey = sIpcHashTable.Lookup(key);
		if (ipcKey == NULL) {
			// The ipc key have probably just been created
			// by the caller, add it to the system
			if (!(flags & IPC_CREAT)) {
				TRACE_ERROR(("xsi_semget: key %d does not exist, but the "
					"caller did not ask for creation\n",(int)key));
				return ENOENT;
			}
			ipcKey = new(std::nothrow) Ipc(key);
			if (ipcKey == NULL) {
				TRACE_ERROR(("xsi_semget: failed to create new Ipc object "
					"for key %d\n",	(int)key));
				return ENOMEM;
			}
			sIpcHashTable.Insert(ipcKey);
		} else if (ipcKey->HasSemaphoreSet()) {
			// The IPC key exist and it already has a semaphore
			if ((flags & IPC_CREAT) && (flags & IPC_EXCL)) {
				TRACE_ERROR(("xsi_semget: key %d already exist\n", (int)key));
				return EEXIST;
			}
			int semaphoreSetID = ipcKey->SemaphoreSetID();

			MutexLocker _(sXsiSemaphoreSetLock);
			semaphoreSet = sSemaphoreHashTable.Lookup(semaphoreSetID);
			if (!semaphoreSet->HasPermission()) {
				TRACE_ERROR(("xsi_semget: calling process has not permission "
					"on semaphore %d, key %d\n", semaphoreSet->ID(),
					(int)semaphoreSet->IpcKey()));
				return EACCES;
			}
			if (numberOfSemaphores > semaphoreSet->NumberOfSemaphores()
				&& numberOfSemaphores != 0) {
				TRACE_ERROR(("xsi_semget: numberOfSemaphores greater than the "
					"one associated with semaphore %d, key %d\n",
					semaphoreSet->ID(), (int)semaphoreSet->IpcKey()));
				return EINVAL;
			}
			create = false;
		} else {
			// The IPC key exist but it has not semaphore associated with it
			if (!(flags & IPC_CREAT)) {
				TRACE_ERROR(("xsi_semget: key %d has not semaphore associated "
					"with it and caller did not ask for creation\n",(int)key));
				return ENOENT;
			}
		}
	} else {
		// TODO: Handle case of private key
	}

	if (create) {
		// Create a new sempahore set for this key
		if (numberOfSemaphores <= 0
			|| numberOfSemaphores >= MAX_XSI_SEMS_PER_TEAM) {
			TRACE_ERROR(("xsi_semget: numberOfSemaphores out of range\n"));
			return EINVAL;
		}
		if (sXsiSemaphoreCount >= MAX_XSI_SEMAPHORE) {
			TRACE_ERROR(("xsi_semget: reached limit of maximum number of "
				"semaphores allowed\n"));
			return ENOSPC;
		}

		semaphoreSet = new(std::nothrow) XsiSemaphoreSet(numberOfSemaphores,
			flags);
		if (semaphoreSet == NULL || !semaphoreSet->InitOK()) {
			TRACE_ERROR(("xsi_semget: failed to allocate a new xsi "
				"semaphore set\n"));
			return ENOMEM;
		}
		atomic_add(&sXsiSemaphoreCount, numberOfSemaphores);

		MutexLocker _(sXsiSemaphoreSetLock);
		semaphoreSet->SetID();
		if (isPrivate)
			semaphoreSet->SetIpcKey((key_t)-1);
		else {
			semaphoreSet->SetIpcKey(key);
			ipcKey->SetSemaphoreSetID(semaphoreSet);
		}
		sSemaphoreHashTable.Insert(semaphoreSet);
	}

	return semaphoreSet->ID();
}


int
_user_xsi_semctl(int semaphoreID, int semaphoreNumber, int command,
	union semun *args)
{
	TRACE(("xsi_semctl: semaphoreID = %d, semaphoreNumber = %d, command = %d\n",
		semaphoreID, semaphoreNumber, command));
	MutexLocker ipcHashLocker(sIpcLock);
	MutexLocker setHashLocker(sXsiSemaphoreSetLock);
	XsiSemaphoreSet *semaphoreSet = sSemaphoreHashTable.Lookup(semaphoreID);
	if (semaphoreSet == NULL) {
		TRACE_ERROR(("xsi_semctl: semaphore set id %d not valid\n",
			semaphoreID));
		return EINVAL;
	}
	if (semaphoreNumber < 0
		|| semaphoreNumber > semaphoreSet->NumberOfSemaphores()) {
		TRACE_ERROR(("xsi_semctl: semaphore number %d not valid for "
			"semaphore %d\n", semaphoreNumber, semaphoreID));
		return EINVAL;
	}
	if (args != 0 && !IS_USER_ADDRESS(args)) {
		TRACE_ERROR(("xsi_semctl: semun address is not valid\n"));
		return B_BAD_ADDRESS;
	}

	// Lock the semaphore set itself and release both the semaphore
	// set hash table lock and the ipc hash table lock _only_ if 
	// the command it's not IPC_RMID, this prevents undesidered 
	// situation from happening while (hopefully) improving the
	// concurrency.
	MutexLocker setLocker;
	if (command != IPC_RMID) {
		setLocker.SetTo(&semaphoreSet->Lock(), false);
		setHashLocker.Unlock();
		ipcHashLocker.Unlock();
	}

	int result = 0;
	XsiSemaphore *semaphore = semaphoreSet->Semaphore(semaphoreNumber);
	switch (command) {
		case GETVAL: {
			if (!semaphoreSet->HasReadPermission()) {
				TRACE_ERROR(("xsi_semctl: calling process has not permission "
					"on semaphore %d, key %d\n", semaphoreSet->ID(),
					(int)semaphoreSet->IpcKey()));
				result = EACCES;
			} else
				result = semaphore->Value();
			break;
		}

		case SETVAL: {
			if (!semaphoreSet->HasPermission()) {
				TRACE_ERROR(("xsi_semctl: calling process has not permission "
					"on semaphore %d, key %d\n", semaphoreSet->ID(),
					(int)semaphoreSet->IpcKey()));
				result = EACCES;
			} else {
				int value;
				if (user_memcpy(&value, &args->val, sizeof(int)) < B_OK) {
					TRACE_ERROR(("xsi_semctl: user_memcpy failed\n"));
					result = B_BAD_ADDRESS;
				} else if (value > USHRT_MAX) {
					TRACE_ERROR(("xsi_semctl: value %d out of range\n", value));
					result = ERANGE;
				} else {
					semaphore->SetValue(value);
					semaphoreSet->ClearUndo(semaphoreNumber);
				}
			}
			break;
		}

		case GETPID: {
			if (!semaphoreSet->HasReadPermission()) {
				TRACE_ERROR(("xsi_semctl: calling process has not permission "
					"on semaphore %d, key %d\n", semaphoreSet->ID(),
					(int)semaphoreSet->IpcKey()));
				result = EACCES;
			} else
				result = semaphore->LastPid();
			break;
		}

		case GETNCNT: {
			if (!semaphoreSet->HasReadPermission()) {
				TRACE_ERROR(("xsi_semctl: calling process has not permission "
					"on semaphore %d, key %d\n", semaphoreSet->ID(),
					(int)semaphoreSet->IpcKey()));
				result = EACCES;
			} else
				result = semaphore->ThreadsWaitingToIncrease();
			break;
		}

		case GETZCNT: {
			if (!semaphoreSet->HasReadPermission()) {
				TRACE_ERROR(("xsi_semctl: calling process has not permission "
					"on semaphore %d, key %d\n", semaphoreSet->ID(),
					(int)semaphoreSet->IpcKey()));
				result = EACCES;
			} else
				result = semaphore->ThreadsWaitingToBeZero();
			break;
		}

		case GETALL: {
			if (!semaphoreSet->HasReadPermission()) {
				TRACE_ERROR(("xsi_semctl: calling process has not read "
					"permission on semaphore %d, key %d\n", semaphoreSet->ID(),
					(int)semaphoreSet->IpcKey()));
				result = EACCES;
			} else
				for (int i = 0; i < semaphoreSet->NumberOfSemaphores(); i++) {
					semaphore = semaphoreSet->Semaphore(i);
					unsigned short value = semaphore->Value();
					if (user_memcpy(&args->array[i], &value,
						sizeof(unsigned short)) < B_OK) {
						TRACE_ERROR(("xsi_semctl: user_memcpy failed\n"));
						result = B_BAD_ADDRESS;
						break;
					}
				}
			break;
		}

		case SETALL: {
			if (!semaphoreSet->HasPermission()) {
				TRACE_ERROR(("xsi_semctl: calling process has not permission "
					"on semaphore %d, key %d\n", semaphoreSet->ID(),
					(int)semaphoreSet->IpcKey()));
				result = EACCES;
			} else {
				bool doClear = true;
				for (int i = 0; i < semaphoreSet->NumberOfSemaphores(); i++) {
					semaphore = semaphoreSet->Semaphore(i);
					unsigned short value;
					if (user_memcpy(&value, &args->array[i], sizeof(unsigned short))
						< B_OK) {
						TRACE_ERROR(("xsi_semctl: user_memcpy failed\n"));
						result = B_BAD_ADDRESS;
						doClear = false;
						break;
					} else
						semaphore->SetValue(value);
				}
				if (doClear)
					semaphoreSet->ClearUndos();
			}
			break;
		}

		case IPC_STAT: {
			if (!semaphoreSet->HasReadPermission()) {
				TRACE_ERROR(("xsi_semctl: calling process has not read "
					"permission on semaphore %d, key %d\n", semaphoreSet->ID(),
					(int)semaphoreSet->IpcKey()));
				result = EACCES;
			} else {
				struct semid_ds sem;
				sem.sem_perm = semaphoreSet->IpcPermission();
				sem.sem_nsems = semaphoreSet->NumberOfSemaphores();
				sem.sem_otime = semaphoreSet->LastSemopTime();
				sem.sem_ctime = semaphoreSet->LastSemctlTime();
				if (user_memcpy(args->buf, &sem, sizeof(struct semid_ds))
					< B_OK) {
					TRACE_ERROR(("xsi_semctl: user_memcpy failed\n"));
					result = B_BAD_ADDRESS;
				}
			}
			break;
		}

		case IPC_SET: {
			if (!semaphoreSet->HasPermission()) {
				TRACE_ERROR(("xsi_semctl: calling process has not "
					"permission on semaphore %d, key %d\n",
					semaphoreSet->ID(),	(int)semaphoreSet->IpcKey()));
				result = EACCES;
			} else {
				struct semid_ds sem;
				if (user_memcpy(&sem, args->buf, sizeof(struct semid_ds))
					< B_OK) {
					TRACE_ERROR(("xsi_semctl: user_memcpy failed\n"));
					result = B_BAD_ADDRESS;
				} else
					semaphoreSet->DoIpcSet(&sem);
			}
			break;
		}

		case IPC_RMID: {
			// If this was the command, we are still holding
			// the semaphore set hash table lock along with the
			// ipc hash table lock, but not the semaphore set
			// itself lock as it would be useless in this case
			if (!semaphoreSet->HasPermission()) {
				TRACE_ERROR(("xsi_semctl: calling process has not "
					"permission on semaphore %d, key %d\n",
					semaphoreSet->ID(),	(int)semaphoreSet->IpcKey()));
				return EACCES;
			}
			key_t key = semaphoreSet->IpcKey();
			Ipc *ipcKey = NULL;
			if (key != -1) {
				ipcKey = sIpcHashTable.Lookup(key);
				sIpcHashTable.Remove(ipcKey);
			}
			sSemaphoreHashTable.Remove(semaphoreSet);
			// Wake up of threads waiting on this set
			// happens in the destructor
			if (key != -1)
				delete ipcKey;
			atomic_add(&sXsiSemaphoreCount, -semaphoreSet->NumberOfSemaphores());
			// Remove any sem_undo request
			while (struct sem_undo *entry
					= semaphoreSet->GetUndoList().RemoveHead()) {
				MutexLocker _(entry->team->xsi_sem_context->lock);
				entry->team->xsi_sem_context->undo_list.Remove(entry);
				delete entry;
			}

			delete semaphoreSet;
			return 0;
		}

		default:
			TRACE_ERROR(("xsi_semctl: command %d not valid\n", command));
			result = EINVAL;
	}

	return result;
}


status_t
_user_xsi_semop(int semaphoreID, struct sembuf *ops, size_t numOps)
{
	TRACE(("xsi_semop: semaphoreID = %d, ops = %p, numOps = %ld\n",
		semaphoreID, ops, numOps));
	MutexLocker setHashLocker(sXsiSemaphoreSetLock);
	XsiSemaphoreSet *semaphoreSet = sSemaphoreHashTable.Lookup(semaphoreID);
	if (semaphoreSet == NULL) {
		TRACE_ERROR(("xsi_semop: semaphore set id %d not valid\n",
			semaphoreID));
		return EINVAL;
	}
	MutexLocker setLocker(semaphoreSet->Lock());
	setHashLocker.Unlock();

	if (!IS_USER_ADDRESS(ops)) {
		TRACE_ERROR(("xsi_semop: sembuf address is not valid\n"));
		return B_BAD_ADDRESS;
	}

	if (numOps < 0 || numOps >= MAX_XSI_SEMS_PER_TEAM) {
		TRACE_ERROR(("xsi_semop: numOps out of range\n"));
		return EINVAL;
	}

	struct sembuf *operations
		= (struct sembuf *)malloc(sizeof(struct sembuf) * numOps);
	if (operations == NULL) {
		TRACE_ERROR(("xsi_semop: failed to allocate sembuf struct\n"));
		return B_NO_MEMORY;
	}

	if (user_memcpy(operations, ops,
		(sizeof(struct sembuf) * numOps)) < B_OK) {
		TRACE_ERROR(("xsi_semop: user_memcpy failed\n"));
		free(operations);
		return B_BAD_ADDRESS;
	}

	// We won't do partial request, that is operations
	// only on some sempahores belonging to the set and then
	// going to sleep. If we must wait on a semaphore, we undo
	// all the operations already done and go to sleep, otherwise
	// we may caused some unwanted deadlock among threads
	// fighting for the same set.
	bool notDone = true;
	status_t result = 0;
	while (notDone) {
		XsiSemaphore *semaphore = NULL;
		short numberOfSemaphores = semaphoreSet->NumberOfSemaphores();
		bool goToSleep = false;

		uint32 i = 0;
		for (; i < numOps; i++) {
			short semaphoreNumber = operations[i].sem_num;
			if (semaphoreNumber >= numberOfSemaphores) {
				TRACE_ERROR(("xsi_semop: %ld invalid semaphore number\n", i));
				result = EINVAL;
				break;
			}
			semaphore = semaphoreSet->Semaphore(semaphoreNumber);
			unsigned short value = semaphore->Value();
			short operation = operations[i].sem_op;
			TRACE(("xsi_semop: semaphoreNumber = %d, value = %d\n",
				semaphoreNumber, value));
			if (operation < 0) {
				if (semaphore->Add(operation)) {
					if (operations[i].sem_flg & IPC_NOWAIT)
						result = EAGAIN;
					else
						goToSleep = true;
					break;
				}
			} else if (operation == 0) {
				if (value == 0)
					continue;
				else if (operations[i].sem_flg & IPC_NOWAIT) {
					result = EAGAIN;
					break;
				} else {
					goToSleep = true;
					break;
				}
			} else {
				// Operation must be greater than zero,
				// just add the value and continue
				semaphore->Add(operation);
			}
		}

		// Either we have to wait or an error occured
		if (goToSleep || result != 0) {
			// Undo all previously done operations
			for (uint32 j = 0; j < i; j++) {
				short semaphoreNumber = operations[j].sem_num;
				semaphore = semaphoreSet->Semaphore(semaphoreNumber);
				short operation = operations[j].sem_op;
				if (operation != 0)
					semaphore->Revert(operation);
			}
			if (result != 0)
				return result;

			bool waitOnZero = true;
			if (operations[i].sem_op != 0)
				waitOnZero = false;

			setLocker.Unlock();
			result = semaphore->Wait((int32)operations[i].sem_op, waitOnZero);
			TRACE(("xsi_semop: back to life\n"));

			// We are back to life.
			// Find out why!
			setHashLocker.Lock();
			semaphoreSet = sSemaphoreHashTable.Lookup(semaphoreID);
			if (result == EIDRM || result == B_INTERRUPTED
				|| semaphoreSet == NULL) {
				TRACE_ERROR(("xsi_semop: semaphore set id %d got destroyed\n",
					semaphoreID));
				result = EIDRM;
				notDone = false;
			} else {
				setLocker.Lock();
				setHashLocker.Unlock();
			}
		} else {
			// everything worked like a charm (so far)
			notDone = false;
			TRACE(("xsi_semop: semaphore acquired succesfully\n"));
			// We acquired the semaphore, now records the sem_undo
			// requests
			XsiSemaphore *semaphore = NULL;
			uint32 i = 0;
			for (; i < numOps; i++) {
				short semaphoreNumber = operations[i].sem_num;
				semaphore = semaphoreSet->Semaphore(semaphoreNumber);
				short operation = operations[i].sem_op;
				if (operations[i].sem_flg & SEM_UNDO)
					if (semaphoreSet->RecordUndo(semaphoreNumber, operation)
						!= B_OK) {
						// Unlikely scenario, but we might get here.
						// Undo everything!
						// Start with semaphore operations
						for (uint32 j = 0; j < numOps; j++) {
							short semaphoreNumber = operations[j].sem_num;
							semaphore = semaphoreSet->Semaphore(semaphoreNumber);
							short operation = operations[j].sem_op;
							if (operation != 0)
								semaphore->Revert(operation);
						}
						// Remove all previously registered sem_undo request
						for (uint32 j = 0; j < i; j++) {
							if (operations[j].sem_flg & SEM_UNDO)
								semaphoreSet->RevertUndo(operations[j].sem_num,
									operations[j].sem_op);
						}
						result = ENOSPC;
					}
			}
		}
	}

	// We did it. Set the pid of all semaphores used
	if (result == 0) {
		for (uint32 i = 0; i < numOps; i++) {
			short semaphoreNumber = operations[i].sem_num;
			XsiSemaphore *semaphore = semaphoreSet->Semaphore(semaphoreNumber);
			semaphore->SetPid(getpid());
		}
	}
	return result;
}
