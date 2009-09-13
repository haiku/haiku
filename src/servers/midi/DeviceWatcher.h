/*
 * Copyright 2004-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Matthijs Hollemans
 *		Jerome Leveque
 *		Philippe Houdoin
 */
#ifndef DEVICE_WATCHER_H
#define DEVICE_WATCHER_H

#include <Looper.h>


class BBitmap;
class BMidiEndpoint;

class DeviceWatcher : public BLooper {
public:
				DeviceWatcher();
				~DeviceWatcher();
	
	void		MessageReceived(BMessage* message);

	status_t 	Start();
	status_t	Stop();

private:
	void _ScanDevices(const char* path);
	void _AddDevice(const char* path);
	void _RemoveDevice(const char* path);
	void _SetIcons(BMidiEndpoint* endp);

	BBitmap* fLargeIcon;
	BBitmap* fMiniIcon;
};

#endif // DEVICE_WATCHER_H
