#include <SafetyLock.h>
#include <Locker.h>

SafetyLock::SafetyLock(BLocker *locker) 
	: fLocker(locker)
{
	if (fLocker)
		fLocker->Lock();
}

SafetyLock::~SafetyLock() {
	if (fLocker)
		fLocker->Unlock();
}