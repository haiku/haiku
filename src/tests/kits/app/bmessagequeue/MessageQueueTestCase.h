/*
	$Id: MessageQueueTestCase.h 383 2002-07-22 09:28:00Z tylerdauwalder $
	
	This file defines a set of classes for testing BMessageQueue
	functionality.
	
	*/


#ifndef MessageQueueTestCase_H
#define MessageQueueTestCase_H


#include <List.h>
#include <Locker.h>
#include <Message.h>
#include <MessageQueue.h>
#include "ThreadedTestCase.h"


//
// The SafetyLock class is a utility class for use in actual tests
// of the BMessageQueue interfaces.  It is used to make sure that if the
// test fails and an exception is thrown with the lock held on the message
// queue, that lock will be released.  Without this SafetyLock, there
// could be deadlocks if one thread in a test has a failure while holding
// the lock.  It should be used like so:
//
//  void myTestClass::myTestFunc(void)
// {
//   SafetyLock mySafetyLock(theMessageQueue);
//   ...perform tests without worrying about holding the lock on assert...
//

 class SafetyLock {
private:
	BMessageQueue *theMessageQueue;
	
public:
	SafetyLock(BMessageQueue *aMessageQueue) {theMessageQueue = aMessageQueue;}
	virtual ~SafetyLock() {if (theMessageQueue != NULL) theMessageQueue->Unlock(); };
	};

	
//
// The testMessageClass is used to count the number of messages that are
// deleted.  This is used to check that all messages on the message queue
// were properly free'ed.
//

class testMessageClass : public BMessage
{
public:
	static int messageDestructorCount;
	virtual ~testMessageClass() { messageDestructorCount++; };
	testMessageClass(int what) : BMessage(what) { };
};

	
 class MessageQueueTestCase : public BThreadedTestCase {
	
private:
	BList messageList;

protected:
	BMessageQueue *theMessageQueue;
	
	void AddMessage(BMessage *message);
	void RemoveMessage(BMessage *message);
	BMessage *NextMessage(void);
	BMessage *FindMessage(uint32 what, int index);
	
	void CheckQueueAgainstList(void);
	
public:
	MessageQueueTestCase(std::string);
	virtual ~MessageQueueTestCase();
	};
	
#endif



