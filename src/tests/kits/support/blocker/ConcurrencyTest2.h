/*
	$Id: ConcurrencyTest2.h,v 1.2 2002/07/18 05:32:00 tylerdauwalder Exp $
	
	This file defines a classes for performing one test of BLocker
	functionality.
	
	*/


#ifndef ConcurrencyTest2_H
#define ConcurrencyTest2_H


#include "LockerTestCase.h"

class CppUnit::Test;

class ConcurrencyTest2 : public LockerTestCase {
	
private:
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
	static CppUnit::Test *suite(void);
	};
	
#endif



