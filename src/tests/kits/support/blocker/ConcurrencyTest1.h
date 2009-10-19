/*
	$Id: ConcurrencyTest1.h 301 2002-07-18 05:32:00Z tylerdauwalder $
	
	This file defines a class for performing one test of BLocker
	functionality.
	
	*/


#ifndef ConcurrencyTest1_H
#define ConcurrencyTest1_H


#include "LockerTestCase.h"

	
class ConcurrencyTest1 :
	public LockerTestCase {
	
private:
	bool lockTestValue;

	bool AcquireLock(int, bool);
	
public:
	ConcurrencyTest1(std::string, bool);
	virtual ~ConcurrencyTest1();
	void setUp(void);
	void TestThread(void);
	static Test *suite(void);
};
	
#endif



