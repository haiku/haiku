/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
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
	virtual	status_t			RegisterEvent(Event* event) = 0;
	virtual	void				UnregisterEvent(Event* event) = 0;
};


class Event {
public:
								Event(Event* parent);
	virtual						~Event();

	virtual	status_t			Register(
									EventRegistrator& registrator) const = 0;
	virtual	void				Unregister(
									EventRegistrator& registrator) const = 0;

			bool				Triggered() const;
	virtual	void				Trigger();

	virtual	BaseJob*			Owner() const;
	virtual	void				SetOwner(BaseJob* owner);

			Event*				Parent() const;

	virtual	BString				ToString() const = 0;

private:
			Event*				fParent;
			bool				fTriggered;
};


class Events {
public:
	static	Event*			FromMessage(const BMessenger& target,
								const BMessage& message);
	static	Event*			AddOnDemand(Event* event);
	static	bool			TriggerDemand(Event* event);
};


#endif // EVENTS_H
