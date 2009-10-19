/*
	$Id: LockerTestCase.h 301 2002-07-18 05:32:00Z tylerdauwalder $
	
	This file defines a couple of common classes for testing BLocker
	functionality.
	
	*/


#ifndef LockerTestCase_H
#define LockerTestCase_H

#include <ThreadedTestCase.h>
#include <string>

class BLocker;

//
// The SafetyLock class is a utility class for use in actual tests
// of the BLocker interfaces.  It is used to make sure that if the
// test fails and an exception is thrown with the lock held, that
// lock will be released.  Without this SafetyLock, there could be
// deadlocks if one thread in a test has a failure while holding the
// lock.  It should be used like so:
//
// void myTestClass::myTestFunc(void)
// {
//   SafetyLock mySafetyLock(theLocker);
//   ...perform tests without worrying about holding the lock on assert...
//

class SafetyLock {
private:
	BLocker *theLocker;
	
public:
	SafetyLock(BLocker *aLock) {theLocker = aLock;};
	virtual ~SafetyLock() {if (theLocker != NULL) theLocker->Unlock(); };
};


//
// All BLocker tests should be derived from the LockerTestCase class.
// This class provides a BLocker allocated on construction to the
// derived class.  This BLocker is the member "theLocker".  Also,
// there is a member function called CheckLock() which ensures that
// the lock is sane.
//
	
class LockerTestCase : public BThreadedTestCase {
	
protected:
	BLocker *theLocker;

	void CheckLock(int);
	
public:
	LockerTestCase(std::string name, bool);
	virtual ~LockerTestCase();
};
	
#endif



