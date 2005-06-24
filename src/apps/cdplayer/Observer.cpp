/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

// This defines the Observer and Notifier classes

#include <Message.h>
#include <Looper.h>
#include <Debug.h>
#include "Observer.h"

Observer::Observer(Notifier *target)
	:	observedList(4, true)
{
	if (target)
		StartObserving(target);
}

Observer::~Observer()
{
	StopObserving();
		// tell everyone to stop sending us notices
}

void
Observer::StartObserving(Notifier *target)
{
	ASSERT(target->RecipientHandler());
	ASSERT(RecipientHandler()->Looper());

	// send a message to the notifier to start sending us notices
	if (target->RecipientHandler()->Looper()) {
		BMessage *message = new BMessage(kStartObserving);
		message->AddPointer("observer", this);
		message->AddPointer("observed", target);
		
		// send the message to the looper associated with the
		// notifier
		target->RecipientHandler()->Looper()->PostMessage(message,
			target->RecipientHandler());
		NotifierListEntry *entry = new NotifierListEntry;
		
		entry->observed = target;
		entry->handler = target->RecipientHandler();
		entry->looper = target->RecipientHandler()->Looper();
		
		observedList.AddItem(entry);
	}
}

void
Observer::SendStopObserving(NotifierListEntry *target)
{
	// send a message to the notifier to start sending us notices
	if (target->looper) {
		BMessage *message = new BMessage(kEndObserving);
		message->AddPointer("observer", this);
		message->AddPointer("observed", target->observed);
		target->looper->PostMessage(message, target->handler);
	}
}

NotifierListEntry *
StopObservingOne(NotifierListEntry *observed, void *castToObserver)
{
	((Observer *)castToObserver)->SendStopObserving(observed);
	return 0;
}

void
Observer::StopObserving()
{
	// send a message to all the notifiers to start sending us notices
	observedList.EachElement(StopObservingOne, this);
	observedList.MakeEmpty();
}

bool
Observer::HandleObservingMessages(const BMessage *message)
{
	switch (message->what) {
		case kNoticeChange:
		{
			// look for notice messages from notifiers
			Notifier *observed = NULL;
			Observer *observer = NULL;
			message->FindPointer("observed", (void**)&observed);
			message->FindPointer("observer", (void**)&observer);
			ASSERT(observed);
			ASSERT(observer);
			if (!observed || !observer)
				return false;
			
			ASSERT(dynamic_cast<Observer *>(observer));

			// this is a notice for us, call the NoticeChange function
			observer->NoticeChange(observed);
			return true;
		}
		default:
			return false;
	}
}

static ObserverListEntry *
NotifyOne(ObserverListEntry *observer, void *castToObserved)
{
	if (observer->looper) {
		BMessage *message = new BMessage(kNoticeChange);
		message->AddPointer("observed", castToObserved);
		message->AddPointer("observer", observer->observer);
		observer->looper->PostMessage(message, observer->handler);
	}
	return 0;
}

void
Notifier::Notify()
{
	// send notices to all the observers
	observerList.EachElement(NotifyOne, this);
}

static ObserverListEntry *
FindItemWithObserver(ObserverListEntry *item, void *castToObserver)
{
	if (item->observer == castToObserver)
		return item;
	return 0;
}

void 
Notifier::AddObserver(Observer *observer)
{
	ObserverListEntry *item = new ObserverListEntry;
	item->observer = observer;
	item->handler = observer->RecipientHandler();
	item->looper = observer->RecipientHandler()->Looper();
	ASSERT(item->looper);
	observerList.AddUnique(item);
}

void 
Notifier::RemoveObserver(Observer *observer)
{
	ObserverListEntry *item = observerList.EachElement(
		FindItemWithObserver, observer);

	observerList.RemoveItem(item);
	delete item;
}

bool
Notifier::HandleObservingMessages(const BMessage *message)
{
	switch (message->what) {
		case kStartObserving:
		case kEndObserving:
		{
			// handle messages about stopping and starting observing
			Observer *observer = 0;
			Notifier *observed = 0;
			message->FindPointer("observer", (void**)&observer);
			message->FindPointer("observed", (void**)&observed);
			ASSERT(observer);
			ASSERT(observed);
			if (!observer || !observed)
				return false;

			if (message->what == kStartObserving)
				observed->AddObserver(observer);
			else
				observed->RemoveObserver(observer);
			return true;
		}
		default:
			return false;
	}
}
