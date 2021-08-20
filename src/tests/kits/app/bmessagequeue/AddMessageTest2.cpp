/*
	$Id: AddMessageTest2.cpp 383 2002-07-22 09:28:00Z tylerdauwalder $
	
	This file implements the second test for the Haiku BMessageQueue code.
	It tests the Add Message 2 use case.  It does so by doing the following:
		- creates 4 BMessages
		- it adds them to the queue and a list
		- it checks the queue against the list
		- it adds the second one to the queue and the list again
		- it checks the queue against the list again (expecting an error)
	
	*/


#include "ThreadedTestCaller.h"
#include "AddMessageTest2.h"
#include <Message.h>
#include <MessageQueue.h>
#include "MessageQueue.h"


/*
 *  Method:  AddMessageTest2::AddMessageTest2()
 *   Descr:  This is the constructor for this class.
 */
		

	AddMessageTest2::AddMessageTest2(std::string name) :
					MessageQueueTestCase(name)
{
	}


/*
 *  Method:  AddMessageTest2::~AddMessageTest2()
 *   Descr:  This is the destructor for this class.
 */


	AddMessageTest2::~AddMessageTest2()
{
	}


/*
 *  Method:  AddMessageTest2::PerformTest()
 *   Descr:  This member function performs this test.  It adds
 *           4 messages to the message queue and confirms that
 *           the queue contains the right messages.  Then it re-adds
 *           the second message again.  It checks the queue against
 *           the list and expects to find a difference.  Then, it
 *           checks each element on the queue one by one.
 */


	void AddMessageTest2::PerformTest(void)
{
	NextSubTest();
	BMessage *firstMessage = new BMessage(1);
	BMessage *secondMessage = new BMessage(2);
	BMessage *thirdMessage = new BMessage(3);
	BMessage *fourthMessage = new BMessage(4);
	
	AddMessage(firstMessage);
	AddMessage(secondMessage);
	AddMessage(thirdMessage);
	AddMessage(fourthMessage);
	
	NextSubTest();
	CheckQueueAgainstList();
	
	AddMessage(secondMessage);
	
	// Re-adding a message to the message queue will result in the list
	// not matching the message queue.  Therefore, we expect an exception
	// to be raised.  An error occurs if it doesn't get raised.
	bool exceptionRaised = false;
	try {
		CheckQueueAgainstList();
	}
	catch (CppUnit::Exception e) {
		exceptionRaised = true;
	}
	CPPUNIT_ASSERT(exceptionRaised);
	
	NextSubTest();
	CPPUNIT_ASSERT(theMessageQueue->FindMessage((int32)0) == firstMessage);
	CPPUNIT_ASSERT(theMessageQueue->FindMessage((int32)1) == secondMessage);
	CPPUNIT_ASSERT(theMessageQueue->FindMessage((int32)2) == NULL);
	CPPUNIT_ASSERT(theMessageQueue->FindMessage((int32)3) == NULL);
	CPPUNIT_ASSERT(theMessageQueue->FindMessage((int32)4) == NULL);
}


/*
 *  Method:  AddMessageTest2::suite()
 *   Descr:  This static member function returns a test caller for performing 
 *           all combinations of "AddMessageTest2".  The test
 *           is created as a ThreadedTestCase (typedef'd as
 *           AddMessageTest2Caller) with only one thread.
 */

 Test *AddMessageTest2::suite(void)
{	
	typedef BThreadedTestCaller<AddMessageTest2>
		AddMessageTest2Caller;
	AddMessageTest2 *theTest = new AddMessageTest2("");
	AddMessageTest2Caller *testCaller = new AddMessageTest2Caller("BMessageQueue::Add Message Test #2", theTest);
	testCaller->addThread("A", &AddMessageTest2::PerformTest);
				
	return(testCaller);
	}
	

