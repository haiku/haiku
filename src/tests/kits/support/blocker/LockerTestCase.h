/*
	$Id: LockerTestCase.h,v 1.1 2002/07/09 12:24:58 ejakowatz Exp $
	
	This file defines a couple of common classes for testing BLocker
	functionality.
	
	*/


#ifndef LockerTestCase_H
#define LockerTestCase_H


#include "TestCase.h"

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

template<class Locker> class SafetyLock {
private:
	Locker *theLocker;
	
public:
	SafetyLock(Locker *aLock) {theLocker = aLock;}
	virtual ~SafetyLock() {if (theLocker != NULL) theLocker->Unlock(); };
	};


//
// All BLocker tests should be derived from the LockerTestCase class.
// This class provides a BLocker allocated on construction to the
// derived class.  This BLocker is the member "theLocker".  Also,
// there is a member function called CheckLock() which ensures that
// the lock is sane.
//
	
template<class Locker> class LockerTestCase : public TestCase {
	
protected:
	Locker *theLocker;

	void CheckLock(int);
	
public:
	LockerTestCase(std::string name, bool);
	virtual ~LockerTestCase();
	};
	
#endif