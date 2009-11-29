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

#include "HashMap.h"
#include "HashString.h"

class BBitmap;
class BMidiEndpoint;
class DeviceEndpoints;

class DeviceWatcher : public BLooper {
public:
				DeviceWatcher();
				~DeviceWatcher();
	
	void		MessageReceived(BMessage* message);

	status_t 	Start();
	status_t	Stop();

private:
	static int32 _InitialDevicesScanThread(void* data);
	void _ScanDevices(const char* path);
	void _AddDevice(const char* path);
	void _RemoveDevice(const char* path);
	void _SetIcons(BMidiEndpoint* endp);

	typedef HashMap<HashString, DeviceEndpoints*> DeviceEndpointsMap;
	DeviceEndpointsMap		fDeviceEndpointsMap;

	uint8* fVectorIconData;
	size_t fVectorIconDataSize;
	BBitmap* fLargeIcon;
	BBitmap* fMiniIcon;
};

#endif // DEVICE_WATCHER_H
