#ifndef _beos_test_suite_addon_h_
#define _beos_test_suite_addon_h_

class BTestSuite;

extern "C" {
	BTestSuite* getTestSuite();
}

#endif	// _beos_test_suite_addon_h_
