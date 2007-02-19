// Lockable.h

#ifndef NET_FS_LOCKABLE_H
#define NET_FS_LOCKABLE_H

#include "Blocker.h"
#include "DLList.h"

// LockerCandidate
class LockerCandidate : public DLListLinkImpl<LockerCandidate> {
public:
								LockerCandidate(Blocker blocker);

			thread_id			GetThread() const;

			status_t			Block();
			status_t			Unblock(bool success);

private:
			Blocker				fBlocker;
			thread_id			fThread;
};

typedef DLList<LockerCandidate> LockerCandidateList;

// Lockable
class Lockable {
public:
								Lockable();
								~Lockable();

			bool				Lock();		// non-blocking
			void				Unlock();
			bool				IsLocked() const;

			void				QueueLockerCandidate(
									LockerCandidate* candidate);

private:
			LockerCandidateList	fLockerCandidates;
			thread_id			fLockOwner;
			int32				fLockCounter;
};

#endif	// NET_FS_LOCKABLE_H
