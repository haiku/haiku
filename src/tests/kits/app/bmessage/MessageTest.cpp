//#include "MessageTest.h"
#include "../common.h"
#include "MessageConstructTest.h"
#include "MessageOpAssignTest.h"
#include "MessageInt32ItemTest.h"
#include "MessageEasyFindTest.h"

Test* MessageTestSuite()
{
	TestSuite* tests = new TestSuite();

	tests->addTest(TMessageConstructTest::Suite());
	tests->addTest(TMessageOpAssignTest::Suite());
	tests->addTest(TMessageInt32ItemTest::Suite());
	tests->addTest(TMessageEasyFindTest::Suite());

	return tests;
}

