/*
	$Id: BenaphoreLockCountTest1.h,v 1.1 2002/07/09 12:24:58 ejakowatz Exp $
	
	This file defines a classes for performing one test of BLocker
	functionality.
	
	*/


#ifndef BenaphoreLockCountTest1_H
#define BenaphoreLockCountTest1_H


#include "LockerTestCase.h"
#include "ThreadedTestCaller.h"

	
template<class Locker> class BenaphoreLockCountTest1 :
	public LockerTestCase<Locker> {
	
private:
	typedef ThreadedTestCaller <BenaphoreLockCountTest1<Locker> >
		BenaphoreLockCountTest1Caller;
	
	Locker thread2Lock;
	Locker thread3Lock;
	
	bool CheckLockRequests(int);
	
protected:
	
public:
	void TestThread1(void);
	void TestThread2(void);
	void TestThread3(void);
	BenaphoreLockCountTest1(std::string);
	virtual ~BenaphoreLockCountTest1();
	static Test *suite(void);
	};
	
#endif