/*
	$Id: DestructionTest2.h,v 1.1 2002/07/09 12:24:58 ejakowatz Exp $
	
	This file defines a class for performing one test of BLocker
	functionality.
	
	*/


#ifndef DestructionTest2_H
#define DestructionTest2_H


#include "LockerTestCase.h"
#include "ThreadedTestCaller.h"

	
template<class Locker> class DestructionTest2 : public LockerTestCase<Locker> {
	
private:
	typedef ThreadedTestCaller <DestructionTest2<Locker> >
		DestructionTest2Caller;
	
protected:
	
public:
	void TestThread1(void);
	void TestThread2(void);
	DestructionTest2(std::string name, bool isBenaphore);
	virtual ~DestructionTest2();
	static Test *suite(void);
	};
	
#endif