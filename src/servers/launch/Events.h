/*
 * Copyright 2015-2018, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef EVENTS_H
#define EVENTS_H


#include <Messenger.h>
#include <String.h>


class BaseJob;
class Event;


class EventRegistrator {
public:
	virtual	status_t			RegisterExternalEvent(Event* event,
									const char* name,
									const BStringList& arguments) = 0;
	virtual	void				UnregisterExternalEvent(Event* event,
									const char* name) = 0;
};


class Event {
public:
								Event(Event* parent);
	virtual						~Event();

	virtual	status_t			Register(
									EventRegistrator& registrator) = 0;
	virtual	void				Unregister(
									EventRegistrator& registrator) = 0;

			bool				Triggered() const;
	virtual	void				Trigger(Event* origin);
	virtual	void				ResetTrigger();

	virtual	BaseJob*			Owner() const;
	virtual	void				SetOwner(BaseJob* owner);

			Event*				Parent() const;

	virtual	BString				ToString() const = 0;

protected:
			Event*				fParent;
			bool				fTriggered;
};


class Events {
public:
	static	Event*			FromMessage(const BMessenger& target,
								const BMessage& message);
	static	Event*			AddOnDemand(const BMessenger& target, Event* event);
	static	Event*			ResolveExternalEvent(Event* event,
								const char* name, uint32 flags);
	static	void			TriggerExternalEvent(Event* event);
	static	void			ResetStickyExternalEvent(Event* event);
	static	bool			TriggerDemand(Event* event, bool testOnly = false);
};


#endif // EVENTS_H
