/*
 * Copyright 2009, Oliver Ruiz Dorantes, <oliver.ruiz.dorantes_at_gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef DEVICELISTITEM_H_
#define DEVICELISTITEM_H_

#include <ListItem.h>
#include <String.h>

#include "bluetooth/RemoteDevice.h"

namespace Bluetooth {

class DeviceListItem : public BListItem
{
	public:
		DeviceListItem(RemoteDevice*	bDevice);

		~DeviceListItem();

		void DrawItem(BView*, BRect, bool = false);
		void Update(BView* owner, const BFont* font);

		static int Compare(const void* firstArg, const void* secondArg);
		void SetDevice(RemoteDevice* bDevice);
		RemoteDevice* Device() const;

	private:
		RemoteDevice*	fDevice;
		bdaddr_t			fAddress;
		DeviceClass			fClass;
		BString				fName;
		int32				fRSSI;

};

}


#endif
