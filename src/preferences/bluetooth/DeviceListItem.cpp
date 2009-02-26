/*
 * Copyright 2009, Oliver Ruiz Dorantes, <oliver.ruiz.dorantes_at_gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "DeviceListItem.h"
#include <bluetooth/bdaddrUtils.h>


#include <Bitmap.h>
#include <View.h>

namespace bluetooth {

RangeItem::RangeItem(uint32 lowAddress, uint32 highAddress, const char* name)
	: BListItem(),
	fLowAddress(lowAddress), 
	fHighAddress(highAddress)
{
	fName = strdup(name);
}

RangeItem::~RangeItem()
{
	delete fName;
}

/***********************************************************
 * DrawItem
 ***********************************************************/
void
RangeItem::DrawItem(BView *owner, BRect itemRect, bool complete)
{
	rgb_color kBlack = { 0,0,0,0 };
	rgb_color kHighlight = { 156,154,156,0 };
	
	if (IsSelected() || complete) {
		rgb_color color;
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
	
	BFont font = be_plain_font;
	font_height	finfo;
	font.GetHeight(&finfo);
	
	BPoint point = BPoint(itemRect.left + 17, itemRect.bottom - finfo.descent + 1);
	owner->SetFont(be_fixed_font);
	owner->SetHighColor(kBlack);
	owner->MovePenTo(point);
	
/*	if (fLowAddress >= 0) {
		char string[255];
		sprintf(string, "0x%04lx - 0x%04lx", fLowAddress, fHighAddress);
		owner->DrawString(string);
	}
	point += BPoint(174, 0);*/
	owner->SetFont(be_plain_font);
	owner->MovePenTo(point);
	owner->DrawString(fName);
}

int 
RangeItem::Compare(const void *firstArg, const void *secondArg)
{
	const RangeItem *item1 = *static_cast<const RangeItem * const *>(firstArg);
	const RangeItem *item2 = *static_cast<const RangeItem * const *>(secondArg);
	
	if (item1->fLowAddress < item2->fLowAddress) {
		return -1;
	} else if (item1->fLowAddress > item2->fLowAddress) {
		return 1;
	} else 
		return 0;

}

}
