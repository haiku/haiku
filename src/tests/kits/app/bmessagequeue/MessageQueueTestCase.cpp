/*
	$Id: MessageQueueTestCase.cpp,v 1.1 2002/07/09 12:24:56 ejakowatz Exp $
	
	This file implements a base class for testing BMessageQueue functionality.
	
	*/


#include "MessageQueueTestCase.h"
#include <MessageQueue.h>
#include "MessageQueue.h"


int testMessageClass::messageDestructorCount = 0;

		
template<class MessageQueue>
	MessageQueueTestCase<MessageQueue>::MessageQueueTestCase(
		std::string name) : TestCase(name),
							theMessageQueue(new MessageQueue)
{
	}


template<class MessageQueue>
	MessageQueueTestCase<MessageQueue>::~MessageQueueTestCase()
{
	delete theMessageQueue;
	theMessageQueue = NULL;
	}
	
	
template<class MessageQueue>
	void MessageQueueTestCase<MessageQueue>::CheckQueueAgainstList(void)
{
	SafetyLock<MessageQueue> theSafetyLock(theMessageQueue);
	
	if (theMessageQueue->Lock()) {
		assert(theMessageQueue->CountMessages() == messageList.CountItems());
		int i;
		for (i = 0; i < theMessageQueue->CountMessages(); i++) {
			assert(theMessageQueue->FindMessage((int32)i) ==
					messageList.ItemAt(i));
		}
		theMessageQueue->Unlock();
	}
}


template<class MessageQueue>
	void MessageQueueTestCase<MessageQueue>::AddMessage(BMessage *message)
{
	if (theMessageQueue->Lock()) {
		theMessageQueue->AddMessage(message);
		messageList.AddItem(message);
		theMessageQueue->Unlock();
	}
}
	
template<class MessageQueue>
	void MessageQueueTestCase<MessageQueue>::RemoveMessage(BMessage *message)
{
	if (theMessageQueue->Lock()) {
		theMessageQueue->RemoveMessage(message);
		messageList.RemoveItem((void *)message);
		theMessageQueue->Unlock();
	}
}
	
template<class MessageQueue>
	BMessage *MessageQueueTestCase<MessageQueue>::NextMessage(void)
{
	SafetyLock<MessageQueue> theSafetyLock(theMessageQueue);
		
	BMessage *result = NULL;
	if (theMessageQueue->Lock()) {
		result = theMessageQueue->NextMessage();
		assert(result == messageList.RemoveItem((int32)0));
		theMessageQueue->Unlock();
	}
	return(result);
}


template<class MessageQueue>
	BMessage *MessageQueueTestCase<MessageQueue>::FindMessage(uint32 what, int index)
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
	

template class MessageQueueTestCase<BMessageQueue>;
template class MessageQueueTestCase<OpenBeOS::BMessageQueue>;