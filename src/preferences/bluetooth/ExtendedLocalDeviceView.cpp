/*
 * Copyright 2008-09, Oliver Ruiz Dorantes, <oliver.ruiz.dorantes_at_gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "ExtendedLocalDeviceView.h"

#include <bluetooth/bdaddrUtils.h>

#include "defs.h"

#include <Bitmap.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <GroupLayoutBuilder.h>
#include <SpaceLayoutItem.h>
#include <StringView.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Extended local device view"

ExtendedLocalDeviceView::ExtendedLocalDeviceView(BRect frame,
	LocalDevice* bDevice, uint32 resizingMode, uint32 flags)
	:
	BView(frame,"ExtendedLocalDeviceView", resizingMode, flags | B_WILL_DRAW),
	fDevice(bDevice),
	fScanMode(0)
{
	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(0, 0, 0);
	BRect iDontCare(0, 0, 0, 0);

	SetLayout(new BGroupLayout(B_HORIZONTAL));

	fDeviceView = new BluetoothDeviceView(BRect(0, 0, 5, 5), bDevice);

	fDiscoverable = new BCheckBox(iDontCare, "Discoverable",
		B_TRANSLATE("Discoverable"), new BMessage(SET_DISCOVERABLE));
	fVisible = new BCheckBox(iDontCare, "Visible",
		B_TRANSLATE("Show name"), new BMessage(SET_VISIBLE));
	fAuthentication = new BCheckBox(iDontCare, "Authenticate",
		B_TRANSLATE("Authenticate"), new BMessage(SET_AUTHENTICATION));

	fDiscoverable->SetEnabled(false);
	fVisible->SetEnabled(false);

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 0)
				.Add(fDeviceView)
				.Add(BGroupLayoutBuilder(B_HORIZONTAL)
						.AddGlue()
						.Add(fDiscoverable)
						.Add(fVisible)
						.SetInsets(5, 5, 5, 5)
					)
				.Add(fAuthentication)
				.Add(BSpaceLayoutItem::CreateVerticalStrut(0))
			.SetInsets(5, 5, 5, 5)
	);

}


ExtendedLocalDeviceView::~ExtendedLocalDeviceView(void)
{

}


void
ExtendedLocalDeviceView::SetLocalDevice(LocalDevice* lDevice)
{
	if (lDevice != NULL) {
		fDevice = lDevice;
		SetName(lDevice->GetFriendlyName().String());
		fDeviceView->SetBluetoothDevice(lDevice);
	}
}


void
ExtendedLocalDeviceView::AttachedToWindow()
{
	fDiscoverable->SetTarget(this);
	fVisible->SetTarget(this);
	fAuthentication->SetTarget(this);
}


void
ExtendedLocalDeviceView::SetTarget(BHandler* target)
{

}


void
ExtendedLocalDeviceView::MessageReceived(BMessage* message)
{
	if (message->WasDropped()) {

	}

	switch (message->what)
	{
		case SET_DISCOVERABLE:
		case SET_VISIBLE:
			fScanMode = 0;

			if (fDiscoverable->Value()) {
				fScanMode = 1;
				fVisible->SetEnabled(true);
			} else {
				fVisible->SetValue(false);
				fVisible->SetEnabled(false);
			}

			if (fVisible->Value())
				fScanMode |= 2;
			if (fDevice != NULL)
				fDevice->SetDiscoverable(fScanMode);

			break;
		case SET_AUTHENTICATION:
			fDevice->SetAuthentication(fAuthentication->Value());
			break;

		default:
			BView::MessageReceived(message);
		break;
	}
}


void
ExtendedLocalDeviceView::SetEnabled(bool value)
{
	fDiscoverable->SetEnabled(value);
}
