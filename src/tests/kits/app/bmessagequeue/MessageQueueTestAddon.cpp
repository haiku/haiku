/*
	$Id: MessageQueueTestAddon.cpp 10 2002-07-09 12:24:59Z ejakowatz $
	
	This file declares the addonTestName string and addonTestFunc
	function for the BMessageQueue tests.  These symbols will be used
	when the addon is loaded.
	
	*/
	

#include "AddMessageTest1.h"	
#include "AddMessageTest2.h"
#include "ConcurrencyTest1.h"
#include "ConcurrencyTest2.h"
#include "FindMessageTest1.h"
#include <MessageQueue.h>
#include "MessageQueue.h"
#include "TestAddon.h"
#include "TestSuite.h"


/*
 *  Function:  addonTestFunc()
 *     Descr:  This function is called by the test application to
 *             get a pointer to the test to run.  The BMessageQueue test
 *             is a test suite.  A series of tests are added to
 *             the suite.  Each test appears twice, once for
 *             the Be implementation of BMessageQueue, once for the
 *             Haiku implementation.
 */

Test *addonTestFunc(void)
{
	TestSuite *testSuite = new TestSuite("BMessageQueue");
	
	testSuite->addTest(AddMessageTest1<BMessageQueue>::suite());
	testSuite->addTest(AddMessageTest2<BMessageQueue>::suite());
	testSuite->addTest(ConcurrencyTest1<BMessageQueue>::suite());
	testSuite->addTest(ConcurrencyTest2<BMessageQueue>::suite());
	testSuite->addTest(FindMessageTest1<BMessageQueue>::suite());
	
	testSuite->addTest(AddMessageTest1<OpenBeOS::BMessageQueue>::suite());
	testSuite->addTest(AddMessageTest2<OpenBeOS::BMessageQueue>::suite());
	testSuite->addTest(ConcurrencyTest1<OpenBeOS::BMessageQueue>::suite());
	testSuite->addTest(ConcurrencyTest2<OpenBeOS::BMessageQueue>::suite());
	testSuite->addTest(FindMessageTest1<OpenBeOS::BMessageQueue>::suite());

	return(testSuite);
}
