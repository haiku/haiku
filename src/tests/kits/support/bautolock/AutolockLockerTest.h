/*
	$Id: AutolockLockerTest.h,v 1.2 2002/07/19 06:45:28 tylerdauwalder Exp $
	
	This file defines the class for performing all BAutolock tests on a
	BLocker.
	
	*/


#ifndef AutolockLockerTest_H
#define AutolockLockerTest_H

#include "ThreadedTestCase.h"
#include <string>

class BLocker;
class CppUnit::Test;
	
class AutolockLockerTest : public BThreadedTestCase {
	
private:
	BLocker *theLocker;
	
public:
	static CppUnit::Test *suite(void);
	void TestThread1(void);
	void TestThread2(void);
	void TestThread3(void);
	AutolockLockerTest(std::string);
	virtual ~AutolockLockerTest();
};
	
#endif




