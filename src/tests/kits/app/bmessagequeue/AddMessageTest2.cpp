/*
	$Id: AddMessageTest2.cpp,v 1.1 2002/07/09 12:24:56 ejakowatz Exp $
	
	This file implements the second test for the OpenBeOS BMessageQueue code.
	It tests the Add Message 2 use case.  It does so by doing the following:
		- creates 4 BMessages
		- it adds them to the queue and a list
		- it checks the queue against the list
		- it adds the second one to the queue and the list again
		- it checks the queue against the list again (expecting an error)
	
	*/


#include "AddMessageTest2.h"
#include <Message.h>
#include <MessageQueue.h>
#include "MessageQueue.h"


/*
 *  Method:  AddMessageTest2<MessageQueue>::AddMessageTest2()
 *   Descr:  This is the constructor for this class.
 */
		
template<class MessageQueue>
	AddMessageTest2<MessageQueue>::AddMessageTest2(std::string name) :
					MessageQueueTestCase<MessageQueue>(name)
{
	}


/*
 *  Method:  AddMessageTest2<MessageQueue>::~AddMessageTest2()
 *   Descr:  This is the destructor for this class.
 */

template<class MessageQueue>
	AddMessageTest2<MessageQueue>::~AddMessageTest2()
{
	}


/*
 *  Method:  AddMessageTest2<MessageQueue>::PerformTest()
 *   Descr:  This member function performs this test.  It adds
 *           4 messages to the message queue and confirms that
 *           the queue contains the right messages.  Then it re-adds
 *           the second message again.  It checks the queue against
 *           the list and expects to find a difference.  Then, it
 *           checks each element on the queue one by one.
 */

template<class MessageQueue>
	void AddMessageTest2<MessageQueue>::PerformTest(void)
{
	BMessage *firstMessage = new BMessage(1);
	BMessage *secondMessage = new BMessage(2);
	BMessage *thirdMessage = new BMessage(3);
	BMessage *fourthMessage = new BMessage(4);
	
	AddMessage(firstMessage);
	AddMessage(secondMessage);
	AddMessage(thirdMessage);
	AddMessage(fourthMessage);
	
	CheckQueueAgainstList();
	
	AddMessage(secondMessage);
	
	// Re-adding a message to the message queue will result in the list
	// not matching the message queue.  Therefore, we expect an exception
	// to be raised.  An error occurs if it doesn't get raised.
	bool exceptionRaised = false;
	try {
		CheckQueueAgainstList();
	}
	catch (CppUnitException e) {
		exceptionRaised = true;
	}
	assert(exceptionRaised);
	
	assert(theMessageQueue->FindMessage((int32)0) == firstMessage);
	assert(theMessageQueue->FindMessage((int32)1) == secondMessage);
	assert(theMessageQueue->FindMessage((int32)2) == NULL);
	assert(theMessageQueue->FindMessage((int32)3) == NULL);
	assert(theMessageQueue->FindMessage((int32)4) == NULL);
}


/*
 *  Method:  AddMessageTest2<Locker>::suite()
 *   Descr:  This static member function returns a test caller for performing 
 *           all combinations of "AddMessageTest2".  The test
 *           is created as a ThreadedTestCase (typedef'd as
 *           AddMessageTest2Caller) with only one thread.
 */

template<class MessageQueue> Test *AddMessageTest2<MessageQueue>::suite(void)
{	
	AddMessageTest2<MessageQueue> *theTest = new AddMessageTest2<MessageQueue>("");
	AddMessageTest2Caller *testCaller = new AddMessageTest2Caller("", theTest);
	testCaller->addThread(":Thread1", &AddMessageTest2<MessageQueue>::PerformTest);
				
	return(testCaller);
	}
	

template class AddMessageTest2<BMessageQueue>;
template class AddMessageTest2<OpenBeOS::BMessageQueue>;