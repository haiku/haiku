/*
	$Id: BenaphoreLockCountTest1.h,v 1.2 2002/07/18 05:32:00 tylerdauwalder Exp $
	
	This file defines a classes for performing one test of BLocker
	functionality.
	
	*/


#ifndef BenaphoreLockCountTest1_H
#define BenaphoreLockCountTest1_H


#include "LockerTestCase.h"
#include <string>

class CppUnit::Test;
	
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



