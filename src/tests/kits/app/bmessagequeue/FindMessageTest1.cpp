/*
	$Id: FindMessageTest1.cpp,v 1.1 2002/07/09 12:24:56 ejakowatz Exp $
	
	This file implements the second test for the OpenBeOS BMessageQueue code.
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
 *  Method:  FindMessageTest1<MessageQueue>::FindMessageTest1()
 *   Descr:  This is the constructor for this test class.
 */
		
template<class MessageQueue>
	FindMessageTest1<MessageQueue>::FindMessageTest1(std::string name) :
		MessageQueueTestCase<MessageQueue>(name)
{
	}


/*
 *  Method:  FindMessageTest1<MessageQueue>::~FindMessageTest1()
 *   Descr:  This is the destructor for this test class.
 */

template<class MessageQueue>
	FindMessageTest1<MessageQueue>::~FindMessageTest1()
{
	}


/*
 *  Method:  FindMessageTest1<MessageQueue>::PerformTest()
 *   Descr:  This is the member function for executing the test.
 *           It adds numPerWhatCode*numWhatCodes messages to the
 *           queue.  Then, it uses FindMessage() to search for
 *           each message using the queue and the list.  It
 *           checks that the queue and list return the same result.
 */	

template<class MessageQueue>
	void FindMessageTest1<MessageQueue>::PerformTest(void)
{
	int whatCode;
	int i;
	BMessage *theMessage;
	BMessage *listMessage;
	
	for (i = 0; i < numPerWhatCode; i++) {
		for (whatCode = 1; whatCode <= numWhatCodes; whatCode++) {
			AddMessage(new testMessageClass(whatCode));
		}
	}
	
	for (whatCode = 0; whatCode <= numWhatCodes + 1; whatCode++) {
		int index = 0;
		while ((theMessage = theMessageQueue->FindMessage(whatCode,
														   index)) != NULL) {
			listMessage = FindMessage(whatCode, index);
			assert(listMessage == theMessage);
			index++;
		}
		listMessage = FindMessage(whatCode, index);
		assert(listMessage == theMessage);
	}
}


/*
 *  Method:  FindMessageTest1<Locker>::suite()
 *   Descr:  This static member function returns a test caller for performing 
 *           all combinations of "FindMessageTest1".  The test
 *           is created as a ThreadedTestCase (typedef'd as
 *           FindMessageTest1Caller) with only one thread.
 */

template<class MessageQueue> Test *FindMessageTest1<MessageQueue>::suite(void)
{	
	FindMessageTest1<MessageQueue> *theTest = new FindMessageTest1<MessageQueue>("");
	FindMessageTest1Caller *testCaller = new FindMessageTest1Caller("", theTest);
	testCaller->addThread(":Thread1", &FindMessageTest1<MessageQueue>::PerformTest);
				
	return(testCaller);
	}


template class FindMessageTest1<BMessageQueue>;
template class FindMessageTest1<OpenBeOS::BMessageQueue>;