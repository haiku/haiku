#ifndef _beos_test_suite_addon_h_
#define _beos_test_suite_addon_h_

#include <cppunit/Portability.h>

class BTestSuite;

extern "C" CPPUNIT_API BTestSuite* getTestSuite();
extern "C" CPPUNIT_API const char* getTestSuiteName();

#endif	// _beos_test_suite_addon_h_
