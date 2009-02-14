/*
 * Copyright 2008-09, Oliver Ruiz Dorantes, <oliver.ruiz.dorantes_at_gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "BluetoothDeviceView.h"
#include <bluetooth/bdaddrUtils.h>


#include <Bitmap.h>
#include <GroupLayoutBuilder.h>
#include <SpaceLayoutItem.h>
#include <StringView.h>
#include <TextView.h>


BluetoothDeviceView::BluetoothDeviceView(BRect frame, BluetoothDevice* bDevice, uint32 resizingMode, uint32 flags)
 :	BView(frame,"BluetoothDeviceView", resizingMode, flags | B_WILL_DRAW), 
    fDevice(bDevice)
{
	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(0,0,0);
	BRect iDontCare(0,0,0,0);

	SetLayout(new BGroupLayout(B_VERTICAL));

	fName = new BStringView(iDontCare, "name", "");
	fName->SetFont(be_bold_font);
		
	fBdaddr = new BStringView(iDontCare, "bdaddr", bdaddrUtils::ToString(bdaddrUtils::NullAddress()));
	
	fClassService = new BStringView(iDontCare, "ServiceClass", "Service Classes: ");

	fClass = new BStringView(iDontCare, "class", "- / -");

	fIcon = new BitmapView(new BBitmap(BRect(0, 0, 64 - 1, 64 - 1), B_RGBA32));
	
	fIcon->SetViewColor(0,0,0);
	
	SetBluetoothDevice(bDevice);

	AddChild(BGroupLayoutBuilder(B_HORIZONTAL, 10)
				.Add(fIcon)
				.Add(BGroupLayoutBuilder(B_VERTICAL)
						.Add(fName)
						.AddGlue()			
						.Add(fBdaddr)
						.AddGlue()
						.AddGlue()
						.Add(fClass)
						.AddGlue()
						.Add(fClassService)
						
						.SetInsets(5, 25, 25, 25)
					)
				.Add(BSpaceLayoutItem::CreateHorizontalStrut(10)
			).SetInsets(20, 20, 20, 20)
	);

}


BluetoothDeviceView::~BluetoothDeviceView(void)
{

}


void
BluetoothDeviceView::SetBluetoothDevice(BluetoothDevice* bDevice)
{
	if (bDevice != NULL) {

		SetName(bDevice->GetFriendlyName().String());	
		
		fName->SetText(bDevice->GetFriendlyName().String());
		fBdaddr->SetText(bdaddrUtils::ToString(bDevice->GetBluetoothAddress()));
	
		BString str("Service Classes: ");
		bDevice->GetDeviceClass().GetServiceClass(str);
		fClassService->SetText(str.String());
	
		str = "";
		bDevice->GetDeviceClass().GetMajorDeviceClass(str);
		str << " / ";
		bDevice->GetDeviceClass().GetMinorDeviceClass(str);
		fClass->SetText(str.String());	

	}
	
	
}


void
BluetoothDeviceView::SetTarget(BHandler *tgt)
{

}


void
BluetoothDeviceView::MessageReceived(BMessage *msg)
{
	// If we received a dropped message, try to see if it has color data
	// in it
	if(msg->WasDropped()) {

	}

	// The default
	BView::MessageReceived(msg);
}


void
BluetoothDeviceView::SetEnabled(bool value)
{

}
