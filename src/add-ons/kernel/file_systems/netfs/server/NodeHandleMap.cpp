// NodeHandleMap.cpp

#include "NodeHandleMap.h"

#include "AutoLocker.h"
#include "GlobalBlockerPool.h"


// constructor
NodeHandleMap::NodeHandleMap(const char* name)
	: Locker(name),
	  fNextNodeHandleCookie(0)
{
}

// destructor
NodeHandleMap::~NodeHandleMap()
{
}

// Init
status_t
NodeHandleMap::Init()
{
	// check semaphore
	if (Sem() < 0)
		return Sem();

	return B_OK;
}

// AddNodeHandle
status_t
NodeHandleMap::AddNodeHandle(NodeHandle* handle)
{
	if (!handle)
		return B_BAD_VALUE;

	AutoLocker<Locker> _(this);

	handle->SetCookie(_NextNodeHandleCookie());

	status_t error = Put(handle->GetCookie(), handle);
	if (error == B_OK)
		handle->AcquireReference();

	return error;
}

// RemoveNodeHandle
bool
NodeHandleMap::RemoveNodeHandle(NodeHandle* handle)
{
	if (!handle)
		return false;

	AutoLocker<Locker> _(this);

	if (Get(handle->GetCookie()) != handle)
		return false;

	Remove(handle->GetCookie());
	handle->ReleaseReference();
	return true;
}

// LockNodeHandle
//
// VolumeManager must NOT be locked.
status_t
NodeHandleMap::LockNodeHandle(int32 cookie, NodeHandle** _handle)
{
	if (!_handle)
		return B_BAD_VALUE;

	NodeHandle* handle;
	{
		AutoLocker<Locker> _(this);

		// get the node handle
		handle = Get(cookie);
		if (!handle)
			return B_ENTRY_NOT_FOUND;
		handle->AcquireReference();

		// first attempt: we just try to lock the node handle, which will fail,
		// if someone else has the lock at the momemt
		if (handle->Lock()) {
			*_handle = handle;
			return B_OK;
		}
	}

	// someone else is locking, get a blocker and wait for the lock
	BlockerPool* blockerPool = GlobalBlockerPool::GetDefault();
	Blocker blocker = blockerPool->GetBlocker();
	BlockerPutter blockerPutter(*blockerPool, blocker);
	LockerCandidate lockerCandidate(blocker);
	{
		AutoLocker<Locker> _(this);

		if (handle->Lock()) {
			*_handle = handle;
			return B_OK;
		}
		handle->QueueLockerCandidate(&lockerCandidate);
	}

	// wait for the lock
	status_t error = lockerCandidate.Block();
	if (error != B_OK) {
		handle->ReleaseReference();
		return error;
	}

	*_handle = handle;
	return B_OK;
}

// UnlockNodeHandle
//
// VolumeManager may or may not be locked.
void
NodeHandleMap::UnlockNodeHandle(NodeHandle* nodeHandle)
{
	if (!nodeHandle)
		return;

	if (nodeHandle->IsLocked()) {
		AutoLocker<Locker> _(this);

		nodeHandle->Unlock();
		nodeHandle->ReleaseReference();
	}
}

// _NextNodeHandleCookie
int32
NodeHandleMap::_NextNodeHandleCookie()
{
	int32 cookie;

	do {
		if (fNextNodeHandleCookie < 0)
			fNextNodeHandleCookie = 0;
		cookie = fNextNodeHandleCookie++;
	} while (ContainsKey(cookie));

	return cookie;
}

