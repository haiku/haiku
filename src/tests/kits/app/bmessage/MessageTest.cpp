#define __SGI_STL_INTERNAL_BVECTOR_H
//#include "MessageTest.h"
#include "../common.h"
#include "MessageConstructTest.h"
#include "MessageOpAssignTest.h"
#include "MessageBoolItemTest.h"
#include "MessageFloatItemTest.h"
#include "MessageDoubleItemTest.h"
#include "MessageInt8ItemTest.h"
#include "MessageInt16ItemTest.h"
#include "MessageInt32ItemTest.h"
#include "MessageInt64ItemTest.h"
#include "MessageBRectItemTest.h"
#include "MessageBPointItemTest.h"
#include "MessageEasyFindTest.h"

Test* MessageTestSuite()
{
	TestSuite* tests = new TestSuite();

	tests->addTest(TMessageConstructTest::Suite());
	tests->addTest(TMessageOpAssignTest::Suite());
	tests->addTest(TMessageBoolItemTest::Suite());
	tests->addTest(TMessageFloatItemTest::Suite());
	tests->addTest(TMessageDoubleItemTest::Suite());
	tests->addTest(TMessageInt8ItemTest::Suite());
	tests->addTest(TMessageInt16ItemTest::Suite());
	tests->addTest(TMessageInt32ItemTest::Suite());
	tests->addTest(TMessageInt64ItemTest::Suite());
	tests->addTest(TMessageBRectItemTest::Suite());
	tests->addTest(TMessageBPointItemTest::Suite());
	tests->addTest(TMessageEasyFindTest::Suite());

	return tests;
}

