//
//	$Id: Autolock.h 3246 2003-05-14 17:21:46Z haydentech $
//
//	This is the BAutolock interface for OpenBeOS.  It has been created to
//	be source and binary compatible with the BeOS version of BAutolock.
//  To that end, all members are inline just as with the BeOS version.
//


#ifndef	_OPENBEOS_AUTOLOCK_H
#define	_OPENBEOS_AUTOLOCK_H


#include <Locker.h>
#include <Looper.h>


#ifdef USE_OPENBEOS_NAMESPACE
namespace OpenBeOS {
#endif

class BAutolock {
public:
	inline BAutolock(BLooper *looper);
	inline BAutolock(BLocker *locker);
	inline BAutolock(BLocker &locker);
	
	inline ~BAutolock();
	
	inline bool IsLocked(void);

private:
	BLocker *fTheLocker;
	BLooper *fTheLooper;
	bool	fIsLocked;
};


inline BAutolock::BAutolock(BLooper *looper) :
	fTheLocker(NULL), fTheLooper(looper), fIsLocked(looper->Lock())
{
}


inline BAutolock::BAutolock(BLocker *locker) :
	fTheLocker(locker), fTheLooper(NULL), fIsLocked(locker->Lock())
{
}


inline BAutolock::BAutolock(BLocker &locker) :
	fTheLocker(&locker), fTheLooper(NULL), fIsLocked(locker.Lock())
{
}


inline BAutolock::~BAutolock()
{
	if (fIsLocked) {
		if (fTheLooper != NULL) {
			fTheLooper->Unlock();
		} else {
			fTheLocker->Unlock();
		}
	}
}


inline bool BAutolock::IsLocked()
{
	return fIsLocked;
}


#ifdef USE_OPENBEOS_NAMESPACE
}
#endif

#endif // _OPENBEOS_LOCKER_H
