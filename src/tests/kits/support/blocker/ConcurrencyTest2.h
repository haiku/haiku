/*
	$Id: ConcurrencyTest2.h,v 1.1 2002/07/09 12:24:58 ejakowatz Exp $
	
	This file defines a classes for performing one test of BLocker
	functionality.
	
	*/


#ifndef ConcurrencyTest2_H
#define ConcurrencyTest2_H


#include "LockerTestCase.h"
#include "ThreadedTestCaller.h"

	
template<class Locker> class ConcurrencyTest2 : public LockerTestCase<Locker> {
	
private:
	typedef ThreadedTestCaller <ConcurrencyTest2<Locker> >
		ConcurrencyTest2Caller;
	bool lockTestValue;

	void TestThread(void);
	bool AcquireLock(int, bool);
	void LockingLoop(void);
	
public:
	ConcurrencyTest2(std::string, bool);
	virtual ~ConcurrencyTest2();
	void setUp(void);
	void AcquireThread(void);
	void TimeoutThread(void);
	static Test *suite(void);
	};
	
#endif