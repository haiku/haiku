//------------------------------------------------------------------------------
// IconListItem.cpp
//------------------------------------------------------------------------------
// A ListItem implementation that displays an icon and its label.
//
// IconListItem implementation Copyright (C) 1999 Fabien Fulhaber <fulhaber@evhr.net>
// Special thanks to Brendan Allen <darkmoon96@tcon.net> for his help.
// Thanks to NPC community (http://www.beroute.tzo.com/npc/).
// This code is free to use in any way so long as the credits above remain intact.
// This code carries no warranties or guarantees of any kind. Use at your own risk.
//------------------------------------------------------------------------------
// I N C L U D E S
//------------------------------------------------------------------------------

#include "IconListItem.h"

//------------------------------------------------------------------------------
// I M P L E M E N T A T I O N
//------------------------------------------------------------------------------

IconListItem::IconListItem(BBitmap *mini_icon,char *text, uint32 data, int level, bool expanded) :
	BListItem(level,expanded)
{
	if (mini_icon)
	{
		BRect rect(0.0, 0.0, 15.0, 15.0);
		icon = new BBitmap(rect, B_CMAP8);

		if ((mini_icon->BytesPerRow() == icon->BytesPerRow()) &&
        	(mini_icon->BitsLength() == icon->BitsLength()))
        	memcpy(icon->Bits(), mini_icon->Bits(), mini_icon->BitsLength());
        else
        	delete icon;
	} 

	SetHeight(18.0);
	label.SetTo(text);
	extra = data;
	BFont font(be_plain_font);
	float width_of_item = font.StringWidth(text);
	SetWidth(width_of_item);
}

IconListItem::~IconListItem() 
{
	if (icon)
	{
		delete icon;
		icon = NULL;
	}
}

void IconListItem::DrawItem(BView *owner, BRect frame, bool complete)
{
	rgb_color color;

	if (icon)
	{
		frame.left += 18;
		frame.top += 3;
		frame.bottom = frame.top + 10;
		BRect rect(frame.left - 15.0, frame.top - 2.0, frame.left, frame.top + 13.0);
		owner->MovePenTo(0, frame.top);
		if (IsSelected())
		{
			owner->SetHighColor(255, 255, 255, 255);
			owner->FillRect(rect);
			owner->SetDrawingMode(B_OP_INVERT);
			owner->DrawBitmap(icon, rect);
			owner->SetDrawingMode(B_OP_ALPHA); 
			owner->SetHighColor(0, 0, 0, 180);       
			owner->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE);
			owner->DrawBitmap(icon, rect);
		}
		else
		{
			owner->SetDrawingMode(B_OP_OVER);
			owner->DrawBitmap(icon, rect);
		}

		owner->SetDrawingMode(B_OP_OVER);
	}

	frame.left += 5.0;
	frame.top += 1.0;
	frame.right = frame.left + Width() + 1.0;
	frame.bottom += 3.0;
	if (IsSelected() || complete)
	{
		if (IsSelected())
			color.red = color.green = color.blue = color.alpha = 0;
		else
			color=owner->ViewColor();

		owner->SetHighColor(color);
		owner->FillRect(frame);
	}

	float middle = (frame.bottom + frame.top) / 2.0;
	owner->MovePenTo(frame.left + 1.0, (middle + 4.0));
	if (IsSelected())
		color.red = color.green = color.blue = color.alpha = 255;
	else
		color.red = color.green = color.blue = color.alpha = 0;

	owner->SetHighColor(color);
	owner->DrawString(label.String());
} 

void IconListItem::Update(BView *owner, const BFont *font)
{
}
//---------------------------------------------------------- IconListMenu.cpp --
