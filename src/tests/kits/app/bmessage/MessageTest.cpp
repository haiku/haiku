#define __SGI_STL_INTERNAL_BVECTOR_H
//#include "MessageTest.h"
#include "../common.h"
#include "MessageConstructTest.h"
#include "MessageDestructTest.h"
#include "MessageOpAssignTest.h"
#include "MessageEasyFindTest.h"
#include "MessageBoolItemTest.h"
#include "MessageInt8ItemTest.h"
#include "MessageInt16ItemTest.h"
#include "MessageInt32ItemTest.h"
#include "MessageInt64ItemTest.h"
#include "MessageBRectItemTest.h"
#include "MessageBPointItemTest.h"
#include "MessageFloatItemTest.h"
#include "MessageDoubleItemTest.h"
#include "MessageMessageItemTest.h"
#include "MessageRefItemTest.h"
#include "MessageBStringItemTest.h"
#include "MessageCStringItemTest.h"
#include "MessageMessengerItemTest.h"
#include "MessagePointerItemTest.h"
#include "MessageFlattenableItemTest.h"
#include "MessageSpeedTest.h"

Test* MessageTestSuite()
{
	TestSuite* tests = new TestSuite();

	tests->addTest(TMessageConstructTest::Suite());
	tests->addTest(TMessageDestructTest::Suite());
	tests->addTest(TMessageOpAssignTest::Suite());
	tests->addTest(TMessageEasyFindTest::Suite());
	tests->addTest(TMessageBoolItemTest::Suite());
	tests->addTest(TMessageInt8ItemTest::Suite());
	tests->addTest(TMessageInt16ItemTest::Suite());
	tests->addTest(TMessageInt32ItemTest::Suite());
	tests->addTest(TMessageInt64ItemTest::Suite());
	tests->addTest(TMessageBRectItemTest::Suite());
	tests->addTest(TMessageBPointItemTest::Suite());
	tests->addTest(TMessageFloatItemTest::Suite());
	tests->addTest(TMessageDoubleItemTest::Suite());
	tests->addTest(TMessageMessageItemTest::Suite());
	tests->addTest(TMessageRefItemTest::Suite());
	tests->addTest(TMessageBStringItemTest::Suite());
	tests->addTest(TMessageCStringItemTest::Suite());
	tests->addTest(TMessageMessengerItemTest::Suite());
	tests->addTest(TMessagePointerItemTest::Suite());
	tests->addTest(TMessageFlattenableItemTest::Suite());
	tests->addTest(TMessageSpeedTest::Suite());

	return tests;
}

