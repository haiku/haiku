#ifndef _beos_safety_lock_
#define _beos_safety_lock_

class BLocker;

//
// The SafetyLock class is a utility class for use in actual tests
// of the BLocker interfaces.  It is used to make sure that if the
// test fails and an exception is thrown with the lock held, that
// lock will be released.  Without this SafetyLock, there could be
// deadlocks if one thread in a test has a failure while holding the
// lock.  It should be used like so:
//
// template<class Locker> void myTestClass<Locker>::myTestFunc(void)
// {
//   SafetyLock<Locker> mySafetyLock(theLocker);
//   ...perform tests without worrying about holding the lock on assert...
//
class SafetyLock {
public:
	SafetyLock(BLocker *locker); 
	virtual ~SafetyLock(); 

private:
	BLocker *fLocker;
	
};

#endif // _beos_safety_lock_

