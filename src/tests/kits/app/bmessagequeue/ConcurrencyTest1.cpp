/*
	$Id: ConcurrencyTest1.cpp 383 2002-07-22 09:28:00Z tylerdauwalder $
	
	This file implements a test class for testing BMessageQueue functionality.
	It tests use cases Destruction, Add Message 1, Add Message 3, Remove Message 1,
	Remove Message 2, Count Messages, Find Message 1, Next Message 1 and Next Message
	2.
	
	The test works like the following:
		- It does the test two times, one using an internal BList to keep track of
		  what should be in the queue and once without.  It does it without because
		  using the internal BList isn't a good test of mutual exclusion because an
		  explicit lock is used.  However, it is worth testing with the BList to show
		  that the result is what was expected.
	    - It starts three threads.
	    - In one thread, numAddMessages are added to the queue
	    - In the second thread, numNextMessages are removed from the queue using
	      NextMessage().
	    - In the third thread, numAddMessages are added to the queue however every
	      numRemovedMessagesPerAdd messages is removed using RemoveMessage().
	    - Once all three threads complete, the number of elements on the queue is
	      compared to what it expects.
	    - If the BList is being used internally, the queue is checked against the list.
	    - It checks that no BMessages have been deleted.
	    - It deletes the message queue.
	    - It checks that the expected number of BMessages have been deleted.
	    - It creates a new message queue and restarts the test again if necessary
	      (ie perform it without the internal list).
		
	*/


#include "ThreadedTestCaller.h"
#include "ConcurrencyTest1.h"
#include "ThreadedTestCaller.h"
#include "TestSuite.h"
#include <MessageQueue.h>
#include <Autolock.h>


// This constant indicates how many messages to add to the queue.
const int numAddMessages = 5000;

// This constant indicates how many times to call NextMessage() on the queue.
const int numNextMessages = 100;

// This constant says how many adds to perform before doing a remove.
const int numAddMessagesPerRemove = 5;


/*
 *  Method:  ConcurrencyTest1::ConcurrencyTest1()
 *   Descr:  This is the constructor for this test.
 */
		

	ConcurrencyTest1::ConcurrencyTest1(std::string name, bool listFlag) :
		MessageQueueTestCase(name), useList(listFlag)
{
	}


/*
 *  Method:  ConcurrencyTest1::~ConcurrencyTest1()
 *   Descr:  This is the destructor for this test.
 */


	ConcurrencyTest1::~ConcurrencyTest1()
{
	}


/*
 *  Method:  ConcurrencyTest1::setUp()
 *   Descr:  This member function prepares the enviroment for test execution.
 *           It resets the message destructor count and puts "numAddMessages"
 *           messages on the queue.
 */	

 void ConcurrencyTest1::setUp(void)
{
	testMessageClass::messageDestructorCount = 0;

	int i;
	for (i=0; i < numAddMessages; i++) {
		BMessage *theMessage = new testMessageClass(i);
		if (useList) {
			AddMessage(theMessage);
		} else {
			theMessageQueue->AddMessage(theMessage);
		}
	}
}


/*
 *  Method:  ConcurrencyTest1::TestThread1()
 *   Descr:  This member function is one of the three threads for performing
 *           this test.  It waits until the other two threads complete and
 *           then checks the state of the message queue.  If the list is
 *           being used, the queue is compared to the list.  It checks that
 *           the expected number of messages are on the queue and that they
 *           are all deleted when the message queue is destructed.
 */

 void ConcurrencyTest1::TestThread1(void)
{
	NextSubTest();
	snooze(1000000);
	thread2Lock.Lock();
	thread3Lock.Lock();
	
	if (useList) {
		CheckQueueAgainstList();
	}
	
	int numMessages = (2*numAddMessages) -
						(numAddMessages / numAddMessagesPerRemove) -
						numNextMessages;
	CPPUNIT_ASSERT(numMessages == theMessageQueue->CountMessages());
	CPPUNIT_ASSERT(testMessageClass::messageDestructorCount == 0);
	
	delete theMessageQueue;
	theMessageQueue = NULL;
	CPPUNIT_ASSERT(testMessageClass::messageDestructorCount == numMessages);
}


