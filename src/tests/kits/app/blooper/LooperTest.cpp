//#include "LooperTest.h"
#include "../common.h"
#include "IsMessageWaitingTest.h"
#include "RemoveHandlerTest.h"
#include "IndexOfTest.h"
#include "CountHandlersTest.h"
#include "HandlerAtTest.h"
#include "AddHandlerTest.h"
#include "PerformTest.h"
#include "RunTest.h"

Test* LooperTestSuite()
{
	TestSuite* tests = new TestSuite();

	tests->addTest(TIsMessageWaitingTest::Suite());
	tests->addTest(TRemoveHandlerTest::Suite());
	tests->addTest(TIndexOfTest::Suite());
	tests->addTest(TCountHandlersTest::Suite());
	tests->addTest(THandlerAtTest::Suite());
	tests->addTest(TAddHandlerTest::Suite());
	tests->addTest(TPerformTest::Suite());
	tests->addTest(TRunTest::Suite());

	return tests;
}

