/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef OBSERVABLE_H
#define OBSERVABLE_H

#include <List.h>

class Observer;

class Observable {
 public:
								Observable();
	virtual						~Observable();

			bool				AddObserver(Observer* observer);
			bool				RemoveObserver(Observer* observer);

			int32				CountObservers() const;
			Observer*			ObserverAtFast(int32 index) const;
	
	 		void				Notify() const;

			void				SuspendNotifications(bool suspend);

			bool				HasPendingNotifications() const
									{ return fPendingNotifications; }

 private:
			BList				fObservers;

			int32				fSuspended;
	mutable	bool				fPendingNotifications;
};

class AutoNotificationSuspender {
 public:
								AutoNotificationSuspender(Observable* object)
									: fObject(object)
								{
									fObject->SuspendNotifications(true);
								}

	virtual						~AutoNotificationSuspender()
								{
									fObject->SuspendNotifications(false);
								}
 private:
			Observable*			fObject;
};

#endif // OBSERVABLE_H
