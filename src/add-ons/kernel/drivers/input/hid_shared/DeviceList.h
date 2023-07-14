/*
	Generic device list for use in drivers.
	Copyright (C) 2008 Michael Lotz <mmlr@mlotz.ch>
	Distributed under the terms of the MIT license.
*/
#ifndef _DEVICE_LIST_H_
#define _DEVICE_LIST_H_

#include <OS.h>

struct device_list_entry;

class DeviceList {
public:
								DeviceList();
								~DeviceList();

		status_t				AddDevice(const char *name, void *device);
		status_t				RemoveDevice(const char *name, void *device = NULL);
		void *					FindDevice(const char *name, void *device = NULL);

		int32					CountDevices(const char *baseName = NULL);
		void *					DeviceAt(int32 index);

		const char **			PublishDevices();

private:
		void					_FreePublishList();

		device_list_entry *		fDeviceList;
		int32					fDeviceCount;
		char **					fPublishList;
};

#endif // _DEVICE_LIST_H_
