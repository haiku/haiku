/*
 * Copyright 2008-09, Oliver Ruiz Dorantes, <oliver.ruiz.dorantes_at_gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef BLUETOOTHDEVICEVIEW_H_
#define BLUETOOTHDEVICEVIEW_H_

#include <View.h>
#include <Message.h>
#include <Invoker.h>
#include <Box.h>
#include <Bitmap.h>

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
	
			void SetBluetoothDevice(BluetoothDevice* bdev);

	virtual void MessageReceived(BMessage *msg);
	virtual void SetTarget(BHandler *tgt);
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
	
	BStringView*		fBuffersProperties;

	BitmapView*			fIcon;

};


class BitmapView : public BView
{
	public:
		BitmapView(BBitmap *bitmap) : BView(bitmap->Bounds(), NULL,
			B_FOLLOW_NONE, B_WILL_DRAW)
		{
			fBitmap = bitmap;

			SetDrawingMode(B_OP_ALPHA);
			SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		}

		~BitmapView()
		{
			delete fBitmap;
		}

		virtual void AttachedToWindow()
		{
			//SetViewColor(Parent()->ViewColor());

			//MoveTo((Parent()->Bounds().Width() - Bounds().Width()) / 2,	Frame().top);
		}

		virtual void Draw(BRect updateRect)
		{
			DrawBitmap(fBitmap, updateRect, updateRect);
		}

	private:
		BBitmap *fBitmap;
};




#endif
