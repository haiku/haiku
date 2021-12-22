/*
 * Copyright 2015-2018, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "Events.h"

#include <stdio.h>

#include <Entry.h>
#include <LaunchRoster.h>
#include <Message.h>
#include <ObjectList.h>
#include <Path.h>
#include <StringList.h>

#include "BaseJob.h"
#include "LaunchDaemon.h"
#include "NetworkWatcher.h"
#include "Utility.h"
#include "VolumeWatcher.h"


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

	virtual	status_t			Register(EventRegistrator& registrator);
	virtual	void				Unregister(EventRegistrator& registrator);

	virtual	void				Trigger(Event* origin);

	virtual	BaseJob*			Owner() const;
	virtual	void				SetOwner(BaseJob* owner);

protected:
			void				AddEventsToString(BString& string) const;

protected:
			BaseJob*			fOwner;
			BMessenger			fTarget;
			BObjectList<Event>	fEvents;
			bool				fRegistered;
};


class OrEvent : public EventContainer {
public:
								OrEvent(Event* parent, const BMessenger* target,
									const BMessage& args);
								OrEvent(BaseJob* owner,
									const BMessenger& target);

	virtual	void				ResetTrigger();

	virtual	BString				ToString() const;
};


class StickyEvent : public Event {
public:
								StickyEvent(Event* parent);
	virtual						~StickyEvent();

	virtual	void				ResetSticky();
	virtual	void				ResetTrigger();
};


class DemandEvent : public Event {
public:
								DemandEvent(Event* parent);

	virtual	status_t			Register(EventRegistrator& registrator);
	virtual	void				Unregister(EventRegistrator& registrator);

	virtual	BString				ToString() const;
};


class ExternalEvent : public Event {
public:
								ExternalEvent(Event* parent, const char* name,
									const BMessage& args);

			const BString&		Name() const;
			bool				Resolve(uint32 flags);

			void				ResetSticky();
	virtual	void				ResetTrigger();

	virtual	status_t			Register(EventRegistrator& registrator);
	virtual	void				Unregister(EventRegistrator& registrator);

	virtual	BString				ToString() const;

private:
			BString				fName;
			BStringList			fArguments;
			uint32				fFlags;
			bool				fResolved;
};


class FileCreatedEvent : public Event {
public:
								FileCreatedEvent(Event* parent,
									const BMessage& args);

	virtual	status_t			Register(EventRegistrator& registrator);
	virtual	void				Unregister(EventRegistrator& registrator);

	virtual	BString				ToString() const;

private:
			BPath				fPath;
};


class VolumeMountedEvent : public Event, public VolumeListener {
public:
								VolumeMountedEvent(Event* parent,
									const BMessage& args);

	virtual	status_t			Register(EventRegistrator& registrator);
	virtual	void				Unregister(EventRegistrator& registrator);

	virtual	BString				ToString() const;

	virtual	void				VolumeMounted(dev_t device);
	virtual	void				VolumeUnmounted(dev_t device);
};


class NetworkAvailableEvent : public StickyEvent, public NetworkListener {
public:
								NetworkAvailableEvent(Event* parent,
									const BMessage& args);

	virtual	status_t			Register(EventRegistrator& registrator);
	virtual	void				Unregister(EventRegistrator& registrator);

	virtual	BString				ToString() const;

	virtual	void				NetworkAvailabilityChanged(bool available);
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
	if (strcmp(name, "volume_mounted") == 0)
		return new VolumeMountedEvent(parent, args);
	if (strcmp(name, "network_available") == 0)
		return new NetworkAvailableEvent(parent, args);

	return new ExternalEvent(parent, name, args);
}


// #pragma mark -


Event::Event(Event* parent)
	:
	fParent(parent),
	fTriggered(false)
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
Event::Trigger(Event* origin)
{
	fTriggered = true;
	if (fParent != NULL)
		fParent->Trigger(origin);
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
	fEvents(5, true),
	fRegistered(false)
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
	fEvents(5, true),
	fRegistered(false)
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
EventContainer::Register(EventRegistrator& registrator)
{
	if (fRegistered)
		return B_OK;

	int32 count = fEvents.CountItems();
	for (int32 index = 0; index < count; index++) {
		Event* event = fEvents.ItemAt(index);
		status_t status = event->Register(registrator);
		if (status != B_OK)
			return status;
	}

	fRegistered = true;
	return B_OK;
}


void
EventContainer::Unregister(EventRegistrator& registrator)
{
	int32 count = fEvents.CountItems();
	for (int32 index = 0; index < count; index++) {
		Event* event = fEvents.ItemAt(index);
		event->Unregister(registrator);
	}
}


void
EventContainer::Trigger(Event* origin)
{
	Event::Trigger(origin);

	if (Parent() == NULL && Owner() != NULL) {
		BMessage message(kMsgEventTriggered);
		message.AddPointer("event", origin);
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
EventContainer::AddEventsToString(BString& string) const
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


void
OrEvent::ResetTrigger()
{
	fTriggered = false;

	int32 count = fEvents.CountItems();
	for (int32 index = 0; index < count; index++) {
		Event* event = fEvents.ItemAt(index);
		event->ResetTrigger();
		fTriggered |= event->Triggered();
	}
}


BString
OrEvent::ToString() const
{
	BString string = "or ";
	EventContainer::AddEventsToString(string);
	return string;
}


// #pragma mark - StickyEvent


StickyEvent::StickyEvent(Event* parent)
	:
	Event(parent)
{
}


StickyEvent::~StickyEvent()
{
}


void
StickyEvent::ResetSticky()
{
	Event::ResetTrigger();
}


void
StickyEvent::ResetTrigger()
{
	// This is a sticky event; we don't reset the trigger here
}


// #pragma mark - demand


DemandEvent::DemandEvent(Event* parent)
	:
	Event(parent)
{
}


status_t
DemandEvent::Register(EventRegistrator& registrator)
{
	return B_OK;
}


void
DemandEvent::Unregister(EventRegistrator& registrator)
{
}


BString
DemandEvent::ToString() const
{
	return "demand";
}


// #pragma mark - External event


ExternalEvent::ExternalEvent(Event* parent, const char* name,
	const BMessage& args)
	:
	Event(parent),
	fName(name),
	fFlags(0),
	fResolved(false)
{
	const char* argument;
	for (int32 index = 0; args.FindString("args", index, &argument) == B_OK;
			index++) {
		fArguments.Add(argument);
	}
}


const BString&
ExternalEvent::Name() const
{
	return fName;
}


bool
ExternalEvent::Resolve(uint32 flags)
{
	if (fResolved)
		return false;

	fResolved = true;
	fFlags = flags;
	return true;
}


void
ExternalEvent::ResetSticky()
{
	if ((fFlags & B_STICKY_EVENT) != 0)
		Event::ResetTrigger();
}


void
ExternalEvent::ResetTrigger()
{
	if ((fFlags & B_STICKY_EVENT) == 0)
		Event::ResetTrigger();
}


status_t
ExternalEvent::Register(EventRegistrator& registrator)
{
	return registrator.RegisterExternalEvent(this, Name().String(), fArguments);
}


void
ExternalEvent::Unregister(EventRegistrator& registrator)
{
	registrator.UnregisterExternalEvent(this, Name().String());
}


BString
ExternalEvent::ToString() const
{
	return fName;
}


// #pragma mark - file_created


FileCreatedEvent::FileCreatedEvent(Event* parent, const BMessage& args)
	:
	Event(parent)
{
	fPath.SetTo(args.GetString("args", NULL));
}


status_t
FileCreatedEvent::Register(EventRegistrator& registrator)
{
	// TODO: implement!
	return B_ERROR;
}


void
FileCreatedEvent::Unregister(EventRegistrator& registrator)
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


VolumeMountedEvent::VolumeMountedEvent(Event* parent, const BMessage& args)
	:
	Event(parent)
{
}


status_t
VolumeMountedEvent::Register(EventRegistrator& registrator)
{
	VolumeWatcher::Register(this);
	return B_OK;
}


void
VolumeMountedEvent::Unregister(EventRegistrator& registrator)
{
	VolumeWatcher::Unregister(this);
}


BString
VolumeMountedEvent::ToString() const
{
	return "volume_mounted";
}


void
VolumeMountedEvent::VolumeMounted(dev_t device)
{
	Trigger(this);
}


void
VolumeMountedEvent::VolumeUnmounted(dev_t device)
{
}


// #pragma mark -


NetworkAvailableEvent::NetworkAvailableEvent(Event* parent,
	const BMessage& args)
	:
	StickyEvent(parent)
{
}


status_t
NetworkAvailableEvent::Register(EventRegistrator& registrator)
{
	NetworkWatcher::Register(this);
	return B_OK;
}


void
NetworkAvailableEvent::Unregister(EventRegistrator& registrator)
{
	NetworkWatcher::Unregister(this);
}


BString
NetworkAvailableEvent::ToString() const
{
	return "network_available";
}


void
NetworkAvailableEvent::NetworkAvailabilityChanged(bool available)
{
	if (available)
		Trigger(this);
	else
		ResetSticky();
}


// #pragma mark -


/*static*/ Event*
Events::FromMessage(const BMessenger& target, const BMessage& message)
{
	return create_event(NULL, "or", &target, message);
}


