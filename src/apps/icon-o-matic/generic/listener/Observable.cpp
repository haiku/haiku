/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "Observable.h"

#include <stdio.h>
#include <typeinfo>

#include <OS.h>

#include "Observer.h"

// constructor
Observable::Observable()
	: fObservers(2),
	  fSuspended(0),
	  fPendingNotifications(false)
{
}

// destructor
Observable::~Observable()
{
	if (fObservers.CountItems() > 0) {
		char message[256];
		Observer* o = (Observer*)fObservers.ItemAt(0);
		sprintf(message, "Observable::~Observable() - %ld "
						 "observers still watching, first: %s\n",
						 fObservers.CountItems(), typeid(*o).name());
		debugger(message);
	}
}

// AddObserver
bool
Observable::AddObserver(Observer* observer)
{
	if (observer && !fObservers.HasItem((void*)observer)) {
		return fObservers.AddItem((void*)observer);
	}
	return false;
}

// RemoveObserver
bool
Observable::RemoveObserver(Observer* observer)
{
	return fObservers.RemoveItem((void*)observer);
}

// CountObservers
int32
Observable::CountObservers() const
{
	return fObservers.CountItems();
}

// ObserverAtFast
Observer*
Observable::ObserverAtFast(int32 index) const
{
	return (Observer*)fObservers.ItemAtFast(index);
}

// #pragma mark -

// Notify
void
Observable::Notify() const
{
	if (!fSuspended) {
		BList observers(fObservers);
		int32 count = observers.CountItems();
		for (int32 i = 0; i < count; i++)
			((Observer*)observers.ItemAtFast(i))->ObjectChanged(this);
		fPendingNotifications = false;
	} else {
		fPendingNotifications = true;
	}
}

// SuspendNotifications
void
Observable::SuspendNotifications(bool suspend)
{
	if (suspend)
		fSuspended++;
	else
		fSuspended--;

	if (fSuspended < 0) {
		fprintf(stderr, "Observable::SuspendNotifications(false) - "
						"error: suspend level below zero!\n");
		fSuspended = 0;
	}

	if (!fSuspended && fPendingNotifications)
		Notify();
}

