/*
	$Id: DestructionTest2.h,v 1.2 2002/07/18 05:32:00 tylerdauwalder Exp $
	
	This file defines a class for performing one test of BLocker
	functionality.
	
	*/


#ifndef DestructionTest2_H
#define DestructionTest2_H

#include "LockerTestCase.h"

class CppUnit::Test;

class DestructionTest2 : public LockerTestCase {
public:
	void TestThread1(void);
	void TestThread2(void);
	DestructionTest2(std::string name, bool isBenaphore);
	virtual ~DestructionTest2();
	static CppUnit::Test *suite(void);
};
	
#endif



