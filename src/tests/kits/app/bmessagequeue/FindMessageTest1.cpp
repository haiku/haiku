/*
	$Id: FindMessageTest1.cpp 383 2002-07-22 09:28:00Z tylerdauwalder $
	
	This file implements the second test for the Haiku BMessageQueue code.
	It tests the Add Message 1 and Find Message 2 use cases.  It does so by
	doing the following:
		- creates numPerWhatCode * numWhatCodes messages and puts them on
		  the queue and the list
		- searches for the xth instance of a what code using FindMessage()
		  and by searching the list
		- compares the result of the two
		- there are many cases where FindMessage() returns NULL and those are
		  checked also
		- it does all searches for all the what codes used plus two not
		  actually used
	
	*/


#include "ThreadedTestCaller.h"
#include "FindMessageTest1.h"
#include <Message.h>
#include <MessageQueue.h>
#include "MessageQueue.h"


// This constant indicates the number of different what codes to use
// in messages added to the queue.
const int numWhatCodes = 20;

// This constant indicates the number of times a what code should be
// added to the list.
const int numPerWhatCode = 50;


/*
 *  Method:  FindMessageTest1::FindMessageTest1()
 *   Descr:  This is the constructor for this test class.
 */
		

	FindMessageTest1::FindMessageTest1(std::string name) :
		MessageQueueTestCase(name)
{
	}


/*
 *  Method:  FindMessageTest1::~FindMessageTest1()
 *   Descr:  This is the destructor for this test class.
 */


	FindMessageTest1::~FindMessageTest1()
{
	}


/*
 *  Method:  FindMessageTest1::PerformTest()
 *   Descr:  This is the member function for executing the test.
 *           It adds numPerWhatCode*numWhatCodes messages to the
 *           queue.  Then, it uses FindMessage() to search for
 *           each message using the queue and the list.  It
 *           checks that the queue and list return the same result.
 */	


	void FindMessageTest1::PerformTest(void)
{
	int whatCode;
	int i;
	BMessage *theMessage;
	BMessage *listMessage;
	
	for (i = 0; i < numPerWhatCode; i++) {
		if (i % (numPerWhatCode / 10) == 0)
			NextSubTest();
	
		for (whatCode = 1; whatCode <= numWhatCodes; whatCode++) {
			AddMessage(new testMessageClass(whatCode));
		}
	}
	
	for (whatCode = 0; whatCode <= numWhatCodes + 1; whatCode++) {
		if (i % (numWhatCodes / 10) == 0)
			NextSubTest();

		int index = 0;
		while ((theMessage = theMessageQueue->FindMessage(whatCode,
														   index)) != NULL) {
			listMessage = FindMessage(whatCode, index);
			CPPUNIT_ASSERT(listMessage == theMessage);
			index++;
		}
		listMessage = FindMessage(whatCode, index);
		CPPUNIT_ASSERT(listMessage == theMessage);
	}
}


/*
 *  Method:  FindMessageTest1::suite()
 *   Descr:  This static member function returns a test caller for performing 
 *           all combinations of "FindMessageTest1".  The test
 *           is created as a ThreadedTestCase (typedef'd as
 *           FindMessageTest1Caller) with only one thread.
 */

 Test *FindMessageTest1::suite(void)
{	
	typedef BThreadedTestCaller<FindMessageTest1>
		FindMessageTest1Caller;

	FindMessageTest1 *theTest = new FindMessageTest1("");
	FindMessageTest1Caller *testCaller = new FindMessageTest1Caller("BMessageQueue::Find Message Test", theTest);
	testCaller->addThread("A", &FindMessageTest1::PerformTest);
				
	return(testCaller);
	}


