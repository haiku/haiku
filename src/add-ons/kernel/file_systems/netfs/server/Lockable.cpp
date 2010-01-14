// Lockable.cpp

#include "Lockable.h"


// LockerCandidate

// constructor
LockerCandidate::LockerCandidate(Blocker blocker)
	: fBlocker(blocker),
	  fThread(find_thread(NULL))
{
}

// GetThread
thread_id
LockerCandidate::GetThread() const
{
	return fThread;
}

// Block
status_t
LockerCandidate::Block()
{
	int32 userData;
	status_t error = fBlocker.Block(&userData);
	if (error != B_OK)
		return error;

	return (userData ? B_OK : B_ENTRY_NOT_FOUND);
}

// Unblock
status_t
LockerCandidate::Unblock(bool success)
{
	return fBlocker.Unblock(success);
}


// #pragma mark -

// Lockable

// constructor
Lockable::Lockable()
	: fLockOwner(-1),
	  fLockCounter(0)
{
}

// destructor
Lockable::~Lockable()
{
	// unblock all
	while (LockerCandidate* candidate = fLockerCandidates.First()) {
		fLockerCandidates.Remove(candidate);
		candidate->Unblock(false);
	}
}

// Lock
bool
Lockable::Lock()
{
	thread_id thread = find_thread(NULL);
	if (fLockOwner >= 0 && fLockOwner != thread)
		return false;
	fLockOwner = thread;
	fLockCounter++;
	return true;
}

// Unlock
void
Lockable::Unlock()
{
	thread_id thread = find_thread(NULL);
	if (fLockOwner != thread)
		return;
	if (--fLockCounter > 0)
		return;
	if (LockerCandidate* candidate = fLockerCandidates.First()) {
		fLockerCandidates.Remove(candidate);
		fLockOwner = candidate->GetThread();
		fLockCounter = 1;
		candidate->Unblock(true);
	} else
		fLockOwner = -1;
}

// IsLocked
bool
Lockable::IsLocked() const
{
	return (fLockOwner == find_thread(NULL));
}

// QueueLockerCandidate
void
Lockable::QueueLockerCandidate(LockerCandidate* candidate)
{
	if (!candidate)
		return;
	if (fLockOwner >= 0) {
		fLockerCandidates.Insert(candidate);
	} else {
		// if the object is not locked, wake up the candidate right now
		fLockOwner = candidate->GetThread();
		fLockCounter = 1;
		candidate->Unblock(true);
	}
}
