/*
	$Id: MessageQueueTestCase.h,v 1.1 2002/07/09 12:24:56 ejakowatz Exp $
	
	This file defines a set of classes for testing BMessageQueue
	functionality.
	
	*/


#ifndef MessageQueueTestCase_H
#define MessageQueueTestCase_H


#include <List.h>
#include <Locker.h>
#include <Message.h>
#include "TestCase.h"


//
// The SafetyLock class is a utility class for use in actual tests
// of the BMessageQueue interfaces.  It is used to make sure that if the
// test fails and an exception is thrown with the lock held on the message
// queue, that lock will be released.  Without this SafetyLock, there
// could be deadlocks if one thread in a test has a failure while holding
// the lock.  It should be used like so:
//
// template<class MessageQueue> void myTestClass<MessageQueue>::myTestFunc(void)
// {
//   SafetyLock<MessageQueue> mySafetyLock(theMessageQueue);
//   ...perform tests without worrying about holding the lock on assert...
//

template<class MessageQueue> class SafetyLock {
private:
	MessageQueue *theMessageQueue;
	
public:
	SafetyLock(MessageQueue *aMessageQueue) {theMessageQueue = aMessageQueue;}
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

	
template<class MessageQueue> class MessageQueueTestCase : public TestCase {
	
private:
	BList messageList;

protected:
	MessageQueue *theMessageQueue;
	
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