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
	void _SetProperties(int fd, const char* path, BMidiEndpoint* endp);
	status_t _GetVectorIcon(int fd, uint8** _data, size_t * _size);

	typedef HashMap<HashString, DeviceEndpoints*> DeviceEndpointsMap;
	DeviceEndpointsMap		fDeviceEndpointsMap;

	uint8* fDefaultVectorIconData;
	size_t fDefaultVectorIconDataSize;
	BBitmap* fDefaultLargeIcon;
	BBitmap* fDefaultMiniIcon;
};

#endif // DEVICE_WATCHER_H
