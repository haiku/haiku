/*
	$Id: BenaphoreLockCountTest1.h 301 2002-07-18 05:32:00Z tylerdauwalder $
	
	This file defines a classes for performing one test of BLocker
	functionality.
	
	*/


#ifndef BenaphoreLockCountTest1_H
#define BenaphoreLockCountTest1_H


#include <string>

#include "LockerTestCase.h"

	
class BenaphoreLockCountTest1 : public LockerTestCase {	
private:
	BLocker thread2Lock;
	BLocker thread3Lock;
	
	bool CheckLockRequests(int);
	
protected:
	
public:
	void TestThread1(void);
	void TestThread2(void);
	void TestThread3(void);
	BenaphoreLockCountTest1(std::string);
	virtual ~BenaphoreLockCountTest1();
	static CppUnit::Test *suite(void);
};
	
#endif



