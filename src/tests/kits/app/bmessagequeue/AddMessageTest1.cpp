/*
	$Id: AddMessageTest1.cpp,v 1.1 2002/07/09 12:24:56 ejakowatz Exp $
	
	This file implements the first test for the OpenBeOS BMessageQueue code.
	It tests the Construction, Destruction, Add Message 1, Count Messages,
	Is Empty and Find Message 1 use cases.  It does so by doing the following:
		- checks that the queue is empty according to IsEmpty() and CountMessages()
		- adds 10000 messages to the queue and a BList
		- as each is added, checks that IsEmpty() is false and CountMessages()
		  returns what is expected
		- the BList contents is compared to the queue contents (uses FindMessage())
		- the number of BMessages destructed is checked, no messages should have been
		  deleted so far.
		- the queue is deleted
		- the number of BMessages destructed should now be 10000
	
	*/


#include "AddMessageTest1.h"
#include <Message.h>
#include <MessageQueue.h>
#include "MessageQueue.h"


/*
 *  Method:  AddMessageTest1<MessageQueue>::AddMessageTest1()
 *   Descr:  This is the constructor for this class.
 */
		
template<class MessageQueue>
	AddMessageTest1<MessageQueue>::AddMessageTest1(std::string name) :
		MessageQueueTestCase<MessageQueue>(name)
{
	}


/*
 *  Method:  AddMessageTest1<MessageQueue>::~AddMessageTest1()
 *   Descr:  This is the destructor for this class.
 */
 
template<class MessageQueue>
	AddMessageTest1<MessageQueue>::~AddMessageTest1()
{
	}
	
	
/*
 *  Method:  AddMessageTest1<MessageQueue>::setUp()
 *   Descr:  This member function is called just prior to running the test.
 *           It resets the destructor count for testMessageClass.
 */

template<class MessageQueue>
	void AddMessageTest1<MessageQueue>::setUp(void)
{
	testMessageClass::messageDestructorCount = 0;
	}


/*
 *  Method:  AddMessageTest1<MessageQueue>::PerformTest()
 *   Descr:  This member function performs this test.  It adds
 *           10000 messages to the message queue and confirms that
 *           the queue contains the right messages.  Then it confirms
 *           that all 10000 messages are deleted when the message
 *           queue is deleted.
 */	

template<class MessageQueue>
	void AddMessageTest1<MessageQueue>::PerformTest(void)
{
	assert(theMessageQueue->IsEmpty());
	assert(theMessageQueue->CountMessages() == 0);
	
	int i;
	for(i = 0; i < 10000; i++) {
		BMessage *theMessage = new testMessageClass(i);
		AddMessage(theMessage);
		assert(!theMessageQueue->IsEmpty());
		assert(theMessageQueue->CountMessages() == i + 1);
	}

	CheckQueueAgainstList();
	
	assert(testMessageClass::messageDestructorCount == 0);
	delete theMessageQueue;
	theMessageQueue = NULL;
	assert(testMessageClass::messageDestructorCount == 10000);
}


/*
 *  Method:  AddMessageTest1<Locker>::suite()
 *   Descr:  This static member function returns a test caller for performing 
 *           all combinations of "AddMessageTest1".  The test
 *           is created as a ThreadedTestCase (typedef'd as
 *           AddMessageTest1Caller) with only one thread.
 */

template<class MessageQueue> Test *AddMessageTest1<MessageQueue>::suite(void)
{	
	AddMessageTest1<MessageQueue> *theTest = new AddMessageTest1<MessageQueue>("");
	AddMessageTest1Caller *testCaller = new AddMessageTest1Caller("", theTest);
	testCaller->addThread(":Thread1", &AddMessageTest1<MessageQueue>::PerformTest);
				
	return(testCaller);
	}
 

template class AddMessageTest1<BMessageQueue>;
template class AddMessageTest1<OpenBeOS::BMessageQueue>;