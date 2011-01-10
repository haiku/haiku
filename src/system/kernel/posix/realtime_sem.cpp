/*
 * Copyright 2008-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <posix/realtime_sem.h>

#include <string.h>

#include <new>

#include <OS.h>

#include <AutoDeleter.h>
#include <fs/KPath.h>
#include <kernel.h>
#include <lock.h>
#include <syscall_restart.h>
#include <team.h>
#include <thread.h>
#include <util/atomic.h>
#include <util/AutoLock.h>
#include <util/khash.h>
#include <util/OpenHashTable.h>


class SemInfo {
public:
	SemInfo()
		:
		fSemaphoreID(-1)
	{
	}

	virtual ~SemInfo()
	{
		if (fSemaphoreID >= 0)
			delete_sem(fSemaphoreID);
	}

	sem_id SemaphoreID() const			{ return fSemaphoreID; }

	status_t Init(int32 semCount, const char* name)
	{
		fSemaphoreID = create_sem(semCount, name);
		if (fSemaphoreID < 0)
			return fSemaphoreID;

		return B_OK;
	}

	virtual sem_id ID() const = 0;
	virtual SemInfo* Clone() = 0;
	virtual void Delete() = 0;

private:
	sem_id	fSemaphoreID;
};


class NamedSem : public SemInfo {
public:
	NamedSem()
		:
		fName(NULL),
		fRefCount(1)
	{
	}

	virtual ~NamedSem()
	{
		free(fName);
	}

	const char* Name() const		{ return fName; }

	status_t Init(const char* name, mode_t mode, int32 semCount)
	{
		status_t error = SemInfo::Init(semCount, name);
		if (error != B_OK)
			return error;

		fName = strdup(name);
		if (fName == NULL)
			return B_NO_MEMORY;

		fUID = geteuid();
		fGID = getegid();
		fPermissions = mode;

		return B_OK;
	}

	void AcquireReference()
	{
		atomic_add(&fRefCount, 1);
	}

	void ReleaseReference()
	{
		if (atomic_add(&fRefCount, -1) == 1)
			delete this;
	}

	bool HasPermissions() const
	{
		if ((fPermissions & S_IWOTH) != 0)
			return true;

		uid_t uid = geteuid();
		if (uid == 0 || (uid == fUID && (fPermissions & S_IWUSR) != 0))
			return true;

		gid_t gid = getegid();
		if (gid == fGID && (fPermissions & S_IWGRP) != 0)
			return true;

		return false;
	}

	virtual sem_id ID() const
	{
		return SemaphoreID();
	}

	virtual SemInfo* Clone()
	{
		AcquireReference();
		return this;
	}

	virtual void Delete()
	{
		ReleaseReference();
	}

	NamedSem*& HashLink()
	{
		return fHashLink;
	}

private:
	char*		fName;
	vint32		fRefCount;
	uid_t		fUID;
	gid_t		fGID;
	mode_t		fPermissions;

	NamedSem*	fHashLink;
};


class UnnamedSem : public SemInfo {
public:
	UnnamedSem()
		:
		fID(0)
	{
	}

	virtual ~UnnamedSem()
	{
	}

	status_t Init(int32 semCount, const char* name)
	{
		return SemInfo::Init(semCount, name);
	}

	void SetID(sem_id id)
	{
		fID = id;
	}

	virtual sem_id ID() const
	{
		return fID;
	}

	virtual SemInfo* Clone()
	{
		sem_info info;
		if (get_sem_info(SemaphoreID(), &info) != B_OK)
			return NULL;

		UnnamedSem* clone = new(std::nothrow) UnnamedSem;
		if (clone == NULL)
			return NULL;

		if (clone->Init(info.count, info.name) != B_OK) {
			delete clone;
			return NULL;
		}

		clone->SetID(fID);

		return clone;
	}

	virtual void Delete()
	{
		delete this;
	}

private:
	sem_id	fID;
};


class UnnamedSharedSem : public SemInfo {
public:
	UnnamedSharedSem()
	{
	}

	virtual ~UnnamedSharedSem()
	{
	}

	status_t Init(int32 semCount, const char* name)
	{
		return SemInfo::Init(semCount, name);
	}

	virtual sem_id ID() const
	{
		return SemaphoreID();
	}

	virtual SemInfo* Clone()
	{
		// Can't be cloned.
		return NULL;
	}

	virtual void Delete()
	{
		delete this;
	}

	UnnamedSharedSem*& HashLink()
	{
		return fHashLink;
	}

private:
	UnnamedSharedSem*	fHashLink;
};


struct NamedSemHashDefinition {
	typedef const char*	KeyType;
	typedef NamedSem	ValueType;

	size_t HashKey(const KeyType& key) const
	{
		return hash_hash_string(key);
	}

	size_t Hash(NamedSem* semaphore) const
	{
		return HashKey(semaphore->Name());
	}

	bool Compare(const KeyType& key, NamedSem* semaphore) const
	{
		return strcmp(key, semaphore->Name()) == 0;
	}

	NamedSem*& GetLink(NamedSem* semaphore) const
	{
		return semaphore->HashLink();
	}
};


struct UnnamedSemHashDefinition {
	typedef sem_id				KeyType;
	typedef UnnamedSharedSem	ValueType;

	size_t HashKey(const KeyType& key) const
	{
		return (size_t)key;
	}

	size_t Hash(UnnamedSharedSem* semaphore) const
	{
		return HashKey(semaphore->SemaphoreID());
	}

	bool Compare(const KeyType& key, UnnamedSharedSem* semaphore) const
	{
		return key == semaphore->SemaphoreID();
	}

	UnnamedSharedSem*& GetLink(UnnamedSharedSem* semaphore) const
	{
		return semaphore->HashLink();
	}
};


class GlobalSemTable {
public:
	GlobalSemTable()
		:
		fSemaphoreCount(0)
	{
		mutex_init(&fLock, "global named sem table");
	}

	~GlobalSemTable()
	{
		mutex_destroy(&fLock);
	}

	status_t Init()
	{
		status_t error = fNamedSemaphores.Init();
		if (error != B_OK)
			return error;
		return fUnnamedSemaphores.Init();
	}

	status_t OpenNamedSem(const char* name, int openFlags, mode_t mode,
		uint32 semCount, NamedSem*& _sem, bool& _created)
	{
		MutexLocker _(fLock);

		NamedSem* sem = fNamedSemaphores.Lookup(name);
		if (sem != NULL) {
			if ((openFlags & O_EXCL) != 0)
				return EEXIST;

			if (!sem->HasPermissions())
				return EACCES;

			sem->AcquireReference();
			_sem = sem;
			_created = false;
			return B_OK;
		}

		if ((openFlags & O_CREAT) == 0)
			return ENOENT;

		// does not exist yet -- create
		if (fSemaphoreCount >= MAX_POSIX_SEMS)
			return ENOSPC;

		sem = new(std::nothrow) NamedSem;
		if (sem == NULL)
			return B_NO_MEMORY;

		status_t error = sem->Init(name, mode, semCount);
		if (error != B_OK) {
			delete sem;
			return error;
		}

		error = fNamedSemaphores.Insert(sem);
		if (error != B_OK) {
			delete sem;
			return error;
		}

		// add one reference for the table
		sem->AcquireReference();

		fSemaphoreCount++;

		_sem = sem;
		_created = true;
		return B_OK;
	}

	status_t UnlinkNamedSem(const char* name)
	{
		MutexLocker _(fLock);

		NamedSem* sem = fNamedSemaphores.Lookup(name);
		if (sem == NULL)
			return ENOENT;

		if (!sem->HasPermissions())
			return EACCES;

		fNamedSemaphores.Remove(sem);
		sem->ReleaseReference();
			// release the table reference
		fSemaphoreCount--;

		return B_OK;
	}

	status_t CreateUnnamedSem(uint32 semCount, int32_t& _id)
	{
		MutexLocker _(fLock);

		if (fSemaphoreCount >= MAX_POSIX_SEMS)
			return ENOSPC;

		UnnamedSharedSem* sem = new(std::nothrow) UnnamedSharedSem;
		if (sem == NULL)
			return B_NO_MEMORY;

		status_t error = sem->Init(semCount, "unnamed shared sem");
		if (error == B_OK)
			error = fUnnamedSemaphores.Insert(sem);
		if (error != B_OK) {
			delete sem;
			return error;
		}

		fSemaphoreCount++;

		_id = sem->SemaphoreID();
		return B_OK;
	}

	status_t DeleteUnnamedSem(sem_id id)
	{
		MutexLocker _(fLock);

		UnnamedSharedSem* sem = fUnnamedSemaphores.Lookup(id);
		if (sem == NULL)
			return B_BAD_VALUE;

		fUnnamedSemaphores.Remove(sem);
		delete sem;

		fSemaphoreCount--;

		return B_OK;
	}

	bool IsUnnamedValidSem(sem_id id)
	{
		MutexLocker _(fLock);

		return fUnnamedSemaphores.Lookup(id) != NULL;
	}

private:
	typedef BOpenHashTable<NamedSemHashDefinition, true> NamedSemTable;
	typedef BOpenHashTable<UnnamedSemHashDefinition, true> UnnamedSemTable;

	mutex			fLock;
	NamedSemTable	fNamedSemaphores;
	UnnamedSemTable	fUnnamedSemaphores;
	int32			fSemaphoreCount;
};


static GlobalSemTable sSemTable;


class TeamSemInfo {
public:
	TeamSemInfo(SemInfo* semaphore, sem_t* userSem)
		:
		fSemaphore(semaphore),
		fUserSemaphore(userSem),
		fOpenCount(1)
	{
	}

	~TeamSemInfo()
	{
		if (fSemaphore != NULL)
			fSemaphore->Delete();
	}

	sem_id ID() const				{ return fSemaphore->ID(); }
	sem_id SemaphoreID() const		{ return fSemaphore->SemaphoreID(); }
	sem_t* UserSemaphore() const	{ return fUserSemaphore; }

	void Open()
	{
		fOpenCount++;
	}

	bool Close()
	{
		return --fOpenCount == 0;
	}

	TeamSemInfo* Clone() const
	{
		SemInfo* sem = fSemaphore->Clone();
		if (sem == NULL)
			return NULL;

		TeamSemInfo* clone = new(std::nothrow) TeamSemInfo(sem, fUserSemaphore);
		if (clone == NULL) {
			sem->Delete();
			return NULL;
		}

		clone->fOpenCount = fOpenCount;

		return clone;
	}

	TeamSemInfo*& HashLink()
	{
		return fHashLink;
	}

private:
	SemInfo*		fSemaphore;
	sem_t*			fUserSemaphore;
	int32			fOpenCount;

	TeamSemInfo*	fHashLink;
};


struct TeamSemHashDefinition {
	typedef sem_id		KeyType;
	typedef TeamSemInfo	ValueType;

	size_t HashKey(const KeyType& key) const
	{
		return (size_t)key;
	}

	size_t Hash(TeamSemInfo* semaphore) const
	{
		return HashKey(semaphore->ID());
	}

	bool Compare(const KeyType& key, TeamSemInfo* semaphore) const
	{
		return key == semaphore->ID();
	}

	TeamSemInfo*& GetLink(TeamSemInfo* semaphore) const
	{
		return semaphore->HashLink();
	}
};


struct realtime_sem_context {
	realtime_sem_context()
		:
		fSemaphoreCount(0)
	{
		mutex_init(&fLock, "realtime sem context");
	}

	~realtime_sem_context()
	{
		mutex_lock(&fLock);

		// delete all semaphores.
		SemTable::Iterator it = fSemaphores.GetIterator();
		while (TeamSemInfo* sem = it.Next()) {
			// Note, this uses internal knowledge about how the iterator works.
			// Ugly, but there's no good alternative.
			fSemaphores.RemoveUnchecked(sem);
			delete sem;
		}

		mutex_destroy(&fLock);
	}

	status_t Init()
	{
		fNextPrivateSemID = -1;
		return fSemaphores.Init();
	}

	realtime_sem_context* Clone()
	{
		// create new context
		realtime_sem_context* context = new(std::nothrow) realtime_sem_context;
		if (context == NULL)
			return NULL;
		ObjectDeleter<realtime_sem_context> contextDeleter(context);

		MutexLocker _(fLock);

		context->fNextPrivateSemID = fNextPrivateSemID;

		// clone all semaphores
		SemTable::Iterator it = fSemaphores.GetIterator();
		while (TeamSemInfo* sem = it.Next()) {
			TeamSemInfo* clonedSem = sem->Clone();
			if (clonedSem == NULL)
				return NULL;

			if (context->fSemaphores.Insert(clonedSem) != B_OK) {
				delete clonedSem;
				return NULL;
			}
			context->fSemaphoreCount++;
		}

		contextDeleter.Detach();
		return context;
	}

	status_t CreateUnnamedSem(uint32 semCount, bool shared, int32_t& _id)
	{
		if (shared)
			return sSemTable.CreateUnnamedSem(semCount, _id);

		UnnamedSem* sem = new(std::nothrow) UnnamedSem;
		if (sem == NULL)
			return B_NO_MEMORY;
		ObjectDeleter<UnnamedSem> semDeleter(sem);

		status_t error = sem->Init(semCount, "unnamed sem");
		if (error != B_OK)
			return error;

		TeamSemInfo* teamSem = new(std::nothrow) TeamSemInfo(sem, NULL);
		if (teamSem == NULL)
			return B_NO_MEMORY;
		semDeleter.Detach();

		MutexLocker _(fLock);

		if (fSemaphoreCount >= MAX_POSIX_SEMS_PER_TEAM) {
			delete teamSem;
			return ENOSPC;
		}

		sem->SetID(_NextPrivateSemID());

		error = fSemaphores.Insert(teamSem);
		if (error != B_OK) {
			delete teamSem;
			return error;
		}

		fSemaphoreCount++;

		_id = teamSem->ID();
		return B_OK;
	}

	status_t OpenSem(const char* name, int openFlags, mode_t mode,
		uint32 semCount, sem_t* userSem, sem_t*& _usedUserSem, int32_t& _id,
		bool& _created)
	{
		NamedSem* sem = NULL;
		status_t error = sSemTable.OpenNamedSem(name, openFlags, mode, semCount,
			sem, _created);
		if (error != B_OK)
			return error;

		MutexLocker _(fLock);

		TeamSemInfo* teamSem = fSemaphores.Lookup(sem->ID());
		if (teamSem != NULL) {
			// already open -- just increment the open count
			teamSem->Open();
			sem->ReleaseReference();
			_usedUserSem = teamSem->UserSemaphore();
			_id = teamSem->ID();
			return B_OK;
		}

		// not open yet -- create a new team sem

		// first check the semaphore limit, though
		if (fSemaphoreCount >= MAX_POSIX_SEMS_PER_TEAM) {
			sem->ReleaseReference();
			if (_created)
				sSemTable.UnlinkNamedSem(name);
			return ENOSPC;
		}

		teamSem = new(std::nothrow) TeamSemInfo(sem, userSem);
		if (teamSem == NULL) {
			sem->ReleaseReference();
			if (_created)
				sSemTable.UnlinkNamedSem(name);
			return B_NO_MEMORY;
		}

		error = fSemaphores.Insert(teamSem);
		if (error != B_OK) {
			delete teamSem;
			if (_created)
				sSemTable.UnlinkNamedSem(name);
			return error;
		}

		fSemaphoreCount++;

		_usedUserSem = teamSem->UserSemaphore();
		_id = teamSem->ID();

		return B_OK;
	}

	status_t CloseSem(sem_id id, sem_t*& deleteUserSem)
	{
		deleteUserSem = NULL;

		MutexLocker _(fLock);

		TeamSemInfo* sem = fSemaphores.Lookup(id);
		if (sem == NULL)
			return sSemTable.DeleteUnnamedSem(id);

		if (sem->Close()) {
			// last reference closed
			fSemaphores.Remove(sem);
			fSemaphoreCount--;
			deleteUserSem = sem->UserSemaphore();
			delete sem;
		}

		return B_OK;
	}

	status_t AcquireSem(sem_id id, bigtime_t timeout)
	{
		MutexLocker locker(fLock);

		TeamSemInfo* sem = fSemaphores.Lookup(id);
		if (sem == NULL) {
			if (!sSemTable.IsUnnamedValidSem(id))
				return B_BAD_VALUE;
		} else
			id = sem->SemaphoreID();

		locker.Unlock();

		status_t error;
		if (timeout == 0) {
			error = acquire_sem_etc(id, 1, B_CAN_INTERRUPT | B_RELATIVE_TIMEOUT,
				0);
		} else if (timeout == B_INFINITE_TIMEOUT) {
			error = acquire_sem_etc(id, 1, B_CAN_INTERRUPT, 0);
		} else {
			error = acquire_sem_etc(id, 1,
				B_CAN_INTERRUPT | B_ABSOLUTE_REAL_TIME_TIMEOUT, timeout);
		}

		return error == B_BAD_SEM_ID ? B_BAD_VALUE : error;
	}

	status_t ReleaseSem(sem_id id)
	{
		MutexLocker locker(fLock);

		TeamSemInfo* sem = fSemaphores.Lookup(id);
		if (sem == NULL) {
			if (!sSemTable.IsUnnamedValidSem(id))
				return B_BAD_VALUE;
		} else
			id = sem->SemaphoreID();

		locker.Unlock();

		status_t error = release_sem(id);
		return error == B_BAD_SEM_ID ? B_BAD_VALUE : error;
	}

	status_t GetSemCount(sem_id id, int& _count)
	{
		MutexLocker locker(fLock);

		TeamSemInfo* sem = fSemaphores.Lookup(id);
		if (sem == NULL) {
			if (!sSemTable.IsUnnamedValidSem(id))
				return B_BAD_VALUE;
		} else
			id = sem->SemaphoreID();

		locker.Unlock();

		int32 count;
		status_t error = get_sem_count(id, &count);
		if (error != B_OK)
			return error;

		_count = count;
		return B_OK;
	}

private:
	sem_id _NextPrivateSemID()
	{
		while (true) {
			if (fNextPrivateSemID >= 0)
				fNextPrivateSemID = -1;

			sem_id id = fNextPrivateSemID--;
			if (fSemaphores.Lookup(id) == NULL)
				return id;
		}
	}

private:
	typedef BOpenHashTable<TeamSemHashDefinition, true> SemTable;

	mutex		fLock;
	SemTable	fSemaphores;
	int32		fSemaphoreCount;
	sem_id		fNextPrivateSemID;
};


// #pragma mark - implementation private


static realtime_sem_context*
get_current_team_context()
{
	Team* team = thread_get_current_thread()->team;

	// get context
	realtime_sem_context* context = atomic_pointer_get(
		&team->realtime_sem_context);
	if (context != NULL)
		return context;

	// no context yet -- create a new one
	context = new(std::nothrow) realtime_sem_context;
	if (context == NULL || context->Init() != B_OK) {
		delete context;
		return NULL;
	}

	// set the allocated context
	realtime_sem_context* oldContext = atomic_pointer_test_and_set(
		&team->realtime_sem_context, context, (realtime_sem_context*)NULL);
	if (oldContext == NULL)
		return context;

	// someone else was quicker
	delete context;
	return oldContext;
}


static status_t
copy_sem_name_to_kernel(const char* userName, KPath& buffer, char*& name)
{
	if (userName == NULL)
		return B_BAD_VALUE;
	if (!IS_USER_ADDRESS(userName))
		return B_BAD_ADDRESS;

	if (buffer.InitCheck() != B_OK)
		return B_NO_MEMORY;

	// copy userland path to kernel
	name = buffer.LockBuffer();
	ssize_t actualLength = user_strlcpy(name, userName, buffer.BufferSize());

	if (actualLength < 0)
		return B_BAD_ADDRESS;
	if ((size_t)actualLength >= buffer.BufferSize())
		return ENAMETOOLONG;

	return B_OK;
}


// #pragma mark - kernel internal


void
realtime_sem_init()
{
	new(&sSemTable) GlobalSemTable;
	if (sSemTable.Init() != B_OK)
		panic("realtime_sem_init() failed to init global sem table");
}


void
delete_realtime_sem_context(realtime_sem_context* context)
{
	delete context;
}


realtime_sem_context*
clone_realtime_sem_context(realtime_sem_context* context)
{
	if (context == NULL)
		return NULL;

	return context->Clone();
}


// #pragma mark - syscalls


status_t
_user_realtime_sem_open(const char* userName, int openFlagsOrShared,
	mode_t mode, uint32 semCount, sem_t* userSem, sem_t** _usedUserSem)
{
	realtime_sem_context* context = get_current_team_context();
	if (context == NULL)
		return B_NO_MEMORY;

	if (semCount > MAX_POSIX_SEM_VALUE)
		return B_BAD_VALUE;

	// userSem must always be given
	if (userSem == NULL)
		return B_BAD_VALUE;
	if (!IS_USER_ADDRESS(userSem))
		return B_BAD_ADDRESS;

	// unnamed semaphores are less work -- deal with them first
	if (userName == NULL) {
		int32_t id;
		status_t error = context->CreateUnnamedSem(semCount, openFlagsOrShared,
			id);
		if (error != B_OK)
			return error;

		if (user_memcpy(&userSem->id, &id, sizeof(int)) != B_OK) {
			sem_t* dummy;
			context->CloseSem(id, dummy);
			return B_BAD_ADDRESS;
		}

		return B_OK;
	}

	// check user pointers
	if (_usedUserSem == NULL)
		return B_BAD_VALUE;
	if (!IS_USER_ADDRESS(_usedUserSem) || !IS_USER_ADDRESS(userName))
		return B_BAD_ADDRESS;

	// copy name to kernel
	KPath nameBuffer(B_PATH_NAME_LENGTH);
	char* name;
	status_t error = copy_sem_name_to_kernel(userName, nameBuffer, name);
	if (error != B_OK)
		return error;

	// open the semaphore
	sem_t* usedUserSem;
	bool created;
	int32_t id;
	error = context->OpenSem(name, openFlagsOrShared, mode, semCount, userSem,
		usedUserSem, id, created);
	if (error != B_OK)
		return error;

	// copy results back to userland
	if (user_memcpy(&userSem->id, &id, sizeof(int)) != B_OK
		|| user_memcpy(_usedUserSem, &usedUserSem, sizeof(sem_t*)) != B_OK) {
		if (created)
			sSemTable.UnlinkNamedSem(name);
		sem_t* dummy;
		context->CloseSem(id, dummy);
		return B_BAD_ADDRESS;
	}

	return B_OK;
}


status_t
_user_realtime_sem_close(sem_id semID, sem_t** _deleteUserSem)
{
	if (_deleteUserSem != NULL && !IS_USER_ADDRESS(_deleteUserSem))
		return B_BAD_ADDRESS;

	realtime_sem_context* context = get_current_team_context();
	if (context == NULL)
		return B_BAD_VALUE;

	// close sem
	sem_t* deleteUserSem;
	status_t error = context->CloseSem(semID, deleteUserSem);
	if (error != B_OK)
		return error;

	// copy back result to userland
	if (_deleteUserSem != NULL
		&& user_memcpy(_deleteUserSem, &deleteUserSem, sizeof(sem_t*))
			!= B_OK) {
		return B_BAD_ADDRESS;
	}

	return B_OK;
}


status_t
_user_realtime_sem_unlink(const char* userName)
{
	// copy name to kernel
	KPath nameBuffer(B_PATH_NAME_LENGTH);
	char* name;
	status_t error = copy_sem_name_to_kernel(userName, nameBuffer, name);
	if (error != B_OK)
		return error;

	return sSemTable.UnlinkNamedSem(name);
}


status_t
_user_realtime_sem_get_value(sem_id semID, int* _value)
{
	if (_value == NULL)
		return B_BAD_VALUE;
	if (!IS_USER_ADDRESS(_value))
		return B_BAD_ADDRESS;

	realtime_sem_context* context = get_current_team_context();
	if (context == NULL)
		return B_BAD_VALUE;

	// get sem count
	int count;
	status_t error = context->GetSemCount(semID, count);
	if (error != B_OK)
		return error;

	// copy back result to userland
	if (user_memcpy(_value, &count, sizeof(int)) != B_OK)
		return B_BAD_ADDRESS;

	return B_OK;
}


status_t
_user_realtime_sem_post(sem_id semID)
{
	realtime_sem_context* context = get_current_team_context();
	if (context == NULL)
		return B_BAD_VALUE;

	return context->ReleaseSem(semID);
}


status_t
_user_realtime_sem_wait(sem_id semID, bigtime_t timeout)
{
	realtime_sem_context* context = get_current_team_context();
	if (context == NULL)
		return B_BAD_VALUE;

	return syscall_restart_handle_post(context->AcquireSem(semID, timeout));
}
