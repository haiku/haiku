/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

// This defines the Observer and Notifier classes

// The idea of observing is make it easier to support a client-server
// setup where a client want's to react to changes in the server state,
// for instance a view displaying a track number needs to change whenever
// a track changes. Normally this is done by the client periodically checking
// the server from within a Pulse call or a simillar mechanism. With Observer
// and Notifier, the Observer (client) starts observing a Notifier (server)
// and then just sits back and wait to get a notice, whenever the Notifier
// changes.

#ifndef __OBSERVER__
#define __OBSERVER__

#include "TypedList.h"
#include <Handler.h>

const uint32 kNoticeChange = 'notc';
const uint32 kStartObserving = 'stob';
const uint32 kEndObserving = 'edob';

class Notifier;

class NotifierListEntry {
public:
	Notifier *observed;
	BHandler *handler;
	BLooper *looper;
};

class Observer {
public:
	Observer(Notifier *target = NULL);
	virtual ~Observer();

	void StartObserving(Notifier *);
		// start observing a speficied notifier
	void StopObserving(Notifier *);
		// stop observing a speficied notifier
	void StopObserving();
		// stop observing all the observed notifiers

	virtual void NoticeChange(Notifier *) = 0;
		// override this to get your job done, your class will get called
		// whenever the Notifier changes

	static bool HandleObservingMessages(const BMessage *message);
		// call this from subclasses MessageReceived
	virtual BHandler *RecipientHandler() const = 0;
		// hook this up to return subclasses looper
private:
	void SendStopObserving(NotifierListEntry *);

	// keep a list of all the observed notifiers
	TypedList<NotifierListEntry *> observedList;
	
friend NotifierListEntry *StopObservingOne(NotifierListEntry *, void *);
};

class ObserverListEntry {
public:
	Observer *observer;
	BHandler *handler;
	BLooper *looper;
};

class Notifier {
public:
	Notifier()
		{}
	virtual ~Notifier()
		{}

	virtual void Notify();
		// call this when the notifier object changes to send notices
		// to all the observers

	static bool HandleObservingMessages(const BMessage *message);
		// call this from subclasses MessageReceived
	virtual BHandler *RecipientHandler() const = 0;
		// hook this up to return subclasses looper

	// keep a list of all the observers so that we can send them notices
	void AddObserver(Observer *);
	void RemoveObserver(Observer *);

private:
	TypedList<ObserverListEntry *> observerList;

friend class Observer;
};

#endif