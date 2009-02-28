/*
 * Copyright 2009, Oliver Ruiz Dorantes, <oliver.ruiz.dorantes_at_gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <Bitmap.h>
#include <View.h>

#include <bluetooth/bdaddrUtils.h>
#include <bluetooth/BluetoothDevice.h>


#include "DeviceListItem.h"


#define PIXELS_FOR_ICON 32
#define INSETS  2
#define TEXT_ROWS  2

namespace Bluetooth {

DeviceListItem::DeviceListItem(BluetoothDevice*	bDevice) 
				:	BListItem(), 
				fDevice(bDevice)	
{	
			SetDevice(fDevice);
}	


DeviceListItem::DeviceListItem(bdaddr_t	bdaddr,	DeviceClass	dClass,	int32	rssi = 0)
				:	BListItem(),				 
				fDevice(NULL),
				fAddress(bdaddr),
				fClass(dClass),
				fName("unknown"),
				fRSSI(rssi)
{	

}	


void
DeviceListItem::SetDevice(BluetoothDevice* bDevice)
{
	fAddress = bDevice->GetBluetoothAddress();
	fClass = bDevice->GetDeviceClass();
	fName	=	bDevice->GetFriendlyName();
	// AKAIR rssi	can	only be	got	at inquiry time...	
}


DeviceListItem::~DeviceListItem()	
{	

}


/*********************************************************** 
 * DrawItem	
 ***********************************************************/	
void 
DeviceListItem::DrawItem(BView *owner, BRect itemRect, bool	complete)	
{	
	rgb_color	kBlack = { 0,0,0,0 };	
	rgb_color	kHighlight = { 156,154,156,0 };	
	
	if (IsSelected() ||	complete)	{	
					rgb_color	color; 
					if (IsSelected())	
									color	=	kHighlight;	
					else 
									color	=	owner->ViewColor();	
					 
					owner->SetHighColor(color);	
					owner->SetLowColor(color); 
					owner->FillRect(itemRect); 
					owner->SetHighColor(kBlack); 
					 
	}	else { 
					owner->SetLowColor(owner->ViewColor());	
	}	
	 				
	font_height			finfo; 
	be_plain_font->GetHeight(&finfo);	
     				 
	BPoint point = BPoint(itemRect.left	+	PIXELS_FOR_ICON + INSETS,	itemRect.bottom	-	finfo.descent	+	1);	
	owner->SetFont(be_fixed_font); 
	owner->SetHighColor(kBlack); 
	owner->MovePenTo(point); 
	
	owner->DrawString(bdaddrUtils::ToString(fAddress));	 

	point	-= BPoint(0, (finfo.ascent + finfo.descent + finfo.leading)); 

	owner->SetFont(be_plain_font); 
	owner->MovePenTo(point); 
	owner->DrawString(fName.String()); 
	
	// TODO: Stroke icon
	
	// TODO: Draw rssi
} 


void
DeviceListItem::Update(BView *owner, const BFont *font)
{
	BListItem::Update(owner,font);

   	font_height height;
	font->GetHeight(&height);
	//SetHeight(finfo.leading*TEXT_ROWS + TEXT_ROWS*INSETS + 50);
	SetHeight((height.ascent + height.descent + height.leading) * TEXT_ROWS + TEXT_ROWS*INSETS);
	

}
	
int	
DeviceListItem::Compare(const void	*firstArg, const void	*secondArg)	
{ 
	 const DeviceListItem	*item1 = *static_cast<const	DeviceListItem * const *>(firstArg); 
	 const DeviceListItem	*item2 = *static_cast<const	DeviceListItem * const *>(secondArg);	
				
	 return	(int)bdaddrUtils::Compare((bdaddr_t*)&item1->fAddress, (bdaddr_t*)&item2->fAddress);	 
} 

}
