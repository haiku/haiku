/*
	$Id: MessageQueueTestCase.cpp 383 2002-07-22 09:28:00Z tylerdauwalder $
	
	This file implements a base class for testing BMessageQueue functionality.
	
	*/


#include "MessageQueueTestCase.h"
#include <MessageQueue.h>
#include "MessageQueue.h"


int testMessageClass::messageDestructorCount = 0;

		

	MessageQueueTestCase::MessageQueueTestCase(
		std::string name) : BThreadedTestCase(name),
							theMessageQueue(new BMessageQueue())
{
	}



	MessageQueueTestCase::~MessageQueueTestCase()
{
	delete theMessageQueue;
	theMessageQueue = NULL;
	}
	
	

	void MessageQueueTestCase::CheckQueueAgainstList(void)
{
	SafetyLock theSafetyLock(theMessageQueue);
	
	if (theMessageQueue->Lock()) {
		CPPUNIT_ASSERT(theMessageQueue->CountMessages() == messageList.CountItems());
		int i;
		for (i = 0; i < theMessageQueue->CountMessages(); i++) {
			CPPUNIT_ASSERT(theMessageQueue->FindMessage((int32)i) ==
					messageList.ItemAt(i));
		}
		theMessageQueue->Unlock();
	}
}



	void MessageQueueTestCase::AddMessage(BMessage *message)
{
	if (theMessageQueue->Lock()) {
		theMessageQueue->AddMessage(message);
		messageList.AddItem(message);
		theMessageQueue->Unlock();
	}
}
	

	void MessageQueueTestCase::RemoveMessage(BMessage *message)
{
	if (theMessageQueue->Lock()) {
		theMessageQueue->RemoveMessage(message);
		messageList.RemoveItem((void *)message);
		theMessageQueue->Unlock();
	}
}
	

	BMessage *MessageQueueTestCase::NextMessage(void)
{
	SafetyLock theSafetyLock(theMessageQueue);
		
	BMessage *result = NULL;
	if (theMessageQueue->Lock()) {
		result = theMessageQueue->NextMessage();
		CPPUNIT_ASSERT(result == messageList.RemoveItem((int32)0));
		theMessageQueue->Unlock();
	}
	return(result);
}



	BMessage *MessageQueueTestCase::FindMessage(uint32 what, int index)
{
	int listCount = messageList.CountItems();
	int i;
	
	for (i = 0; i < listCount; i++) {
		BMessage *theMessage = (BMessage *)messageList.ItemAt(i);
		if (theMessage->what == what) {
			if (index == 0) {
				return(theMessage);
			}
			index--;
		}
	}
	
	return(NULL);
}
	


