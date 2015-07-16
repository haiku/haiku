/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "Events.h"

#include <stdio.h>

#include <Entry.h>
#include <ObjectList.h>
#include <Message.h>
#include <Path.h>
#include <StringList.h>

#include "BaseJob.h"
#include "LaunchDaemon.h"
#include "Utility.h"


class EventContainer : public Event {
protected:
								EventContainer(Event* parent,
									const BMessenger* target,
									const BMessage& args);
								EventContainer(BaseJob* owner,
									const BMessenger& target);

public:
			void				AddEvent(Event* event);
			BObjectList<Event>&	Events();

			const BMessenger&	Target() const;

	virtual	status_t			Register(EventRegistrator& registrator) const;
	virtual	void				Unregister(EventRegistrator& registrator) const;

	virtual	void				Trigger();

	virtual	BaseJob*			Owner() const;
	virtual	void				SetOwner(BaseJob* owner);

protected:
			void				ToString(BString& string) const;

protected:
			BaseJob*			fOwner;
			BMessenger			fTarget;
			BObjectList<Event>	fEvents;
};


class OrEvent : public EventContainer {
public:
								OrEvent(Event* parent, const BMessenger* target,
									const BMessage& args);
								OrEvent(BaseJob* owner,
									const BMessenger& target);

	virtual	BString				ToString() const;
};


class DemandEvent : public Event {
public:
								DemandEvent(Event* parent);

	virtual	status_t			Register(EventRegistrator& registrator) const;
	virtual	void				Unregister(EventRegistrator& registrator) const;

	virtual	BString				ToString() const;
};


class FileCreatedEvent : public Event {
public:
								FileCreatedEvent(Event* parent,
									const BMessage& args);

	virtual	status_t			Register(EventRegistrator& registrator) const;
	virtual	void				Unregister(EventRegistrator& registrator) const;

	virtual	BString				ToString() const;

private:
			BPath				fPath;
};


static Event*
create_event(Event* parent, const char* name, const BMessenger* target,
	const BMessage& args)
{
	if (strcmp(name, "or") == 0) {
		if (args.IsEmpty())
			return NULL;

		return new OrEvent(parent, target, args);
	}

	if (strcmp(name, "demand") == 0)
		return new DemandEvent(parent);
	if (strcmp(name, "file_created") == 0)
		return new FileCreatedEvent(parent, args);

	return NULL;
}


// #pragma mark -


Event::Event(Event* parent)
	:
	fParent(parent)
{
}


Event::~Event()
{
}


bool
Event::Triggered() const
{
	return fTriggered;
}


void
Event::Trigger()
{
	fTriggered = true;
	if (fParent != NULL)
		fParent->Trigger();
}


void
Event::ResetTrigger()
{
	fTriggered = false;
}


BaseJob*
Event::Owner() const
{
	if (fParent != NULL)
		return fParent->Owner();

	return NULL;
}


void
Event::SetOwner(BaseJob* owner)
{
	if (fParent != NULL)
		fParent->SetOwner(owner);
}


Event*
Event::Parent() const
{
	return fParent;
}


// #pragma mark -


EventContainer::EventContainer(Event* parent, const BMessenger* target,
	const BMessage& args)
	:
	Event(parent),
	fEvents(5, true)
{
	if (target != NULL)
		fTarget = *target;

	char* name;
	type_code type;
	int32 count;
	for (int32 index = 0; args.GetInfo(B_MESSAGE_TYPE, index, &name, &type,
			&count) == B_OK; index++) {
		BMessage message;
		for (int32 messageIndex = 0; args.FindMessage(name, messageIndex,
				&message) == B_OK; messageIndex++) {
			AddEvent(create_event(this, name, target, message));
		}
	}
}


EventContainer::EventContainer(BaseJob* owner, const BMessenger& target)
	:
	Event(NULL),
	fOwner(owner),
	fTarget(target),
	fEvents(5, true)
{
}


void
EventContainer::AddEvent(Event* event)
{
	if (event != NULL)
		fEvents.AddItem(event);
}


BObjectList<Event>&
EventContainer::Events()
{
	return fEvents;
}


