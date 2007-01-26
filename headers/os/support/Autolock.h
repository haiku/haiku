/*
 * Copyright 2001-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_SUPPORT_AUTOLOCK_H
#define	_SUPPORT_AUTOLOCK_H


#include <Locker.h>
#include <Looper.h>


class BAutolock {
	public:
		inline BAutolock(BLooper *looper);
		inline BAutolock(BLocker *locker);
		inline BAutolock(BLocker &locker);
		inline ~BAutolock();

		inline bool IsLocked(void);

	private:
		BLocker *fLocker;
		BLooper *fLooper;
		bool	fIsLocked;
};


inline
BAutolock::BAutolock(BLooper *looper)
	: fLocker(NULL), fLooper(looper), fIsLocked(looper->Lock())
{
}


inline
BAutolock::BAutolock(BLocker *locker)
	: fLocker(locker), fLooper(NULL), fIsLocked(locker->Lock())
{
}


inline
BAutolock::BAutolock(BLocker &locker)
	: fLocker(&locker), fLooper(NULL), fIsLocked(locker.Lock())
{
}


inline
BAutolock::~BAutolock()
{
	if (fIsLocked) {
		if (fLooper != NULL)
			fLooper->Unlock();
		else
			fLocker->Unlock();
	}
}


inline bool
BAutolock::IsLocked()
{
	return fIsLocked;
}

#endif // _SUPPORT_AUTOLOCK_H