/*static*/ Event*
Events::AddOnDemand(const BMessenger& target, Event* event)
{
	OrEvent* orEvent = dynamic_cast<OrEvent*>(event);
	if (orEvent == NULL) {
		EventContainer* container = dynamic_cast<EventContainer*>(event);
		if (container != NULL)
			orEvent = new OrEvent(container->Owner(), container->Target());
		else
			orEvent = new OrEvent(NULL, target);
	}
	if (orEvent != event && event != NULL)
		orEvent->AddEvent(event);

	orEvent->AddEvent(new DemandEvent(orEvent));
	return orEvent;
}


/*static*/ Event*
Events::ResolveExternalEvent(Event* event, const char* name, uint32 flags)
{
	if (event == NULL)
		return NULL;

	if (EventContainer* container = dynamic_cast<EventContainer*>(event)) {
		for (int32 index = 0; index < container->Events().CountItems();
				index++) {
			Event* event = ResolveExternalEvent(container->Events().ItemAt(index), name, flags);
			if (event != NULL)
				return event;
		}
	} else if (ExternalEvent* external = dynamic_cast<ExternalEvent*>(event)) {
		if (external->Name() == name && external->Resolve(flags))
			return external;
	}

	return NULL;
}


/*static*/ void
Events::TriggerExternalEvent(Event* event)
{
	if (event == NULL)
		return;

	ExternalEvent* external = dynamic_cast<ExternalEvent*>(event);
	if (external == NULL)
		return;

	external->Trigger(external);
}


/*static*/ void
Events::ResetStickyExternalEvent(Event* event)
{
	if (event == NULL)
		return;

	ExternalEvent* external = dynamic_cast<ExternalEvent*>(event);
	if (external == NULL)
		return;

	external->ResetSticky();
}


/*!	This will trigger a demand event, if it exists.

	\param testOnly If \c true, the deman will not actually be triggered,
			it will only be checked if it could.
	\return \c true, if there is a demand event, and it has been
			triggered by this call. \c false if not.
*/
/*static*/ bool
Events::TriggerDemand(Event* event, bool testOnly)
{
	if (event == NULL || event->Triggered())
		return false;

	if (EventContainer* container = dynamic_cast<EventContainer*>(event)) {
		for (int32 index = 0; index < container->Events().CountItems();
				index++) {
			Event* childEvent = container->Events().ItemAt(index);
			if (dynamic_cast<DemandEvent*>(childEvent) != NULL) {
				if (testOnly)
					return true;

				childEvent->Trigger(childEvent);
				break;
			}
			if (dynamic_cast<EventContainer*>(childEvent) != NULL) {
				if (TriggerDemand(childEvent, testOnly))
					break;
			}
		}
	}

	return event->Triggered();
}
