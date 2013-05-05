/*
 * Copyright 2008-09, Oliver Ruiz Dorantes, <oliver.ruiz.dorantes_at_gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "BluetoothDeviceView.h"
#include <bluetooth/bdaddrUtils.h>

#include <bluetooth/LocalDevice.h>
#include <bluetooth/HCI/btHCI_command.h>


#include <Bitmap.h>
#include <Catalog.h>
#include <GroupLayoutBuilder.h>
#include <Locale.h>
#include <SpaceLayoutItem.h>
#include <StringView.h>
#include <TextView.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Device View"

BluetoothDeviceView::BluetoothDeviceView(BRect frame, BluetoothDevice* bDevice,
	uint32 resizingMode, uint32 flags)
	:
	BView(frame,"BluetoothDeviceView", resizingMode, flags | B_WILL_DRAW),
	fDevice(bDevice)
{
	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(0, 0, 0);

	SetLayout(new BGroupLayout(B_VERTICAL));

	fName = new BStringView("name", "");
	fName->SetFont(be_bold_font);
	fName->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_MIDDLE));

	fBdaddr = new BStringView("bdaddr",
		bdaddrUtils::ToString(bdaddrUtils::NullAddress()));
	fBdaddr->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_MIDDLE));

	fClassService = new BStringView("ServiceClass",
		B_TRANSLATE("Service classes: "));
	fClassService->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
		B_ALIGN_MIDDLE));

	fClass = new BStringView("class", "- / -");
	fClass->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_MIDDLE));

	fHCIVersionProperties = new BStringView("hci", "");
	fHCIVersionProperties->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
		B_ALIGN_MIDDLE));
	fLMPVersionProperties = new BStringView("lmp", "");
	fLMPVersionProperties->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
		B_ALIGN_MIDDLE));
	fManufacturerProperties = new BStringView("manufacturer", "");
	fManufacturerProperties->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
		B_ALIGN_MIDDLE));
	fACLBuffersProperties = new BStringView("buffers acl", "");
	fACLBuffersProperties->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
		B_ALIGN_MIDDLE));
	fSCOBuffersProperties = new BStringView("buffers sco", "");
	fSCOBuffersProperties->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
		B_ALIGN_MIDDLE));


	fIcon = new BView(BRect(0, 0, 32 - 1, 32 - 1), "Icon", B_FOLLOW_ALL,
		B_WILL_DRAW);
	fIcon->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	SetBluetoothDevice(bDevice);

	AddChild(BGroupLayoutBuilder(B_HORIZONTAL, 5)
				.Add(fIcon)
				.AddGlue()
				.Add(BGroupLayoutBuilder(B_VERTICAL)
						.Add(fName)
						.AddGlue()
						.Add(fBdaddr)
						.AddGlue()
						.Add(fClass)
						.AddGlue()
						.Add(fClassService)
						.AddGlue()
						.AddGlue()
						.Add(fHCIVersionProperties)
						.AddGlue()
						.Add(fLMPVersionProperties)
						.AddGlue()
						.Add(fManufacturerProperties)
						.AddGlue()
						.Add(fACLBuffersProperties)
						.AddGlue()
						.Add(fSCOBuffersProperties)
						.SetInsets(5, 5, 5, 5)
					)
			//	.Add(BSpaceLayoutItem::CreateHorizontalStrut(5))
			.SetInsets(10, 10, 10, 10)
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

		BString string(B_TRANSLATE("Service classes: "));
		bDevice->GetDeviceClass().GetServiceClass(string);
		fClassService->SetText(string.String());

		string = "";
		bDevice->GetDeviceClass().GetMajorDeviceClass(string);
		string << " / ";
		bDevice->GetDeviceClass().GetMinorDeviceClass(string);
		fClass->SetText(string.String());

		bDevice->GetDeviceClass().Draw(fIcon, BPoint(Bounds().left, Bounds().top));

		uint32 value;

		string = "";
		if (bDevice->GetProperty("hci_version", &value) == B_OK)
			string << "HCI ver: " << BluetoothHciVersion(value);
		if (bDevice->GetProperty("hci_revision", &value) == B_OK)
			string << " HCI rev: " << value ;

		fHCIVersionProperties->SetText(string.String());

		string = "";
		if (bDevice->GetProperty("lmp_version", &value) == B_OK)
			string << "LMP ver: " << BluetoothLmpVersion(value);
		if (bDevice->GetProperty("lmp_subversion", &value) == B_OK)
			string << " LMP subver: " << value;
		fLMPVersionProperties->SetText(string.String());

		string = "";
		if (bDevice->GetProperty("manufacturer", &value) == B_OK)
			string << B_TRANSLATE("Manufacturer: ")
			   	<< BluetoothManufacturer(value);
		fManufacturerProperties->SetText(string.String());

		string = "";
		if (bDevice->GetProperty("acl_mtu", &value) == B_OK)
			string << "ACL mtu: " << value;
		if (bDevice->GetProperty("acl_max_pkt", &value) == B_OK)
			string << B_TRANSLATE(" packets: ") << value;
		fACLBuffersProperties->SetText(string.String());

		string = "";
		if (bDevice->GetProperty("sco_mtu", &value) == B_OK)
			string << "SCO mtu: " << value;
		if (bDevice->GetProperty("sco_max_pkt", &value) == B_OK)
			string << B_TRANSLATE(" packets: ") << value;
		fSCOBuffersProperties->SetText(string.String());

	}

}


void
BluetoothDeviceView::SetTarget(BHandler* target)
{

}


void
BluetoothDeviceView::MessageReceived(BMessage* message)
{
	// If we received a dropped message, try to see if it has color data
	// in it
	if (message->WasDropped()) {

	}

	// The default
	BView::MessageReceived(message);
}


void
BluetoothDeviceView::SetEnabled(bool value)
{

}
