#ifndef _GENERIC_NUMBER_FORMAT_CONSTRUCTOR_TEST_
#define _GENERIC_NUMBER_FORMAT_CONSTRUCTOR_TEST_

#include <cppunit/TestCase.h>

class GenericNumberFormatConstructorTest : public CppUnit::TestCase {
	public:
		GenericNumberFormatConstructorTest() {}
		GenericNumberFormatConstructorTest(string name)
			: TestCase(name) {}

		void ConstructorTest1();

		static Test* Suite();
};

#endif	// _GENERIC_NUMBER_FORMAT_CONSTRUCTOR_TEST_