const BMessenger&
EventContainer::Target() const
{
	return fTarget;
}


status_t
EventContainer::Register(EventRegistrator& registrator) const
{
	int32 count = fEvents.CountItems();
	for (int32 index = 0; index < count; index++) {
		Event* event = fEvents.ItemAt(index);
		status_t status = event->Register(registrator);
		if (status != B_OK)
			return status;
	}

	return B_OK;
}


void
EventContainer::Unregister(EventRegistrator& registrator) const
{
	int32 count = fEvents.CountItems();
	for (int32 index = 0; index < count; index++) {
		Event* event = fEvents.ItemAt(index);
		event->Unregister(registrator);
	}
}


void
EventContainer::Trigger()
{
	Event::Trigger();

	if (Parent() == NULL && Owner() != NULL) {
		BMessage message(kMsgEventTriggered);
		message.AddString("owner", Owner()->Name());
		fTarget.SendMessage(&message);
	}
}


BaseJob*
EventContainer::Owner() const
{
	return fOwner;
}


void
EventContainer::SetOwner(BaseJob* owner)
{
	Event::SetOwner(owner);
	fOwner = owner;
}


void
EventContainer::ToString(BString& string) const
{
	string += "[";

	for (int32 index = 0; index < fEvents.CountItems(); index++) {
		if (index != 0)
			string += ", ";
		string += fEvents.ItemAt(index)->ToString();
	}
	string += "]";
}


// #pragma mark - or


OrEvent::OrEvent(Event* parent, const BMessenger* target, const BMessage& args)
	:
	EventContainer(parent, target, args)
{
}


OrEvent::OrEvent(BaseJob* owner, const BMessenger& target)
	:
	EventContainer(owner, target)
{
}


BString
OrEvent::ToString() const
{
	BString string = "or ";
	EventContainer::ToString(string);
	return string;
}


// #pragma mark - demand


DemandEvent::DemandEvent(Event* parent)
	:
	Event(parent)
{
}


status_t
DemandEvent::Register(EventRegistrator& registrator) const
{
	return B_OK;
}


void
DemandEvent::Unregister(EventRegistrator& registrator) const
{
}


BString
DemandEvent::ToString() const
{
	return "event";
}


// #pragma mark - file_created


FileCreatedEvent::FileCreatedEvent(Event* parent, const BMessage& args)
	:
	Event(parent)
{
	fPath.SetTo(args.GetString("args", NULL));
}


status_t
FileCreatedEvent::Register(EventRegistrator& registrator) const
{
	// TODO: implement!
	return B_ERROR;
}


void
FileCreatedEvent::Unregister(EventRegistrator& registrator) const
{
}


BString
FileCreatedEvent::ToString() const
{
	BString string = "file_created ";
	string << fPath.Path();
	return string;
}


// #pragma mark -


/*static*/ Event*
Events::FromMessage(const BMessenger& target, const BMessage& message)
{
	return create_event(NULL, "or", &target, message);
}


/*static*/ Event*
Events::AddOnDemand(Event* event)
{
	OrEvent* or = dynamic_cast<OrEvent*>(event);
	if (or == NULL) {
		EventContainer* container = dynamic_cast<EventContainer*>(event);
		if (container == NULL)
			return NULL;

		or = new OrEvent(container->Owner(), container->Target());
	}
	if (or != event && event != NULL)
		or->AddEvent(event);

	or->AddEvent(new DemandEvent(or));
	return or;
}


/*static*/ bool
Events::TriggerDemand(Event* event)
{
	if (event == NULL)
		return false;

	if (EventContainer* container = dynamic_cast<EventContainer*>(event)) {
		for (int32 index = 0; index < container->Events().CountItems();
				index++) {
			Event* childEvent = container->Events().ItemAt(index);
			if (dynamic_cast<DemandEvent*>(childEvent) != NULL) {
				childEvent->Trigger();
				return true;
			}
			if (dynamic_cast<EventContainer*>(childEvent) != NULL) {
				if (TriggerDemand(childEvent))
					break;
			}
		}
	}

	return event->Triggered();
}
