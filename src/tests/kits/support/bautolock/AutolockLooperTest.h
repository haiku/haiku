/*
	$Id: AutolockLooperTest.h,v 1.1 2002/07/09 12:24:57 ejakowatz Exp $
	
	This file defines the class for performing all BAutolock tests on a
	BLooper.
	
	*/


#ifndef AutolockLooperTest_H
#define AutolockLooperTest_H


#include "ThreadedTestCaller.h"
#include "TestCase.h"
	
template<class Autolock, class Looper> class AutolockLooperTest : public TestCase {
	
private:
	typedef ThreadedTestCaller <AutolockLooperTest<Autolock, Looper> >
		AutolockLooperTestCaller;
		
	Looper *theLooper;
	
public:
	static Test *suite(void);
	void TestThread1(void);
	AutolockLooperTest(std::string);
	virtual ~AutolockLooperTest();
	};
	
#endif