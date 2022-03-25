#ifndef _bitmap_test_h_
#define _bitmap_test_h_

#include <TestCase.h>

class BitmapTest : public BTestCase {
public:
	BitmapTest(std::string name = "");

	static CppUnit::Test* Suite();

	void ResizeTest();
	void ShiftTest();
};

#endif // _bitmap_test_h_
