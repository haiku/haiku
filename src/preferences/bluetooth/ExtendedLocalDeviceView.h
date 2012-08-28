/*
 * Copyright 2008-09, Oliver Ruiz Dorantes, <oliver.ruiz.dorantes_at_gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef EXTENDEDLOCALDEVICEVIEW_H_
#define EXTENDEDLOCALDEVICEVIEW_H_

#include <View.h>
#include <Message.h>
#include <Invoker.h>
#include <Box.h>
#include <Bitmap.h>

#include <bluetooth/LocalDevice.h>

#include "BluetoothDeviceView.h"

class BStringView;
class BitmapView;
class BCheckBox;

class ExtendedLocalDeviceView : public BView
{
public:
	ExtendedLocalDeviceView(BRect frame, LocalDevice* bDevice,
		uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP,
		uint32 flags = B_WILL_DRAW);
	~ExtendedLocalDeviceView(void);

	void SetLocalDevice(LocalDevice* lDevice);


	virtual void MessageReceived(BMessage* message);
	virtual void AttachedToWindow();
	virtual void SetTarget(BHandler* target);
	virtual void SetEnabled(bool value);
			void ClearDevice();

protected:
	LocalDevice*		fDevice;
	BCheckBox*			fAuthentication;
	BCheckBox*			fDiscoverable;
	BCheckBox*			fVisible;
	BluetoothDeviceView* fDeviceView;
	uint8 fScanMode;

};


#endif
