/*
	$Id: ConcurrencyTest2.cpp,v 1.1 2002/07/09 12:24:56 ejakowatz Exp $
	
	This file implements a test class for testing BMessageQueue functionality.
	It tests use cases Destruction, Add Message 3, Remove Message 2,
	Next Message 2, Lock 1, Lock 2, Unlock.
	
	The test works like the following:
		- It does the test two times, one unlocking using Unlock() and the other
		  unlocking using delete.
		- It populates the queue with numAddMessages messages to start.
		- The queue is locked
	    - It starts four threads.
	    - In one thread, a NextMessage() blocks
	    - In the second thread, a RemoveMessage() blocks
	    - In the third thread, an AddMessage() blocks
	    - In the fourth thread, a Lock() blocks
	    - After a short snooze, the queue is released using Unlock() or delete.
	    - Each of the four threads wake up and each checks as best it can that it
	      was successful and did not violate mutual exclusion.
		
	*/


#include "ConcurrencyTest2.h"
#include "ThreadedTestCaller.h"
#include "TestSuite.h"
#include <MessageQueue.h>
#include "MessageQueue.h"


// This constant indicates the number of messages to add to the queue.
const int numAddMessages = 50;

// This constant is used as a base amount of time to snooze.
bigtime_t SNOOZE_TIME = 100000;


/*
 *  Method:  ConcurrencyTest2<MessageQueue>::ConcurrencyTest2()
 *   Descr:  This is the constructor for this test.
 */
		
template<class MessageQueue>
	ConcurrencyTest2<MessageQueue>::ConcurrencyTest2(std::string name, bool unlockFlag) :
		MessageQueueTestCase<MessageQueue>(name), unlockTest(unlockFlag)
{
	}


/*
 *  Method:  ConcurrencyTest2<MessageQueue>::~ConcurrencyTest2()
 *   Descr:  This is the destructor for this test.
 */

template<class MessageQueue>
	ConcurrencyTest2<MessageQueue>::~ConcurrencyTest2()
{
	}


/*
 *  Method:  ConcurrencyTest2<MessageQueue>::setUp()
 *   Descr:  This member functions sets the environment for the test.
 *           it sets the lock flag and resets the message destructor
 *           count.  Finally, it adds numAddMessages messages to the
 *           queue.
 */

template<class MessageQueue>
	void ConcurrencyTest2<MessageQueue>::setUp(void)
{
	isLocked = false;
	testMessageClass::messageDestructorCount = 0;

	int i;
	BMessage *theMessage;
	for (i=0; i < numAddMessages; i++) {
		theMessage = new testMessageClass(i);
		theMessageQueue->AddMessage(theMessage);
	}
	removeMessage = theMessage;
}


/*
 *  Method:  ConcurrencyTest2<MessageQueue>::TestThread1()
 *   Descr:  This member function is one thread within the test.  It
 *           acquires the lock on the queue and then sleeps for a while.
 *           When it wakes up, it releases the lock on the queue by
 *           either doing an Unlock() or deleting the queue.
 */

template<class MessageQueue>
	void ConcurrencyTest2<MessageQueue>::TestThread1(void)
{
	theMessageQueue->Lock();
	isLocked = true;
	
	snooze(SNOOZE_TIME);
		
	isLocked = false;
	if (unlockTest) {
		theMessageQueue->Unlock();
	} else {
		MessageQueue *tmpMessageQueue = theMessageQueue;
		theMessageQueue = NULL;
		delete tmpMessageQueue;
	}	
}


/*
 *  Method:  ConcurrencyTest2<MessageQueue>::TestThread2()
 *   Descr:  This member function is one thread within the test.  It
 *           snoozes for a short time so that TestThread1() will grab
 *           the lock.  If this is the delete test, this thread
 *           terminates since Be's implementation may corrupt memory.
 *           Otherwise, the thread blocks attempting a NextMessage() on
 *           the queue.  The result of NextMessage() is checked finally.
 */

template<class MessageQueue> void ConcurrencyTest2<MessageQueue>::TestThread2(void)
{
	snooze(SNOOZE_TIME/10);
	assert(isLocked);
	if (!unlockTest) {
		// Be's implementation can cause a segv when NextMessage() is in
		// progress when a delete occurs.  The OpenBeOS implementation
		// does not segv, but it won't be tested here because Be's fails.
		return;
	}
	BMessage *theMessage = theMessageQueue->NextMessage();
	assert(!isLocked);

	if (unlockTest) {
		assert(theMessage != NULL);
		assert(theMessage->what == 0);
	} else {
		// The following test passes for the OpenBeOS implementation but
		// fails for the Be implementation.  If the BMessageQueue is deleted
		// while another thread is blocking waiting for NextMessage(), the
		// OpenBeOS implementation detects that the message queue is deleted
		// and returns NULL.  The Be implementation actually returns a message.
		// It must be doing so from freed memory since the queue has been
		// deleted.  The OpenBeOS implementation will not emulate the Be
		// implementation since I consider it a bug.
		//
		// assert(theMessage==NULL);
	}
}


