/*
 * Copyright 2009, Oliver Ruiz Dorantes, <oliver.ruiz.dorantes_at_gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef DEVICELISTITEM_H_
#define DEVICELISTITEM_H_

#include <ListItem.h>
#include <String.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/DeviceClass.h>

class BluetoothDevice;

namespace Bluetooth {

class DeviceListItem : public BListItem
{
	public:
		DeviceListItem(BluetoothDevice*	bDevice);

		~DeviceListItem();

		void DrawItem(BView*, BRect, bool = false);
		void Update(BView* owner, const BFont* font);

		static int Compare(const void* firstArg, const void* secondArg);
		void SetDevice(BluetoothDevice* bDevice);
		BluetoothDevice* Device() const;

	private:
		BluetoothDevice*	fDevice;
		bdaddr_t			fAddress;
		DeviceClass			fClass;
		BString				fName;
		int32				fRSSI;

};

}


#endif
