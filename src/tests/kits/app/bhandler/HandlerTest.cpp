#include "../common.h"
#include "HandlerTest.h"
#include "BHandlerTester.h"
#include "IsWatchedTest.h"
#include "HandlerLooperTest.h"
#include "SetNextHandlerTest.h"
#include "NextHandlerTest.h"
#include "AddFilterTest.h"
#include "RemoveFilterTest.h"
#include "SetFilterListTest.h"
#include "LockLooperTest.h"
#include "LockLooperWithTimeoutTest.h"
#include "UnlockLooperTest.h"

Test* HandlerTestSuite(void)
{
	TestSuite* tests = new TestSuite();

	tests->addTest(TBHandlerTester::Suite());
	tests->addTest(TIsWatchedTest::Suite());
	tests->addTest(TLooperTest::Suite());
	tests->addTest(TSetNextHandlerTest::Suite());
	tests->addTest(TNextHandlerTest::Suite());
	tests->addTest(TAddFilterTest::Suite());
	tests->addTest(TRemoveFilterTest::Suite());
	tests->addTest(TSetFilterListTest::Suite());
	tests->addTest(TLockLooperTest::Suite());
	tests->addTest(TLockLooperWithTimeoutTest::Suite());
	tests->addTest(TUnlockLooperTest::Suite());

	return tests;
}