/*
 *  Method:  ConcurrencyTest2<MessageQueue>::TestThread3()
 *   Descr:  This member function is one thread within the test.  It
 *           snoozes for a short time so that TestThread1() will grab
 *           the lock.  If this is the delete test, this thread
 *           terminates since Be's implementation may corrupt memory.
 *           Otherwise, the thread blocks attempting a RemoveMessage()
 *           on the queue.  The state of the queue is checked finally.
 */

template<class MessageQueue> void ConcurrencyTest2<MessageQueue>::TestThread3(void)
{
	snooze(SNOOZE_TIME/10);
	assert(isLocked);
	if (!unlockTest) {
		// Be's implementation causes a segv when RemoveMessage() is in
		// progress when a delete occurs.  The OpenBeOS implementation
		// does not segv, but it won't be tested here because Be's fails.
		return;
	}
	theMessageQueue->RemoveMessage(removeMessage);
	assert(!isLocked);
	if (unlockTest) {
		assert(theMessageQueue->FindMessage(removeMessage->what, 0) == NULL);
	}
}


/*
 *  Method:  ConcurrencyTest2<MessageQueue>::TestThread1()
 *   Descr:  This member function is one thread within the test.  It
 *           snoozes for a short time so that TestThread1() will grab
 *           the lock.  If this is the delete test, this thread
 *           terminates since Be's implementation may corrupt memory.
 *           Otherwise, the thread blocks attempting a AddMessage() on
 *           the queue.  The state of the queue is checked finally.
 */

template<class MessageQueue> void ConcurrencyTest2<MessageQueue>::TestThread4(void)
{
	snooze(SNOOZE_TIME/10);
	assert(isLocked);
	if (!unlockTest) {
		// Be's implementation can cause a segv when AddMessage() is in
		// progress when a delete occurs.  The OpenBeOS implementation
		// does not segv, but it won't be tested here because Be's fails.
		return;
	}
	theMessageQueue->AddMessage(new testMessageClass(numAddMessages));
	assert(!isLocked);
	if (unlockTest) {
		assert(theMessageQueue->FindMessage(numAddMessages, 0) != NULL);
	}
}


/*
 *  Method:  ConcurrencyTest2<MessageQueue>::TestThread1()
 *   Descr:  This member function is one thread within the test.  It
 *           snoozes for a short time so that TestThread1() will grab
 *           the lock.  The thread blocks attempting to acquire the
 *           lock on the queue.  Once the lock is acquired, mutual
 *           exclusion is checked as well as the result of the lock
 *           acquisition.
 */

template<class MessageQueue> void ConcurrencyTest2<MessageQueue>::TestThread5(void)
{
	SafetyLock<MessageQueue> mySafetyLock(theMessageQueue);
	
	snooze(SNOOZE_TIME/10);
	assert(isLocked);
	bool result = theMessageQueue->Lock();
	assert(!isLocked);
	if (unlockTest) {
		assert(result);
		theMessageQueue->Unlock();
	} else {
		assert(!result);
	}
}


/*
 *  Method:  ConcurrencyTest2<MessageQueue>::suite()
 *   Descr:  This static member function returns a test suite for performing 
 *           all combinations of "ConcurrencyTest2".  The test suite contains
 *           two instances of the test.  One is performed with an unlock,
 *           the other with a delete.  Each individual test
 *           is created as a ThreadedTestCase (typedef'd as
 *           ConcurrencyTest2Caller) with five independent threads.
 */

template<class MessageQueue> Test *ConcurrencyTest2<MessageQueue>::suite(void)
{	
	TestSuite *testSuite = new TestSuite("ConcurrencyTest2");
	
	ConcurrencyTest2<MessageQueue> *theTest = new ConcurrencyTest2<MessageQueue>("WithUnlock", true);
	ConcurrencyTest2Caller *threadedTest1 = new ConcurrencyTest2Caller("", theTest);
	threadedTest1->addThread(":Thread1", &ConcurrencyTest2<MessageQueue>::TestThread1);
	threadedTest1->addThread(":Thread2", &ConcurrencyTest2<MessageQueue>::TestThread2);
	threadedTest1->addThread(":Thread3", &ConcurrencyTest2<MessageQueue>::TestThread3);
	threadedTest1->addThread(":Thread3", &ConcurrencyTest2<MessageQueue>::TestThread4);
	threadedTest1->addThread(":Thread3", &ConcurrencyTest2<MessageQueue>::TestThread5);
							 		
	theTest = new ConcurrencyTest2<MessageQueue>("WithDelete", false);
	ConcurrencyTest2Caller *threadedTest2 = new ConcurrencyTest2Caller("", theTest);
	threadedTest2->addThread(":Thread1", &ConcurrencyTest2<MessageQueue>::TestThread1);
	threadedTest2->addThread(":Thread2", &ConcurrencyTest2<MessageQueue>::TestThread2);
	threadedTest2->addThread(":Thread3", &ConcurrencyTest2<MessageQueue>::TestThread3);
	threadedTest2->addThread(":Thread3", &ConcurrencyTest2<MessageQueue>::TestThread4);
	threadedTest2->addThread(":Thread3", &ConcurrencyTest2<MessageQueue>::TestThread5);
									 		
	testSuite->addTest(threadedTest1);	
	testSuite->addTest(threadedTest2);
	return(testSuite);
	}


template class ConcurrencyTest2<BMessageQueue>;
template class ConcurrencyTest2<OpenBeOS::BMessageQueue>;