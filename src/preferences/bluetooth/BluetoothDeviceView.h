/*
 * Copyright 2008-09, Oliver Ruiz Dorantes, <oliver.ruiz.dorantes_at_gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef BLUETOOTHDEVICEVIEW_H_
#define BLUETOOTHDEVICEVIEW_H_

#include <Box.h>
#include <Bitmap.h>
#include <Invoker.h>
#include <Message.h>
#include <View.h>

#include <bluetooth/BluetoothDevice.h>


class BStringView;
class BitmapView;

class BluetoothDeviceView : public BView
{
public:
	BluetoothDeviceView(BRect frame, BluetoothDevice* bDevice,
		uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP,
		uint32 flags = B_WILL_DRAW);
	~BluetoothDeviceView(void);

			void SetBluetoothDevice(BluetoothDevice* bDevice);

	virtual void MessageReceived(BMessage* message);
	virtual void SetTarget(BHandler* target);
	virtual void SetEnabled(bool value);

protected:
	BluetoothDevice*	fDevice;

	BStringView*		fName;
	BStringView*		fBdaddr;
	BStringView*		fClassService;
	BStringView*		fClass;

	BStringView*		fHCIVersionProperties;
	BStringView*		fLMPVersionProperties;
	BStringView*		fManufacturerProperties;

	BStringView*		fACLBuffersProperties;
	BStringView*		fSCOBuffersProperties;

	BView*				fIcon;
};


#endif
