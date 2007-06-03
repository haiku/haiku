/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef NOTIFIER_H
#define NOTIFIER_H

#include <List.h>

class Listener;

class Notifier {
 public:
								Notifier();
	virtual						~Notifier();

			bool				AddListener(Listener* listener);
			bool				RemoveListener(Listener* listener);

			int32				CountListeners() const;
			Listener*			ListenerAtFast(int32 index) const;
	
	 		void				Notify() const;

			void				SuspendNotifications(bool suspend);

			bool				HasPendingNotifications() const
									{ return fPendingNotifications; }

 private:
			BList				fListeners;

			int32				fSuspended;
	mutable	bool				fPendingNotifications;
};

class AutoNotificationSuspender {
 public:
								AutoNotificationSuspender(Notifier* object)
									: fObject(object)
								{
									fObject->SuspendNotifications(true);
								}

	virtual						~AutoNotificationSuspender()
								{
									fObject->SuspendNotifications(false);
								}
 private:
			Notifier*			fObject;
};

#endif // NOTIFIER_H
