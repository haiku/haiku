/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef VOLUME_WATCHER_H
#define VOLUME_WATCHER_H


#include <Handler.h>
#include <ObjectList.h>


class VolumeListener {
public:
	virtual						~VolumeListener();

	virtual	void				VolumeMounted(dev_t device) = 0;
	virtual	void				VolumeUnmounted(dev_t device) = 0;
};


class VolumeWatcher : public BHandler {
public:
								VolumeWatcher();
	virtual						~VolumeWatcher();

			void				AddListener(VolumeListener* listener);
			void				RemoveListener(VolumeListener* listener);
			int32				CountListeners() const;

	virtual	void				MessageReceived(BMessage* message);

	static	void				Register(VolumeListener* listener);
	static	void				Unregister(VolumeListener* listener);

protected:
			BObjectList<VolumeListener>
								fListeners;
};


#endif // VOLUME_WATCHER_H
