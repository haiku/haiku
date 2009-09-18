/*
 * Copyright 2004-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 * 		Fredrik Modéen
 */

#include "InterfaceStatusItem.h"

#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/sockio.h>

#include "NetworkSetupAddOn.h"
#include "Setting.h"

InterfaceStatusItem::InterfaceStatusItem(const char* name, NetworkStatusAddOn* addon)
	: 
	BListItem(0, false),
	fIcon(NULL), 	
	fSetting(new Setting(name)),
	fAddon(addon)
{
	fEnabled = true;
	_InitIcon();
}


InterfaceStatusItem::~InterfaceStatusItem()
{
	delete fIcon;
}


void InterfaceStatusItem::Update(BView* owner, const BFont* font)
{	
	BListItem::Update(owner,font);	
	font_height height;
	font->GetHeight(&height);

	// TODO: take into account icon height, if he's taller...
	SetHeight((height.ascent+height.descent+height.leading) * 3.0 + 8);
}


void
InterfaceStatusItem::DrawItem(BView* owner, BRect /*bounds*/, bool complete)
{
	BListView* list = dynamic_cast<BListView*>(owner);
	if (!list)
		return;
		
	font_height height;
	BFont font;
	owner->GetFont(&font);
	font.GetHeight(&height);
	float fntheight = height.ascent+height.descent+height.leading;

	BRect bounds = list->ItemFrame(list->IndexOf(this));		
								
	rgb_color oldviewcolor = owner->ViewColor();
	rgb_color oldlowcolor = owner->LowColor();
	rgb_color oldcolor = owner->HighColor();

	rgb_color color = oldviewcolor;
	if ( IsSelected() ) 
		color = tint_color(color, B_HIGHLIGHT_BACKGROUND_TINT);

	owner->SetViewColor( color );
	owner->SetHighColor( color );
	owner->SetLowColor( color );
	owner->FillRect(bounds);

	owner->SetViewColor( oldviewcolor);
	owner->SetLowColor( oldlowcolor );
	owner->SetHighColor( oldcolor );

	BPoint iconPt = bounds.LeftTop() + BPoint(4,4);
	BPoint namePt = iconPt + BPoint(32+8, fntheight);
	BPoint driverPt = iconPt + BPoint(32+8, fntheight*2);
	BPoint commentPt = iconPt + BPoint(32+8, fntheight*3);
		
	drawing_mode mode = owner->DrawingMode();
	if (fEnabled)
		owner->SetDrawingMode(B_OP_OVER);
	else {
		owner->SetDrawingMode(B_OP_ALPHA);
		owner->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
		owner->SetHighColor(0, 0, 0, 32);
	}
	
	owner->DrawBitmapAsync(fIcon, iconPt);

	if (!fEnabled)
		owner->SetHighColor(tint_color(oldcolor, B_LIGHTEN_2_TINT));

	owner->SetFont(be_bold_font);
	owner->DrawString(Name(), namePt);
	owner->SetFont(be_plain_font);

	if (fEnabled) {
		BString str("Enabled, IPv4 address: ");
		str << fSetting->GetIP();
		owner->DrawString(str.String(), driverPt);
		if (fSetting->GetAutoConfigure())
			owner->DrawString("DHCP enabled", commentPt);
		else
			owner->DrawString("DHCP disabled, use static IP adress", commentPt);

		
	} else 
		owner->DrawString("Disabled.", driverPt);

	owner->SetHighColor(oldcolor);
	owner->SetDrawingMode(mode);
}


void
InterfaceStatusItem::_InitIcon()
{
	BBitmap* icon = NULL;

	BResources *resources = fAddon->Resources();
	if (resources) {
		size_t size;
//		printf("Name = %s\n", Name());
//		const void *data = resources->LoadResource('ICON', Name(), &size);
//		if (!data)

		const void *data = resources->LoadResource('ICON', "generic_device", &size);

		if (data) {
			// Now build the bitmap
			icon = new BBitmap(BRect(0, 0, 31, 31), B_CMAP8);
			icon->SetBits(data, size, 0, B_CMAP8);
		}
	}
	fIcon = icon;
}
