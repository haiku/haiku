#ifndef _beos_test_suite_h_
#define _beos_test_suite_h_

#include <cppunit/TestSuite.h>

class TestSuite : public CppUnit::TestSuite {
	//! Calls setUp(), runs the test suite, calls tearDown().
    virtual void run (CppUnit::TestResult *result);
    
	//! This function called *once* before the entire suite of tests is run
	virtual void setUp() {}
	
	//! This function called *once* after the entire suite of tests is run
	virtual void tearDown() {}   

};

#endif // _beos_test_suite_h_