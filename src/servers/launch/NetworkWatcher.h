/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef NETWORK_WATCHER_H
#define NETWORK_WATCHER_H


#include <Handler.h>
#include <ObjectList.h>


class NetworkListener {
public:
	virtual						~NetworkListener();

	virtual	void				NetworkAvailabilityChanged(bool available) = 0;
};


class NetworkWatcher : public BHandler {
public:
								NetworkWatcher();
	virtual						~NetworkWatcher();

			void				AddListener(NetworkListener* listener);
			void				RemoveListener(NetworkListener* listener);
			int32				CountListeners() const;

	virtual	void				MessageReceived(BMessage* message);

	static	void				Register(NetworkListener* listener);
	static	void				Unregister(NetworkListener* listener);

	static	bool				NetworkAvailable(bool immediate);

protected:
			void				UpdateAvailability();

protected:
			BObjectList<NetworkListener>
								fListeners;
			bool				fAvailable;
};


#endif // NETWORK_WATCHER_H
