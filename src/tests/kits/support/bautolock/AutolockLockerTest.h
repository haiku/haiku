/*
	$Id: AutolockLockerTest.h,v 1.1 2002/07/09 12:24:57 ejakowatz Exp $
	
	This file defines the class for performing all BAutolock tests on a
	BLocker.
	
	*/


#ifndef AutolockLockerTest_H
#define AutolockLockerTest_H


#include "ThreadedTestCaller.h"
#include "TestCase.h"
	
template<class Autolock, class Locker> class AutolockLockerTest : public TestCase {
	
private:
	typedef ThreadedTestCaller <AutolockLockerTest<Autolock, Locker> >
		AutolockLockerTestCaller;
		
	Locker *theLocker;
	
public:
	static Test *suite(void);
	void TestThread1(void);
	void TestThread2(void);
	void TestThread3(void);
	AutolockLockerTest(std::string);
	virtual ~AutolockLockerTest();
	};
	
#endif