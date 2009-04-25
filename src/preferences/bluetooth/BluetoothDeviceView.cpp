/*
 * Copyright 2008-09, Oliver Ruiz Dorantes, <oliver.ruiz.dorantes_at_gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "BluetoothDeviceView.h"
#include <bluetooth/bdaddrUtils.h>

#include <bluetooth/LocalDevice.h>
#include <bluetooth/HCI/btHCI_command.h>


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

	SetLayout(new BGroupLayout(B_VERTICAL));

	fName = new BStringView("name", "");
	fName->SetFont(be_bold_font);
		
	fBdaddr = new BStringView("bdaddr", bdaddrUtils::ToString(bdaddrUtils::NullAddress()));
	
	fClassService = new BStringView("ServiceClass", "Service Classes: ");

	fClass = new BStringView("class", "- / -");

	fHCIVersionProperties = new BStringView("hci", "");
	fLMPVersionProperties = new BStringView("lmp", "");
	fManufacturerProperties = new BStringView("manufacturer", "");
	fBuffersProperties = new BStringView("buffers", "");

	fIcon = new BView(BRect(0, 0, 32 - 1, 32 - 1),"Icon", B_FOLLOW_ALL, B_WILL_DRAW);
	fIcon->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	SetBluetoothDevice(bDevice);

	AddChild(BGroupLayoutBuilder(B_HORIZONTAL, 10)
				.Add(fIcon)
				.AddGlue()
				.Add(BGroupLayoutBuilder(B_VERTICAL)
						.Add(fName)
						.AddGlue()
						.Add(fBdaddr)
						.AddGlue()
						.AddGlue()
						.Add(fClass)
						.AddGlue()
						.Add(fClassService)
						.AddGlue()
						.Add(fHCIVersionProperties)
						.AddGlue()
						.Add(fLMPVersionProperties)
						.AddGlue()
						.Add(fManufacturerProperties)
						.AddGlue()
						.Add(fBuffersProperties)
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
		
		bDevice->GetDeviceClass().Draw(fIcon, BPoint(Bounds().left, Bounds().top));
		
		uint32 value;
		
		str = "";
		if (bDevice->GetProperty("hci_version", &value) == B_OK)		
			str << "HCI ver: " << GetHciVersion(value);
		if (bDevice->GetProperty("hci_revision", &value) == B_OK)		
			str << " HCI rev: " << value ;
		
		fHCIVersionProperties->SetText(str.String());	
		
		str = "";
		if (bDevice->GetProperty("lmp_version", &value) == B_OK)		
			str << "LMP ver: " << GetLmpVersion(value);
		if (bDevice->GetProperty("lmp_subversion", &value) == B_OK)		
			str << " LMP subver: " << value;
		fLMPVersionProperties->SetText(str.String());		
		
		str = "";	
		if (bDevice->GetProperty("manufacturer", &value) == B_OK)		
			str << "Manufacturer: " << GetManufacturer(value);
		fManufacturerProperties->SetText(str.String());

		str = "";
		if (bDevice->GetProperty("acl_mtu", &value) == B_OK)		
			str << "ACL mtu: " << value;
		if (bDevice->GetProperty("acl_max_pkt", &value) == B_OK)		
			str << " packets: " << value;
		if (bDevice->GetProperty("sco_mtu", &value) == B_OK)		
			str << " SCO mtu: " << value;
		if (bDevice->GetProperty("sco_max_pkt", &value) == B_OK)		
			str << " packets: " << value;
			
		fBuffersProperties->SetText(str.String());

		

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
