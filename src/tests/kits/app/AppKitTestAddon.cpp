#include <TestSuite.h>
#include <TestSuiteAddon.h>

// ##### Include headers for your tests here #####
#include "bapplication/ApplicationTest.h"
#include "bhandler/HandlerTest.h"
#include "blooper/LooperTest.h"
#include "bmessagequeue/MessageQueueTest.h"
#include "bmessenger/MessengerTest.h"

BTestSuite* getTestSuite() {
	BTestSuite *suite = new BTestSuite("App");

	// ##### Add test suites here #####
	suite->addTest("BApplication", ApplicationTestSuite());
	suite->addTest("BHandler", HandlerTestSuite());
	suite->addTest("BLooper", LooperTestSuite());
	suite->addTest("BMessageQueue", MessageQueueTestSuite());
	suite->addTest("BMessenger", MessengerTestSuite());
	
	return suite;
}
