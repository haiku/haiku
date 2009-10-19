/*
	$Id: DestructionTest1.h 301 2002-07-18 05:32:00Z tylerdauwalder $
	
	This file defines a class for performing one test of BLocker
	functionality.
	
	*/


#ifndef DestructionTest1_H
#define DestructionTest1_H

#include "LockerTestCase.h"
#include <string>
	
class DestructionTest1 : public LockerTestCase {
	
private:

protected:
	
public:
	void TestThread1(void);
	void TestThread2(void);
	DestructionTest1(std::string name, bool isBenaphore);
	virtual ~DestructionTest1();
	static CppUnit::Test *suite(void);
	};
	
#endif



