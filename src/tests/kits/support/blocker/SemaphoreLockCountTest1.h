/*
	$Id: SemaphoreLockCountTest1.h 301 2002-07-18 05:32:00Z tylerdauwalder $
	
	This file defines a classes for performing one test of BLocker
	functionality.
	
	*/


#ifndef SemaphoreLockCountTest1_H
#define SemaphoreLockCountTest1_H


#include "LockerTestCase.h"
#include <string>
	
class SemaphoreLockCountTest1 :
	public LockerTestCase {
	
private:
	
	BLocker thread2Lock;
	BLocker thread3Lock;
	
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



