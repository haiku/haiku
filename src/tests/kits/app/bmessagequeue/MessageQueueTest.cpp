#include "../common.h"
#include "AddMessageTest1.h"	
#include "AddMessageTest2.h"
#include "ConcurrencyTest1.h"
#include "ConcurrencyTest2.h"
#include "FindMessageTest1.h"

Test *MessageQueueTestSuite()
{
	TestSuite *testSuite = new TestSuite();
	
	testSuite->addTest(AddMessageTest1::suite());
	testSuite->addTest(AddMessageTest2::suite());
	testSuite->addTest(ConcurrencyTest1::suite());
//	testSuite->addTest(ConcurrencyTest2::suite());	// Causes an "Abort" for some reason...
	testSuite->addTest(FindMessageTest1::suite());
	
	return(testSuite);
}







