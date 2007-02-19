// BlockerPool.cpp

#include <new>

#include "AutoLocker.h"
#include "BlockerPool.h"
#include "Vector.h"

// BlockerVector
struct BlockerPool::BlockerVector : Vector<Blocker> {
};

// constructor
BlockerPool::BlockerPool(int32 count)
	: Locker("blocker pool"),
	  fFreeBlockersSemaphore(-1),
	  fBlockers(NULL),
	  fInitStatus(B_NO_INIT)
{
	fInitStatus = _Init(count);
	if (fInitStatus != B_OK)
		_Unset();
}

// destructor
BlockerPool::~BlockerPool()
{
	_Unset();
}

// InitCheck
status_t
BlockerPool::InitCheck() const
{
	return fInitStatus;
}

// GetBlocker
Blocker
BlockerPool::GetBlocker()
{
	if (fInitStatus != B_OK)
		return B_NO_INIT;
	status_t error = acquire_sem(fFreeBlockersSemaphore);
	if (error != B_OK)
		return error;
	AutoLocker<BlockerPool> _(this);
	if (fInitStatus != B_OK)
		return fInitStatus;
	Blocker blocker = fBlockers->ElementAt(fBlockers->Count() - 1);
	fBlockers->Erase(fBlockers->Count() - 1);
	return blocker;
}

// PutBlocker
status_t
BlockerPool::PutBlocker(Blocker blocker)
{
	status_t error = blocker.PrepareForUse();
	if (error != B_OK)
		return error;
	AutoLocker<BlockerPool> _(this);
	if (fInitStatus != B_OK)
		return fInitStatus;
	error = fBlockers->PushBack(blocker);
	if (error != B_OK)
		return error;
	return release_sem(fFreeBlockersSemaphore);
}

// _Init
status_t
BlockerPool::_Init(int32 count)
{
	_Unset();
	AutoLocker<BlockerPool> locker(this);
	if (!locker.IsLocked())
		return B_ERROR;
	// create semaphore
	fFreeBlockersSemaphore = create_sem(0, "blocker pool free blockers");
	if (fFreeBlockersSemaphore < 0)
		return fFreeBlockersSemaphore;
	// allocate blocker vector
	fBlockers = new(nothrow) BlockerVector;
	if (!fBlockers)
		return B_NO_MEMORY;
	fInitStatus = B_OK;
	// create and add blockers
	for (int32 i = 0; i < count; i++) {
		Blocker blocker;
		status_t error = blocker.InitCheck();
		if (error != B_OK)
			return error;
		error = PutBlocker(blocker);
		if (error != B_OK)
			return error;
	}
	return B_OK;
}

// _Unset
void
BlockerPool::_Unset()
{
	AutoLocker<BlockerPool> locker(this);
	if (fInitStatus == B_OK)
		fInitStatus = B_NO_INIT;
	delete fBlockers;
	fBlockers = NULL;
	if (fFreeBlockersSemaphore >= 0)
		delete_sem(fFreeBlockersSemaphore);
	fFreeBlockersSemaphore = -1;
}

