/*
 * Copyright 2009, Oliver Ruiz Dorantes, <oliver.ruiz.dorantes_at_gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <Bitmap.h>
#include <View.h>

#include <bluetooth/bdaddrUtils.h>
#include <bluetooth/BluetoothDevice.h>
/*#include "../media/iconfile.h"*/

#include "DeviceListItem.h"

#define INSETS  5
#define TEXT_ROWS  2

namespace Bluetooth {

DeviceListItem::DeviceListItem(BluetoothDevice* bDevice)
	:
	BListItem(),
	fDevice(bDevice),
	fName("unknown")
{
	fAddress = bDevice->GetBluetoothAddress();
	fClass = bDevice->GetDeviceClass();
}


void
DeviceListItem::SetDevice(BluetoothDevice* bDevice)
{
	fAddress = bDevice->GetBluetoothAddress();
	fClass = bDevice->GetDeviceClass();
	fName = bDevice->GetFriendlyName();
	// AKAIR rssi we can just have it @ inquiry time...
}


DeviceListItem::~DeviceListItem()
{

}


void
DeviceListItem::DrawItem(BView* owner, BRect itemRect, bool	complete)
{
	rgb_color	kBlack = { 0, 0, 0, 0 };
	rgb_color	kHighlight = { 156, 154, 156, 0 };

	if (IsSelected() || complete) {
		rgb_color	color;
		if (IsSelected())
			color = kHighlight;
		else
			color = owner->ViewColor();

		owner->SetHighColor(color);
		owner->SetLowColor(color);
		owner->FillRect(itemRect);
		owner->SetHighColor(kBlack);

	} else {
		owner->SetLowColor(owner->ViewColor());
	}

	font_height	finfo;
	be_plain_font->GetHeight(&finfo);

	BPoint point = BPoint(itemRect.left	+ DeviceClass::PixelsForIcon
		+ 2 * INSETS, itemRect.bottom - finfo.descent + 1);
	owner->SetFont(be_fixed_font);
	owner->SetHighColor(kBlack);
	owner->MovePenTo(point);

	BString secondLine;

	secondLine << bdaddrUtils::ToString(fAddress) << "   ";
	fClass.GetMajorDeviceClass(secondLine);
	secondLine << " / ";
	fClass.GetMinorDeviceClass(secondLine);

	owner->DrawString(secondLine.String());

	point -= BPoint(0, (finfo.ascent + finfo.descent + finfo.leading) + INSETS);

	owner->SetFont(be_plain_font);
	owner->MovePenTo(point);
	owner->DrawString(fName.String());

	fClass.Draw(owner, BPoint(itemRect.left, itemRect.top));

#if 0
	switch (fClass.GetMajorDeviceClass()) {
		case 1:
		{
			BRect iconRect(0, 0, 15, 15);
			BBitmap* icon  = new BBitmap(iconRect, B_CMAP8);
			icon->SetBits(kTVBits, kTVWidth * kTVHeight, 0, kTVColorSpace);
			owner->DrawBitmap(icon, iconRect, BRect(itemRect.left + INSETS,
				itemRect.top + INSETS, itemRect.left + INSETS + PIXELS_FOR_ICON,
				itemRect.top + INSETS + PIXELS_FOR_ICON));
			break;
		}
		case 4:
		{
			BRect iconRect(0, 0, 15, 15);
			BBitmap* icon = new BBitmap(iconRect, B_CMAP8);
			icon->SetBits(kMixerBits, kMixerWidth * kMixerHeight, 0, kMixerColorSpace);
			owner->DrawBitmap(icon, iconRect, BRect(itemRect.left + INSETS,
				itemRect.top + INSETS, itemRect.left + INSETS + PIXELS_FOR_ICON,
				itemRect.top + INSETS + PIXELS_FOR_ICON));
			break;
		}
	}
#endif

	owner->SetHighColor(kBlack);

}


void
DeviceListItem::Update(BView* owner, const BFont* font)
{
	BListItem::Update(owner, font);

	font_height height;
	font->GetHeight(&height);
	SetHeight(MAX((height.ascent + height.descent + height.leading) * TEXT_ROWS
		+ (TEXT_ROWS + 1)*INSETS, DeviceClass::PixelsForIcon + 2 * INSETS));

}


int
DeviceListItem::Compare(const void	*firstArg, const void	*secondArg)
{
	const DeviceListItem* item1 = *static_cast<const DeviceListItem* const *>
		(firstArg);
	const DeviceListItem* item2 = *static_cast<const DeviceListItem* const *>
		(secondArg);

	return (int)bdaddrUtils::Compare(item1->fAddress, item2->fAddress);
}


BluetoothDevice*
DeviceListItem::Device() const
{
	return fDevice;
}


}