/*
 *  Method:  ConcurrencyTest1::TestThread2()
 *   Descr:  This member function is one of the three threads for performing
 *           this test.  It performs NextMessage() numNextMessages times on
 *           the queue.  It confirms that it always receives a message from
 *           the queue.
 */

 void ConcurrencyTest1::TestThread2(void)
{
	BAutolock mySafetyLock(&thread2Lock);
	
	int i;
	for(i = 0; i < numNextMessages; i++) {
		if (i % (numNextMessages / 10) == 0)
			NextSubTest();
			
		snooze(5000);
		BMessage *theMessage;
		if (useList) {
			theMessage = NextMessage();
		} else {
			theMessage = theMessageQueue->NextMessage();
		}
		CPPUNIT_ASSERT(theMessage != NULL);
	}
}


/*
 *  Method:  ConcurrencyTest1::TestThread3()
 *   Descr:  This member function is one of the three threads for performing
 *           this test.  It adds numAddMessages messages to the queue.  For
 *           every numAddMessagesPerRemove messages it adds, one message
 *           is removed.
 */

 void ConcurrencyTest1::TestThread3(void)
{
	BAutolock mySafetyLock(&thread3Lock);
	
	int i;
	BList messagesToRemove;
	
	for (i=0; i < numAddMessages; i++) {
		if (i % (numAddMessages / 10) == 0)
			NextSubTest();
	
		BMessage *theMessage = new testMessageClass(i);
		if (useList) {
			AddMessage(theMessage);
		} else {
			theMessageQueue->AddMessage(theMessage);
		}
		if ((i % numAddMessagesPerRemove) == numAddMessagesPerRemove - 1) {
			snooze(500);
			if (useList) {
				RemoveMessage(theMessage);
			} else {
				theMessageQueue->RemoveMessage(theMessage);
			}
		}
	}
}


/*
 *  Method:  ConcurrencyTest1::suite()
 *   Descr:  This static member function returns a test suite for performing 
 *           all combinations of "ConcurrencyTest1".  The test suite contains
 *           two instances of the test.  One is performed with a list,
 *           the other without.  Each individual test
 *           is created as a ThreadedTestCase (typedef'd as
 *           ConcurrencyTest1Caller) with three independent threads.
 */

 Test *ConcurrencyTest1::suite(void)
{	
	typedef BThreadedTestCaller<ConcurrencyTest1>
		ConcurrencyTest1Caller;
	TestSuite *testSuite = new TestSuite("ConcurrencyTest1");
	
	ConcurrencyTest1 *theTest = new ConcurrencyTest1("WithList", true);
	ConcurrencyTest1Caller *threadedTest1 = new ConcurrencyTest1Caller("BMessageQueue::Concurrency Test #1 (with list)", theTest);
	threadedTest1->addThread("A", &ConcurrencyTest1::TestThread1);
	threadedTest1->addThread("B", &ConcurrencyTest1::TestThread2);
	threadedTest1->addThread("C", &ConcurrencyTest1::TestThread3);
							 		
	theTest = new ConcurrencyTest1("WithoutList", false);
	ConcurrencyTest1Caller *threadedTest2 = new ConcurrencyTest1Caller("BMessageQueue::Concurrency Test #1 (without list)", theTest);
	threadedTest2->addThread("A", &ConcurrencyTest1::TestThread1);
	threadedTest2->addThread("B", &ConcurrencyTest1::TestThread2);
	threadedTest2->addThread("C", &ConcurrencyTest1::TestThread3);
									 		
	testSuite->addTest(threadedTest1);	
	testSuite->addTest(threadedTest2);
	return(testSuite);
	}
	
	

