/*
	$Id: SemaphoreLockCountTest1.h,v 1.1 2002/07/09 12:24:58 ejakowatz Exp $
	
	This file defines a classes for performing one test of BLocker
	functionality.
	
	*/


#ifndef SemaphoreLockCountTest1_H
#define SemaphoreLockCountTest1_H


#include "LockerTestCase.h"
#include "ThreadedTestCaller.h"

	
template<class Locker> class SemaphoreLockCountTest1 :
	public LockerTestCase<Locker> {
	
private:
	typedef ThreadedTestCaller <SemaphoreLockCountTest1<Locker> >
		SemaphoreLockCountTest1Caller;
	
	Locker thread2Lock;
	Locker thread3Lock;
	
	bool CheckLockRequests(int);
	
protected:
	
public:
	void TestThread1(void);
	void TestThread2(void);
	void TestThread3(void);
	SemaphoreLockCountTest1(std::string);
	virtual ~SemaphoreLockCountTest1();
	static Test *suite(void);
	};
	
#endif