#include <TestSuite.h>

void
TestSuite::run (CppUnit::TestResult *result) {
	setUp();
    CppUnit::TestSuite::run(result);
    tearDown();
}
