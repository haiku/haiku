#include "ExampleTest.h"

#include <ThreadedTestCaller.h>
#include <cppunit/Test.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>
#include <stdio.h>
#include <iostream>
#include <kernel/OS.h>
#include <TestUtils.h>

ExampleTest::ExampleTest(std::string name)
	: BThreadedTestCase(name)
	, fLocker(new BLocker())
{
}

CppUnit::Test*
ExampleTest::Suite() {
	CppUnit::TestSuite *suite = new CppUnit::TestSuite("Yo");
	BThreadedTestCaller<ExampleTest> *caller;
	
	// Add a multithreaded test
	ExampleTest *test = new ExampleTest("This name is never used, just so you know :-)");
	caller = new BThreadedTestCaller<ExampleTest>("ExampleTests::MultiThreaded Test #1", test);
	caller->addThread("A", &ExampleTest::TestFunc1);
	caller->addThread("B", &ExampleTest::TestFunc2);
	caller->addThread("C", &ExampleTest::TestFunc3);
	suite->addTest(caller);
	
	// And another
	caller = new BThreadedTestCaller<ExampleTest>("ExampleTests::MultiThreaded Test #2");
	caller->addThread("Thread1", &ExampleTest::TestFunc1);
	caller->addThread("Thread2", &ExampleTest::TestFunc1);
	caller->addThread("Thread3", &ExampleTest::TestFunc1);
	suite->addTest(caller);
	
	// And one that fails, if you're so inclined
	caller = new BThreadedTestCaller<ExampleTest>("ExampleTests::MultiThreaded Failing Test");
	caller->addThread("GoodThread1", &ExampleTest::TestFunc1);
	caller->addThread("GoodThread2", &ExampleTest::TestFunc2);
	caller->addThread("BadThread", &ExampleTest::FailureFunc);
	suite->addTest(caller);
	
	// And some single threaded ones
	suite->addTest(new CppUnit::TestCaller<ExampleTest>("ExampleTests::SingleThreaded Test #1", &ExampleTest::TestFunc1));
	suite->addTest(new CppUnit::TestCaller<ExampleTest>("ExampleTests::SingleThreaded Test #2", &ExampleTest::TestFunc2));
	
	return suite;
}

const int sleeptime = 10000;
	
void
ExampleTest::TestFunc1() {
	for (int i = 0; i < 10; i++) {
		// Get the lock and do our business
		NextSubTest();
		fLocker->Lock();
		fNum += 10;
		fLocker->Unlock();
		snooze(sleeptime);
//		Outputf("(1:%d)", i);
	}
}

void
ExampleTest::TestFunc2() {
	for (int i = 0; i < 13; i++) {
		// Get the lock and do our business
		NextSubTest();
		fLocker->Lock();
		fNum *= 2;
		fLocker->Unlock();
		snooze(sleeptime);
//		Outputf("(2:%d)", i);
	}
}

void
ExampleTest::TestFunc3() {
	for (int i = 0; i < 15; i++) {
		// Get the lock and do our business
		NextSubTest();
		fLocker->Lock();
		fNum += 10;
		fLocker->Unlock();
		snooze(sleeptime);
//		Outputf("(3:%d)", i);
	}
}

void
ExampleTest::FailureFunc() {
	CHK(true == false);
}

