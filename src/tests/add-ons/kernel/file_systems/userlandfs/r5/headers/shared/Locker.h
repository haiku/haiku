//
//	$Id: Locker.h,v 1.1 2002/07/09 12:24:33 ejakowatz Exp $
//
//	This is the Locker interface for OpenBeOS.  It has been created to
//	be source and binary compatible with the BeOS version of Locker.
//
// bonefish:
// * Removed `virtual' from destructor and FBC reserved space.
// * Renamed to Locker.


#ifndef	_OPENBEOS_LOCKER_H
#define	_OPENBEOS_LOCKER_H


#include <OS.h>
#include <SupportDefs.h>

namespace UserlandFSUtil {

class Locker {
public:
	Locker();
	Locker(const char *name);
	Locker(bool benaphore_style);
	Locker(const char *name, bool benaphore_style);
	
	// The following constructor is not documented in the BeBook
	// and is only listed here to ensure binary compatibility.
	// DO NOT USE THIS CONSTRUCTOR!
	Locker(const char *name, bool benaphore_style, bool);

	~Locker();

	bool Lock(void);
	status_t LockWithTimeout(bigtime_t timeout);
	void Unlock(void);

	thread_id LockingThread(void) const;
	bool IsLocked(void) const;
	int32 CountLocks(void) const;
	int32 CountLockRequests(void) const;
	sem_id Sem(void) const;

private:
	void InitLocker(const char *name, bool benaphore_style);
	bool AcquireLock(bigtime_t timeout, status_t *error);

	int32 fBenaphoreCount;
	sem_id fSemaphoreID;
	thread_id fLockOwner;
	int32 fRecursiveCount;
};

} // namespace UserlandFSUtil

using UserlandFSUtil::Locker;

#endif // _OPENBEOS_LOCKER_H
